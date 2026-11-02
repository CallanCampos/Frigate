#!/usr/bin/env python3
"""The battle bag's pockets of each item (ARCID_B_BAG_ITEM, a/2/2/8), built from the items' data in res/items/: pack
builds the archive, which the build does, and check compares the original with the items' data.

    battle_bag_data.py check extract/b2_us/files res/items
    battle_bag_data.py pack res/items ARCHIVE

The archive's one file is a byte for each item, in the order of the item data, whose bits are the battle pockets
that BBagItem_MakePocketLists (b_bag_item.c) shows it in: the item's battle_pocket in its data.json, which is also
the item data's ITEM_PARAM_BATTLE_POCKET, so the items' data is its source. Unlike most, the archive names its file,
battle_pocket.dat.

Both versions have the same file.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, value  # noqa: E402
from tools.data.item_data import directory, ordered_items  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

FILE_NAME = b"battle_pocket.dat"  # the original's name for its one file, in its file name table


def pockets(root: Path) -> bytes:
    schema = load_schema(root / "item.schema.json")
    data = bytearray()
    for item in ordered_items():
        path = root / directory(item) / "data.json"
        bits = sum(value(pocket, label(path)) for pocket in set(load(path, schema)["battle_pocket"]))
        if bits > 0xFF:
            raise DataError(f"{label(path)}: a battle pocket past the byte's bits")
        data.append(bits)
    return bytes(data)


def named_narc(data: bytes) -> bytes:
    """A NARC of the one file under its name: write_narc's with a file name table that names it, as the original's."""
    narc = write_narc([data])
    start = narc.index(b"BTNF")
    names = struct.pack("<IHH", 8, 0, 1) + bytes([len(FILE_NAME)]) + FILE_NAME + b"\0"
    names += b"\xff" * (-len(names) % 4)
    fnt = struct.pack("<4sI", b"BTNF", 8 + len(names)) + names
    narc = narc[:start] + fnt + narc[start + 16:]
    return narc[:8] + struct.pack("<I", len(narc)) + narc[12:]


def check(files: Path, root: Path):
    (original,) = read_narc((files / "a/2/2/8").read_bytes())
    if original != pockets(root):
        sys.exit("error: the battle bag's pockets differ from the items' data")
    print(f"the battle bag's pockets of {len(original)} items are the items' data")


def pack(root: Path, output: Path):
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(named_narc(pockets(root)))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    check_parser = commands.add_parser("check", help="compare the original with res/items/")
    check_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    check_parser.add_argument("root", type=Path)
    pack_parser = commands.add_parser("pack", help="build the battle bag's pockets from res/items/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    try:
        if args.command == "check":
            check(args.files, args.root)
        else:
            pack(args.root, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
