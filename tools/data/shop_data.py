#!/usr/bin/env python3
"""The Poké Marts' lists of items in res/field/shops/shops.json: dump writes it from the ROM once, and pack builds the
lists' archive (ARCID_SHOP_ITEMS, a/2/8/2) and their counts' (ARCID_SHOP_ITEM_COUNTS, a/2/8/3) from it, which the
build does.

    shop_data.py dump extract/b2_us/files res/field/shops
    shop_data.py pack res/field/shops ITEMS COUNTS

ShopUI_LoadPokeMartItems (scrcmd_shop.c) reads a list, its items as u16 ITEM_* IDs, and its count from the counts'
one file, a byte per list. The lists are, in the archive's order:
- badge_marts: the Poké Mart whose stock grows with the badges, the list BADGE_SHOP_DAT_IDS gives each number of
  badges: 0 for none, 1 for one or two, 2 for three or four, 3 for five or six, 4 for seven and 5 for eight;
- marts: the other Poké Marts' lists, by the shop ID that the script's command names (file ID + 6);
- special_marts: the eight marts of SPECIAL_POKE_MART_HEADERS, whose stock grows with the key system's progress, each
  a list per level, all empty in both versions.

Both versions have the same lists. shops.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, value, write  # noqa: E402
from tools.data.gen_constants import LISTS_DIR, parse  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

BADGE_MARTS = 6  # BADGE_SHOP_DAT_IDS' lists
MARTS = 26  # the shop IDs below the special marts' files
SPECIAL_MARTS = 8  # SPECIAL_POKE_MART_HEADERS
SPECIAL_MART_LEVELS = 6  # sSpecialMartLevels
LISTS = BADGE_MARTS + MARTS + SPECIAL_MARTS * SPECIAL_MART_LEVELS


def items_json(data: bytes, items: dict[int, str]) -> list:
    return [items.get(item, item) for item in struct.unpack(f"<{len(data) // 2}H", data)]


def dump(files: Path, output: Path):
    lists = read_narc((files / "a/2/8/2").read_bytes())
    counts = read_narc((files / "a/2/8/3").read_bytes())
    if len(lists) != LISTS or len(counts) != 1 or list(counts[0]) != [len(data) // 2 for data in lists] or any(
            len(data) % 2 for data in lists):
        raise ValueError("the shop lists aren't 80 lists of u16 items with a byte of count each")
    # By generated/items.txt alone, as the item icons' ITEM_ICON_* constants share the prefix
    names = {number: item for item, number in reversed(parse(LISTS_DIR / "items.txt").items())}
    items = [items_json(data, names) for data in lists]
    special = items[BADGE_MARTS + MARTS:]
    special_marts = [special[i:i + SPECIAL_MART_LEVELS] for i in range(0, len(special), SPECIAL_MART_LEVELS)]
    write(output / "shops.json", {"$schema": "shops.schema.json", "badge_marts": items[:BADGE_MARTS],
                                  "marts": items[BADGE_MARTS:BADGE_MARTS + MARTS], "special_marts": special_marts})
    print(f"wrote {LISTS} shop lists to {output / 'shops.json'}")


def pack(root: Path, outputs: list[Path]):
    path = root / "shops.json"
    data = load(path, load_schema(root / "shops.schema.json"))
    lists = [*data["badge_marts"], *data["marts"], *(level for mart in data["special_marts"] for level in mart)]
    if len(lists) != LISTS:
        raise DataError(f"{label(path)}: {len(lists)} lists, not {LISTS}")
    members = [struct.pack(f"<{len(items)}H", *(value(item, f"{label(path)}: list {i}") for item in items))
               for i, items in enumerate(lists)]
    for output, archive in zip(outputs, (members, [bytes(len(items) for items in lists)])):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(archive))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/shops/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the shop lists' archives from res/field/shops/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE",
                             help="the lists' and the counts' archives")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
