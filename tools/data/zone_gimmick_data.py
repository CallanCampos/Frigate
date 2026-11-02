#!/usr/bin/env python3
"""The zones' gimmicks in res/field/zone_gimmicks/zone_gimmicks.json: dump writes it from the ROM once, and pack builds
ARCID_GIMMICK_TBL (a/1/0/2) from it, which the build does.

    zone_gimmick_data.py dump extract/b2_us/files res/field/zone_gimmicks
    zone_gimmick_data.py pack res/field/zone_gimmicks ARCHIVE

The archive's one file is a table of ZoneGimmick (event_mapchange.c), a zone and its gimmick each, which
GameData_SetGimmickByZone searches for the zone the player enters: the field then runs the gimmick, its entry of
GIMMICK_OVERLAY_INFO in field_gimmick.c, with its overlay. A zone that isn't listed has none. Zones are their ZONE_*
constants and gimmicks their numbers in GIMMICK_OVERLAY_INFO.

Both versions have the same table. zone_gimmicks.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/1/0/2"
ZONE_GIMMICK = struct.Struct("<IHH")  # ZoneGimmick and its padding
GIMMICKS = 59  # GIMMICK_OVERLAY_INFO


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != 1 or len(members[0]) % ZONE_GIMMICK.size:
        raise ValueError("the gimmick table isn't one file of ZoneGimmicks")
    entries = list(ZONE_GIMMICK.iter_unpack(members[0]))
    if any(padding or gimmick >= GIMMICKS for _, gimmick, padding in entries):
        raise ValueError("unexpected zone gimmick values")
    write(output / "zone_gimmicks.json", {"$schema": "zone_gimmicks.schema.json", "zones": [
        {"zone": name("ZONE_", zone), "gimmick": gimmick} for zone, gimmick, _ in entries]})
    print(f"wrote {len(entries)} zones' gimmicks to {output / 'zone_gimmicks.json'}")


def pack(root: Path, output: Path):
    path = root / "zone_gimmicks.json"
    zones = load(path, load_schema(root / "zone_gimmicks.schema.json"))["zones"]
    data = b"".join(ZONE_GIMMICK.pack(value(entry["zone"], f"{label(path)}: zones[{i}]"), entry["gimmick"], 0)
                    for i, entry in enumerate(zones))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([data]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/zone_gimmicks/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the gimmick table from res/field/zone_gimmicks/")
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
