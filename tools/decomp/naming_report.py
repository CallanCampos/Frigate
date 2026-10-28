#!/usr/bin/env python3
"""Count what is still unnamed in each module's code, for the naming issues and the name-symbols skill.

    naming_report.py                    # a table of every module with something left
    naming_report.py --module ov036     # that module's files, and each item to look at
    naming_report.py --json             # the counts of every module, by file

A module is an overlay (ov036), ARM9 main's game code (main, src/gfl and src/system) or a library (lib/nitro). For
each source file it counts, in Black 2:

- funcs: functions that symbols.txt still names by address (func_...), placed in the file by delinks.txt;
- data: data and bss objects named by address (data_..., bss_...), but not the compiler's unnamed string literals
  and initializers, which the last build's map shows;
- unk: identifiers that name something by an offset instead of a meaning (unk_2E4, unk1C, Btlv_GetUnk2E4);
- params: placeholder parameters and locals (a0, a1, param1, arg2, var3);
- cases: case labels that are bare numbers, to check against a value set;
- compares: == and != against a bare number of 2 or more, to check the same way.

The last two are leads, not faults: a sequence counter or a coordinate is a number. Comments and string literals are
skipped. A header counts toward the module whose sources include it, or toward "shared" when several do.
"""
import argparse
import collections
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CONFIG = ROOT / "config" / "b2_us" / "arm9"
LEX = re.compile(r'(//[^\n]*|/\*.*?\*/)|("(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\')|([A-Za-z_]\w*)', re.S)
UNK = re.compile(r"(?:^|_)(?:unk|UNK)(?:[_0-9A-Fa-f]|$)|Unk(?:[_0-9A-Fa-f]|$)")
PARAM = re.compile(r"^(?:a[0-9]|param[0-9]+|arg[0-9]+|var[0-9]+)$")
CASE = re.compile(r"\bcase\s+(?:-\s*)?(?:0x[0-9a-fA-F]+|[0-9]+)\s*:")
COMPARE = re.compile(r"[!=]=\s*(?:0x[0-9a-fA-F]+|[2-9]|[1-9][0-9]+)\b(?!\s*[.<>*/+-])")
KINDS = ("funcs", "data", "unk", "params", "cases", "compares")
SECTION = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9a-f]+)\s+end:0x([0-9a-f]+)")


def module_of(path: str) -> str:
    """The module a source file belongs to."""
    parts = Path(path).parts
    if parts[0] == "src":
        return parts[1] if parts[1].startswith("ov") else "main"
    if parts[0] == "lib":
        return f"lib/{parts[1]}"
    return "shared"


def code_only(text: str) -> str:
    """The text with comments and literals blanked, keeping line breaks."""
    return LEX.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)) if m.group(1) or m.group(2) else m.group(0), text)


def file_ranges(delinks: Path) -> list[tuple[int, int, str]]:
    """(start, end, source file) of every section of every file that a module's delinks.txt registers."""
    ranges = []
    if delinks.exists():
        current = None
        for line in delinks.read_text().splitlines():
            if line and not line[0].isspace() and line.endswith(":"):
                current = line[:-1]
            elif current:
                m = SECTION.match(line)
                if m:
                    ranges.append((int(m.group(2), 16), int(m.group(3), 16), current))
    return sorted(ranges)


def anonymous_objects() -> set[tuple[str, int]]:
    """(object stem, address) of the compiler's unnamed objects, string literals and local initializers (@1234), from
    the last build's map: C has no name for them, so their address names are not work left."""
    path = ROOT / "build" / "b2_us" / "arm9.o.xMAP"
    out = set()
    if path.exists():
        for line in path.read_text(errors="replace").splitlines():
            m = re.match(r"\s+([0-9A-F]{8}) [0-9A-F]{8} \.\w+\s+@\d+\t\((\w+)\.o\)", line)
            if m:
                out.add((m.group(2), int(m.group(1), 16)))
    return out


