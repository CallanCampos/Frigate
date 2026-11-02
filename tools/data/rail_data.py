#!/usr/bin/env python3
"""The rail maps' data in res/field/rails/, where the player walks on rails rather than a grid: dump writes it from the
ROM once, and pack builds the rail files (a/0/7/8) and the no-grid mapper's headers (a/1/1/0) from it, which the build
does.

    rail_data.py dump extract/b2_us/files res/field/rails
    rail_data.py pack res/field/rails RAILS HEADERS

- headers.json is ARCID_RAIL_DATA's files, by the rail ID that GetRailIDForZone gives a zone (ZONE_RAIL_ASSIGNMENTS
  in zonedata.c): each names its rail file and its camera areas (NoGridMapperHeader, FieldNoGridMapper_Load).
- <n>.json is rail file n: ARCID_RAIL_HEADERS's file n, its points, lines, cameras and curves (field_rail_loader.c),
  and file n plus the count of rail files, the tiles of each of its lines (rail_attr.c), here in the line.

Both versions have the same files. rail.schema.json and headers.schema.json describe the fields.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVES = {"rails": "a/0/7/8", "headers": "a/1/1/0"}

# NoGridMapperHeader
HEADER = struct.Struct("<HH")
NONE = 0xFFFF  # NOGRID_RAIL_NONE, NOGRID_CAMERA_NONE and RAIL_ID_NONE

# RailFileHeader: a tag that the code doesn't read, "RL" and 5, then the offsets of the tile width, points, lines,
# cameras, curves and the end
FILE_HEADER = struct.Struct("<4s6I")
FILE_TAG = b"RL\x05\x00"
TILE_WIDTH = struct.Struct("<i")
NAME_SIZE = 48
POINT = struct.Struct(f"<4I4I4i3iI{NAME_SIZE}s")  # RailPoint
LINE = struct.Struct(f"<6I{NAME_SIZE}s")  # RailLine
CAMERA = struct.Struct("<I8i")  # RailCamera
CURVE = struct.Struct("<I3i20B")  # RailCurve
assert (POINT.size, LINE.size, CAMERA.size, CURVE.size) == (0x70, 0x48, 0x24, 0x24)
LINES_PER_POINT = 4  # RAIL_POINT_LINE_MAX
TILEMAP = struct.Struct("<HH")  # RailTilemap

DIRS = ["none", "up", "right", "down", "left"]  # RAIL_DIR_*
# RAIL_CAMERA_CURVE_FUNC_SET in rail_data_handle.c, by a camera's function ID
CAMERA_FUNCS = ["RailCameraFuncSetStaticPYR", "RailCameraFuncSetStaticPYDist", "FieldRailCamera_BlendAngles",
                "FieldRailCamera_FixedEyeTarget", "FieldRailCamera_BlendEyeTarget", "FieldRailCamera_BlendOrbit",
                "FieldRailCamera_Orbit", "FieldRailCamera_OrbitMovingPoint", "FieldRailCamera_YawFromPoint",
                "FieldRailCamera_FollowHeading", "FieldRailCamera_FollowHeadingSlope",
                "FieldRailCamera_FollowHeadingAtCentre", "FieldRailCamera_ApplyLineStart",
                "FieldRailCamera_ApplyLineEnd", "FieldRailCamera_BlendAnglesAtFocus",
                "FieldRailCamera_FixedAngleAtCentre", "FieldRailCamera_BlendAnglesAtCentre",
                "FieldRailCamera_FixedAngleShift", "FieldRailCamera_BlendAnglesShift",
                "FieldRailCamera_FixedAngleShiftAtFocus"]
# RAIL_CONV_RAIL_TO_WORLD_FUNCS and RAIL_CONV_WORLD_TO_RAIL_FUNCS, by a curve's handler ID
CURVE_KINDS = ["no_curve", "slerp_xz", "slerp_xyz"]
# A point's name is padded with spaces and a line's with NULs, as the ROM's are
NAME_PATTERN = re.compile(rb"[\x21-\x7e]*")


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def ident(number: int):
    check(number <= NONE, "an ID is past RAIL_ID_NONE")
    return None if number == NONE else number


def number(item) -> int:
    return NONE if item is None else item


def vector(values) -> dict:
    return dict(zip("xyz", values))


def name_text(data: bytes, padding: bytes) -> str:
    text = data.rstrip(padding)
    check(NAME_PATTERN.fullmatch(text) is not None, f"a name isn't text padded with {padding!r}: {data!r}")
    return text.decode("ascii")


def name_bytes(text: str, padding: bytes, where: str) -> bytes:
    data = text.encode("ascii")
    if len(data) > NAME_SIZE or not NAME_PATTERN.fullmatch(data):
        raise DataError(f"{where}: the name {text!r} isn't up to {NAME_SIZE} printable ASCII characters")
    return data.ljust(NAME_SIZE, padding)


def tile_rows(data: bytes, offset: int) -> tuple[list[list[str]], int]:
    width, length = TILEMAP.unpack_from(data, offset)
    check(width and length, "a line's tilemap is empty")
    tiles = struct.unpack_from(f"<{width * length}I", data, offset + TILEMAP.size)
    rows = [[f"{tile:#010x}" for tile in tiles[z * width:(z + 1) * width]] for z in range(length)]
    return rows, offset + TILEMAP.size + 4 * width * length


def rail_json(data: bytes, attributes: bytes) -> dict:
    tag, width_offset, *offsets = FILE_HEADER.unpack_from(data)
    check(tag == FILE_TAG and width_offset == FILE_HEADER.size and offsets[0] == width_offset + TILE_WIDTH.size,
          "a rail file's header isn't RL 5 with the tile width after it")
    check(offsets[-1] == len(data), "a rail file's end isn't its size")
    sections = []
    for (start, end), layout in zip(zip(offsets, offsets[1:]), (POINT, LINE, CAMERA, CURVE)):
        check((end - start) % layout.size == 0, "a rail file's section isn't whole records")
        sections.append(list(layout.iter_unpack(data[start:end])))
    points, lines, cameras, curves = sections
    count = struct.unpack_from("<I", attributes)[0]
    check(count == len(lines), "a rail file's tilemaps aren't one per line")
    offset, line_tiles = 4, []
    for _ in lines:
        rows, offset = tile_rows(attributes, offset)
        line_tiles.append(rows)
    check(offset == len(attributes), "a rail file's tilemaps don't end the file")
    rail = {"$schema": "rail.schema.json", "tile_width": TILE_WIDTH.unpack_from(data, width_offset)[0], "points": [],
            "lines": [], "cameras": [], "curves": []}
    for values in points:
        line_ids, dirs, widths, pos, camera, name = (values[0:4], values[4:8], values[8:12], values[12:15], values[15],
                                                     values[16])
        check(all(d < len(DIRS) for d in dirs), "a point's direction isn't a RAIL_DIR")
        rail["points"].append({"name": name_text(name, b" "), "position": vector(pos), "camera": ident(camera),
                               "lines": [{"line": ident(line), "dir": DIRS[d], "width": w}
                                         for line, d, w in zip(line_ids, dirs, widths)]})
    for (point_a, point_b, dir_, curve, camera, length, name), tiles in zip(lines, line_tiles):
        check(dir_ < len(DIRS), "a line's direction isn't a RAIL_DIR")
        rail["lines"].append({"name": name_text(name, b"\0"), "point_a": ident(point_a), "point_b": ident(point_b),
                              "dir": DIRS[dir_], "curve": ident(curve), "camera": ident(camera), "length": length,
                              "tiles": tiles})
    for func, *params in cameras:
        check(func < len(CAMERA_FUNCS), "a camera's function isn't in RAIL_CAMERA_CURVE_FUNC_SET")
        rail["cameras"].append({"function": CAMERA_FUNCS[func], "params": params})
    for kind, x, y, z, *unk10 in curves:
        check(kind < len(CURVE_KINDS), "a curve's handler isn't in RAIL_CONV_RAIL_TO_WORLD_FUNCS")
        rail["curves"].append({"kind": CURVE_KINDS[kind], "center": vector((x, y, z)), "unk10": unk10})
    return rail


def rail_bytes(rail: dict, where: str) -> tuple[bytes, bytes]:
    points = b""
    for i, point in enumerate(rail["points"]):
        slots = point["lines"]
        points += POINT.pack(*(number(s["line"]) for s in slots), *(DIRS.index(s["dir"]) for s in slots),
                             *(s["width"] for s in slots), *(point["position"][axis] for axis in "xyz"),
                             number(point["camera"]), name_bytes(point["name"], b" ", f"{where}: point {i}"))
    lines, attributes = b"", struct.pack("<I", len(rail["lines"]))
    for i, line in enumerate(rail["lines"]):
        lines += LINE.pack(number(line["point_a"]), number(line["point_b"]), DIRS.index(line["dir"]),
                           number(line["curve"]), number(line["camera"]), line["length"],
                           name_bytes(line["name"], b"\0", f"{where}: line {i}"))
        rows = line["tiles"]
        if any(len(row) != len(rows[0]) for row in rows):
            raise DataError(f"{where}: line {i}'s tile rows aren't all as wide")
        attributes += TILEMAP.pack(len(rows[0]), len(rows)) + b"".join(
            struct.pack("<I", int(tile, 16)) for row in rows for tile in row)
    cameras = b"".join(CAMERA.pack(CAMERA_FUNCS.index(c["function"]), *c["params"]) for c in rail["cameras"])
    curves = b"".join(CURVE.pack(CURVE_KINDS.index(c["kind"]), *(c["center"][axis] for axis in "xyz"), *c["unk10"])
                      for c in rail["curves"])
    offsets = [FILE_HEADER.size + TILE_WIDTH.size]
    for section in (points, lines, cameras, curves):
        offsets.append(offsets[-1] + len(section))
    head = FILE_HEADER.pack(FILE_TAG, FILE_HEADER.size, *offsets) + TILE_WIDTH.pack(rail["tile_width"])
    return head + points + lines + cameras + curves, attributes


def dump(files: Path, output: Path):
    rails = read_narc((files / ARCHIVES["rails"]).read_bytes())
    count = len(rails) // 2
    check(len(rails) == 2 * count, "the rail files aren't paired with their tilemaps")
    for i in range(count):
        write(output / f"{i:03d}.json", rail_json(rails[i], rails[count + i]), width=1 << 20)
    headers = []
    for data in read_narc((files / ARCHIVES["headers"]).read_bytes()):
        rail, camera = HEADER.unpack(data)
        check(rail == NONE or rail < count, "a header's rail file isn't one of the files")
        headers.append({"rail": None if rail == NONE else rail, "camera_areas": None if camera == NONE else camera})
    write(output / "headers.json", {"$schema": "headers.schema.json", "headers": headers})
    print(f"wrote {count} rail files and {len(headers)} headers to {output}")


def rail_paths(root: Path) -> list[Path]:
    paths = sorted((p for p in root.glob("*.json") if p.stem.isdigit()), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(len(paths))):
        raise DataError(f"{label(root)}: the rail files are numbered from 000 without gaps")
    return paths


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "rail.schema.json")
    packed = [rail_bytes(load(path, schema), label(path)) for path in rail_paths(root)]
    path = root / "headers.json"
    headers = load(path, load_schema(root / "headers.schema.json"))["headers"]
    for i, header in enumerate(headers):
        if header["rail"] is not None and header["rail"] >= len(packed):
            raise DataError(f"{label(path)}: header {i} names rail file {header['rail']}, which doesn't exist")
    members = ([p[0] for p in packed] + [p[1] for p in packed],
               [HEADER.pack(number(h["rail"]), number(h["camera_areas"])) for h in headers])
    for output, files in zip(outputs, members):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(files))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/rails/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the rail archives from res/field/rails/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE",
                             help="the rail files' and the headers' archives")
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
