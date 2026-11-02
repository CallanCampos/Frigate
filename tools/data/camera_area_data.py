#!/usr/bin/env python3
"""The field's camera areas in res/field/cameras/, one JSON file per archive file: dump writes them from the ROM once,
and pack builds the rail maps' camera areas (a/0/7/9) and the other maps' (a/1/5/6) from them, which the build does.

    camera_area_data.py dump extract/b2_us/files res/field/cameras
    camera_area_data.py pack res/field/cameras RAIL_AREAS AREAS

A file is the camera areas of a zone (CameraArea, fld_scenearea_loader.c): rectangles of the map's tiles across which
the camera blends between two settings, and rings around a point that the camera turns around. FieldCameraArea_Update
calls an area's functions as the player enters it, stays in it and leaves it.
- rail_areas/<n>.json is ARCID_FIELD_SCENE_AREA's file n, which a rail map's header names (res/field/rails/).
- areas/<n>.json is ARCID_FIELD_CAMERA_MAP_PARAM's file n, which GetCameraIDForZone gives a zone
  (ZONE_CAMERA_ASSIGNMENTS in zonedata.c).

Both versions have the same files. area.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVES = {"rail_areas": "a/0/7/9", "areas": "a/1/5/6"}

AREA_SIZE = 0x48
RECT = struct.Struct("<6HHHiHHiI3i3iHH")  # CameraAreaRect
CIRCLE = struct.Struct("<2I2i3iii3i3i")  # CameraAreaCircle
TAIL = struct.Struct("<i4H")  # CameraArea's unk3C and its functions
assert RECT.size == CIRCLE.size == 0x3C and TAIL.size == AREA_SIZE - 0x3C
SHAPES = ["circle", "rect"]  # CAM_COLL_CHECK_FUNCS
CALC_FUNCS = ["FieldCameraAreaCalc_CIRCLE_AimPlayerCentre", "FieldCameraAreaCalc_CIRCLE_FixedLookAt",
              "FieldCameraAreaCalc_RECT_PitchYawTZ", "FieldCameraAreaCalc_RECT_PitchYawTZ_BeginDisableDelay",
              "FieldCameraAreaCalc_RECT_PitchYawTZ_ResetEnableDelay",
              "FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_STAY",
              "FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_BeginDisableDelay",
              "FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_ResetEnableDelay"]  # CAM_CALC_FUNCS
CALLS = ["stay", "enter", "exit"]  # stayFunc, enterFunc, exitFunc
SETTING = ["pitch", "yaw", "distance"]


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def vector(values) -> dict:
    return dict(zip("xyz", values))


def xyz(data: dict) -> tuple:
    return tuple(data[axis] for axis in "xyz")


def area_json(data: bytes) -> dict:
    unk3c, shape, *calls = TAIL.unpack_from(data, 0x3C)
    check(shape < len(SHAPES) and all(c < len(CALC_FUNCS) for c in calls), "a camera area's function is past its table")
    area = {"shape": SHAPES[shape]}
    if SHAPES[shape] == "rect":
        (grid_x, grid_z, grid_w, unk06, grid_h, unk0a, pitch1, yaw1, tz1, pitch2, yaw2, tz2, forbid,
         *rest) = RECT.unpack_from(data)
        check(forbid <= 1, "a camera area's forbidDelayManager isn't 0 or 1")
        offset1, offset2, (fov1, fov2) = rest[0:3], rest[3:6], rest[6:]
        area |= {"grid_x": grid_x, "grid_z": grid_z, "grid_w": grid_w, "unk06": unk06, "grid_h": grid_h,
                 "unk0A": unk0a,
                 "start": {"pitch": pitch1, "yaw": yaw1, "distance": tz1, "target_offset": vector(offset1),
                           "fov": fov1},
                 "end": {"pitch": pitch2, "yaw": yaw2, "distance": tz2, "target_offset": vector(offset2),
                         "fov": fov2},
                 "forbid_delay_manager": bool(forbid)}
    else:
        angle_start, angle_end, radius_start, radius_end, *rest = CIRCLE.unpack_from(data)
        center, (pitch, distance), target, eye = rest[0:3], rest[3:5], rest[5:8], rest[8:11]
        area |= {"angle_start": angle_start, "angle_end": angle_end, "radius_start": radius_start,
                 "radius_end": radius_end, "center": vector(center), "pitch": pitch, "distance": distance,
                 "target": vector(target), "eye": vector(eye)}
    return area | {"unk3C": unk3c, **{call: CALC_FUNCS[c] for call, c in zip(CALLS, calls)}}


def area_bytes(area: dict, where: str) -> bytes:
    if area["shape"] == "rect":
        fields = {"grid_x", "grid_z", "grid_w", "unk06", "grid_h", "unk0A", "start", "end", "forbid_delay_manager"}
    else:
        fields = {"angle_start", "angle_end", "radius_start", "radius_end", "center", "pitch", "distance", "target",
                  "eye"}
    fields |= {"shape", "unk3C", *CALLS}
    if set(area) != fields:
        raise DataError(f"{where}: a {area['shape']} area has {', '.join(sorted(fields))}")
    if area["shape"] == "rect":
        start, end = area["start"], area["end"]
        body = RECT.pack(area["grid_x"], area["grid_z"], area["grid_w"], area["unk06"], area["grid_h"], area["unk0A"],
                         *(start[f] for f in SETTING), *(end[f] for f in SETTING), int(area["forbid_delay_manager"]),
                         *xyz(start["target_offset"]), *xyz(end["target_offset"]), start["fov"], end["fov"])
    else:
        body = CIRCLE.pack(area["angle_start"], area["angle_end"], area["radius_start"], area["radius_end"],
                           *xyz(area["center"]), area["pitch"], area["distance"], *xyz(area["target"]),
                           *xyz(area["eye"]))
    return body + TAIL.pack(area["unk3C"], SHAPES.index(area["shape"]),
                            *(CALC_FUNCS.index(area[call]) for call in CALLS))


def dump(files: Path, output: Path):
    for kind, archive in ARCHIVES.items():
        members = read_narc((files / archive).read_bytes())
        for i, data in enumerate(members):
            check(len(data) % AREA_SIZE == 0, "a camera area file isn't whole areas")
            areas = [area_json(data[o:o + AREA_SIZE]) for o in range(0, len(data), AREA_SIZE)]
            write(output / kind / f"{i:03d}.json", {"$schema": "../area.schema.json", "areas": areas})
        print(f"wrote {len(members)} files of camera areas to {output / kind}")


def numbered(directory: Path) -> list[Path]:
    paths = sorted(directory.glob("*.json"), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(len(paths))):
        raise DataError(f"{label(directory)}: the files are numbered from 000 without gaps")
    return paths


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "area.schema.json")
    for kind, output in zip(ARCHIVES, outputs):
        members = []
        for path in numbered(root / kind):
            areas = load(path, schema)["areas"]
            members.append(b"".join(area_bytes(area, f"{label(path)}: area {i}") for i, area in enumerate(areas)))
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write the camera areas from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the camera areas' archives from res/field/cameras/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE",
                             help="the rail maps' and the other maps' camera areas' archives")
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
