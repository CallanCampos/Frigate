#!/usr/bin/env python3
"""Pack the glTF pilot for map chunk 103 into the original map terrain archive.

Usage: map_model_data.py pack data/map_models black2|white2 OUTPUT
       map_model_data.py extract APICULA OUTPUT_DIR

This first converter edits the position fields of one Nitro display list. The
remaining map chunk data comes from the extracted ROM. Unsupported position
encodings and topology changes fail explicitly.
"""

import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from narc import read_narc, write_narc


ROOT = Path(__file__).resolve().parents[2]
CHUNK = 103
PARAMS = {0x00: 0, 0x21: 1, 0x22: 1, 0x24: 1, 0x25: 1, 0x26: 1,
          0x40: 1, 0x41: 0}


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def one_dict_offset(data, offset):
    # Nitro resource dictionary: header, one tree entry, datum header, datum.
    if data[offset] != 0 or data[offset + 1] != 1:
        raise ValueError("pilot requires a single model and shape")
    if struct.unpack_from("<H", data, offset + 16)[0] != 4:
        raise ValueError("unexpected Nitro dictionary datum size")
    return u32(data, offset + 20)


def gltf_positions(directory):
    doc = json.loads((directory / "map_outgrass.gltf").read_text())
    if doc["asset"]["version"] != "2.0":
        raise ValueError("expected glTF 2.0")
    meshes = [mesh for mesh in doc["meshes"] if mesh.get("name") == "map_outgrass"]
    if len(meshes) != 1 or len(meshes[0]["primitives"]) != 1:
        raise ValueError("expected one map_outgrass mesh with one primitive")
    prim = meshes[0]["primitives"][0]
    if prim.get("mode", 4) != 4:
        raise ValueError("pilot requires triangle topology")
    indices = doc["accessors"][prim["indices"]]
    index_view = doc["bufferViews"][indices["bufferView"]]
    index_buffer = doc["buffers"][index_view["buffer"]]
    index_data = (directory / index_buffer["uri"]).read_bytes()
    index_offset = index_view.get("byteOffset", 0) + indices.get("byteOffset", 0)
    if indices["type"] != "SCALAR" or indices["componentType"] != 5123 or indices["count"] != 6:
        raise ValueError("pilot requires six uint16 triangle indices")
    if struct.unpack_from("<6H", index_data, index_offset) != (0, 1, 2, 0, 2, 3):
        raise ValueError("pilot cannot change triangle topology")
    accessor = doc["accessors"][prim["attributes"]["POSITION"]]
    if accessor["componentType"] != 5126 or accessor["type"] != "VEC3" or accessor["count"] != 4:
        raise ValueError("pilot requires four float32 positions")
    if "sparse" in accessor:
        raise ValueError("sparse positions are unsupported")
    view = doc["bufferViews"][accessor["bufferView"]]
    buffer = doc["buffers"][view["buffer"]]
    uri = buffer["uri"]
    if "/" in uri or "\\" in uri or uri.startswith("."):
        raise ValueError("position buffer must be a local filename")
    data = (directory / uri).read_bytes()
    offset = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    stride = view.get("byteStride", 12)
    if stride < 12 or offset + stride * 3 + 12 > len(data):
        raise ValueError("position buffer is too short")
    return [struct.unpack_from("<3f", data, offset + stride * i) for i in range(4)]


def encode(value, bits, fraction):
    scaled = round(value * (1 << fraction))
    if abs(value * (1 << fraction) - scaled) > 0.0001:
        raise ValueError("vertex coordinate is not representable in Nitro fixed point")
    if not -(1 << (bits - 1)) <= scaled < (1 << (bits - 1)):
        raise ValueError("vertex coordinate exceeds Nitro command range")
    return scaled & ((1 << bits) - 1)


def patch_model(chunk, positions):
    data = bytearray(chunk)
    if data[:2] != b"WB":
        raise ValueError("expected a WB map chunk")
    model_start = u32(data, 4)
    if data[model_start:model_start + 4] != b"BMD0":
        raise ValueError("missing BMD0 model")
    block = model_start + u32(data, model_start + 16)
    if data[block:block + 4] != b"MDL0":
        raise ValueError("missing MDL0 block")
    model = block + one_dict_offset(data, block + 8)
    if data[model_start + 0x34:model_start + 0x40] != b"map_outgrass":
        raise ValueError("wrong model for pilot")
    scale = u32(data, model + 28) / 4096
    shape_dict = model + u32(data, model + 12)
    shape = shape_dict + one_dict_offset(data, shape_dict)
    start = shape + u32(data, shape + 8)
    end = start + u32(data, shape + 12)
    current = [0.0, 0.0, 0.0]
    vertex = 0
    cursor = start
    while cursor < end:
        ops = data[cursor:cursor + 4]
        cursor += 4
        for op in ops:
            if op not in PARAMS:
                raise ValueError(f"unsupported Nitro display list opcode {op:#x}")
            count = PARAMS[op]
            if cursor + 4 * count > end:
                raise ValueError("truncated Nitro display list")
            if op in (0x24, 0x25, 0x26):
                if vertex >= len(positions):
                    raise ValueError("more Nitro vertices than glTF positions")
                target = [v / scale for v in positions[vertex]]
                word = u32(data, cursor)
                if op == 0x24:
                    word = (word & 0xc0000000) | encode(target[0], 10, 6) | (encode(target[1], 10, 6) << 10) | (encode(target[2], 10, 6) << 20)
                    current = target
                else:
                    axes = (0, 1) if op == 0x25 else (0, 2)
                    omitted = 2 if op == 0x25 else 1
                    if abs(target[omitted] - current[omitted]) > 0.0001:
                        raise ValueError(f"vertex {vertex} changes an axis omitted by Nitro opcode {op:#x}")
                    word = encode(target[axes[0]], 16, 12) | (encode(target[axes[1]], 16, 12) << 16)
                    current = target
                struct.pack_into("<I", data, cursor, word)
                vertex += 1
            cursor += 4 * count
    if cursor != end or vertex != len(positions):
        raise ValueError("Nitro and glTF vertex counts differ")
    return bytes(data)


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "extract":
        _, _, apicula, output = sys.argv
        chunk = read_narc((ROOT / "extract/b2_us/files/a/0/0/8").read_bytes())[CHUNK]
        model_offset = u32(chunk, 4)
        model_size = u32(chunk, model_offset + 8)
        texture = read_narc((ROOT / "extract/b2_us/files/a/0/1/4").read_bytes())[0]
        with tempfile.TemporaryDirectory() as temporary:
            model_path = Path(temporary) / "chunk_0103.nsbmd"
            texture_path = Path(temporary) / "area_000.nsbtx"
            model_path.write_bytes(chunk[model_offset:model_offset + model_size])
            texture_path.write_bytes(texture)
            subprocess.run([apicula, "convert", str(model_path), str(texture_path),
                            "-o", output, "-f", "gltf", "--overwrite"], check=True)
        return
    if len(sys.argv) != 5 or sys.argv[1] != "pack":
        raise SystemExit(__doc__)
    _, _, directory, game, output = sys.argv
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
    source = ROOT / "extract" / version / "files/a/0/0/8"
    archive = read_narc(source.read_bytes())
    archive[CHUNK] = patch_model(archive[CHUNK], gltf_positions(Path(directory)))
    dest = Path(output)
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.unlink(missing_ok=True)
    dest.write_bytes(write_narc(archive))


if __name__ == "__main__":
    main()
