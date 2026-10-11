#!/usr/bin/env python3
"""The map matrices in data/map_matrices/, one JSON file per matrix: dump writes them from the ROM once, and pack
builds the archive (a/0/0/9) from them, which the build does.

    map_matrix_data.py dump extract/b2_us/files data/map_matrices
    map_matrix_data.py pack data/map_matrices ARCHIVE

A matrix is a grid of map cells: its header says whether it also gives each cell's zone, as the overworld's do, then
its width and height, then each cell's map, a file of the map archive or 0xffffffff for none, row by row, then the zones.
Each matrix is data/map_matrices/<matrix>.json, named after its constant in data/constants/map_matrices.txt, in whose
order the archive has them. map_matrix.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import ordered_files  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

NONE = 0xFFFFFFFF


def rows(cells: list, width: int) -> list[list]:
    return [cells[i:i + width] for i in range(0, len(cells), width)]


def matrix_json(data: bytes) -> dict:
    has_zones, unk, width, height = struct.unpack_from("<HHHH", data)
    if unk or len(data) != 8 + width * height * 4 * (2 if has_zones else 1):
        raise ValueError("unexpected matrix")
    maps = struct.unpack_from(f"<{width * height}I", data, 8)
    matrix = {"$schema": "map_matrix.schema.json", "maps": rows([None if m == NONE else m for m in maps], width)}
    if has_zones:
        zones = struct.unpack_from(f"<{width * height}I", data, 8 + 4 * width * height)
        matrix["zones"] = rows([None if z == NONE else name("ZONE_", z) for z in zones], width)
    return matrix


def dump(files: Path, output: Path):
    matrices = read_narc((files / "a/0/0/9").read_bytes())
    paths = ordered_files("map_matrices", "MAP_MATRIX_", output, ".json")
    if len(paths) != len(matrices):
        sys.exit(f"{len(matrices)} matrices in the archive and {len(paths)} in map_matrices.txt")
    for path, data in zip(paths, matrices):
        # A grid's row stays on one line, however long, so the file reads as the grid
        write(path, matrix_json(data), width=1 << 20)
    print(f"wrote {len(matrices)} map matrices to {output}")


def matrix_bytes(matrix: dict, where: str) -> bytes:
    maps = matrix["maps"]
    width, height = len(maps[0]), len(maps)
    grids = [maps] + ([matrix["zones"]] if "zones" in matrix else [])
    for grid in grids:
        if len(grid) != height or any(len(row) != width for row in grid):
            raise DataError(f"{where}: the grids' rows aren't all {width} cells, {height} rows")
    data = struct.pack("<HHHH", "zones" in matrix, 0, width, height)
    data += b"".join(struct.pack("<I", NONE if cell is None else cell) for row in maps for cell in row)
    if "zones" in matrix:
        data += b"".join(struct.pack("<I", NONE if cell is None else value(cell, where))
                         for row in matrix["zones"] for cell in row)
    return data


def pack(root: Path, output: Path):
    schema = load_schema(root / "map_matrix.schema.json")
    members = [matrix_bytes(load(path, schema), label(path))
               for path in ordered_files("map_matrices", "MAP_MATRIX_", root, ".json")]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/map_matrices/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the map matrix archive from data/map_matrices/")
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
