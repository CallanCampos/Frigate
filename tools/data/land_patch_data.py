#!/usr/bin/env python3
"""The Join Avenue shops' land data patches in res/field/land_patches/, one JSON file per patch: dump writes them from
the ROM once, and pack builds ARCID_LAND_DATA_PATCH (a/1/5/4) from them, which the build does.

    land_patch_data.py dump extract/b2_us/files res/field/land_patches
    land_patch_data.py pack res/field/land_patches ARCHIVE

<n>.json is the archive's file n, which a Join Avenue shop's column 6 or 7 names (ResortShopData_GetShopParam, for the
occupants at odd and even places): the tiles that resort_mapcreate.c writes over the avenue's terrain where the shop
stands (LandDataPatch_OverwriteAttr), an RD chunk's tiles with their two planes, and the props it adds there
(LoadLandDataPatchBuildings).

Both versions have the same files. land_patch.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/1/5/4"

OFFSETS = struct.Struct("<HH")  # the offsets of the tiles and the props
GRID = struct.Struct("<HH")  # LandPatchGrid's width and height
TILE = struct.Struct("<3h3h2iI")  # RDTile in field_map_chunk_rd.c, LandPatchCell's bytes
PROP = struct.Struct("<3iHBB")  # FieldPropSourceInfo
assert (TILE.size, PROP.size) == (0x18, 0x10)
DIAGONAL = 1 << 31


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def patch_json(data: bytes) -> dict:
    grid_offset, props_offset = OFFSETS.unpack_from(data)
    width, height = GRID.unpack_from(data, grid_offset)
    check(grid_offset == OFFSETS.size and width and height
          and props_offset == grid_offset + GRID.size + TILE.size * width * height,
          "a land data patch's tiles don't follow its offsets")
    count = struct.unpack_from("<I", data, props_offset)[0]
    check(len(data) == props_offset + 4 + PROP.size * count, "a land data patch's props don't end it")
    tiles = []
    for values in TILE.iter_unpack(data[grid_offset + GRID.size:props_offset]):
        tile_type = values[8] & ~DIAGONAL
        tiles.append({"normal0": list(values[0:3]), "normal1": list(values[3:6]), "offset0": values[6],
                      "offset1": values[7], "tile_type": f"{tile_type:#010x}", "diagonal": bool(values[8] & DIAGONAL)})
    props = []
    for x, y, z, rotation, high, low in PROP.iter_unpack(data[props_offset + 4:]):
        props.append({"position": {"x": x, "y": y, "z": z}, "rotation_y": rotation, "model": high << 8 | low})
    return {"$schema": "land_patch.schema.json", "tiles": [tiles[z * width:(z + 1) * width] for z in range(height)],
            "props": props}


def patch_bytes(patch: dict, where: str) -> bytes:
    rows = patch["tiles"]
    if any(len(row) != len(rows[0]) for row in rows):
        raise DataError(f"{where}: the tile rows aren't all as wide")
    grid = GRID.pack(len(rows[0]), len(rows))
    for row in rows:
        for tile in row:
            tile_type = int(tile["tile_type"], 16)
            if tile_type & DIAGONAL:
                raise DataError(f"{where}: a tile_type {tile['tile_type']} doesn't fit in 31 bits")
            grid += TILE.pack(*tile["normal0"], *tile["normal1"], tile["offset0"], tile["offset1"],
                              tile_type | (DIAGONAL if tile["diagonal"] else 0))
    props = struct.pack("<I", len(patch["props"])) + b"".join(
        PROP.pack(*(p["position"][axis] for axis in "xyz"), p["rotation_y"], p["model"] >> 8, p["model"] & 0xFF)
        for p in patch["props"])
    return OFFSETS.pack(OFFSETS.size, OFFSETS.size + len(grid)) + grid + props


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    for i, data in enumerate(members):
        write(output / f"{i:03d}.json", patch_json(data))
    print(f"wrote {len(members)} land data patches to {output}")


def pack(root: Path, output: Path):
    schema = load_schema(root / "land_patch.schema.json")
    paths = sorted((p for p in root.glob("*.json") if p.stem.isdigit()), key=lambda p: int(p.stem))
    if [int(p.stem) for p in paths] != list(range(len(paths))):
        raise DataError(f"{label(root)}: the patches are numbered from 000 without gaps")
    members = [patch_bytes(load(path, schema), label(path)) for path in paths]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/land_patches/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build ARCID_LAND_DATA_PATCH from res/field/land_patches/")
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
