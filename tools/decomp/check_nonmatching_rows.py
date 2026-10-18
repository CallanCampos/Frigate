#!/usr/bin/env python3
"""Check docs/nonmatching-functions.md's function rows against both versions' configs.

Each row names a source file, its functions and their addresses, Black 2's then White 2's (`same` when they are
equal). For every function the row gives an address for, the file must be delinked in a module of both versions, the
function must be a symbol of that module at Black 2's address and at White 2's, and the source file must define it.
Rows about data, whose function column is in parentheses, are left out. It needs no ROM, so CI runs it.

    check_nonmatching_rows.py          # list the rows that are wrong, exit 1 if any
    check_nonmatching_rows.py --fix    # also rewrite White 2 addresses that have one right answer
"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT, parse_symbols  # noqa: E402

DOC = ROOT / "docs" / "nonmatching-functions.md"
VERSIONS = ("b2_us", "w2_us")
ROW_RE = re.compile(r"^\| `([^`]+)` \| (.*?) \| (`0x[0-9a-f]+`.*?) \| ")
PAIR_RE = re.compile(r"`0x([0-9a-f]+)` / (?:`0x([0-9a-f]+)`|(same))")
FILE_RE = re.compile(r"^(\S+\.(?:c|cpp|s)):\s*$")


def modules_by_file(version: str) -> dict[str, str]:
    """Returns the module (`.` or `overlays/ovNNN`) whose delinks.txt lists each source file."""
    arm9 = ROOT / "config" / version / "arm9"
    files = {}
    for path in arm9.rglob("delinks.txt"):
        module = path.parent.relative_to(arm9).as_posix()
        for line in path.read_text().splitlines():
            match = FILE_RE.match(line)
            if match:
                files.setdefault(match.group(1), module)
    return files


def symbols_by_module(version: str) -> dict[str, dict[str, set[int]]]:
    """Returns each module's function addresses by name (Thumb bit cleared)."""
    arm9 = ROOT / "config" / version / "arm9"
    modules = {}
    for path in arm9.rglob("symbols.txt"):
        names = modules.setdefault(path.parent.relative_to(arm9).as_posix(), {})
        for symbol in parse_symbols(path):
            if symbol.kind == "function":
                names.setdefault(symbol.name, set()).add(symbol.addr & ~1)
    return modules


def defines(source: str, name: str) -> bool:
    """Whether a line opens a definition of the function: not a call (no `=` before it) or a prototype (no `;`)."""
    return re.search(r"^\s*[A-Za-z_][^;=\n]*\b" + re.escape(name) + r"\s*\([^;\n]*$", source, re.M) is not None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--fix", action="store_true", help="rewrite wrong White 2 addresses that have one answer")
    args = parser.parse_args()

    files = {v: modules_by_file(v) for v in VERSIONS}
    symbols = {v: symbols_by_module(v) for v in VERSIONS}
    lines = DOC.read_text().split("\n")
    problems = []
    fixed = 0
    for index, line in enumerate(lines):
        row = ROW_RE.match(line)
        if not row or row.group(2).startswith("("):
            continue
        path, names = row.group(1), re.findall(r"`([A-Za-z_]\w*)`", row.group(2))
        pairs = PAIR_RE.findall(row.group(3))
        where = f"{DOC.relative_to(ROOT)}:{index + 1}: {path}"
        if len(pairs) not in (1, len(names)):
            problems.append(f"{where}: {len(names)} functions but {len(pairs)} address pairs")
            continue
        source_path = ROOT / path
        source = source_path.read_text(errors="replace") if source_path.exists() else None
        if source is None:
            problems.append(f"{where}: no such file")
        modules = {v: files[v].get(path) for v in VERSIONS}
        for v, module in modules.items():
            if module is None:
                problems.append(f"{where}: not in any {v} delinks.txt")
        for name, (b2, w2, same) in zip(names, pairs):
            if source is not None and not defines(source, name):
                problems.append(f"{where}: {name} is not defined there")
            if None in modules.values():
                continue
            wanted = {"b2_us": int(b2, 16), "w2_us": int(b2 if same else w2, 16)}
            for v in VERSIONS:
                actual = symbols[v][modules[v]].get(name, set())
                if wanted[v] in actual:
                    continue
                found = ", ".join(f"0x{a:08x}" for a in sorted(actual)) or "none"
                problem = f"{where}: {name} is not at 0x{wanted[v]:08x} in {v} (its symbols: {found})"
                if args.fix and v == "w2_us" and not same and len(actual) == 1:
                    lines[index] = lines[index].replace(f"`0x{w2}`", f"`0x{min(actual):08x}`", 1)
                    fixed += 1
                    problem += " (fixed)"
                problems.append(problem)
    if fixed:
        DOC.write_text("\n".join(lines))
        print(f"fixed {fixed} White 2 address{'es' if fixed > 1 else ''}")
    for problem in problems:
        print(problem)
    sys.exit(1 if problems and not (args.fix and fixed == len(problems)) else 0)


if __name__ == "__main__":
    main()
