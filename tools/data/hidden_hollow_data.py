#!/usr/bin/env python3
"""What the Hidden Grottoes hold, in res/field/hidden_hollows/, one JSON file per grotto: dump writes them from the ROM
once, and pack builds ARCID_SCRCMD_HIDDEN_HOLLOW (a/2/7/3) from them, which the build does.

    hidden_hollow_data.py dump extract/b2_us/files res/field/hidden_hollows
    hidden_hollow_data.py pack res/field/hidden_hollows ARCHIVE

<n>.json is grotto n's HollowFile (scrcmd_hidden_hollow.c), which Scrcmd_HiddenHollow_HiddenHollowGetParam reads
for the slot and group that the grotto rolled (hidden_hollow_generate.c): the Pokémon of slots 0 to 2, a HollowSlot
each, in White 2 and in Black 2 (HOLLOW_FILE_VERSION), the items in balls of slots 3 to 6 and the hidden items of
slots 7 to 10, each of these four values, one per group. Species and items are their SPECIES_* and ITEM_* constants.

Both versions have the same files. hidden_hollow.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import LISTS_DIR, parse  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/7/3"
GROUPS = 4
SLOT = struct.Struct(f"<{GROUPS}H{GROUPS}B{GROUPS}B{GROUPS}B{GROUPS}B2B")  # HollowSlot
SLOT_FIELDS = ["species", "max_level", "min_level", "female_below", "form"]
VERSIONS = ["white2", "black2"]  # HOLLOW_FILE_VERSION 0 and 1
POKEMON_SLOTS = 3
ITEM_SLOTS = 4
ITEMS = struct.Struct(f"<{ITEM_SLOTS * GROUPS}H")
FILE_SIZE = len(VERSIONS) * POKEMON_SLOTS * SLOT.size + 2 * ITEMS.size  # HollowFile


def slot_json(values) -> dict:
    fields = {field: list(values[GROUPS * i:GROUPS * (i + 1)]) for i, field in enumerate(SLOT_FIELDS)}
    fields["species"] = [name("SPECIES_", species) for species in fields["species"]]
    return {**fields, "unk18": list(values[GROUPS * len(SLOT_FIELDS):])}


def slot_bytes(slot: dict, where: str) -> bytes:
    return SLOT.pack(*(value(species, where) for species in slot["species"]),
                     *(level for field in SLOT_FIELDS[1:] for level in slot[field]), *slot["unk18"])


def hollow_json(data: bytes, items: dict[int, str]) -> dict:
    slots = [slot_json(SLOT.unpack_from(data, SLOT.size * i)) for i in range(len(VERSIONS) * POKEMON_SLOTS)]
    if any(low > high for slot in slots for low, high in zip(slot["min_level"], slot["max_level"])):
        raise ValueError("a grotto's minimum level is above its maximum")
    offset = SLOT.size * len(slots)
    lists = [ITEMS.unpack_from(data, offset + ITEMS.size * i) for i in range(2)]
    item_slots = [[[items.get(item, item) for item in group[GROUPS * s:GROUPS * (s + 1)]] for s in range(ITEM_SLOTS)]
                  for group in lists]
    pokemon = {version: slots[POKEMON_SLOTS * v:POKEMON_SLOTS * (v + 1)] for v, version in enumerate(VERSIONS)}
    return {"$schema": "hidden_hollow.schema.json", "pokemon": pokemon, "items": item_slots[0],
            "hidden_items": item_slots[1]}


def hollow_bytes(hollow: dict, where: str) -> bytes:
    return (b"".join(slot_bytes(slot, where) for version in VERSIONS for slot in hollow["pokemon"][version]) +
            b"".join(ITEMS.pack(*(value(item, where) for slot in hollow[kind] for item in slot))
                     for kind in ("items", "hidden_items")))


def dump(files: Path, output: Path):
    hollows = read_narc((files / ARCHIVE).read_bytes())
    if any(len(data) != FILE_SIZE for data in hollows):
        raise ValueError("a grotto's file isn't a HollowFile")
    # By generated/items.txt alone, as the item icons' ITEM_ICON_* constants share the prefix
    items = {number: item for item, number in reversed(parse(LISTS_DIR / "items.txt").items())}
    for i, data in enumerate(hollows):
        write(output / f"{i:02d}.json", hollow_json(data, items))
    print(f"wrote {len(hollows)} grottoes to {output}")


def pack(root: Path, output: Path):
    paths = sorted(root.glob("[0-9]*.json"), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(len(paths))):
        raise DataError(f"{label(root)}: the grottoes are numbered from 00 without gaps")
    schema = load_schema(root / "hidden_hollow.schema.json")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([hollow_bytes(load(path, schema), label(path)) for path in paths]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/hidden_hollows/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the grottoes' archive from res/field/hidden_hollows/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
