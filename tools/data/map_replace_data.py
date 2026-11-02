#!/usr/bin/env python3
"""The map replacements in res/field/matrices/replacements.json: dump writes it from the ROM once, and pack builds
ARCID_MAP_REPLACE (a/0/1/0) from it, which the build does.

    map_replace_data.py dump extract/b2_us/files res/field/matrices/replacements.json
    map_replace_data.py pack res/field/matrices/replacements.json ARCHIVE

The archive's one file is a list of entries (MapReplaceEntry, map_replace.c) that MapMatrix_Patch applies to a map
matrix as the field loads it: each swaps one map model of the matrix for another, or the whole matrix for another, by
the season, the version or an event. Matrices and map models are their MAP_MATRIX_* and MAP_MODEL_* constants.

Both versions have the same file. replacements.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/0/1/0"
SCHEMA = "replacements.schema.json"

VALUE_COUNT = 6
ENTRY = struct.Struct(f"<HBB{VALUE_COUNT}H")  # MapReplaceEntry
KINDS = ["map_model", "matrix"]  # what MapReplace_ResolvePatch's 1 and 2 have MapReplace_Patch replace
PREFIXES = {"map_model": "MAP_MODEL_", "matrix": "MAP_MATRIX_"}
# What MapReplaceVariables gives each condition's values[] index, from 0, and the condition's name; a condition from 3
# on is the event of EVENT_MAP_REPLACE_TABLE with that condition, whose index is whether it happened
CONDITIONS = {0: ("season", ["spring", "summer", "autumn", "winter"]),
              1: ("version", ["black", "white"]),
              2: ("version_season", ["black", "white_spring", "white_summer", "white_autumn", "white_winter"])}
EVENT_KEYS = ["unset", "set"]


def keys(condition) -> list[str]:
    return EVENT_KEYS if isinstance(condition, int) else next(k for n, k in CONDITIONS.values() if n == condition)


def entry_json(values: tuple) -> dict:
    matrix, kind, condition, *slots = values
    if kind >= len(KINDS):
        raise ValueError(f"a map replacement's kind {kind} isn't one MapReplace_ResolvePatch knows")
    kind = KINDS[kind]
    condition = CONDITIONS[condition][0] if condition in CONDITIONS else condition
    used = keys(condition)
    return {"matrix": name("MAP_MATRIX_", matrix), "kind": kind, "condition": condition,
            "values": {key: name(PREFIXES[kind], v) for key, v in zip(used, slots)}, "unused": slots[len(used):]}


def entry_bytes(entry: dict, where: str) -> bytes:
    condition = entry["condition"]
    used = keys(condition)
    if list(entry["values"]) != used:
        raise DataError(f"{where}: a {condition} condition's values are {', '.join(used)}, in that order")
    if len(used) + len(entry["unused"]) != VALUE_COUNT:
        raise DataError(f"{where}: values and unused make {VALUE_COUNT} slots")
    number = condition if isinstance(condition, int) else next(c for c, (n, _) in CONDITIONS.items() if n == condition)
    slots = [value(entry["values"][key], where) for key in used] + entry["unused"]
    return ENTRY.pack(value(entry["matrix"], where), KINDS.index(entry["kind"]), number, *slots)


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != 1 or len(members[0]) % ENTRY.size:
        raise ValueError("ARCID_MAP_REPLACE isn't one file of entries")
    entries = [entry_json(values) for values in ENTRY.iter_unpack(members[0])]
    write(output, {"$schema": SCHEMA, "replacements": entries})
    print(f"wrote {len(entries)} map replacements to {output}")


def pack(source: Path, output: Path):
    entries = load(source, load_schema(source.parent / SCHEMA))["replacements"]
    data = b"".join(entry_bytes(entry, f"{label(source)}: entry {i}") for i, entry in enumerate(entries))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([data]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write replacements.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build ARCID_MAP_REPLACE from replacements.json")
    pack_parser.add_argument("source", type=Path)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.source, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
