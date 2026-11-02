#!/usr/bin/env python3
"""The areas' edge colors and actor material colors in res/lighting/, a JSON file per archive: dump writes them from
the ROM once, and pack builds ARCID_AREA_EDGE_COLOR_TABLES (a/0/1/5) and ARCID_FIELD_BBD_COLOR (a/1/7/3) from them,
which the build does.

    area_color_data.py dump extract/b2_us/files res/lighting
    area_color_data.py pack res/lighting EDGE_COLORS ACTOR_COLORS

An area (res/field/areas/) names one of each by number, as it names its lights:
- edge_colors.json is the edge color tables, by an area's "edge_color_table" (255 for none): the eight colors of the
  outlines that the 3D engine draws around the models, by the top three bits of their polygon IDs, which
  Field_LoadEdgeColorTable sets (G3X_SetEdgeColorTable) and whose last it replaces with gray.
- actor_material_colors.json is the billboard actors' material colors, by an area's "actor_material_color": the
  normal they are lit along and their diffuse, ambient, specular and emission colors (FldActMatColorPreset,
  field_bbd_color.c).

Colors are red, green and blue, 0 to 31. Both versions have the same files. edge_colors.schema.json and
actor_material_colors.schema.json describe the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

KINDS = {"edge_colors": "a/0/1/5", "actor_material_colors": "a/1/7/3"}
EDGE_COLORS = 8
EDGE_TABLE = struct.Struct(f"<{EDGE_COLORS}H")  # GXRgb[8]
PRESET = struct.Struct("<3i12H")  # FldActMatColorPreset after its arcTool: the normal, then four colors' components
MATERIALS = ["diffuse", "ambient", "specular", "emission"]
CHANNELS = ["red", "green", "blue"]


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def gx_rgb(value: int) -> dict:
    check(value < 0x8000, "an edge color has bit 15 set")
    return {"red": value & 31, "green": value >> 5 & 31, "blue": value >> 10 & 31}


def gx_rgb_value(color: dict) -> int:
    return color["red"] | color["green"] << 5 | color["blue"] << 10


def edge_json(data: bytes) -> list:
    return [gx_rgb(value) for value in EDGE_TABLE.unpack(data)]


def edge_bytes(colors: list, where: str) -> bytes:
    return EDGE_TABLE.pack(*map(gx_rgb_value, colors))


def preset_json(data: bytes) -> dict:
    x, y, z, *components = PRESET.unpack(data)
    check(all(c < 32 for c in components), "a material color's component isn't 0 to 31")
    return {"normal": {"x": x, "y": y, "z": z},
            **{material: dict(zip(CHANNELS, components[3 * i:3 * i + 3])) for i, material in enumerate(MATERIALS)}}


def preset_bytes(preset: dict, where: str) -> bytes:
    return PRESET.pack(*(preset["normal"][axis] for axis in "xyz"),
                       *(preset[material][channel] for material in MATERIALS for channel in CHANNELS))


CODECS = {"edge_colors": ("tables", edge_json, edge_bytes), "actor_material_colors": ("presets", preset_json,
                                                                                       preset_bytes)}


def dump(files: Path, output: Path):
    for kind, archive in KINDS.items():
        key, to_json, _ = CODECS[kind]
        entries = [to_json(data) for data in read_narc((files / archive).read_bytes())]
        write(output / f"{kind}.json", {"$schema": f"{kind}.schema.json", key: entries})
        print(f"wrote {len(entries)} {key} to {output / f'{kind}.json'}")


def pack(root: Path, outputs: list[Path]):
    for (kind, _), output in zip(KINDS.items(), outputs):
        key, _, to_bytes = CODECS[kind]
        path = root / f"{kind}.json"
        entries = load(path, load_schema(root / f"{kind}.schema.json"))[key]
        members = [to_bytes(entry, f"{label(path)}: entry {i}") for i, entry in enumerate(entries)]
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write the colors' JSON from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the edge colors' and actor material colors' archives")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=len(KINDS), metavar="ARCHIVE",
                             help="the edge color tables' and the actor material colors' archives")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
