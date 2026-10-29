#!/usr/bin/env python3
"""The field camera's data in res/field/cameras/, one JSON file per entry: dump writes them from the ROM once, and pack
builds the camera setups' file (a/0/5/9), the zones' camera boundaries (a/1/0/9) and the entrance cameras (a/1/7/7)
from them, which the build does.

    field_camera_data.py dump extract/b2_us/files res/field/cameras
    field_camera_data.py pack res/field/cameras SETUPS BOUNDARIES ENTRANCES

- setups/<n>.json is camera setup n, the zone's "camera" (res/field/zones/), which FieldCamera_CoordsLoadDefaults
  applies: ARCID_FIELD_CAMERA_DEFAULT's one file holds them in order.
- boundaries/<n>.json is boundary set n, the zone's "cam_bound": the rectangles and circles that keep the camera's
  target or eye inside or out of them, which FieldCameraBoundary_LoadFile reads, up to four.
- entrances/<n>.json is the entrance camera of transition type n, a warp's "transition" (res/field/events/), from
  WARP_TRANSITION_CAMERA (7) on: where the camera moves to as the player goes in or comes out (entrance_camera.c).

Both versions have the same files. setup.schema.json, boundary.schema.json and entrance.schema.json describe the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVES = {"setups": "a/0/5/9", "boundaries": "a/1/0/9", "entrances": "a/1/7/7"}

# FieldCameraDefaults
SETUP = struct.Struct("<iIIIBBHiiI3i")
assert SETUP.size == 0x2C
BUFFER_MODES = ["z", "w"]  # GX_BUFFERMODE_Z, GX_BUFFERMODE_W

# FieldCameraBoundary: a rectangle's or a circle's shape and what it applies to, and four words
BOUNDARY = struct.Struct("<II4i")
CIRCLE_ANGLES = struct.Struct("<HH")
SHAPES = ["none", "inside_rect", "inside_circle", "outside_rect"]  # MM_CAMBOUND_FUNCS
APPLY_TO = ["target", "eye"]
MAX_BOUNDARIES = 4
RECT_FIELDS = ["min_x", "max_x", "min_z", "max_z"]  # in whole units
CIRCLE_FIELDS = ["center_x", "center_z", "radius", "angle_a", "angle_b"]  # in fx32, then the arc's two angles

# EntranceCameraFile, whose distance, target and offset the code shifts into fx32, so only their low 20 bits count
ENTRANCE = struct.Struct("<16I")
FIRST_ENTRANCE = 7  # WARP_TRANSITION_CAMERA
WHOLE_BITS = 20


def vector(values) -> dict:
    return dict(zip("xyz", values))


def setup_json(values) -> dict:
    zoom, pitch, yaw, unk_c, unk_10, swap_mode, fov, z_near, z_far, bind, *target = values
    if pitch >> 16 or yaw >> 16 or bind > 1 or swap_mode >= len(BUFFER_MODES):
        raise ValueError("unexpected camera setup values")
    return {"$schema": "../setup.schema.json", "distance": zoom, "pitch": pitch, "yaw": yaw, "fov": fov,
            "z_near": z_near, "z_far": z_far, "buffer_mode": BUFFER_MODES[swap_mode], "bind": bool(bind),
            "target": vector(target), "unkC": unk_c, "unk10": unk_10}


def setup_values(data: dict) -> tuple:
    return (data["distance"], data["pitch"], data["yaw"], data["unkC"], data["unk10"],
            BUFFER_MODES.index(data["buffer_mode"]), data["fov"], data["z_near"], data["z_far"], int(data["bind"]),
            *(data["target"][axis] for axis in "xyz"))


def boundary_json(data: bytes) -> dict:
    count = struct.unpack_from("<I", data)[0]
    if count > MAX_BOUNDARIES or len(data) != 4 + count * BOUNDARY.size:
        raise ValueError("a boundary set isn't its count of boundaries")
    boundaries = []
    for shape, apply_to, *words in BOUNDARY.iter_unpack(data[4:]):
        boundary = {"shape": SHAPES[shape], "apply_to": APPLY_TO[apply_to]}
        if SHAPES[shape] == "inside_circle":
            angles = CIRCLE_ANGLES.unpack(struct.pack("<i", words[3]))
            boundary |= dict(zip(CIRCLE_FIELDS, [*words[:3], *angles]))
        else:
            boundary |= dict(zip(RECT_FIELDS, words))
        boundaries.append(boundary)
    return {"$schema": "../boundary.schema.json", "boundaries": boundaries}


def boundary_bytes(data: dict, where: str) -> bytes:
    out = struct.pack("<I", len(data["boundaries"]))
    for i, boundary in enumerate(data["boundaries"]):
        fields = CIRCLE_FIELDS if boundary["shape"] == "inside_circle" else RECT_FIELDS
        if set(boundary) != {"shape", "apply_to", *fields}:
            raise DataError(f"{where}: boundary {i}, {boundary['shape']}, has {', '.join(fields)}")
        head = (SHAPES.index(boundary["shape"]), APPLY_TO.index(boundary["apply_to"]))
        if boundary["shape"] == "inside_circle":
            angles = struct.unpack("<i", CIRCLE_ANGLES.pack(boundary["angle_a"], boundary["angle_b"]))[0]
            out += BOUNDARY.pack(*head, boundary["center_x"], boundary["center_z"], boundary["radius"], angles)
        else:
            out += BOUNDARY.pack(*head, *(boundary[side] for side in RECT_FIELDS))
    return out


def whole(value: int) -> int:
    if value >> WHOLE_BITS:
        raise ValueError("an entrance camera's whole-unit value has high bits")
    return value - (1 << WHOLE_BITS) if value >> (WHOLE_BITS - 1) else value


def entrance_json(index: int, data: bytes) -> dict:
    (transition, pitch, yaw, distance, use_target, *rest) = ENTRANCE.unpack(data)
    target, offset, (frames, departure, arrival, release_bind, disable_boundary) = rest[:3], rest[3:6], rest[6:]
    if transition != FIRST_ENTRANCE + index or pitch >> 16 or yaw >> 16:
        raise ValueError("unexpected entrance camera values")
    if any(flag > 1 for flag in (use_target, departure, arrival, release_bind, disable_boundary)):
        raise ValueError("an entrance camera's flag isn't 0 or 1")
    return {"$schema": "../entrance.schema.json", "pitch": pitch, "yaw": yaw, "distance": whole(distance),
            "use_target": bool(use_target), "target": vector(map(whole, target)),
            "offset": vector(map(whole, offset)), "frames": frames, "departure_animation": bool(departure),
            "arrival_animation": bool(arrival), "release_bind": bool(release_bind),
            "disable_boundary": bool(disable_boundary)}


def entrance_bytes(transition: int, data: dict) -> bytes:
    mask = (1 << WHOLE_BITS) - 1
    return ENTRANCE.pack(transition, data["pitch"], data["yaw"], data["distance"] & mask, int(data["use_target"]),
                         *(data["target"][axis] & mask for axis in "xyz"),
                         *(data["offset"][axis] & mask for axis in "xyz"), data["frames"],
                         int(data["departure_animation"]), int(data["arrival_animation"]), int(data["release_bind"]),
                         int(data["disable_boundary"]))


def dump(files: Path, output: Path):
    setups = list(SETUP.iter_unpack(read_narc((files / ARCHIVES["setups"]).read_bytes())[0]))
    for i, values in enumerate(setups):
        write(output / "setups" / f"{i:03d}.json", setup_json(values))
    boundaries = read_narc((files / ARCHIVES["boundaries"]).read_bytes())
    for i, data in enumerate(boundaries):
        write(output / "boundaries" / f"{i:03d}.json", boundary_json(data))
    entrances = read_narc((files / ARCHIVES["entrances"]).read_bytes())
    for i, data in enumerate(entrances):
        write(output / "entrances" / f"{FIRST_ENTRANCE + i:03d}.json", entrance_json(i, data))
    print(f"wrote {len(setups)} camera setups, {len(boundaries)} boundary sets and {len(entrances)} entrance cameras "
          f"to {output}")


def numbered(directory: Path, first: int = 0) -> list[Path]:
    paths = sorted(directory.glob("*.json"), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(first, first + len(paths))):
        raise DataError(f"{label(directory)}: the files are numbered from {first:03d} without gaps")
    return paths


def pack(root: Path, outputs: list[Path]):
    schemas = {kind: load_schema(root / f"{kind}.schema.json") for kind in ("setup", "boundary", "entrance")}
    setups = b"".join(SETUP.pack(*setup_values(load(path, schemas["setup"]))) for path in numbered(root / "setups"))
    boundaries = [boundary_bytes(load(path, schemas["boundary"]), label(path))
                  for path in numbered(root / "boundaries")]
    entrances = [entrance_bytes(int(path.stem), load(path, schemas["entrance"]))
                 for path in numbered(root / "entrances", FIRST_ENTRANCE)]
    for output, members in zip(outputs, ([setups], boundaries, entrances)):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/cameras/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the camera archives from res/field/cameras/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=3, metavar="ARCHIVE",
                             help="the setups', boundaries' and entrance cameras' archives")
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