def address_names() -> dict[str, collections.Counter]:
    """Address-named functions and data by source file (the module name where no file holds them), leaving out the
    compiler's unnamed objects."""
    import bisect
    anonymous = anonymous_objects()
    counts = collections.defaultdict(collections.Counter)
    for symbols in CONFIG.rglob("symbols.txt"):
        # Overlays share addresses: a module's symbols are placed by its own delinks.txt only
        ranges = file_ranges(symbols.parent / "delinks.txt")
        starts = [r[0] for r in ranges]
        module = symbols.parent.name if symbols.parent.parent.name == "overlays" else "main"
        for line in symbols.read_text().splitlines():
            name, _, rest = line.partition(" ")
            kind = "funcs" if name.startswith("func_") else "data" if name.startswith(("data_", "bss_")) else None
            if not kind:
                continue
            m = re.search(r"addr:0x([0-9a-f]+)", rest)
            addr = int(m.group(1), 16)
            i = bisect.bisect_right(starts, addr) - 1
            owner = ranges[i][2] if i >= 0 and ranges[i][0] <= addr < ranges[i][1] else module
            if kind == "data" and (Path(owner).stem, addr) in anonymous:
                continue
            counts[owner][kind] += 1
    return counts


def scan_sources() -> tuple[dict, dict]:
    """Counts by file of the identifier and number leads, and the items of each file."""
    counts = collections.defaultdict(collections.Counter)
    items = collections.defaultdict(list)
    files = sorted(list((ROOT / "src").rglob("*.[ch]")) + list((ROOT / "include").rglob("*.h"))
                   + list((ROOT / "lib").rglob("*.[ch]")))
    for f in files:
        rel = str(f.relative_to(ROOT))
        code = code_only(f.read_text(errors="replace"))
        seen = set()
        for m in re.finditer(r"\b[A-Za-z_]\w*\b", code):
            name = m.group(0)
            for kind, pattern in (("unk", UNK), ("params", PARAM)):
                if pattern.search(name) and (kind, name) not in seen:
                    seen.add((kind, name))
                    counts[rel][kind] += 1
                    items[rel].append((kind, code.count("\n", 0, m.start()) + 1, name))
        for kind, pattern in (("cases", CASE), ("compares", COMPARE)):
            for m in pattern.finditer(code):
                counts[rel][kind] += 1
                items[rel].append((kind, code.count("\n", 0, m.start()) + 1, m.group(0).strip()))
    return counts, items


def header_modules() -> dict[str, str]:
    """The module of each header under include/: the one whose sources include it, or shared."""
    users = collections.defaultdict(set)
    for f in list((ROOT / "src").rglob("*.c")) + list((ROOT / "lib").rglob("*.c")):
        module = module_of(str(f.relative_to(ROOT)))
        for inc in re.findall(r'#include\s+"([^"]+)"', f.read_text(errors="replace")):
            users[f"include/{inc}"].add(module)
    return {h: next(iter(m)) if len(m) == 1 else "shared" for h, m in users.items()}


def report() -> tuple[dict, dict]:
    """Counts by module, then by file, and the items of each file."""
    headers = header_modules()
    by_module = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))
    counts, items = scan_sources()
    for path, c in list(address_names().items()) + list(counts.items()):
        module = headers.get(path, "shared") if path.startswith("include/") else (
            module_of(path) if "/" in path else path)
        by_module[module][path].update(c)
    return by_module, items


def total(files: dict) -> collections.Counter:
    t = collections.Counter()
    for c in files.values():
        t.update(c)
    return t


def module_key(m: str) -> tuple:
    return (0, int(m[2:])) if re.fullmatch(r"ov\d+", m) else (1, m)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--module", help="list one module's files and items, e.g. ov036, main, lib/nitro")
    parser.add_argument("--json", action="store_true", help="print the counts of every module as JSON")
    args = parser.parse_args()
    by_module, items = report()
    if args.json:
        print(json.dumps({m: {f: dict(c) for f, c in files.items()} for m, files in by_module.items()}, indent=1,
                         sort_keys=True))
        return
    if args.module:
        files = by_module.get(args.module)
        if not files:
            sys.exit(f"{args.module}: nothing left, or no such module")
        print("file".ljust(60) + "".join(k.rjust(9) for k in KINDS))
        for path in sorted(files):
            print(path.ljust(60) + "".join(str(files[path][k]).rjust(9) for k in KINDS))
        for path in sorted(files):
            for kind, line, text in items.get(path, []):
                print(f"{path}:{line}: {kind}: {text}")
        return
    print("module".ljust(12) + "".join(k.rjust(9) for k in KINDS))
    for module in sorted(by_module, key=module_key):
        t = total(by_module[module])
        if any(t.values()):
            print(module.ljust(12) + "".join(str(t[k]).rjust(9) for k in KINDS))


if __name__ == "__main__":
    main()
