#!/usr/bin/env python3
"""The zones' fog in res/field/fog/fog.json: dump writes it from the ROM once, and pack builds ARCID_ZONE_FOG_TABLE
(a/0/8/0) from it, which the build does.

    zone_fog_data.py dump extract/b2_us/files res/field/fog
    zone_fog_data.py pack res/field/fog ARCHIVE

The archive's files are the fogs, two words each that FieldFogCtrl loads (field_zonefog.c) and the sunny weather sets
the fog with (weather_sunny.c): its offset, the depth where it starts, and its slope, WeatherTask_InitFog's depth
shift. Its last file, ZONE_FOG_TABLE_FILE in zonedata.c, is the table of the zones that have a fog, a ZoneValue each,
the zone and its fog's file, which GetZoneFogIndex searches. The fog.json file has the fogs in order, numbered by
their place, and the table, with zones as their ZONE_* constants.

Both versions have the same fog. fog.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/0/8/0"
FOG = struct.Struct("<II")
ZONE_VALUE = struct.Struct("<HH")  # ZoneValue
TABLE_FILE = 29  # ZONE_FOG_TABLE_FILE


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    *fogs, table = members
    if len(fogs) != TABLE_FILE or any(len(data) != FOG.size for data in fogs) or len(table) % ZONE_VALUE.size:
        raise ValueError("the fog archive isn't 29 fogs and a table of zones")
    fogs = [FOG.unpack(data) for data in fogs]
    zones = list(ZONE_VALUE.iter_unpack(table))
    if any(offset > 0x7fff or slope > 10 for offset, slope in fogs) or any(fog >= TABLE_FILE for _, fog in zones):
        raise ValueError("unexpected fog values")
    write(output / "fog.json", {
        "$schema": "fog.schema.json", "fogs": [{"offset": offset, "slope": slope} for offset, slope in fogs],
        "zones": [{"zone": name("ZONE_", zone), "fog": fog} for zone, fog in zones]})
    print(f"wrote {len(fogs)} fogs and {len(zones)} zones to {output / 'fog.json'}")


def pack(root: Path, output: Path):
    path = root / "fog.json"
    data = load(path, load_schema(root / "fog.schema.json"))
    if len(data["fogs"]) != TABLE_FILE:
        raise DataError(f"{label(path)}: {len(data['fogs'])} fogs, but the code reads the table from file {TABLE_FILE}")
    for i, entry in enumerate(data["zones"]):
        if entry["fog"] >= len(data["fogs"]):
            raise DataError(f"{label(path)}: zones[{i}]: there is no fog {entry['fog']}")
    members = [FOG.pack(fog["offset"], fog["slope"]) for fog in data["fogs"]]
    members.append(b"".join(ZONE_VALUE.pack(value(entry["zone"], f"{label(path)}: zones[{i}]"), entry["fog"])
                            for i, entry in enumerate(data["zones"])))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/fog/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the fog archive from res/field/fog/")
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
