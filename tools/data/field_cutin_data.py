#!/usr/bin/env python3
"""The field's 3D cut-in effects in res/field/cutins/, a JSON file for each effect: dump writes them from the ROM once,
and pack builds the effects' files (ARCID_FLD3D_CI_FILE, a/1/1/8) and their bitmaps (ARCID_FLD3D_CI_BITMAP, a/2/6/8)
from them, which the build does.

    field_cutin_data.py dump extract/b2_us/files res/field/cutins
    field_cutin_data.py pack res/field/cutins FILES BITMAPS

<n>.json is effect n of fld3d_ci.c (FLD3DCI_* in field_event.h, such as FLD3DCI_POKE_SPRITE, 3): the field move's
Pokémon sprite, the battle cut-ins and Alder's flight. Its scene (Fld3DCiFile, which Fld3DCi_LoadG3DSceneTemplate
builds the scene from) names its models and animations by their files of ARCID_FLD3D_CI_RESOURCES (a/1/1/6), null for
none; its bitmap (Fld3DCiBitmapEntry) is the graphics that EventFld3DCiBitmapConv_Callback puts into the model's
textures and palettes, and a flag that EventFieldEffect_CreateBattleCutin passes the cut-in. Both versions have the same
files. cutin.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

FILES, BITMAPS = "a/1/1/8", "a/2/6/8"
SCENE = struct.Struct("<I2H2H2H4H4HHH")
NONE = 0xFFFF
# The bitmap file: the count of its entries, the cut-in's flag and two zeros, then the entries
BITMAP_HEADER = struct.Struct("<4B")
# A texture's or palette's name, in 0x14 bytes: its terminator, then spaces
NAME_SIZE = 0x14
ENTRY = struct.Struct(f"<{NAME_SIZE}s{NAME_SIZE}s8B")
ENTRY_FIELDS = ["resource", "char_file", "palette_file", "palette_offset", "width", "height", "source_stride",
                "source_offset"]


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def ids(numbers) -> list:
    return [None if n == NONE else n for n in numbers]


def raw_ids(items: list) -> list[int]:
    return [NONE if n is None else n for n in items]


def name_json(data: bytes) -> str:
    text, _, rest = data.partition(b"\0")
    check(set(rest) <= {ord(" ")}, "a bitmap name has more than spaces after its terminator")
    return text.decode("ascii")


def name_bytes(text: str, where: str) -> bytes:
    data = text.encode("ascii")
    if len(data) >= NAME_SIZE:
        raise DataError(f"{where}: {text!r} is longer than {NAME_SIZE - 1} characters")
    return (data + b"\0").ljust(NAME_SIZE, b" ")


def cutin_json(scene: bytes, bitmap: bytes) -> dict:
    particle, *rest = SCENE.unpack(scene)
    count, flag, *zeros = BITMAP_HEADER.unpack_from(bitmap)
    check(not any(zeros), "a bitmap's header has more than its count and flag")
    check(len(bitmap) == BITMAP_HEADER.size + count * ENTRY.size, "a bitmap file isn't its count of entries")
    entries = []
    for i in range(count):
        texture, palette, *fields = ENTRY.unpack_from(bitmap, BITMAP_HEADER.size + i * ENTRY.size)
        entries.append({"texture": name_json(texture), "palette": name_json(palette),
                        **dict(zip(ENTRY_FIELDS, fields))})
    return {"$schema": "cutin.schema.json", "particle": particle, "particle_delays": rest[0:2],
            "models": ids(rest[2:4]),
            "actor_delays": rest[4:6], "animations1": ids(rest[6:10]), "animations2": ids(rest[10:14]),
            "dim_capture": rest[14], "key_list": rest[15], "bitmap_flag": flag, "bitmaps": entries}


def cutin_bytes(cutin: dict, where: str) -> tuple[bytes, bytes]:
    scene = SCENE.pack(cutin["particle"], *cutin["particle_delays"], *raw_ids(cutin["models"]), *cutin["actor_delays"],
                       *raw_ids(cutin["animations1"]), *raw_ids(cutin["animations2"]), cutin["dim_capture"],
                       cutin["key_list"])
    bitmap = BITMAP_HEADER.pack(len(cutin["bitmaps"]), cutin["bitmap_flag"], 0, 0)
    for i, entry in enumerate(cutin["bitmaps"]):
        bitmap += ENTRY.pack(name_bytes(entry["texture"], f"{where}: bitmaps[{i}]"),
                             name_bytes(entry["palette"], f"{where}: bitmaps[{i}]"),
                             *(entry[field] for field in ENTRY_FIELDS))
    return scene, bitmap


def dump(files: Path, output: Path):
    scenes = read_narc((files / FILES).read_bytes())
    bitmaps = read_narc((files / BITMAPS).read_bytes())
    check(len(scenes) == len(bitmaps), "the effects' files and bitmaps have different counts")
    for i, (scene, bitmap) in enumerate(zip(scenes, bitmaps)):
        write(output / f"{i}.json", cutin_json(scene, bitmap))
    print(f"wrote {len(scenes)} effects to {output}")


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "cutin.schema.json")
    scenes, bitmaps = [], []
    while (root / f"{len(scenes)}.json").exists():
        path = root / f"{len(scenes)}.json"
        scene, bitmap = cutin_bytes(load(path, schema), label(path))
        scenes.append(scene)
        bitmaps.append(bitmap)
    for output, members in zip(outputs, (scenes, bitmaps)):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/cutins/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the effects' files and bitmaps")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE",
                             help="the files' and the bitmaps' archives")
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
