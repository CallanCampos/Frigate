#!/usr/bin/env python3
"""Export the map model archive to glTF 2.0 and pack positions back into Nitro.

Usage: map_model_data.py extract APICULA OUTPUT_DIR
       map_model_data.py pack data/map_models black2|white2 OUTPUT

Mesh positions are editable for all models through direct, translated or skinned
coordinates. The original Nitro resource supplies command order and materials.
"""

import hashlib
import json
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from narc import read_narc, write_narc


ROOT = Path(__file__).resolve().parents[2]
GPU_PARAMS = {0x00: 0, 0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2, 0x24: 1,
              0x25: 1, 0x26: 1, 0x27: 1, 0x28: 1, 0x40: 1, 0x41: 0}
SBC_PARAMS = {0x00: 0, 0x01: 0, 0x02: 2, 0x03: 1, 0x04: 1, 0x05: 1,
              0x06: 3, 0x07: 1, 0x08: 1, 0x0b: 0, 0x0c: 2, 0x0d: 2,
              0x24: 1, 0x26: 4, 0x2b: 0, 0x40: 0, 0x44: 1, 0x46: 4,
              0x47: 2, 0x66: 5, 0x80: 0}


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def models(directory):
    manifest = json.loads((directory / "manifest.json").read_text())
    if manifest["archive"] != "a/0/0/8":
        raise ValueError("unexpected map model archive")
    entries = manifest["models"]
    if [entry["chunk"] for entry in entries] != list(range(1065)):
        raise ValueError("map model manifest must contain all 1,065 chunks in archive order")
    return entries


def dictionary_offsets(data, offset):
    # Nitro resource dictionary: header, tree entries, datum header, data.
    if data[offset] != 0:
        raise ValueError("unexpected Nitro dictionary header")
    count = data[offset + 1]
    datum_header = offset + 12 + 4 * count
    if struct.unpack_from("<H", data, datum_header)[0] != 4:
        raise ValueError("expected 32-bit Nitro dictionary offsets")
    return [u32(data, datum_header + 4 + 4 * i) for i in range(count)]


def model_bytes(chunk):
    if chunk[:2] not in (b"WB", b"GC", b"NG", b"RD"):
        raise ValueError("unknown map chunk type")
    offset = u32(chunk, 4)
    if chunk[offset:offset + 4] != b"BMD0":
        raise ValueError("missing BMD0 model")
    return chunk[offset:offset + u32(chunk, offset + 8)]


def accessor_values(doc, directory, accessor_index):
    accessor = doc["accessors"][accessor_index]
    if "sparse" in accessor:
        raise ValueError("sparse glTF accessors are unsupported")
    fmt = {(5121, "SCALAR"): "B", (5123, "SCALAR"): "H",
           (5125, "SCALAR"): "I", (5126, "VEC3"): "3f",
           (5121, "VEC4"): "4B", (5123, "VEC4"): "4H"}.get(
               (accessor["componentType"], accessor["type"]))
    if fmt is None:
        raise ValueError("unsupported glTF accessor format")
    view = doc["bufferViews"][accessor["bufferView"]]
    uri = doc["buffers"][view["buffer"]]["uri"]
    if "/" in uri or "\\" in uri or uri.startswith("."):
        raise ValueError("glTF buffer must be a local filename")
    data = (directory / uri).read_bytes()
    size = struct.calcsize("<" + fmt)
    stride = view.get("byteStride", size)
    offset = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    if stride < size or offset + stride * (accessor["count"] - 1) + size > len(data):
        raise ValueError("glTF accessor exceeds its buffer")
    return [struct.unpack_from("<" + fmt, data, offset + stride * i)
            for i in range(accessor["count"])]


def mesh_primitives(directory, name):
    doc = json.loads((directory / "model.gltf").read_text())
    if doc["asset"]["version"] != "2.0":
        raise ValueError("expected glTF 2.0")
    meshes = [mesh for mesh in doc["meshes"] if mesh.get("name") == name]
    if len(meshes) != 1:
        raise ValueError(f"expected one {name} mesh")
    primitives = meshes[0]["primitives"]
    if any(prim.get("mode", 4) != 4 for prim in primitives):
        raise ValueError("only triangle primitives are supported")
    return doc, primitives


