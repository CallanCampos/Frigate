#!/usr/bin/env python3
"""Rename a constant of a list in data/constants/, and every use of it.

    rename_constant.py ZONE_CASTELIA_CITY_12 ZONE_CASTELIA_CITY_NARROW_STREET

The name is replaced as a whole word in the lists and in the tracked sources, headers, data, scripts and docs, and a
data file or directory named after the constant, such as data/zones/castelia_city_12.json, is renamed with it. A
message file's IDs and its header, text/<archive>/<bank>.h, are renamed with it too. The new name must not be taken
already. Rebuild afterwards: the build checks that nothing still uses the old name.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.gen_constants import LISTS_DIR, parse  # noqa: E402
from tools.text.text_data import bank_prefix  # noqa: E402

ROOT = LISTS_DIR.parents[1]
PATHS = ["data", "src", "include", "lib", "docs", "tools"]
SUFFIXES = {".txt", ".s", ".inc", ".c", ".h", ".cpp", ".md", ".py", ".json"}
# The data named after the constants of a list: the list, the directory, the constants' prefix, and the suffix of each
# constant's file, or "/" for a directory
NAMED_DATA = [
    ("species", "data/pokemon", "SPECIES_", "/"),
    ("moves", "data/moves", "MOVE_", "/"),
    ("items", "data/items", "ITEM_", "/"),
    ("abilities", "data/abilities", "ABILITY_", ".json"),
    ("types", "data/types", "TYPE_", ".json"),
    ("trades", "data/trades", "TRADE_", ".json"),
    ("trainer_classes", "data/trainer_classes", "TRAINER_CLASS_", ".json"),
    ("natures", "data/natures", "NATURE_", ".json"),
    ("places", "data/places", "PLACE_", ".json"),
    ("field_scripts", "data/field_scripts", "SCRIPTS_", ".s"),
    ("map_matrices", "data/map_matrices", "MAP_MATRIX_", ".json"),
    ("tr_ai_scripts", "data/tr_ai", "TR_AI_SCRIPT_", ".s"),
    ("text_banks", "data/text/system", "TEXT_BANK_", ".txt"),
    ("script_text_banks", "data/text/script", "SCRIPT_TEXT_", ".txt"),
    ("trainers", "data/trainers", "TRAINER_", ".json"),
    ("encounters", "data/encounters", "ENCOUNTERS_", ".json"),
    ("zones", "data/zones", "ZONE_", ".json"),
    ("zones", "data/events", "ZONE_", ".json"),
]


def data_path(constant: str, directory: str, prefix: str, suffix: str) -> Path:
    return ROOT / directory / (constant.removeprefix(prefix).lower() + ("" if suffix == "/" else suffix))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("old")
    parser.add_argument("new")
    args = parser.parse_args()
    if not args.new.isidentifier():
        sys.exit(f"{args.new} is not a valid name")
    lists = {path: parse(path) for path in sorted(LISTS_DIR.glob("*.txt"))}
    if not any(args.old in names for names in lists.values()):
        sys.exit(f"{args.old} is in no list in {LISTS_DIR.relative_to(ROOT)}")
    if any(args.new in names for names in lists.values()):
        sys.exit(f"{args.new} is already a constant")
    moves = []
    for list_name, directory, prefix, suffix in NAMED_DATA:
        if args.old in lists[LISTS_DIR / f"{list_name}.txt"]:
            old_path, new_path = (data_path(c, directory, prefix, suffix) for c in (args.old, args.new))
            if old_path.exists():
                if not args.new.startswith(prefix):
                    sys.exit(f"{args.new} must start with {prefix}, since its data is named after it")
                moves.append((old_path, new_path))

    # A message file's IDs start with its name, and the code includes its header by the file's name
    renames = [(re.compile(rf"\b{re.escape(args.old)}\b"), args.new)]
    for old_path, new_path in moves:
        if old_path.parent.parent == ROOT / "data/text":
            renames.append((re.compile(rf"\b{re.escape(bank_prefix(old_path))}(?=\w)"), bank_prefix(new_path)))
            header = f"text/{old_path.parent.name}/"
            renames.append((re.compile(rf"(?<=[\"<]){re.escape(header + old_path.stem)}\.h(?=[\">])"),
                            f"{header}{new_path.stem}.h"))
    files = subprocess.run(["git", "ls-files", "-z", *PATHS], cwd=ROOT, capture_output=True, check=True,
                           text=True).stdout.split("\0")
    changed = 0
    for name in files:
        path = ROOT / name
        if path.suffix not in SUFFIXES or not path.is_file():
            continue
        text = path.read_text()
        new_text = text
        for pattern, replacement in renames:
            new_text = pattern.sub(replacement, new_text)
        if new_text != text:
            path.write_text(new_text)
            changed += 1
    for old_path, new_path in moves:
        subprocess.run(["git", "mv", str(old_path), str(new_path)], cwd=ROOT, check=True)
        print(f"moved {old_path.relative_to(ROOT)} to {new_path.relative_to(ROOT)}")
    print(f"renamed {args.old} to {args.new} in {changed} files")


if __name__ == "__main__":
    main()
