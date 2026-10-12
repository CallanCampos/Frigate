"""Editable tile terrain for field map chunks.

The terrain sources use integer Nitro slope and height-table indices. A split
tile's offset names a row in its layer's splits CSV. RD chunks store their two
planes directly instead of using the shared tables.
"""

import csv
import struct


HEIGHT_FIELDS = ("x", "z", "shape", "slope", "offset", "tile_type", "diagonal")
SPLIT_FIELDS = ("index", "unused", "slope0", "slope1", "offset0", "offset1")
RD_FIELDS = ("x", "z", "normal0_x", "normal0_y", "normal0_z",
             "normal1_x", "normal1_y", "normal1_z", "offset0", "offset1",
             "tile_type", "diagonal")
SLOPE_COUNT = 317
OFFSET_COUNT = 1285


def terrain_ranges(chunk):
    magic = chunk[:2]
    if magic == b"GC":
        offsets = struct.unpack_from("<III", chunk, 8)
    elif magic in (b"WB", b"RD"):
        offsets = struct.unpack_from("<II", chunk, 8)
    elif magic == b"NG":
        return []
    else:
        raise ValueError(f"unknown map chunk type {magic!r}")
    if any(start >= end or end > len(chunk) for start, end in zip(offsets, offsets[1:])):
        raise ValueError("invalid map terrain section offsets")
    return list(zip(offsets, offsets[1:]))


def write_csv(path, fields, rows):
    with path.open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def read_csv(path, fields):
    with path.open(newline="") as source:
        reader = csv.DictReader(source)
        if tuple(reader.fieldnames or ()) != fields:
            raise ValueError(f"{path}: expected columns {', '.join(fields)}")
        rows = []
        for row in reader:
            if None in row or any(value is None for value in row.values()):
                raise ValueError(f"{path}: malformed CSV row")
            try:
                rows.append({key: int(value, 0) if value.startswith("0x") else int(value)
                             for key, value in row.items()})
            except ValueError as error:
                raise ValueError(f"{path}: expected integer fields") from error
        return rows


def checked(value, bits, signed=False):
    lower = -(1 << (bits - 1)) if signed else 0
    upper = (1 << (bits - 1)) if signed else (1 << bits)
    if not lower <= value < upper:
        raise ValueError(f"terrain value {value} exceeds {'signed ' if signed else ''}{bits}-bit range")
    return value


def export_terrains(chunk, directory):
    for layer, (start, end) in enumerate(terrain_ranges(chunk)):
        width, height = struct.unpack_from("<HH", chunk, start)
        count = width * height
        if chunk[:2] == b"RD":
            if start + 4 + count * 24 != end:
                raise ValueError("RD terrain size does not match dimensions")
            rows = []
            for index in range(count):
                values = struct.unpack_from("<6h2iI", chunk, start + 4 + 24 * index)
                tile_type = values[-1] & 0x7fffffff
                diagonal = values[-1] >> 31
                rows.append(dict(zip(RD_FIELDS, (index % width, index // width,
                                                 *values[:-1], tile_type, diagonal))))
            write_csv(directory / f"terrain_{layer}.csv", RD_FIELDS, rows)
        else:
            split_start = start + 4 + count * 8
            if split_start > end or (end - split_start) % 8:
                raise ValueError("height terrain size does not match dimensions")
            rows = []
            for index in range(count):
                shape_slope, offset, flags = struct.unpack_from("<HHI", chunk, start + 4 + 8 * index)
                rows.append(dict(zip(HEIGHT_FIELDS,
                                     (index % width, index // width, shape_slope & 3,
                                      shape_slope >> 2, offset, flags & 0x7fffffff, flags >> 31))))
            splits = []
            for index in range((end - split_start) // 8):
                first, slope1, offset0, offset1 = struct.unpack_from("<4H", chunk, split_start + 8 * index)
                splits.append(dict(zip(SPLIT_FIELDS,
                                       (index, first & 3, first >> 2, slope1, offset0, offset1))))
            write_csv(directory / f"terrain_{layer}.csv", HEIGHT_FIELDS, rows)
            write_csv(directory / f"terrain_{layer}_splits.csv", SPLIT_FIELDS, splits)


def patch_terrains(chunk, directory):
    result = bytearray(chunk)
    shift = 0
    for layer, (old_start, old_end) in enumerate(terrain_ranges(chunk)):
        start, end = old_start + shift, old_end + shift
        width, height = struct.unpack_from("<HH", result, start)
        count = width * height
        path = directory / f"terrain_{layer}.csv"
        if chunk[:2] == b"RD":
            rows = read_csv(path, RD_FIELDS)
            splits = []
        else:
            rows = read_csv(path, HEIGHT_FIELDS)
            split_path = directory / f"terrain_{layer}_splits.csv"
            splits = read_csv(split_path, SPLIT_FIELDS)
        if len(rows) != count:
            raise ValueError(f"{path}: expected {count} tiles")
        split_start = start + 4 + count * 8
        if chunk[:2] != b"RD" and (split_start > end or (end - split_start) % 8):
            raise ValueError(f"{path}: invalid original split section")
        if len(splits) > 65535:
            raise ValueError(f"{path}: too many split planes")
        for index, row in enumerate(rows):
            if (row["x"], row["z"]) != (index % width, index // width):
                raise ValueError(f"{path}: tile order or dimensions changed")
            diagonal = checked(row["diagonal"], 1)
            tile_type = checked(row["tile_type"], 31)
            flags = tile_type | diagonal << 31
            if chunk[:2] == b"RD":
                values = [checked(row[name], 16, True) for name in RD_FIELDS[2:8]]
                values += [checked(row[name], 32, True) for name in ("offset0", "offset1")]
                struct.pack_into("<6h2iI", result, start + 4 + 24 * index, *values, flags)
            else:
                shape = checked(row["shape"], 2)
                slope = checked(row["slope"], 14)
                offset = checked(row["offset"], 16)
                if shape == 0 and (slope >= SLOPE_COUNT or offset >= OFFSET_COUNT):
                    raise ValueError(f"{path}: plane tile {index} exceeds slope or height table")
                if shape == 2 and offset >= len(splits):
                    raise ValueError(f"{path}: split tile {index} refers past split table")
                if shape not in (0, 2):
                    raise ValueError(f"{path}: unsupported tile shape {shape}")
                struct.pack_into("<HHI", result, start + 4 + 8 * index,
                                 shape | slope << 2, offset, flags)
        if chunk[:2] != b"RD":
            encoded = bytearray(8 * len(splits))
            for index, row in enumerate(splits):
                if row["index"] != index:
                    raise ValueError(f"{split_path}: split order changed")
                first = checked(row["unused"], 2) | checked(row["slope0"], 14) << 2
                values = [checked(row[name], 16) for name in SPLIT_FIELDS[3:]]
                if row["slope0"] >= SLOPE_COUNT or row["slope1"] >= SLOPE_COUNT:
                    raise ValueError(f"{split_path}: split {index} exceeds slope table")
                if row["offset0"] >= OFFSET_COUNT or row["offset1"] >= OFFSET_COUNT:
                    raise ValueError(f"{split_path}: split {index} exceeds height table")
                struct.pack_into("<4H", encoded, 8 * index, first, *values)
            result[split_start:end] = encoded
            delta = split_start + len(encoded) - end
            if delta:
                fields = {b"WB": (8, 12, 16), b"GC": (8, 12, 16, 20)}[chunk[:2]]
                for field in fields:
                    offset = struct.unpack_from("<I", result, field)[0]
                    if offset >= end:
                        struct.pack_into("<I", result, field, offset + delta)
                shift += delta
    return bytes(result)
