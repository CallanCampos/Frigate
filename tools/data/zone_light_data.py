#!/usr/bin/env python3
"""The lights that zones have instead of their area's, in res/lighting/zones/: dump writes them from the ROM once, and
pack builds ARCID_ZONE_LIGHT_TABLE (a/0/8/1) from them, which the build does.

    zone_light_data.py dump extract/b2_us/files res/lighting/zones
    zone_light_data.py pack res/lighting/zones ARCHIVE

The archive's files are lighting sets, as res/lighting/field/'s (light_data.py), and its last, ZONE_LIGHT_TABLE_FILE
in zonedata.c, is the table of the zones that have one, a ZoneValue each, the zone and its set's file, which
GetZoneStaticLightDataIndex searches: FieldLightStatus_Get and the sunny weather (weather_sunny.c) light such a zone
with its set. <n>.json is set n and zones.json the table, with zones as their ZONE_* constants. The table is empty in
both versions, so no zone uses the one set.

Both versions have the same lights. The sets follow ../lights.schema.json and the table zones.schema.json.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.light_data import set_bytes, set_json  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/0/8/1"
ZONE_VALUE = struct.Struct("<HH")  # ZoneValue
TABLE_FILE = 1  # ZONE_LIGHT_TABLE_FILE


def dump(files: Path, output: Path):
    *sets, table = read_narc((files / ARCHIVE).read_bytes())
    if len(sets) != TABLE_FILE or len(table) % ZONE_VALUE.size:
        raise ValueError("the zone light archive isn't a lighting set and a table of zones")
    for i, data in enumerate(sets):
        write(output / f"{i:03d}.json", set_json(data))
    zones = [{"zone": name("ZONE_", zone), "lights": lights} for zone, lights in ZONE_VALUE.iter_unpack(table)]
    write(output / "zones.json", {"$schema": "zones.schema.json", "zones": zones})
    print(f"wrote {len(sets)} lighting sets and {len(zones)} zones to {output}")


def pack(root: Path, output: Path):
    paths = sorted(root.glob("[0-9]*.json"), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(TABLE_FILE)):
        raise DataError(f"{label(root)}: the sets are 000 to {TABLE_FILE - 1:03d}, as the code reads the table from "
                        f"file {TABLE_FILE}")
    schema = load_schema(root.parent / "lights.schema.json")
    members = [set_bytes(load(path, schema)) for path in paths]
    path = root / "zones.json"
    zones = load(path, load_schema(root / "zones.schema.json"))["zones"]
    for i, entry in enumerate(zones):
        if entry["lights"] >= len(paths):
            raise DataError(f"{label(path)}: zones[{i}]: there is no lighting set {entry['lights']}")
    members.append(b"".join(ZONE_VALUE.pack(value(entry["zone"], f"{label(path)}: zones[{i}]"), entry["lights"])
                            for i, entry in enumerate(zones)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/lighting/zones/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the zone light archive from res/lighting/zones/")
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