def inverse_joint_position(doc, joint, position):
    if len(doc.get("skins", [])) != 1:
        raise ValueError("expected one glTF skin")
    nodes = doc["nodes"]
    parents = {}
    for parent, node in enumerate(nodes):
        for child in node.get("children", []):
            parents[child] = parent
    index = doc["skins"][0]["joints"][joint]
    path = []
    while True:
        path.append(nodes[index])
        if index not in parents:
            break
        index = parents[index]
    point = list(position)
    for node in reversed(path):
        if "matrix" in node:
            raise ValueError("glTF matrix node transforms are unsupported")
        translation = node.get("translation", (0, 0, 0))
        point = [point[i] - translation[i] for i in range(3)]
        x, y, z, w = node.get("rotation", (0, 0, 0, 1))
        # Apply the inverse quaternion, then inverse scale (TRS order).
        u = (-x, -y, -z)
        cross = (u[1] * point[2] - u[2] * point[1],
                 u[2] * point[0] - u[0] * point[2],
                 u[0] * point[1] - u[1] * point[0])
        doubled = [2 * item for item in cross]
        cross_again = (u[1] * doubled[2] - u[2] * doubled[1],
                       u[2] * doubled[0] - u[0] * doubled[2],
                       u[0] * doubled[1] - u[1] * doubled[0])
        point = [point[i] + w * doubled[i] + cross_again[i] for i in range(3)]
        scale = node.get("scale", (1, 1, 1))
        point = [point[i] / scale[i] for i in range(3)]
    return tuple(point)


def positions_from_gltf(directory, name, nitro):
    mapping = json.loads((directory / "nitro.json").read_text())
    if mapping["nitro_sha256"] != hashlib.sha256(nitro).hexdigest():
        raise ValueError(f"{name}: Nitro model differs from the glTF mapping")
    block = u32(nitro, 16)
    model = block + dictionary_offsets(nitro, block + 8)[0]
    shape_dict = model + u32(nitro, model + 12)
    shapes = [shape_dict + offset for offset in dictionary_offsets(nitro, shape_dict)]
    order = draw_order(nitro, model)
    vertex_count, original_index_sets = canonical_indices(nitro, shapes, order)
    raw_positions = decoded_positions(nitro, shapes, order, u32(nitro, model + 28) / 4096)
    baseline = raw_positions if mapping["direct"] else mapping["positions"]
    if vertex_count != len(baseline):
        raise ValueError(f"{name}: vertex count differs from Nitro")
    doc, primitives = mesh_primitives(directory, name)
    if len(primitives) != len(original_index_sets):
        raise ValueError(f"{name}: primitive count changed")
    positions = [None] * len(baseline)
    joints = [None] * len(baseline)
    for prim, original_indices in zip(primitives, original_index_sets):
        current_indices = [item[0] for item in accessor_values(doc, directory, prim["indices"])]
        current_positions = accessor_values(doc, directory, prim["attributes"]["POSITION"])
        current_joints = (accessor_values(doc, directory, prim["attributes"]["JOINTS_0"])
                          if not mapping["direct"] and "JOINTS_0" in prim["attributes"] else None)
        current_weights = (accessor_values(doc, directory, prim["attributes"]["WEIGHTS_0"])
                           if not mapping["direct"] and "WEIGHTS_0" in prim["attributes"] else None)
        if len(current_indices) != len(original_indices):
            raise ValueError(f"{name}: triangle count changed")
        for original, current in zip(original_indices, current_indices):
            if current >= len(current_positions):
                raise ValueError(f"{name}: triangle index out of range")
            value = current_positions[current]
            old = positions[original]
            if old is not None and any(abs(a - b) > 0.001 for a, b in zip(old, value)):
                raise ValueError(f"{name}: triangle topology or shared vertex changed")
            positions[original] = value
            if current_joints is not None and current_weights is not None:
                if current_weights[current] != (255, 0, 0, 0):
                    raise ValueError(f"{name}: blended skin weights are unsupported")
                joint = current_joints[current][0]
                if joints[original] is not None and joints[original] != joint:
                    raise ValueError(f"{name}: shared vertex has multiple joints")
                joints[original] = joint
    result = [value if value is not None else original
              for value, original in zip(positions, baseline)]
    if mapping["direct"]:
        return result, True
    if all(joint is not None for joint in joints):
        try:
            restored = [inverse_joint_position(doc, joint, value)
                        for joint, value in zip(joints, baseline)]
            if all(all(abs(a - b) <= 0.001 for a, b in zip(actual, original))
                   for actual, original in zip(restored, raw_positions)):
                adjusted = []
                for actual, original, raw, joint in zip(result, baseline, raw_positions, joints):
                    if all(abs(a - b) < 0.000001 for a, b in zip(actual, original)):
                        adjusted.append(raw)
                    else:
                        local = inverse_joint_position(doc, joint, actual)
                        adjusted.append(tuple(round(axis * 4096) / 4096 for axis in local))
                return adjusted, True
        except (KeyError, IndexError, ValueError, ZeroDivisionError):
            pass
    # Apicula bakes the scene graph into the glTF positions. A translation that
    # is constant across a Nitro shape can be inverted without changing the
    # display-list commands or the shape's local coordinate system.
    translated = []
    cursor = 0
    for index in order:
        shape = shapes[index]
        start = shape + u32(nitro, shape + 8)
        end = start + u32(nitro, shape + 12)
        count = sum(op in (0x23, 0x24, 0x25, 0x26, 0x27, 0x28)
                    for op, _ in gpu_commands(nitro, start, end))
        if count == 0:
            continue
        delta = tuple(baseline[cursor][axis] - raw_positions[cursor][axis]
                      for axis in range(3))
        for item in range(cursor, cursor + count):
            if any(abs(baseline[item][axis] - raw_positions[item][axis] - delta[axis]) > 0.001
                   for axis in range(3)):
                if any(any(abs(a - b) > 0.001 for a, b in zip(actual, original))
                       for actual, original in zip(result, baseline)):
                    raise ValueError(f"{name}: this model's transform is not yet editable")
                return result, False
            translated.append(tuple(result[item][axis] - delta[axis] for axis in range(3)))
        cursor += count
    return translated, True


