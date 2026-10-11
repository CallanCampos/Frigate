#!/usr/bin/env python3
"""The item data in data/items/, one JSON file per item: dump writes them from the ROM once, and pack builds the
archive (a/0/2/4) from them, which the build does.

    item_data.py dump extract/b2_us/files data/items
    item_data.py pack data/items ARCHIVE

Each item is data/items/<item>/data.json, named after its constant (master_ball/ for ITEM_MASTER_BALL), and the archive
has them in the order of data/constants/items.txt. An item's file also holds its name, plural and description, which
the text files take with \\from{items.name} and the like (see text_sources.py). item.schema.json describes each field.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_data import message_lines  # noqa: E402
from tools.text.text_sources import item_article, to_json  # noqa: E402

HEADER = struct.Struct("<HBBBBBBHBBBBBB")
RECORD_SIZE = 36
WORK = slice(HEADER.size, RECORD_SIZE)
WORK_VALUE, WORK_PARAMS = 0, 1
NO_TYPE = 31
# Files of the system messages (a/0/0/2) with the items' names, names with an article, plurals and descriptions
NAMES, NAMES_WITH_ARTICLE, PLURALS, DESCRIPTIONS = 64, 481, 482, 63
# The bitfields of ItemParams: name, byte of the work, first bit, width
BITS = [
    ("sleep_heal", 0, 0, 1), ("poison_heal", 0, 1, 1), ("burn_heal", 0, 2, 1), ("freeze_heal", 0, 3, 1),
    ("paralysis_heal", 0, 4, 1), ("confusion_heal", 0, 5, 1), ("infatuation_heal", 0, 6, 1), ("guard_spec", 0, 7, 1),
    ("revive", 1, 0, 1), ("revive_all", 1, 1, 1), ("level_up", 1, 2, 1), ("evolve", 1, 3, 1),
    ("attack_stages", 1, 4, 4), ("defense_stages", 2, 0, 4), ("sp_attack_stages", 2, 4, 4),
    ("sp_defense_stages", 3, 0, 4), ("speed_stages", 3, 4, 4), ("accuracy_stages", 4, 0, 4), ("crit_stages", 4, 4, 2),
    ("pp_up", 4, 6, 1), ("pp_max", 4, 7, 1), ("pp_restore", 5, 0, 1), ("pp_restore_all", 5, 1, 1),
    ("hp_restore", 5, 2, 1), ("hp_ev_up", 5, 3, 1), ("attack_ev_up", 5, 4, 1), ("defense_ev_up", 5, 5, 1),
    ("speed_ev_up", 5, 6, 1), ("sp_attack_ev_up", 5, 7, 1), ("sp_defense_ev_up", 6, 0, 1),
    ("friendship_up_1", 6, 1, 1), ("friendship_up_2", 6, 2, 1), ("friendship_up_3", 6, 3, 1), ("unk6_4", 6, 4, 1),
]
# The bytes after the bitfields, and whether each is signed
BYTES = [("hp_ev", True), ("attack_ev", True), ("defense_ev", True), ("speed_ev", True), ("sp_attack_ev", True),
         ("sp_defense_ev", True), ("hp_restore_amount", False), ("pp_restore_amount", False), ("friendship_1", True),
         ("friendship_2", True), ("friendship_3", True)]
BITS_SIZE = 7


def directory(item: str) -> str:
    return item.removeprefix("ITEM_").lower()


def ordered_items() -> list[str]:
    """The items in ID order, by their first constant: ITEM_LAST, a second name of the last item, is left out."""
    items: dict[int, str] = {}
    for item, number in load_list("items").items():
        items.setdefault(number, item)
    if sorted(items) != list(range(len(items))):
        sys.exit("items.txt doesn't number the items from 0 without gaps")
    return [items[number] for number in range(len(items))]


# Dumping


def effects_json(work: bytes) -> dict:
    effects = {}
    for field, byte, bit, width in BITS:
        bits = work[byte] >> bit & ((1 << width) - 1)
        effects[field] = bool(bits) if width == 1 else bits
    for i, (field, signed) in enumerate(BYTES):
        effects[field] = struct.unpack_from("<b" if signed else "<B", work, BITS_SIZE + i)[0]
    return effects


def item_json(record: bytes) -> dict:
    (price, hold_effect, hold_param, pluck, fling, fling_power, gift_power, bits, field_func, battle_func, work_type,
     kind, unk_e, sort_index) = HEADER.unpack_from(record)
    work = record[WORK]
    data = {
        "price": price,
        "hold_effect": name("HOLD_EFFECT_", hold_effect),
        "hold_param": hold_param,
        "pluck_effect": pluck,
        "fling_effect": fling,
        "fling_power": fling_power,
        "natural_gift_power": gift_power,
        "natural_gift_type": None if bits & 0x1F == NO_TYPE else name("TYPE_", bits & 0x1F),
        "important": bool(bits >> 5 & 1),
        "registrable": bool(bits >> 6 & 1),
        "field_pocket": name("BAG_POCKET_", bits >> 7 & 0xF),
        "battle_pocket": [name("BATTLE_POCKET_", 1 << bit) for bit in range(5) if bits >> 11 >> bit & 1],
        "field_func": field_func,
        "battle_func": battle_func,
        "kind": kind,
        "unk_e": unk_e,
        "sort_index": sort_index,
    }
    if work_type == WORK_VALUE and not any(work[1:]):
        data["value"] = work[0]
    elif work_type == WORK_PARAMS and not any(work[BITS_SIZE + len(BYTES):]):
        data["effects"] = effects_json(work)
    else:
        raise ValueError("an item's work isn't a value or effects")
    return data


def dump(files: Path, output: Path):
    members = read_narc((files / "a/0/2/4").read_bytes())
    items = ordered_items()
    if len(members) != len(items):
        sys.exit(f"{len(members)} items in the archive and {len(items)} in items.txt")
    names, with_article, plurals, descriptions = (message_lines(files / "a/0/0/2", n)
                                                  for n in (NAMES, NAMES_WITH_ARTICLE, PLURALS, DESCRIPTIONS))
    for index, (item, member) in enumerate(zip(items, members)):
        text = {"name": to_json(names[index])}
        # ITEM_NONE's other names are "???"
        if index:
            match = re.fullmatch(r"\{bd01\}(?:(.+) )?\{ff00:255\}(.*)", with_article[index])
            if not match or match[2] != names[index]:
                text["name_with_article"] = to_json(with_article[index])
            elif (match[1] or "") != item_article(text):
                text["name_article"] = match[1] or ""
        text["name_plural"] = to_json(plurals[index])
        text["description"] = to_json(descriptions[index])
        write(output / directory(item) / "data.json", {"$schema": "../item.schema.json", **text, **item_json(member)})
    print(f"wrote {len(members)} items to {output}")


# Packing


def item_bytes(data: dict, where: str) -> bytes:
    if ("value" in data) == ("effects" in data):
        raise DataError(f"{where}: an item has either value or effects")
    gift_type = NO_TYPE if data["natural_gift_type"] is None else value(data["natural_gift_type"], where)
    bits = (gift_type | data["important"] << 5 | data["registrable"] << 6 | value(data["field_pocket"], where) << 7
            | sum(value(pocket, where) for pocket in data["battle_pocket"]) << 11)
    if "value" in data:
        work_type, work = WORK_VALUE, bytes([data["value"]]) + bytes(RECORD_SIZE - HEADER.size - 1)
    else:
        effects = data["effects"]
        flags = bytearray(BITS_SIZE)
        for field, byte, bit, width in BITS:
            flags[byte] |= int(effects[field]) << bit
        rest = b"".join(struct.pack("<b" if signed else "<B", effects[field]) for field, signed in BYTES)
        work_type, work = WORK_PARAMS, bytes(flags) + rest
        work += bytes(RECORD_SIZE - HEADER.size - len(work))
    return HEADER.pack(
        data["price"], value(data["hold_effect"], where), data["hold_param"], data["pluck_effect"],
        data["fling_effect"], data["fling_power"], data["natural_gift_power"], bits, data["field_func"],
        data["battle_func"], work_type, data["kind"], data["unk_e"], data["sort_index"],
    ) + work


def pack(root: Path, output: Path):
    schema = load_schema(root / "item.schema.json")
    members = []
    for item in ordered_items():
        path = root / directory(item) / "data.json"
        members.append(item_bytes(load(path, schema), label(path)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/items/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the item data archive from data/items/")
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
