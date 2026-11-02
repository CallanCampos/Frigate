#!/usr/bin/env python3
"""The field actors' scripted animations in res/field/actors/animations/, one JSON file per animation: dump writes
them from the ROM once, and pack builds ARCID_FIELD_ACTOR_ANIMATION (a/2/0/0) from them, which the build does.

    field_actor_animation_data.py dump extract/b2_us/files res/field/actors/animations
    field_actor_animation_data.py pack res/field/actors/animations ARCHIVE

<n>.json is animation n, which a script plays on an actor by its number (Scrcmd_FieldActorAnimation_ActorAnimationPlay,
field_actor_animation.c): a curve (ica_anime.c), a scale, a rotation and a translation for each frame, of which the
file has any. The values are the file's floats, written as the shortest decimals that give them back.

Both versions have the same files. animation.schema.json describes the fields.
"""
import argparse
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/0/0"
HEADER = struct.Struct("<I4B")  # the frame count, whether it has a scale, a rotation and a translation, a pad byte
VALUES = ["scale", "rotation", "translation"]  # in each frame's order (GFL_G3DCurvePreloadFrames)
FLOAT = struct.Struct("<f")


def shortest(value: float) -> float:
    """Returns the float with the shortest decimal that packs into the same 32-bit float as the value."""
    if not math.isfinite(value):
        raise ValueError("a curve's value isn't a finite float")
    bits = FLOAT.pack(value)
    for digits in range(1, 10):
        candidate = float(f"{value:.{digits}g}")
        if FLOAT.pack(candidate) == bits:
            return candidate
    return value


def curve_json(data: bytes) -> dict:
    count, *flags, pad = HEADER.unpack_from(data)
    if pad or any(flag > 1 for flag in flags):
        raise ValueError("a curve's flags aren't 0 or 1, or its pad byte isn't 0")
    present = [value for value, flag in zip(VALUES, flags) if flag]
    if len(data) != HEADER.size + 12 * len(present) * count:
        raise ValueError("a curve isn't its count of frames")
    floats = [shortest(f) for (f,) in FLOAT.iter_unpack(data[HEADER.size:])]
    size = 3 * len(present)
    frames = []
    for i in range(count):
        frame = floats[size * i:size * (i + 1)]
        frames.append({value: frame[3 * k:3 * k + 3] for k, value in enumerate(present)})
    return {"$schema": "../animation.schema.json", "values": present, "frames": frames}


def curve_bytes(curve: dict, where: str) -> bytes:
    present = curve["values"]
    if [value for value in VALUES if value in present] != present:
        raise DataError(f"{where}: values must be some of {', '.join(VALUES)}, in that order")
    data = HEADER.pack(len(curve["frames"]), *(int(value in present) for value in VALUES), 0)
    for i, frame in enumerate(curve["frames"]):
        if list(frame) != present:
            raise DataError(f"{where}: frame {i} must have {', '.join(present)}, in that order")
        data += b"".join(FLOAT.pack(f) for value in present for f in frame[value])
    return data


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    for i, data in enumerate(members):
        write(output / f"{i:03d}.json", curve_json(data))
    print(f"wrote {len(members)} field actor animations to {output}")


def pack(root: Path, output: Path):
    schema = load_schema(root.parent / "animation.schema.json")
    paths = sorted(root.glob("*.json"), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(len(paths))):
        raise DataError(f"{label(root)}: the animations are numbered from 000 without gaps")
    members = [curve_bytes(load(path, schema), label(path)) for path in paths]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/actors/animations/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build ARCID_FIELD_ACTOR_ANIMATION from the animations")
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