def draw_order(data, model):
    cursor = model + u32(data, model + 4)
    order = []
    while True:
        op = data[cursor]
        cursor += 1
        if op not in SBC_PARAMS:
            raise ValueError(f"unsupported Nitro render opcode {op:#x}")
        params = data[cursor:cursor + SBC_PARAMS[op]]
        cursor += len(params)
        if op == 0x05:
            order.append(params[0])
        if op == 0x01:
            return order


def encode(value, bits, fraction):
    scaled = round(value * (1 << fraction))
    if abs(value * (1 << fraction) - scaled) > 0.001:
        raise ValueError("vertex coordinate is not representable in Nitro fixed point")
    if not -(1 << (bits - 1)) <= scaled < (1 << (bits - 1)):
        raise ValueError("vertex coordinate exceeds Nitro command range")
    return scaled & ((1 << bits) - 1)


def gpu_commands(data, start, end):
    cursor = start
    while cursor < end:
        if cursor + 4 > end:
            raise ValueError("truncated Nitro display list")
        ops = data[cursor:cursor + 4]
        cursor += 4
        for op in ops:
            if op not in GPU_PARAMS:
                raise ValueError(f"unsupported Nitro display list opcode {op:#x}")
            count = GPU_PARAMS[op]
            if cursor + 4 * count > end:
                raise ValueError("truncated Nitro display list parameters")
            yield op, cursor
            cursor += 4 * count


def canonical_indices(data, shapes, order):
    vertex = 0
    all_indices = []
    for index in order:
        shape = shapes[index]
        start = shape + u32(data, shape + 8)
        end = start + u32(data, shape + 12)
        indices = []
        first = vertex
        primitive_type = None

        def finish():
            if primitive_type == 0:
                for i in range(first, vertex - 2, 3):
                    indices.extend((i, i + 1, i + 2))
            elif primitive_type == 1:
                for i in range(first, vertex - 3, 4):
                    indices.extend((i, i + 1, i + 2, i, i + 2, i + 3))
            elif primitive_type == 2:
                for i in range(first, vertex - 2):
                    indices.extend((i, i + 1, i + 2) if (i - first) % 2 == 0 else (i, i + 2, i + 1))
            elif primitive_type == 3:
                for i in range(first, vertex - 3, 2):
                    indices.extend((i, i + 1, i + 3, i, i + 3, i + 2))

        for op, cursor in gpu_commands(data, start, end):
            if op == 0x40:
                finish()
                primitive_type = u32(data, cursor) & 3
                first = vertex
            elif op == 0x41:
                finish()
                primitive_type = None
                first = vertex
            elif op in (0x23, 0x24, 0x25, 0x26, 0x27, 0x28):
                vertex += 1
        finish()
        all_indices.append(indices)
    return vertex, all_indices


def signed(value, bits):
    return (value ^ (1 << (bits - 1))) - (1 << (bits - 1))


