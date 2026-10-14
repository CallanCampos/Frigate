#!/usr/bin/env python3
"""The areas in data/areas/<number>.json: dump writes them from the ROM once, and pack builds the file (a/0/1/3)
from them, which the build does.

    area_data.py dump extract/b2_us/files data/areas
    area_data.py pack data/areas FILE

The file is an array of 10-byte records (AreaData), in order: a zone's header names its area by its number, and an area
with seasons has a record per season after it. An area names its texture set by its MAP_TEXTURE_SET_* constant and its
props by a FIELD_PROPS_EXTERIOR_* or FIELD_PROPS_INTERIOR_* one, as it is exterior or not, or by number where the archive
has no such file. area.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402

RECORD = struct.Struct("<HHBBBBBB")
NO_ANIMATION = 0xFF  # AREA_ANM_NONE
FIELDS = ["prop_bundle", "texture_set", "srt_animation", "pattern_animation", "exterior", "lights", "edge_color_table",
          "actor_material_color"]


def props_prefix(exterior: bool) -> str:
    return "FIELD_PROPS_EXTERIOR_" if exterior else "FIELD_PROPS_INTERIOR_"


def dump(files: Path, output: Path):
    data = (files / "a/0/1/3").read_bytes()
    if len(data) % RECORD.size:
        sys.exit("the file isn't whole records")
    for i, values in enumerate(RECORD.iter_unpack(data)):
        area = dict(zip(FIELDS, values))
        for key in ("srt_animation", "pattern_animation"):
            area[key] = None if area[key] == NO_ANIMATION else area[key]
        if area["exterior"] > 1:
            sys.exit("an area's exterior isn't 0 or 1")
        area["exterior"] = bool(area["exterior"])
        area["prop_bundle"] = name(props_prefix(area["exterior"]), area["prop_bundle"])
        area["texture_set"] = name("MAP_TEXTURE_SET_", area["texture_set"])
        write(output / f"{i:03d}.json", {"$schema": "area.schema.json", **area})
    print(f"wrote {len(data) // RECORD.size} areas to {output}")


def pack(root: Path, output: Path):
    schema = load_schema(root / "area.schema.json")
    paths = sorted(path for path in root.glob("*.json") if not path.name.endswith(".schema.json"))
    if [path.name for path in paths] != [f"{i:03d}.json" for i in range(len(paths))]:
        raise DataError(f"{label(root)}: areas must be numbered from 000 without gaps")
    data = b""
    for path in paths:
        area = load(path, schema)
        values = dict(area)
        for key in ("srt_animation", "pattern_animation"):
            values[key] = NO_ANIMATION if values[key] is None else values[key]
        if isinstance(area["prop_bundle"], str) and not area["prop_bundle"].startswith(props_prefix(area["exterior"])):
            raise DataError(f"{label(path)}: an {'exterior' if area['exterior'] else 'interior'} area's props are "
                            f"{props_prefix(area['exterior'])}*")
        for key in ("prop_bundle", "texture_set"):
            values[key] = value(area[key], label(path))
        data += RECORD.pack(*(int(values[key]) for key in FIELDS))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/areas/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the area file from data/areas/")
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
