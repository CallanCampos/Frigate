#!/usr/bin/env python3
"""The battle backgrounds' tables (ARCID_BATTLE_BG_DATA, a/1/5/1) in res/battle/backgrounds/, a JSON file for each of
the archive's three files: dump writes them from the ROM once, and pack builds the archive from them, which the build
does.

    battle_bg_data.py dump extract/b2_us/files res/battle/backgrounds
    battle_bg_data.py pack res/battle/backgrounds ARCHIVE

- types.json (file 0, BtlvEffectBgData in btlv_effect.c) is each background type, a battle's fieldEnv.bgType (a zone's
  battle_bg): whether the zone's light lights the battle, whether the season picks the variant, and by the battle's
  terrain, the field (the backdrop) and the stage (the ground the Pokémon stand on) it shows. The stage is also the
  terrain that Burmy changes its form for (EventBattleReturn_GetBurmyTerrain).
- fields.json (file 1, BtlvFieldResIds in btlv_field.c) is each field: its model and its three animations, files of
  ARCID_BTLV (a/0/1/1), each for the eight variants, null for none.
- stages.json (file 2, BtlvStageData in btlv_stage.c) is each stage: its edge color and its models and three
  animations, files of ARCID_BTLV, two to a variant, the two sides' grounds, for four variants.

Both versions have the same archive. backgrounds.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/1/5/1"
FILES = ["types", "fields", "stages"]
TERRAINS = 20
TYPE = struct.Struct(f"<BB{TERRAINS}B{TERRAINS}B2B")
VARIANTS = 8
RESOURCES = struct.Struct(f"<{4 * VARIANTS}H")  # the model's row, then the three animations'
STAGE = struct.Struct(f"<H{4 * VARIANTS}H")
NONE = 0xFFFF
# Each file is its entries, padded to a multiple of four bytes
ALIGN = 4


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def entries(data: bytes, entry: struct.Struct) -> list[tuple]:
    count = len(data) // entry.size
    check(len(data) == -(-count * entry.size // ALIGN) * ALIGN and not any(data[count * entry.size:]),
          f"a file of {len(data)} bytes isn't entries of {entry.size} bytes")
    return [entry.unpack_from(data, i * entry.size) for i in range(count)]


def resource(number: int):
    return None if number == NONE else number


def rows(numbers: tuple) -> dict:
    listed = [resource(n) for n in numbers]
    return {"models": listed[:VARIANTS],
            "animations": [listed[VARIANTS * (row + 1):VARIANTS * (row + 2)] for row in range(3)]}


def color_json(color: int) -> dict:
    check(not color >> 15, "an edge color has its top bit set")
    return {"red": color & 31, "green": color >> 5 & 31, "blue": color >> 10 & 31}


def type_json(fields: tuple) -> dict:
    light, seasonal, *rest = fields
    check(light in (0, 1) and seasonal in (0, 1), "a background type's flags aren't 0 or 1")
    return {"light": bool(light), "seasonal": bool(seasonal), "fields": rest[:TERRAINS],
            "stages": rest[TERRAINS:2 * TERRAINS], "unk2a": rest[2 * TERRAINS:]}


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    check(len(members) == len(FILES), f"{ARCHIVE} doesn't have {len(FILES)} files")
    tables = {
        "types": [type_json(e) for e in entries(members[0], TYPE)],
        "fields": [rows(e) for e in entries(members[1], RESOURCES)],
        "stages": [{"edge_color": color_json(e[0]), **rows(e[1:])} for e in entries(members[2], STAGE)],
    }
    for kind, table in tables.items():
        write(output / f"{kind}.json", {"$schema": "backgrounds.schema.json", kind: table})
        print(f"wrote {len(table)} {kind} to {output / f'{kind}.json'}")


def numbers(item: dict) -> list[int]:
    return [NONE if n is None else n for n in item["models"] + [n for row in item["animations"] for n in row]]


def padded(data: bytes) -> bytes:
    return data + bytes(-len(data) % ALIGN)


def pack(root: Path, output: Path):
    schema = load_schema(root / "backgrounds.schema.json")
    tables = {kind: load(root / f"{kind}.json", schema)[kind] for kind in FILES}
    for kind in FILES:
        if not tables[kind]:
            raise DataError(f"{label(root / f'{kind}.json')}: {kind} is empty")
    members = [
        b"".join(TYPE.pack(int(t["light"]), int(t["seasonal"]), *t["fields"], *t["stages"], *t["unk2a"])
                 for t in tables["types"]),
        b"".join(RESOURCES.pack(*numbers(f)) for f in tables["fields"]),
        b"".join(STAGE.pack(s["edge_color"]["red"] | s["edge_color"]["green"] << 5 | s["edge_color"]["blue"] << 10,
                            *numbers(s)) for s in tables["stages"]),
    ]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([padded(member) for member in members]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/battle/backgrounds/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the battle backgrounds' tables")
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
