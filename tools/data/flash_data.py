#!/usr/bin/env python3
"""The zones where Flash lights the way, in res/field/flash/flash.json: dump writes it from the ROM once, and pack
builds ARCID_MAPEFF_SKILL_TBL (a/1/4/9) from it, which the build does.

    flash_data.py dump extract/b2_us/files res/field/flash
    flash_data.py pack res/field/flash ARCHIVE

The archive's one file is a table of zones and their Flash flags (ZoneValue), which GetZoneFlashFlags looks a zone up
in (zonedata.c), 0 for a zone it doesn't list: whether Flash can be used there (FLD_FLASH_ALLOW), which
GameData_UpdateFlashStatus turns into FLD_FLASH_ACTIVE once it has been, and FLD_FLASH_ACTIVE itself, which no zone
has. A zone with neither forgets that Flash was used (ZoneChange_OnMapChange). Zones are their ZONE_* constants.

Both versions have the same file. flash.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/1/4/9"
ENTRY = struct.Struct("<HH")  # ZoneValue
FLAGS = {"allow": 0x1, "active": 0x2}  # FLD_FLASH_ALLOW, FLD_FLASH_ACTIVE in include/field/field_status.h


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != 1 or len(members[0]) % ENTRY.size:
        raise ValueError("ARCID_MAPEFF_SKILL_TBL isn't one file of zones and flags")
    zones = []
    for zone, flags in ENTRY.iter_unpack(members[0]):
        if flags & ~sum(FLAGS.values()):
            raise ValueError(f"a zone's Flash flags {flags:#x} aren't FLD_FLASH_ALLOW and FLD_FLASH_ACTIVE")
        zones.append({"zone": name("ZONE_", zone), **{key: bool(flags & bit) for key, bit in FLAGS.items()}})
    write(output / "flash.json", {"$schema": "flash.schema.json", "zones": zones})
    print(f"wrote {len(zones)} zones to {output / 'flash.json'}")


def pack(root: Path, output: Path):
    path = root / "flash.json"
    zones = load(path, load_schema(root / "flash.schema.json"))["zones"]
    data = b"".join(ENTRY.pack(value(entry["zone"], f"{label(path)}: zone {i}"),
                               sum(bit for key, bit in FLAGS.items() if entry[key]))
                    for i, entry in enumerate(zones))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([data]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write flash.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build ARCID_MAPEFF_SKILL_TBL from flash.json")
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
