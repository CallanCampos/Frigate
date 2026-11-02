#!/usr/bin/env python3
"""The event camera's stored positions in res/field/cameras/event/, one JSON file each: dump writes them from the ROM
once, and pack builds ARCID_SCRCMD_CAMERA (a/1/6/1) from them, which the build does.

    event_camera_data.py dump extract/b2_us/files res/field/cameras/event
    event_camera_data.py pack res/field/cameras/event ARCHIVE

<n>.json is position n, an EvCameraMoveData (scrcmd_camera.c), which the script command
Scrcmd_Camera_EvCameraMoveToCommon names by its number and moves the event camera to over a number of frames: the
camera's pitch, yaw, distance and target, and which of them the move animates (FieldEvCameraAnimationFlags).
Angles are in 65536ths of a turn and the distance and target in 4096ths of a unit, as the code passes them on.

Both versions have the same positions. event.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/1/6/1"
MOVE = struct.Struct("<HHi3i6I")  # EvCameraMoveData
FLAGS = ["extra_translation", "pitch", "yaw", "distance", "fov", "target"]  # FieldEvCameraAnimationFlags' animate*


def move_json(data: bytes) -> dict:
    pitch, yaw, distance, *rest = MOVE.unpack(data)
    target, flags = rest[:3], rest[3:]
    if any(flag > 1 for flag in flags):
        raise ValueError("an event camera position's flag isn't TRUE or FALSE")
    return {"$schema": "event.schema.json", "pitch": pitch, "yaw": yaw, "distance": distance,
            "target": dict(zip("xyz", target)), "animate": {flag: bool(on) for flag, on in zip(FLAGS, flags)}}


def move_bytes(move: dict) -> bytes:
    return MOVE.pack(move["pitch"], move["yaw"], move["distance"], *(move["target"][axis] for axis in "xyz"),
                     *(int(move["animate"][flag]) for flag in FLAGS))


def dump(files: Path, output: Path):
    moves = read_narc((files / ARCHIVE).read_bytes())
    if any(len(data) != MOVE.size for data in moves):
        raise ValueError("an event camera position isn't an EvCameraMoveData")
    for i, data in enumerate(moves):
        write(output / f"{i:03d}.json", move_json(data))
    print(f"wrote {len(moves)} event camera positions to {output}")


def pack(root: Path, output: Path):
    paths = sorted(root.glob("[0-9]*.json"), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(len(paths))):
        raise DataError(f"{label(root)}: the positions are numbered from 000 without gaps")
    schema = load_schema(root / "event.schema.json")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([move_bytes(load(path, schema)) for path in paths]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/cameras/event/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the event camera's positions from res/field/cameras/event/")
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
