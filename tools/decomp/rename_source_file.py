#!/usr/bin/env python3
"""Rename a registered source file in every version's delinks.txt, such as a library file rewritten in assembly.

    rename_source_file.py lib/msl/src/fp/fp_fastI.c lib/msl/src/fp/fp_fastI.s

The file keeps its sections and its complete marker; only the path that heads its entry changes, and the path in the
records of config/fixes.txt that move or add one of its sections. Every version that registers the old path is checked
before any is written: each must register it once, and none may register the new one. Renaming the file itself, its
settings in a library.toml, and the docs that name it, are left to the caller.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import config_lock  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
# The fixes whose argument starts with a source path, "path:section[:size]"
SECTION_FIXES = {"section_start", "section_end", "remove_section", "add_section"}


def plan(old: str, new: str, root: Path = ROOT) -> list[tuple[Path, str]]:
    """Returns each delinks.txt, and fixes.txt if it names the file, to rewrite with its new text, or raises ValueError
    if any version can't be renamed."""
    planned = []
    for delinks in sorted(root.glob("config/*/arm9/**/delinks.txt")):
        lines = delinks.read_text().split("\n")
        if f"{new}:" in lines:
            raise ValueError(f"{new} is already registered in {delinks.relative_to(root)}")
        count = lines.count(f"{old}:")
        if count > 1:
            raise ValueError(f"{old} is registered {count} times in {delinks.relative_to(root)}")
        if count:
            lines[lines.index(f"{old}:")] = f"{new}:"
            planned.append((delinks, "\n".join(lines)))
    if not planned:
        raise ValueError(f"{old} is not registered in any version")
    fixes = root / "config" / "fixes.txt"
    if fixes.exists():
        lines = fixes.read_text().split("\n")
        renamed = False
        for i, line in enumerate(lines):
            fields = line.split(" ", 3)
            if not line.startswith("#") and len(fields) == 4 and fields[2] in SECTION_FIXES \
                    and fields[3].startswith(f"{old}:"):
                lines[i] = " ".join([*fields[:3], new + fields[3][len(old):]])
                renamed = True
        if renamed:
            planned.append((fixes, "\n".join(lines)))
    return planned


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("old")
    parser.add_argument("new")
    args = parser.parse_args()
    with config_lock():
        try:
            planned = plan(args.old, args.new)
        except ValueError as e:
            print(e)
            return 1
        for delinks, text in planned:
            delinks.write_text(text)
            print(f"{args.old} -> {args.new} in {delinks.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
