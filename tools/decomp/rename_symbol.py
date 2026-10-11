#!/usr/bin/env python3
"""Rename symbols in every version, and keep the names across config regeneration.

Each rename is recorded in config/names.txt by module and address in the primary version, and applied to the paired
symbol of every other version through the version maps. `--apply` renames every symbol listed in names.txt, which
regenerate_configs.py runs after importing names from swan.

    rename_symbol.py func_ov035_0217ed70 ElScoreboard_Create
    rename_symbol.py sButtonAnims sTDownloadButtonAnims --module overlays/ov326   # a name several symbols share
    rename_symbol.py --file renames.txt

The sources are rewritten everywhere, except for a static: only its own file, the one delinks.txt places it in, can
refer to it, and another file may have a static of the same name.
    rename_symbol.py --apply
"""
import argparse
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT, SYMBOL_RE, config_lock  # noqa: E402

PRIMARY = "b2_us"
OTHERS = ["w2_us"]
NAMES = ROOT / "config" / "names.txt"
MAPS = [ROOT / "build" / "version_map.tsv", ROOT / "build" / "version_map_symbols.tsv"]
SOURCE_DIRS = [ROOT / "src", ROOT / "include", ROOT / "lib"]
IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
# The names dsd generates, such as func_ov035_0217ec9c
DEFAULT_NAME_RE = re.compile(r"^(func|data|bss)_(ov\d{3}_)?[0-9a-f]{8}$")


def symbol_files(version: str) -> dict[str, Path]:
    """Returns the symbols.txt of every module of a version, keyed by module path."""
    arm9 = ROOT / "config" / version / "arm9"
    return {str(path.parent.relative_to(arm9)): path for path in sorted(arm9.rglob("symbols.txt"))}


def find_symbols(version: str, name: str) -> list[tuple[str, int]]:
    """Returns every (module, address) with a symbol of this name. Statics in different files can share one."""
    found = []
    for module, path in symbol_files(version).items():
        for line in path.read_text().splitlines():
            match = SYMBOL_RE.match(line)
            if match and match.group(1) == name:
                found.append((module, int(match.group(4), 16)))
    return found


def name_in_use(version: str, name: str) -> bool:
    return bool(find_symbols(version, name))


def owning_source(module: str, addr: int) -> Path | None:
    """Returns the source file whose sections in the primary version's delinks.txt contain an address."""
    path = ROOT / "config" / PRIMARY / "arm9" / module / "delinks.txt"
    source = None
    for line in path.read_text().splitlines():
        if line.endswith(":") and not line.startswith(" "):
            source = line[:-1]
        elif source and (m := re.search(r"start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)", line)):
            if int(m.group(1), 16) <= addr < int(m.group(2), 16):
                return ROOT / source
    return None


def load_pairs() -> dict[str, dict[tuple[str, int], int]]:
    """Returns (module, primary address) -> address in each other version."""
    pairs: dict[str, dict[tuple[str, int], int]] = {other: {} for other in OTHERS}
    for path in MAPS:
        with path.open() as f:
            for row in csv.DictReader(f, delimiter="\t"):
                for other in OTHERS:
                    key = (row["module"], int(row[f"{PRIMARY}_addr"], 16))
                    pairs[other].setdefault(key, int(row[f"{other}_addr"], 16))
    return pairs


def set_name(version: str, module: str, addr: int, name: str) -> str | None:
    """Renames the function or data symbol at an address. Returns the old name, or None if there is no symbol."""
    path = symbol_files(version).get(module)
    if path is None:
        return None
    lines = path.read_text().splitlines()
    for i, line in enumerate(lines):
        match = SYMBOL_RE.match(line)
        if match and int(match.group(4), 16) == addr and match.group(2) in ("function", "data", "bss"):
            old = match.group(1)
            lines[i] = name + line[len(old):]
            path.write_text("\n".join(lines) + "\n")
            return old
    return None


def load_names() -> list[tuple[str, int, str]]:
    names = []
    if NAMES.exists():
        for line in NAMES.read_text().splitlines():
            if line.strip() and not line.startswith("#"):
                module, addr, name = line.split()
                names.append((module, int(addr, 16), name))
    return names


def save_names(names: list[tuple[str, int, str]]):
    header = "# Names that are not in swan, applied by tools/decomp/rename_symbol.py\n# module address(b2_us) name\n"
    lines = [f"{module} {addr:#010x} {name}" for module, addr, name in sorted(names, key=lambda n: (n[0], n[1]))]
    NAMES.write_text(header + "\n".join(lines) + "\n")


