#!/usr/bin/env python3
"""The lens flares of the zone transitions in res/field/lens_flares/lens_flares.json: dump writes it from the ROM once,
and pack builds ARCID_FIELD_GOOUT_EFFECT_DATA (a/2/4/1) from it, which the build does.

    lens_flare_data.py dump extract/b2_us/files res/field/lens_flares
    lens_flare_data.py pack res/field/lens_flares ARCHIVE

The archive's one file is a table of FieldLensFlareEntry (field_lens_flare.h): when the player goes from one zone into
another in sunny weather or none and without fog, FieldLensFlare_DecideForZoneTransit finds the entry of the two
zones, and the field shows one of the lens flares of its effect set for the season and the time of day
(FieldLensFlare_Create, field_goout_effect.c), an index of LENS_FLARE_IDS in field_goout_effect_data.c, picked at
random. Zones are their ZONE_* constants.

Both versions have the same table. lens_flares.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/4/1"
ENTRY = struct.Struct("<2H12H")  # FieldLensFlareEntry
SEASONS = ["spring", "summer", "autumn", "winter"]
TIMES = ["day", "evening", "night"]  # FieldLensFlare_GetSubIndexForDayPeriod
EFFECT_SETS = 10  # LENS_FLARE_IDS
MAX_ENTRIES = 0x43  # FieldLensFlare_Create uses no entry from this on


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != 1 or len(members[0]) % ENTRY.size:
        raise ValueError("the lens flares' table isn't one file of FieldLensFlareEntry")
    entries = []
    for zone, from_zone, *sets in ENTRY.iter_unpack(members[0]):
        if any(effect_set >= EFFECT_SETS for effect_set in sets):
            raise ValueError("a lens flare entry's effect set isn't one of LENS_FLARE_IDS")
        entries.append({"zone": name("ZONE_", zone), "from_zone": name("ZONE_", from_zone), "effect_sets": {
            season: dict(zip(TIMES, sets[3 * i:3 * i + 3])) for i, season in enumerate(SEASONS)}})
    write(output / "lens_flares.json", {"$schema": "lens_flares.schema.json", "transitions": entries})
    print(f"wrote {len(entries)} zone transitions to {output / 'lens_flares.json'}")


def pack(root: Path, output: Path):
    path = root / "lens_flares.json"
    entries = load(path, load_schema(root / "lens_flares.schema.json"))["transitions"]
    if len(entries) > MAX_ENTRIES:
        raise DataError(f"{label(path)}: {len(entries)} transitions, but the field uses only the first {MAX_ENTRIES}")
    data = b"".join(ENTRY.pack(value(entry["zone"], f"{label(path)}: transitions[{i}]"),
                               value(entry["from_zone"], f"{label(path)}: transitions[{i}]"),
                               *(entry["effect_sets"][season][time] for season in SEASONS for time in TIMES))
                    for i, entry in enumerate(entries))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([data]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/lens_flares/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the lens flares' table from res/field/lens_flares/")
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
