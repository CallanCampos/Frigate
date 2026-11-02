#!/usr/bin/env python3
"""What the item collectors pay, in res/field/item_collectors/item_collectors.json: dump writes it from the ROM once,
and pack builds ARCID_ITEM_COLLECTOR (a/2/0/5) from it, which the build does.

    item_collector_data.py dump extract/b2_us/files res/field/item_collectors
    item_collector_data.py pack res/field/item_collectors ARCHIVE

Each of the archive's files is the price list of a group of collectors, an ItemCollectorPrice per item (scrcmd_misc.c),
which ITEM_COLLECTOR_FILES gives each ITEM_COLLECTOR_* group: the script commands look an item's price up in it
(Scrcmd_Misc_ItemCollectorGetPrice, ...Sell and ...WordSetLoadItemCollectorPrice) or check whether the player has any
of its items (Scrcmd_Misc_ItemCollectorCheckGroup). The file has the lists by group, in the archive's order. Items are
their ITEM_* constants.

Both versions have the same lists. item_collectors.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, value, write  # noqa: E402
from tools.data.gen_constants import LISTS_DIR, parse  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/0/5"
PRICE = struct.Struct("<II")  # ItemCollectorPrice
# The groups whose lists the archive's files are, in its order (ITEM_COLLECTOR_FILES)
GROUPS = ["gourmet", "rare_bones", "mulch", "treasures", "minerals"]


def dump(files: Path, output: Path):
    lists = read_narc((files / ARCHIVE).read_bytes())
    if len(lists) != len(GROUPS) or any(len(data) % PRICE.size for data in lists):
        raise ValueError("the item collectors' archive isn't five lists of ItemCollectorPrice")
    # By generated/items.txt alone, as the item icons' ITEM_ICON_* constants share the prefix
    items = {number: item for item, number in reversed(parse(LISTS_DIR / "items.txt").items())}
    write(output / "item_collectors.json", {"$schema": "item_collectors.schema.json", **{
        group: [{"item": items.get(item, item), "price": price} for item, price in PRICE.iter_unpack(data)]
        for group, data in zip(GROUPS, lists)}})
    print(f"wrote {len(GROUPS)} item collectors' lists to {output / 'item_collectors.json'}")


def pack(root: Path, output: Path):
    path = root / "item_collectors.json"
    data = load(path, load_schema(root / "item_collectors.schema.json"))
    members = [b"".join(PRICE.pack(value(entry["item"], f"{label(path)}: {group}[{i}]"), entry["price"])
                        for i, entry in enumerate(data[group])) for group in GROUPS]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/item_collectors/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the item collectors' archive from res/field/item_collectors/")
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
