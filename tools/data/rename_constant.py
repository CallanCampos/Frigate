#!/usr/bin/env python3
"""Rename a constant of a list in generated/, and every use of it.

    rename_constant.py ZONE_CASTELIA_CITY_12 ZONE_CASTELIA_CITY_NARROW_STREET
    rename_constant.py --batch renames.txt        # OLD NEW a line, all in one pass

The name is replaced as a whole word in the lists and in the tracked sources, headers, data, scripts and docs, and a
data file or directory named after the constant, such as res/field/zones/castelia_city_12.json, is renamed with it. A
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
from tools.data.field_actor_data import directory as field_actor_directory  # noqa: E402
from tools.data.item_icon_data import member_path as item_icon_path  # noqa: E402
from tools.data.sound_data import entry_dir as sound_entry_dir  # noqa: E402
from tools.data.species_data import directory as species_directory  # noqa: E402
from tools.text.text_data import bank_prefix  # noqa: E402

ROOT = LISTS_DIR.parent
PATHS = ["res", "generated", "src", "include", "lib", "docs", "tools"]
SUFFIXES = {".txt", ".s", ".inc", ".c", ".h", ".cpp", ".md", ".py", ".json"}
# The data named after the constants of a list: the list, the directory, the constants' prefix, and the suffix of each
# constant's file, or "/" for a directory
NAMED_DATA = [
    ("species", "res/pokemon", "SPECIES_", "/"),
    ("moves", "res/moves", "MOVE_", "/"),
    ("items", "res/items", "ITEM_", "/"),
    ("abilities", "res/abilities", "ABILITY_", ".json"),
    ("types", "res/types", "TYPE_", ".json"),
    ("trades", "res/field/trades", "TRADE_", ".json"),
    ("trainer_classes", "res/trainers/classes", "TRAINER_CLASS_", ".json"),
    ("natures", "res/natures", "NATURE_", ".json"),
    ("places", "res/field/places", "PLACE_", ".json"),
    ("field_scripts", "res/field/scripts", "SCRIPTS_", ".s"),
    ("map_matrices", "res/field/matrices", "MAP_MATRIX_", ".json"),
    ("areas", "res/field/areas", "AREA_", ".json"),
    ("map_models", "res/field/maps/models", "MAP_MODEL_", "/"),
    ("map_texture_sets", "res/field/maps/textures", "MAP_TEXTURE_SET_", "/"),
    ("field_props_exterior", "res/field/props/exterior/models", "FIELD_PROPS_EXTERIOR_", "/"),
    ("field_props_exterior", "res/field/props/exterior/textures", "FIELD_PROPS_EXTERIOR_", "/"),
    ("field_props_interior", "res/field/props/interior/models", "FIELD_PROPS_INTERIOR_", "/"),
    ("field_props_interior", "res/field/props/interior/textures", "FIELD_PROPS_INTERIOR_", "/"),
    ("tr_ai_scripts", "res/trainers/ai", "TR_AI_SCRIPT_", ".s"),
    ("text_banks", "res/text/system", "TEXT_BANK_", ".txt"),
    ("script_text_banks", "res/text/script", "SCRIPT_TEXT_", ".txt"),
    ("trainers", "res/trainers/data", "TRAINER_", ".json"),
    ("encounters", "res/field/encounters", "ENCOUNTERS_", ".json"),
    ("zones", "res/field/zones", "ZONE_", ".json"),
    ("zones", "res/field/events", "ZONE_", ".json"),
    ("obj_codes", "res/field/actors", "OBJ_CODE_", "/"),
    ("sound", "res/sound/sequences", "SEQ_", "/"),
    ("sound_banks", "res/sound/banks", "BANK_", "/"),
    ("sound_wave_archives", "res/sound/wave_archives", "WAVE_", "/"),
    ("sound_streams", "res/sound/streams", "STRM_", "/"),
]

# The textures' directories, and the script that relinks the models that show them
RELINKS = [
    ("res/field/maps/textures", "tools/data/map_model_data.py", "res/field/maps/models"),
    ("res/field/props/exterior/textures", "tools/data/field_prop_model_data.py", "res/field/props/exterior/models"),
    ("res/field/props/interior/textures", "tools/data/field_prop_model_data.py", "res/field/props/interior/models"),
]


def data_path(constant: str, directory: str, prefix: str, suffix: str) -> Path:
    if prefix == "SPECIES_":
        return ROOT / directory / species_directory(constant)
    if prefix == "OBJ_CODE_":
        return field_actor_directory(ROOT / directory, constant)
    if directory.startswith("res/sound/"):
        return sound_entry_dir(ROOT / "res/sound", directory.removeprefix("res/sound/"), constant)
    return ROOT / directory / (constant.removeprefix(prefix).lower() + ("" if suffix == "/" else suffix))


def plan(old: str, new: str, lists: dict) -> tuple[list, list]:
    """Returns what renaming one constant does: the files and directories it moves, and the text it rewrites, as
    (pattern, replacement, the text a file must hold for the pattern to apply)."""
    if not new.isidentifier():
        sys.exit(f"{new} is not a valid name")
    if not any(old in names for names in lists.values()):
        sys.exit(f"{old} is in no list in {LISTS_DIR.relative_to(ROOT)}")
    if any(new in names for names in lists.values()):
        sys.exit(f"{new} is already a constant")
    moves = []
    for list_name, directory, prefix, suffix in NAMED_DATA:
        if old in lists[LISTS_DIR / f"{list_name}.txt"]:
            old_path, new_path = (data_path(c, directory, prefix, suffix) for c in (old, new))
            if old_path.exists():
                if not new.startswith(prefix):
                    sys.exit(f"{new} must start with {prefix}, since its data is named after it")
                moves.append((old_path, new_path))

    # An item icon's file is named after its constant, in its item's directory or res/items/shared/
    if old in lists[LISTS_DIR / "item_icons.txt"]:
        old_path, new_path = (item_icon_path(ROOT / "res/items", c) for c in (old, new))
        if old_path.exists() and old_path != new_path:
            if not new_path.parent.is_dir():
                sys.exit(f"{new_path.parent.relative_to(ROOT)} doesn't exist: an icon's name must be an item's")
            moves.append((old_path, new_path))

    for old_path, new_path in moves:
        if new_path.exists():
            sys.exit(f"{new_path.relative_to(ROOT)} already exists")

    # A message file's IDs start with its name, and the code includes its header by the file's name
    renames = [(re.compile(rf"\b{re.escape(old)}\b"), new, old)]
    # An item's icons are named after it and are files of its directory (item_icon_data.py), so they follow it
    if old in lists[LISTS_DIR / "items.txt"]:
        old_item, new_item = old.removeprefix("ITEM_"), new.removeprefix("ITEM_")
        renames.append((re.compile(rf"\b(ITEM_ICON_(?:PALETTE_)?){re.escape(old_item)}(_LAUNCHER)?\b"),
                        rf"\g<1>{new_item}\g<2>", old_item))
    for old_path, new_path in moves:
        if old_path.parent.parent == ROOT / "res/text":
            renames.append((re.compile(rf"\b{re.escape(bank_prefix(old_path))}(?=\w)"), bank_prefix(new_path),
                            bank_prefix(old_path)))
            header = f"text/{old_path.parent.name}/"
            renames.append((re.compile(rf"(?<=[\"<]){re.escape(header + old_path.stem)}\.h(?=[\">])"),
                            f"{header}{new_path.stem}.h", header + old_path.stem + ".h"))
    return moves, renames


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("old", nargs="?")
    parser.add_argument("new", nargs="?")
    parser.add_argument("--batch", type=Path, help="a file of renames, OLD NEW a line, done in one pass")
    args = parser.parse_args()
    if args.batch:
        pairs = [line.split() for line in args.batch.read_text().splitlines() if line.strip() and not line.startswith("#")]
        if any(len(pair) != 2 for pair in pairs):
            sys.exit(f"{args.batch}: each line is OLD NEW")
    elif args.old and args.new:
        pairs = [[args.old, args.new]]
    else:
        parser.error("give OLD NEW or --batch FILE")
    olds, news = [old for old, _ in pairs], [new for _, new in pairs]
    if len(set(olds)) != len(olds) or len(set(news)) != len(news) or set(olds) & set(news):
        sys.exit("each constant is renamed once, to a name no other rename uses or frees")
    lists = {path: parse(path) for path in sorted(LISTS_DIR.glob("*.txt"))}
    moves, renames = [], []
    for old, new in pairs:
        pair_moves, pair_renames = plan(old, new, lists)
        moves += pair_moves
        renames += pair_renames
    targets = [new_path for _, new_path in moves]
    if len(set(targets)) != len(targets):
        sys.exit("two renames would move files to one path")

    # Only the files that hold an old name can change: git grep finds them in one pass
    searches = sorted({needle for _, _, needle in renames})
    found = subprocess.run(["git", "grep", "-lzF", *(arg for needle in searches for arg in ("-e", needle)), "--",
                            *PATHS], cwd=ROOT, capture_output=True, text=True).stdout.split("\0")
    changed = 0
    for name in filter(None, found):
        path = ROOT / name
        if path.suffix not in SUFFIXES or not path.is_file():
            continue
        text = path.read_text()
        new_text = text
        for pattern, replacement, needle in renames:
            if needle in new_text:
                new_text = pattern.sub(replacement, new_text)
        if new_text != text:
            path.write_text(new_text)
            changed += 1
    for old_path, new_path in moves:
        subprocess.run(["git", "mv", str(old_path), str(new_path)], cwd=ROOT, check=True)
        print(f"moved {old_path.relative_to(ROOT)} to {new_path.relative_to(ROOT)}")
    # The models' GLBs show their textures by path, so a moved texture set or prop bundle needs them pointed at it
    relinks = {(script, models) for old_path, _ in moves for directory, script, models in RELINKS
               if old_path.parent == ROOT / directory}
    for script, models in sorted(relinks):
        subprocess.run([sys.executable, str(ROOT / script), "relink", str(ROOT / models)], cwd=ROOT, check=True)
    print(f"renamed {len(pairs)} constant{'s' if len(pairs) > 1 else ''} in {changed} files")


if __name__ == "__main__":
    main()