def decoded_positions(data, shapes, order, scale):
    positions = []
    for index in order:
        shape = shapes[index]
        start = shape + u32(data, shape + 8)
        end = start + u32(data, shape + 12)
        current = [0.0, 0.0, 0.0]
        for op, cursor in gpu_commands(data, start, end):
            if op == 0x23:
                a, b = struct.unpack_from("<II", data, cursor)
                current = [signed(a & 0xffff, 16) / 4096,
                           signed(a >> 16, 16) / 4096,
                           signed(b & 0xffff, 16) / 4096]
            elif op == 0x24:
                word = u32(data, cursor)
                current = [signed((word >> (10 * i)) & 0x3ff, 10) / 64 for i in range(3)]
            elif op in (0x25, 0x26, 0x27):
                word = u32(data, cursor)
                axes = {0x25: (0, 1), 0x26: (0, 2), 0x27: (1, 2)}[op]
                current[axes[0]] = signed(word & 0xffff, 16) / 4096
                current[axes[1]] = signed(word >> 16, 16) / 4096
            elif op == 0x28:
                word = u32(data, cursor)
                current = [current[i] + signed((word >> (10 * i)) & 0x3ff, 10) / 4096
                           for i in range(3)]
            else:
                continue
            positions.append(tuple(value * scale for value in current))
    return positions


def patch_display_list(data, start, end, positions, vertex, scale):
    current = [0.0, 0.0, 0.0]
    for op, cursor in gpu_commands(data, start, end):
        if op in (0x23, 0x24, 0x25, 0x26, 0x27, 0x28):
            if vertex >= len(positions):
                raise ValueError("more Nitro vertices than glTF positions")
            target = [v / scale for v in positions[vertex]]
            word = u32(data, cursor)
            if op == 0x23:
                first = encode(target[0], 16, 12) | (encode(target[1], 16, 12) << 16)
                second = (u32(data, cursor + 4) & 0xffff0000) | encode(target[2], 16, 12)
                struct.pack_into("<II", data, cursor, first, second)
            elif op == 0x24:
                word = (word & 0xc0000000) | sum(encode(target[i], 10, 6) << (10 * i)
                                                   for i in range(3))
                struct.pack_into("<I", data, cursor, word)
            elif op == 0x28:
                word = (word & 0xc0000000) | sum(encode(target[i] - current[i], 10, 12) << (10 * i)
                                                   for i in range(3))
                struct.pack_into("<I", data, cursor, word)
            else:
                axes = {0x25: (0, 1), 0x26: (0, 2), 0x27: (1, 2)}[op]
                omitted = ({0, 1, 2} - set(axes)).pop()
                if abs(target[omitted] - current[omitted]) > 0.0001:
                    raise ValueError(f"vertex {vertex} changes an axis omitted by Nitro opcode {op:#x}")
                word = encode(target[axes[0]], 16, 12) | (encode(target[axes[1]], 16, 12) << 16)
                struct.pack_into("<I", data, cursor, word)
            current = target
            vertex += 1
    return vertex


def patch_model(chunk, positions, name):
    data = bytearray(chunk)
    model_start = u32(data, 4)
    if data[:2] not in (b"WB", b"GC", b"NG", b"RD") or data[model_start:model_start + 4] != b"BMD0":
        raise ValueError("expected a map chunk with a BMD0 model")
    block = model_start + u32(data, model_start + 16)
    if data[block:block + 4] != b"MDL0":
        raise ValueError("missing MDL0 block")
    offsets = dictionary_offsets(data, block + 8)
    if len(offsets) != 1:
        raise ValueError("expected a single model per map chunk")
    model = block + offsets[0]
    model_name = data[block + 32:block + 48].split(b"\0")[0].decode("ascii")
    if model_name != name:
        raise ValueError(f"expected {name}, found {model_name}")
    scale = u32(data, model + 28) / 4096
    shape_dict = model + u32(data, model + 12)
    shapes = [shape_dict + offset for offset in dictionary_offsets(data, shape_dict)]
    order = draw_order(data, model)
    if sorted(order) != list(range(len(shapes))):
        raise ValueError("expected each Nitro shape to be drawn once")
    vertex = 0
    for index in order:
        shape = shapes[index]
        start = shape + u32(data, shape + 8)
        vertex = patch_display_list(data, start, start + u32(data, shape + 12),
                                    positions, vertex, scale)
    if vertex != len(positions):
        raise ValueError("Nitro and glTF vertex counts differ")
    return bytes(data)


