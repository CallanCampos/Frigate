#!/usr/bin/env python3
"""Check docs/nonmatching-functions.md's function rows against both versions' configs.

Each row names a source file, its functions and their addresses, Black 2's then White 2's (`same` when they are
equal). For every function the row gives an address for, the file must be delinked in a module of both versions, the
function must be a symbol of that module at Black 2's address and at White 2's, and the source file must define it.
Rows about data, whose function column is in parentheses, are left out. Merge markers and duplicate function
entries are rejected. It needs no ROM, so CI runs it.

    check_nonmatching_rows.py          # list the rows that are wrong, exit 1 if any
    check_nonmatching_rows.py --fix    # also rewrite White 2 addresses that have one right answer
"""
import argparse
import re
import sys
import tomllib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT, parse_symbols  # noqa: E402

DOC = ROOT / "docs" / "nonmatching-functions.md"
VERSIONS = ("b2_us", "w2_us")
ROW_RE = re.compile(r"^\| `([^`]+)` \| (.*?) \| (`0x[0-9a-f]+`.*?) \| ")
PAIR_RE = re.compile(r"`0x([0-9a-f]+)` / (?:`0x([0-9a-f]+)`|(same))")
FILE_RE = re.compile(r"^(\S+\.(?:c|cpp|s)):\s*$")
MERGE_MARKER_RE = re.compile(r"^(?:<{7,}|\|{7,}|={7,}|>{7,})(?:\s.*)?$")


def structural_problems(lines: list[str]) -> list[str]:
    """Reject merge debris and repeated function entries, including those in grouped rows."""
    problems = []
    seen = {}
    for number, line in enumerate(lines, 1):
        where = f"{DOC.relative_to(ROOT)}:{number}"
        if MERGE_MARKER_RE.match(line):
            problems.append(f"{where}: unresolved merge marker")
        row = ROW_RE.match(line)
        if not row or row.group(2).startswith("("):
            continue
        path = row.group(1)
        for name in re.findall(r"`([A-Za-z_]\w*)`", row.group(2)):
            key = (path, name)
            if key in seen:
                problems.append(f"{where}: {path}: duplicate function {name} (first listed on line {seen[key]})")
            else:
                seen[key] = number
    return problems


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


def merged_parts() -> dict[str, list[str]]:
    """Returns the parts of each merged file (config/merged_sources.toml), which delinks.txt lists under their own
    names, in their own modules, while the whole's source holds their code."""
    path = ROOT / "config" / "merged_sources.toml"
    parts = {}
    if path.exists():
        for part, whole in tomllib.loads(path.read_text())["merged"].items():
            parts.setdefault(whole, []).append(part)
    return parts


def defines(source: str, name: str) -> bool:
    """Whether a line opens a definition of the function: not a call (no `=` before it) or a prototype (no `;`)."""
    return re.search(r"^\s*[A-Za-z_][^;=\n]*\b" + re.escape(name) + r"\s*\([^;\n]*$", source, re.M) is not None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--fix", action="store_true", help="rewrite wrong White 2 addresses that have one answer")
    args = parser.parse_args()

    files = {v: modules_by_file(v) for v in VERSIONS}
    symbols = {v: symbols_by_module(v) for v in VERSIONS}
    parts = merged_parts()
    lines = DOC.read_text().split("\n")
    problems = structural_problems(lines)
    fixed = 0
    repaired_problems = 0
    for index, line in enumerate(lines):
        row = ROW_RE.match(line)
        if not row or row.group(2).startswith("("):
            continue
        path, names = row.group(1), re.findall(r"`([A-Za-z_]\w*)`", row.group(2))
        pairs = list(PAIR_RE.finditer(row.group(3)))
        shared_pair = len(pairs) == 1 and len(names) > 1
        where = f"{DOC.relative_to(ROOT)}:{index + 1}: {path}"
        if len(pairs) not in (1, len(names)):
            problems.append(f"{where}: {len(names)} functions but {len(pairs)} address pairs")
            continue
        if len(pairs) == 1:
            pairs *= len(names)
        source_path = ROOT / path
        source = source_path.read_text(errors="replace") if source_path.exists() else None
        if source is None:
            problems.append(f"{where}: no such file")
        modules = {v: files[v].get(path) for v in VERSIONS}
        for v, module in modules.items():
            if module is None:
                problems.append(f"{where}: not in any {v} delinks.txt")
        # A merged file's functions may be in a part's module
        searched = {v: [m for m in (module, *(files[v].get(part) for part in parts.get(path, ()))) if m is not None]
                    for v, module in modules.items()}

        def addresses(v: str, name: str) -> set[int]:
            return set().union(*(symbols[v][m].get(name, set()) for m in searched[v]))

        can_fix_white = True
        if shared_pair and modules["w2_us"] is not None:
            targets = [addresses("w2_us", name) for name in names]
            can_fix_white = all(len(target) == 1 and target == targets[0] for target in targets)
        edits = {}
        for name, pair in zip(names, pairs):
            b2, w2, same = pair.groups()
            if source is not None and not defines(source, name):
                problems.append(f"{where}: {name} is not defined there")
            if None in modules.values():
                continue
            wanted = {"b2_us": int(b2, 16), "w2_us": int(b2 if same else w2, 16)}
            for v in VERSIONS:
                actual = addresses(v, name)
                if wanted[v] in actual:
                    continue
                found = ", ".join(f"0x{a:08x}" for a in sorted(actual)) or "none"
                problem = f"{where}: {name} is not at 0x{wanted[v]:08x} in {v} (its symbols: {found})"
                if args.fix and v == "w2_us" and not same and len(actual) == 1 and can_fix_white:
                    start, end = pair.span(2)
                    offset = row.start(3)
                    edits[(offset + start, offset + end)] = f"{min(actual):08x}"
                    repaired_problems += 1
                    problem += " (fixed)"
                problems.append(problem)
        for (start, end), replacement in sorted(edits.items(), reverse=True):
            lines[index] = lines[index][:start] + replacement + lines[index][end:]
        fixed += len(edits)
    if fixed:
        DOC.write_text("\n".join(lines))
        print(f"fixed {fixed} White 2 address{'es' if fixed > 1 else ''}")
    for problem in problems:
        print(problem)
    sys.exit(1 if problems and not (args.fix and repaired_problems == len(problems)) else 0)


if __name__ == "__main__":
    main()
