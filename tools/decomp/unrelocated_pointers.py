#!/usr/bin/env python3
"""Find words that look like pointers into code or data that moves when code is added or removed, but that dsd did not
record as relocations. A shifted build leaves such a word pointing at the old address, so each one is a place where a
mod that changes code sizes can break. The checks of `ninja` cannot see them, since the matching build never moves
anything.

    unrelocated_pointers.py                      # summary per module and section
    unrelocated_pointers.py --list overlays/ov012  # every suspect in one module
    unrelocated_pointers.py --version w2_us

A suspect is an aligned word in a module's data, or in a literal pool of its code, whose value lies in the main
module or in an overlay (ITCM, DTCM and the autoloads have fixed addresses). Code words are only counted between the
end of a function's instructions and the next function, where literal pools sit, so instructions that happen to look
like addresses are left out; data that only looks like an address (a large integer or fixed-point value) can remain.
"""
import argparse
import bisect
import sys
from collections import Counter
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import load_modules, parse_sections  # noqa: E402

# Modules whose addresses move with the code before them: everything linked after the start of the main module
FIXED_MODULES = ("itcm", "dtcm", "autoload_2", "autoload_3", "ltd_autoload_0")


def shifting_ranges(modules) -> list[tuple[int, int, str]]:
    """Returns the address range of each module that moves, as (start, end, name)."""
    ranges = []
    for name, module in modules.items():
        if name in FIXED_MODULES or not (module.config_dir / "delinks.txt").exists():
            continue
        sections = parse_sections(module.config_dir / "delinks.txt")
        start = min(s for s, _ in sections.values())
        end = max(e for _, e in sections.values())
        ranges.append((start, end, name))
    return sorted(ranges)


_SYMBOL_NAMES: dict[int, str] = {}


def symbol_names(modules) -> dict[int, str]:
    """Returns the name of the symbol at each address of the modules that move."""
    if not _SYMBOL_NAMES:
        for name, module in modules.items():
            if name not in FIXED_MODULES:
                for symbol in module.symbols:
                    _SYMBOL_NAMES.setdefault(symbol.addr, symbol.name)
        # Section boundaries, which the SDK's startup code and heap setup point at
        for name, module in modules.items():
            if name not in FIXED_MODULES and (module.config_dir / "delinks.txt").exists():
                for section, (start, end) in parse_sections(module.config_dir / "delinks.txt").items():
                    _SYMBOL_NAMES.setdefault(start, f"{name} {section} start")
                    _SYMBOL_NAMES.setdefault(end, f"{name} {section} end")
    return _SYMBOL_NAMES


def scan(modules, version_ranges, module_name: str):
    """Yields (address, value, section, target module, symbol at the value or None) for every suspect word."""
    module = modules[module_name]
    if not (module.config_dir / "delinks.txt").exists():
        return
    sections = parse_sections(module.config_dir / "delinks.txt")
    data = module.data()
    # Every relocated word, including link-time constants and overlay IDs, which have no target address
    relocated = {int(line.split()[0][5:], 16) for line in (module.config_dir / "relocs.txt").read_text().splitlines()
                 if line.startswith("from:")}
    starts = [r[0] for r in version_ranges]
    symbols = symbol_names(modules)
    for name, (start, end) in sections.items():
        if name == ".bss" or start == end:
            continue
        code = name in (".text", ".init")
        pools = pool_word_set(module, data, start, end) if code else None
        for addr in range((start + 3) & ~3, end - 3, 4):
            if addr in relocated:
                continue
            if code and addr not in pools:
                continue
            offset = addr - module.base
            if offset + 4 > len(data):
                break
            value = int.from_bytes(data[offset:offset + 4], "little")
            i = bisect.bisect_right(starts, value) - 1
            if i < 0:
                continue
            r_start, r_end, target = version_ranges[i]
            if value < r_end:
                yield addr, value, name, target, symbols.get(value) or symbols.get(value & ~1)


def pool_word_set(module, data: bytes, start: int, end: int) -> set[int]:
    """Returns the addresses of the literal pool words in a code section: every word that a function's own
    `ldr rX, [pc, #imm]` reads."""
    words = set()
    for function in module.functions():
        if not (start <= function.addr < end) or not function.size:
            continue
        f_start = function.addr & ~1
        f_end = f_start + function.size
        if function.thumb:
            for addr in range(f_start, f_end - 1, 2):
                half = int.from_bytes(data[addr - module.base:addr - module.base + 2], "little")
                if half & 0xF800 == 0x4800:  # ldr rX, [pc, #imm8 * 4]
                    target = ((addr + 4) & ~3) + (half & 0xFF) * 4
                    if f_start <= target < f_end:
                        words.add(target)
        else:
            for addr in range(f_start, f_end - 3, 4):
                word = int.from_bytes(data[addr - module.base:addr - module.base + 4], "little")
                # ldr rX, [pc, #+/-imm12]
                if word & 0x0F7F0000 == 0x051F0000:
                    imm = word & 0xFFF
                    target = addr + 8 + (imm if word & 0x00800000 else -imm)
                    if f_start <= target < f_end:
                        words.add(target)
    return words


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default="b2_us")
    parser.add_argument("--list", metavar="MODULE", help="list the suspects in one module, e.g. . or overlays/ov012")
    parser.add_argument("--all", action="store_true", help="with --list, also list words that point at no symbol")
    args = parser.parse_args()

    modules = load_modules(args.version)
    ranges = shifting_ranges(modules)
    if args.list:
        for addr, value, section, target, symbol in scan(modules, ranges, args.list):
            if symbol or args.all:
                print(f"{addr:#010x} {section:8} {value:#010x} -> {target} {symbol or ''}")
        return
    likely, other = Counter(), Counter()
    for name in sorted(modules):
        for _addr, _value, section, _target, symbol in scan(modules, ranges, name):
            (likely if symbol else other)[(name, section)] += 1
    for (name, section), count in sorted(likely.items(), key=lambda item: -item[1]):
        print(f"{count:6} {name} {section}")
    print(f"{sum(likely.values())} words that point at a symbol in {len({n for n, _ in likely})} modules, and "
          f"{sum(other.values())} more that only look like addresses (--all lists those too)")


if __name__ == "__main__":
    main()