def export(apicula, output):
    archive = read_narc((ROOT / "extract/b2_us/files/a/0/0/8").read_bytes())
    textures = read_narc((ROOT / "extract/b2_us/files/a/0/1/4").read_bytes())
    output.mkdir(parents=True, exist_ok=True)
    texture_root = output.parent / "map_texture_sets"
    for entry in models(ROOT / "data/map_models"):
        chunk_id, name, texture_id = entry["chunk"], entry["name"], entry["texture_set"]
        nitro = model_bytes(archive[chunk_id])
        model_dir = output / f"{chunk_id:04d}"
        model_dir.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory() as temporary:
            model_path = Path(temporary) / f"chunk_{chunk_id:04d}.nsbmd"
            converted = Path(temporary) / "converted"
            model_path.write_bytes(nitro)
            inputs = [str(model_path)]
            if texture_id is not None:
                texture_path = Path(temporary) / f"area_{texture_id:03d}.nsbtx"
                texture_path.write_bytes(textures[texture_id])
                inputs.append(str(texture_path))
            subprocess.run([apicula, "convert", *inputs, "-o", str(converted), "-f", "gltf"],
                           check=True, capture_output=True)
            for path in converted.iterdir():
                if path.suffix == ".png":
                    if texture_id is None:
                        raise ValueError(f"{chunk_id}: unexpected texture without a texture set")
                    texture_dir = texture_root / f"{texture_id:03d}"
                    texture_dir.mkdir(parents=True, exist_ok=True)
                    target = texture_dir / path.name
                    if target.exists():
                        if target.read_bytes() != path.read_bytes():
                            raise ValueError(f"{chunk_id}: {target} differs across models")
                    else:
                        shutil.copyfile(path, target)
                else:
                    target = "model.gltf" if path.suffix == ".gltf" else "model.bin" if path.suffix == ".bin" else path.name
                    shutil.copyfile(path, model_dir / target)
        document = json.loads((model_dir / "model.gltf").read_text())
        for buffer in document["buffers"]:
            buffer["uri"] = "model.bin"
        if texture_id is not None:
            for picture in document.get("images", []):
                if "uri" in picture:
                    picture["uri"] = f"../../map_texture_sets/{texture_id:03d}/{picture['uri']}"
        (model_dir / "model.gltf").write_text(json.dumps(document, indent=2) + "\n")
        doc, primitives = mesh_primitives(model_dir, name)
        positions = accessor_values(doc, model_dir, primitives[0]["attributes"]["POSITION"])
        block = u32(nitro, 16)
        model = block + dictionary_offsets(nitro, block + 8)[0]
        shape_dict = model + u32(nitro, model + 12)
        shapes = [shape_dict + offset for offset in dictionary_offsets(nitro, shape_dict)]
        direct_positions = decoded_positions(nitro, shapes, draw_order(nitro, model),
                                             u32(nitro, model + 28) / 4096)
        direct = len(positions) == len(direct_positions) and all(
            all(abs(a - b) < 0.001 for a, b in zip(actual, raw))
            for actual, raw in zip(positions, direct_positions))
        if direct:
            try:
                direct = patch_model(archive[chunk_id], positions, name) == archive[chunk_id]
            except ValueError:
                direct = False
        mapping = {"nitro_sha256": hashlib.sha256(nitro).hexdigest(), "direct": direct}
        if not direct:
            mapping["positions"] = positions
        (model_dir / "nitro.json").write_text(json.dumps(mapping, indent=2) + "\n")


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "extract":
        export(sys.argv[2], Path(sys.argv[3]))
        return
    if len(sys.argv) != 5 or sys.argv[1] != "pack":
        raise SystemExit(__doc__)
    _, _, directory, game, output = sys.argv
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
    source = ROOT / "extract" / version / "files/a/0/0/8"
    archive = read_narc(source.read_bytes())
    for entry in models(Path(directory)):
        chunk_id, name = entry["chunk"], entry["name"]
        positions, direct = positions_from_gltf(Path(directory) / f"{chunk_id:04d}",
                                                name, model_bytes(archive[chunk_id]))
        if direct:
            archive[chunk_id] = patch_model(archive[chunk_id], positions, name)
    dest = Path(output)
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.unlink(missing_ok=True)
    dest.write_bytes(write_narc(archive))


if __name__ == "__main__":
    main()