def apply(module: str, addr: int, name: str, pairs) -> list[str]:
    """Renames a symbol in every version. Returns the old names."""
    old_names = []
    old = set_name(PRIMARY, module, addr, name)
    if old is None:
        sys.exit(f"{PRIMARY}: no symbol at {module} {addr:#010x}")
    old_names.append(old)
    for other in OTHERS:
        other_addr = pairs[other].get((module, addr))
        if other_addr is None:
            print(f"{other}: {name} has no counterpart, only renamed in {PRIMARY}")
            continue
        old = set_name(other, module, other_addr, name)
        if old is None:
            sys.exit(f"{other}: no symbol at {module} {other_addr:#010x}")
        old_names.append(old)
    return old_names


def is_static(source: Path, name: str) -> bool:
    """Whether a source file defines a name as static, such as `static const u16 sTable[] = {...}`."""
    return re.search(rf"^\s*static\b[^;{{(]*\b{re.escape(name)}\b", source.read_text(), re.MULTILINE) is not None


def rename_in_sources(old_names: list[str], new: str, only: Path | None = None):
    """Rewrites the old names in the sources, or only in one file, the one that defines them as static."""
    patterns = [re.compile(rf"\b{re.escape(old)}\b") for old in set(old_names) if old != new]
    for directory in SOURCE_DIRS:
        for path in sorted(directory.rglob("*")):
            if path.suffix not in (".c", ".h", ".inc", ".s"):
                continue
            if only is not None and path != only:
                continue
            text = path.read_text()
            new_text = text
            for pattern in patterns:
                new_text = pattern.sub(new, new_text)
            if new_text != text:
                path.write_text(new_text)
                print(f"updated {path.relative_to(ROOT)}")


def rename(old: str, new: str, pairs, names: list, module: str | None = None) -> list:
    """Renames a symbol in every version and the sources, and returns the updated names."""
    if not IDENTIFIER_RE.match(new):
        sys.exit(f"{new!r} is not a valid identifier")
    found = find_symbols(PRIMARY, old)
    if module is not None:
        found = [f for f in found if f[0] == module]
    if not found:
        sys.exit(f"{old} is not a symbol in {PRIMARY}" + (f" {module}" if module else ""))
    if len(found) > 1:
        places = ", ".join(f"{m} {a:#010x}" for m, a in found)
        sys.exit(f"{old} names several symbols ({places}): give --module")
    for version in [PRIMARY, *OTHERS]:
        if name_in_use(version, new):
            sys.exit(f"{new} is already a symbol in {version}")

    module, addr = found[0]
    # Statics in different files can share a name, including one whose symbol still has its default name. A static is
    # only visible in its own file, so only that file is rewritten; rewriting everywhere would rename the others too
    only = owning_source(module, addr)
    if only is not None and not (only.exists() and is_static(only, old)):
        only = None
    old_names = apply(module, addr, new, pairs)
    # A default name is not recorded, so renaming a symbol back to it removes its entry
    names = [n for n in names if (n[0], n[1]) != (module, addr)]
    if not DEFAULT_NAME_RE.match(new):
        names.append((module, addr, new))
    save_names(names)
    rename_in_sources(old_names, new, only)
    print(f"{old} -> {new} ({module} {addr:#010x})")
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("old", nargs="?", help="current name in the primary version")
    parser.add_argument("new", nargs="?", help="new name")
    parser.add_argument("--apply", action="store_true", help="apply every rename in config/names.txt")
    parser.add_argument("--file", type=Path, help="rename each `old new` pair, one to a line, of a file")
    parser.add_argument("--module", help="the module of the symbol, e.g. overlays/ov326, when several share the name")
    args = parser.parse_args()

    pairs = load_pairs()
    names = load_names()

    if args.apply:
        for module, addr, name in names:
            apply(module, addr, name, pairs)
        print(f"applied {len(names)} names")
        return
    if args.file:
        for line in args.file.read_text().splitlines():
            if line.strip():
                old, new = line.split()
                names = rename(old, new, pairs, names, args.module)
        return

    if not args.old or not args.new:
        parser.error("give the old and new names, a --file of them, or --apply")
    rename(args.old, args.new, pairs, names, args.module)


if __name__ == "__main__":
    with config_lock():
        main()
