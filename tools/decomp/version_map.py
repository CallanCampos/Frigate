#!/usr/bin/env python3
"""Pair up the functions of two game versions, such as Black 2 and White 2.

Functions are aligned per module by their bytes, with relocated words masked. The result is a TSV with one line per
function pair: module, addresses and names in both versions, and whether their code is identical.
"""
import argparse
import difflib
import hashlib
import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import load_modules, masked_function_bytes, parse_sections, reloc_module_names  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", help="primary version, e.g. b2_us")
    parser.add_argument("other", help="version to map, e.g. w2_us")
    parser.add_argument("-o", "--output", required=True, help="output TSV of function pairs")
    parser.add_argument("--symbols-output", help="output TSV of other symbol pairs, derived from relocations")
    args = parser.parse_args()

    base_modules = load_modules(args.base)
    other_modules = load_modules(args.other)

    totals = {"identical": 0, "different": 0, "unpaired": 0}
    rows = []
    for name, base in base_modules.items():
        other = other_modules.get(name)
        base_funcs = base.functions()
        if other is None:
            totals["unpaired"] += len(base_funcs)
            continue
        other_funcs = other.functions()
        base_data, other_data = base.data(), other.data()
        base_keys = [hashlib.sha1(masked_function_bytes(base, base_data, f)).digest() for f in base_funcs]
        other_keys = [hashlib.sha1(masked_function_bytes(other, other_data, f)).digest() for f in other_funcs]

        matcher = difflib.SequenceMatcher(None, base_keys, other_keys, autojunk=False)
        unpaired_base, unpaired_other = [], []
        for tag, i1, i2, j1, j2 in matcher.get_opcodes():
            if tag == "equal":
                pairs = [(base_funcs[i], other_funcs[j], "identical") for i, j in zip(range(i1, i2), range(j1, j2))]
            elif tag == "replace" and i2 - i1 == j2 - j1:
                # Same number of functions in between, so they are the same functions with different code
                pairs = [(base_funcs[i], other_funcs[j], "different") for i, j in zip(range(i1, i2), range(j1, j2))]
            else:
                unpaired_base.extend(base_funcs[i1:i2])
                unpaired_other.extend(other_funcs[j1:j2])
                continue
            for base_func, other_func, status in pairs:
                totals[status] += 1
                rows.append((name, base_func, other_func, status))

        # Functions that moved, such as the BIOS calls in the shuffled secure area, can be paired by their known name
        other_by_name = {f.name: f for f in unpaired_other if not f.name.startswith("func_")}
        for base_func in unpaired_base:
            other_func = other_by_name.pop(base_func.name, None)
            if other_func is None:
                totals["unpaired"] += 1
                print(f"unpaired {args.base} {name} {base_func.name}")
                continue
            base_key = masked_function_bytes(base, base_data, base_func)
            other_key = masked_function_bytes(other, other_data, other_func)
            status = "identical" if base_key == other_key else "different"
            totals[status] += 1
            rows.append((name, base_func, other_func, status))
        for other_func in other_by_name.values():
            print(f"unpaired {args.other} {name} {other_func.name}")
        totals["unpaired"] += sum(1 for f in unpaired_other if f.name.startswith("func_"))

    num_base = sum(len(m.functions()) for m in base_modules.values())
    print(f"{num_base} functions in {args.base}")
    if args.symbols_output:
        write_symbol_pairs(args, base_modules, other_modules, rows)

    with open(args.output, "w") as f:
        f.write(f"module\t{args.base}_addr\t{args.other}_addr\t{args.base}_name\t{args.other}_name\tstatus\n")
        for name, base_func, other_func, status in rows:
            f.write(f"{name}\t{base_func.addr:#010x}\t{other_func.addr:#010x}\t{base_func.name}\t{other_func.name}\t{status}\n")

    print(", ".join(f"{count} {kind}" for kind, count in totals.items()))


def write_symbol_pairs(args, base_modules, other_modules, rows):
    """Pairs the targets of relocations at the same offsets in identical functions."""
    target_pairs = {}  # (module, base addr) -> other addr, or None if inconsistent
    for name, base_func, other_func, status in rows:
        if status != "identical":
            continue
        base, other = base_modules[name], other_modules[name]
        for source, (kind, base_target, module) in base.relocs.items():
            offset = source - base_func.addr
            if not 0 <= offset < (base_func.size or 0):
                continue
            other_reloc = other.relocs.get(other_func.addr + offset)
            if other_reloc is None or other_reloc[0] != kind:
                continue
            for target_module in reloc_module_names(module):
                key = (target_module, base_target)
                other_target = other_reloc[1]
                if target_pairs.get(key, other_target) != other_target:
                    target_pairs[key] = None
                else:
                    target_pairs[key] = other_target

    # Sections with the same size in both versions have the same layout, so symbols pair up by offset
    for module_name, base in base_modules.items():
        other = other_modules.get(module_name)
        delinks = base.config_dir / "delinks.txt"
        if other is None or not delinks.exists():
            continue
        base_sections = parse_sections(delinks)
        other_sections = parse_sections(other.config_dir / "delinks.txt")
        other_addrs = {s.addr for s in other.symbols if s.kind != "function"}
        for section, (start, end) in base_sections.items():
            other_range = other_sections.get(section)
            if other_range is None or other_range[1] - other_range[0] != end - start:
                continue
            delta = other_range[0] - start
            for symbol in base.symbols:
                if symbol.kind == "function" or not start <= symbol.addr < end or symbol.addr + delta not in other_addrs:
                    continue
                key = (module_name, symbol.addr)
                if target_pairs.get(key, symbol.addr + delta) != symbol.addr + delta:
                    target_pairs[key] = None
                else:
                    target_pairs[key] = symbol.addr + delta

    count = 0
    with open(args.symbols_output, "w") as f:
        f.write(f"module\t{args.base}_addr\t{args.other}_addr\t{args.base}_name\t{args.other}_name\tkind\n")
        for (module_name, base_addr), other_addr in sorted(target_pairs.items()):
            if other_addr is None or module_name not in base_modules or module_name not in other_modules:
                continue
            base_symbols = {s.addr: s for s in base_modules[module_name].symbols if s.kind != "function"}
            other_symbols = {s.addr: s for s in other_modules[module_name].symbols if s.kind != "function"}
            base_symbol, other_symbol = base_symbols.get(base_addr), other_symbols.get(other_addr)
            if base_symbol is None or other_symbol is None or base_symbol.kind != other_symbol.kind:
                continue
            f.write(f"{module_name}\t{base_addr:#010x}\t{other_addr:#010x}\t{base_symbol.name}\t{other_symbol.name}\t{base_symbol.kind}\n")
            count += 1
    print(f"{count} other symbols paired through relocations")


if __name__ == "__main__":
    sys.exit(main())
