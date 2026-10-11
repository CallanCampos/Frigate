#!/usr/bin/env python3
"""The lighting sets in data/lights/, one JSON file per set: dump writes them from the ROM once, and pack builds the
field and battle lighting archives (a/0/6/0 and a/0/6/1) from them, which the build does.

    light_data.py dump extract/b2_us/files data/lights
    light_data.py pack data/lights FIELD BATTLE

A set is a file of 0x34-byte periods, which FieldLight_FlushCore applies: when it ends, four lights (whether each is on,
its color and its direction), then the materials' diffuse, ambient, specular and emission colors, the fog's and the
clear color. The field's sets are data/lights/field/<n>.json, which areas name by their lights, and the battle's
data/lights/battle/<n>.json, numbered in archive order. lights.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

PERIOD = struct.Struct("<Hh4B4H12h6H")
assert PERIOD.size == 0x34
ARCHIVES = {"field": "a/0/6/0", "battle": "a/0/6/1"}
COLORS = ["diffuse", "ambient", "specular", "emission", "fog", "clear"]


def color(value: int) -> dict:
    return {"red": value & 31, "green": value >> 5 & 31, "blue": value >> 10 & 31}


def rgb(c: dict) -> int:
    return c["red"] | c["green"] << 5 | c["blue"] << 10


def set_json(data: bytes) -> dict:
    if len(data) % PERIOD.size:
        raise ValueError("a lighting set isn't whole periods")
    periods = []
    for values in PERIOD.iter_unpack(data):
        hour, minutes, *enabled = values[:6]
        light_colors, directions, colors = values[6:10], values[10:22], values[22:28]
        if any(e > 1 for e in enabled) or any(c >> 15 for c in light_colors + colors):
            raise ValueError("unexpected lighting bits")
        periods.append({
            "end": {"hour": hour, "minutes": minutes},
            "lights": [{"enabled": bool(enabled[i]), "color": color(light_colors[i]),
                        "direction": dict(zip("xyz", directions[3 * i:3 * i + 3]))} for i in range(4)],
            **{name: color(c) for name, c in zip(COLORS, colors)},
        })
    return {"$schema": "../lights.schema.json", "periods": periods}


def dump(files: Path, output: Path):
    for kind, archive in ARCHIVES.items():
        sets = read_narc((files / archive).read_bytes())
        for i, data in enumerate(sets):
            write(output / kind / f"{i:03d}.json", set_json(data))
        print(f"wrote {len(sets)} {kind} lighting sets to {output / kind}")


def set_bytes(data: dict) -> bytes:
    out = b""
    for period in data["periods"]:
        lights = period["lights"]
        out += PERIOD.pack(period["end"]["hour"], period["end"]["minutes"], *(int(l["enabled"]) for l in lights),
                           *(rgb(l["color"]) for l in lights),
                           *(l["direction"][axis] for l in lights for axis in "xyz"),
                           *(rgb(period[name]) for name in COLORS))
    return out


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "lights.schema.json")
    for kind, output in zip(ARCHIVES, outputs):
        paths = sorted((root / kind).glob("*.json"), key=lambda p: int(p.stem))
        if [int(p.stem) for p in paths] != list(range(len(paths))):
            raise DataError(f"{label(root / kind)}: the sets are numbered from 000 without gaps")
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc([set_bytes(load(path, schema)) for path in paths]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/lights/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the lighting archives from data/lights/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE", help="the field and battle archives")
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
