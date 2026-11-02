#!/usr/bin/env python3
"""The Join Avenue's tables in res/join_avenue/ (ARCID_RESORT_BINARY, a/2/4/4), a JSON file per table: dump writes them
from the ROM once, and pack builds the archive from them, which the build does.

    join_avenue_data.py dump extract/b2_us/files res/join_avenue
    join_avenue_data.py pack res/join_avenue ARCHIVE

Each of the archive's 13 files is a table of rows of u16 columns, which ResortBinary_Load reads with its number of
columns (system/resort_binary.h), and each JSON file is a list of a table's rows, in order, each column by its name.
A column that the code doesn't read is column_<n>; a group of columns read as a list is a list of objects or numbers.
65535 is none where the code says so. Messages are numbers of SCRIPT_TEXT_GLOBAL_10685, an item of the goods is its
ITEM_* constant when the goods are an item, and an NPC's model and movement are their OBJ_CODE_* and MOVE_CODE_*
constants.

Both versions have the same file. join_avenue.schema.json describes the tables and their columns.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/4/4"
NONE = 0xFFFF

# The tables by file, each its JSON file's name and its columns in order: a name for one column; (name, fields, count)
# for count groups of columns, each an object of the fields or, for one field, the plain values; (name, "s16", count)
# for signed columns; (name, "none", count) for a list ended by 65535s, which it leaves out
TABLES = [
    ("person_scripts", ["person_type", "shop_kind", "gender", "script"]),
    ("shops", ["kind", "variant", "level", "exp_to_next", "message", "script_value", "patch_odd", "patch_even",
               "column_8", ("goods", "none", 16)]),
    ("goods", ["hidden_until_unlocked", "custom_text", "name_message", "description_message", "price", "kind", "item",
               "count", "message_male", "message_female"]),
    ("occupant_places", ["column_0", "x", "z", "entity"]),
    ("person_types", ["name_male", "name_female", "unlock", "tier", "base_value", "column_5",
                      ("conditions", ["condition", "value", "priority", "message_male", "message_female", "unk5"], 6)]),
    ("npcs", ["model", "zone", "x", "z", "rank", "move_code", "area_w", "area_h", "script", "text_row", "column_10"]),
    ("npc_texts", [("messages", ["message"], 19), "column_19"]),
    ("ranks", ["rank_start", "rank_end", ("tier_weights", ["weight"], 3), "popularity_needed", "rank_message",
               "raffle", "raffle_chance", "unlock", "new_entry_chance", "script_value", "column_12"]),
    ("raffles", ["total_weight", "column_1", ("prizes", ["weight", "prize"], 10)]),
    ("visitors", ["shop_kind", "name_message", "trainer_type", "gender", "trainer_id", "shop", "person_type",
                  "greeting_message", "message_1", "message_2", ("queries", ["value"], 13)]),
    ("trainer_type_points", [("days", ["points"], 7)]),
    ("shop_kind_points", [("days", ["points"], 7)]),
    ("visit_groups", ["weight", ("bonus_days", "s16", 7)]),
]
GOODS_ITEM_KIND = 0  # the goods' kind that is an item of the bag (scrcmd_resort_shop.c)
# The columns written by their constants
CONSTANTS = {("npcs", "model"): "OBJ_CODE_", ("npcs", "move_code"): "MOVE_CODE_"}


def width(columns) -> int:
    return sum(1 if isinstance(c, str) else (len(c[1]) if isinstance(c[1], list) else 1) * c[2] for c in columns)


def row_json(table: str, columns, row: list[int]) -> dict:
    entry, i = {}, 0
    for column in columns:
        if isinstance(column, str):
            entry[column] = row[i]
            i += 1
            continue
        key, fields, count = column
        if fields == "s16":
            entry[key] = [v - 0x10000 if v & 0x8000 else v for v in row[i:i + count]]
            i += count
        elif fields == "none":
            values = row[i:i + count]
            used = values.index(NONE) if NONE in values else count
            if any(v != NONE for v in values[used:]):
                raise ValueError(f"{table}: a list has a value after its end")
            entry[key] = values[:used]
            i += count
        else:
            groups = []
            for _ in range(count):
                group = row[i:i + len(fields)]
                groups.append(group[0] if len(fields) == 1 else dict(zip(fields, group)))
                i += len(fields)
            entry[key] = groups
    for (constant_table, column), prefix in CONSTANTS.items():
        if constant_table == table:
            entry[column] = name(prefix, entry[column])
    if table == "goods" and entry["kind"] == GOODS_ITEM_KIND:
        entry["item"] = name("ITEM_", entry["item"])
    return entry


def row_values(columns, entry: dict, where: str) -> list[int]:
    row = []
    for column in columns:
        if isinstance(column, str):
            row.append(value(entry[column], where))
            continue
        key, fields, count = column
        values = entry[key]
        if len(values) > count or (fields != "none" and len(values) != count):
            raise DataError(f"{where}: {key} has {len(values)}, not {count}")
        if fields == "s16":
            row += [v & 0xFFFF for v in values]
        elif fields == "none":
            row += values + [NONE] * (count - len(values))
        else:
            for group in values:
                row += [group] if len(fields) == 1 else [group[field] for field in fields]
    return row


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != len(TABLES):
        raise ValueError(f"{len(members)} tables, not {len(TABLES)}")
    for (table, columns), data in zip(TABLES, members):
        size = 2 * width(columns)
        if len(data) % size:
            raise ValueError(f"{table} isn't rows of {size} bytes")
        rows = [list(struct.unpack_from(f"<{size // 2}H", data, offset)) for offset in range(0, len(data), size)]
        write(output / f"{table}.json", {"$schema": "join_avenue.schema.json",
                                         table: [row_json(table, columns, row) for row in rows]})
    print(f"wrote the {len(TABLES)} tables to {output}")


def pack(root: Path, output: Path):
    schema = load_schema(root / "join_avenue.schema.json")
    members = []
    for table, columns in TABLES:
        path = root / f"{table}.json"
        rows = load(path, schema)[table]
        data = b""
        for i, entry in enumerate(rows):
            row = row_values(columns, entry, f"{label(path)}: row {i}")
            data += struct.pack(f"<{len(row)}H", *row)
        members.append(data)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/join_avenue/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the Join Avenue's tables")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
