#!/usr/bin/env python3
"""Export field map GLBs and terrain, then pack edited geometry back into Nitro.

Usage: map_model_data.py extract APICULA OUTPUT_DIR [CHUNK_ID ...]
       map_model_data.py relink SOURCE_DIR
       map_model_data.py pack data/map_models black2|white2 OUTPUT

Each map model is a directory named after its constant in data/constants/map_models.txt, with its GLB, its terrain
CSVs and its data.json (map_model.schema.json). relink points every GLB's images at its texture set's directory again,
after a texture set is renamed.

Unchanged topology keeps the original Nitro display lists byte for byte. A mesh
that changes its triangles is encoded as new DS GPU commands for each draw call.
"""

import json
import math
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from narc import read_narc, write_narc
from map_texture_data import decode, decode_png, palette_for, read_png, resources
from map_terrain_data import export_terrains, patch_terrains
from nitro_dictionary import build_dictionary, replace_names
from graphics_paths import directories


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.data.datajson import DataError, load, load_schema, value, write  # noqa: E402
GLB_JSON = 0x4e4f534a
GLB_BIN = 0x004e4942
PROP_NAME = re.compile(r"bw2_prop_(\d+)_id_(\d+)(?:\.\d+)?$")
PROP_OFFSETS = {b"WB": (12, 16), b"GC": (16, 20), b"NG": (8, 12), b"RD": (12, 16)}
GPU_PARAMS = {0x00: 0, 0x14: 1, 0x1b: 3, 0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2, 0x24: 1,
              0x25: 1, 0x26: 1, 0x27: 1, 0x28: 1, 0x40: 1, 0x41: 0}
# The errors of an edit that the original draw commands cannot hold, which re-encode the model's triangles instead
FALLBACK_ERRORS = ("triangle count changed", "triangle topology or shared vertex changed",
                   "one Nitro vertex has multiple edited UVs", "vertices sharing a Nitro UV command",
                   "this shape has no Nitro texture commands", "this UV has no editable Nitro texture command",
                   "UV before the first Nitro texture command", "changes an axis omitted by Nitro opcode",
                   "vertex coordinate is not representable", "vertex coordinate exceeds Nitro command range")
SBC_PARAMS = {0x00: 0, 0x01: 0, 0x02: 2, 0x03: 1, 0x04: 1, 0x05: 1,
              0x06: 3, 0x07: 1, 0x08: 1, 0x0b: 0, 0x0c: 2, 0x0d: 2,
              0x24: 1, 0x26: 4, 0x2b: 0, 0x40: 0, 0x44: 1, 0x46: 4,
              0x47: 2, 0x66: 5, 0x80: 0}


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def models(directory, count):
    """Returns the map models' directories, in archive order, and their data, which name their texture sets."""
    folders = directories("a/0/0/8", directory)
    if len(folders) != count:
        raise ValueError(f"a/0/0/8 has {count} map models and map_models.txt {len(folders)}")
    schema = load_schema(ROOT / "data/map_models/map_model.schema.json")
    return [(folder, load(folder / "data.json", schema)) for folder in folders]


def texture_set(data, where):
    """Returns the texture set whose PNGs a model's GLB shows: its own, or else the one it is previewed with."""
    name = data["texture_set"] if data["texture_set"] is not None else data.get("texture_preview")
    return value(name, where) if name is not None else None


def model_name(nitro):
    """Returns the name of a BMD0 file's one model."""
    block = u32(nitro, 16)
    if len(dictionary_offsets(nitro, block + 8)) != 1:
        raise ValueError("expected one model")
    return nitro[block + 32:block + 48].split(b"\0")[0].decode("ascii")


def image_uri(texture_dir, filename):
    """Returns a GLB's path to one of its texture set's PNGs, from data/map_models/<model>/."""
    return f"../../map_texture_sets/{texture_dir.name}/{filename}"


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


def read_glb(path):
    data = path.read_bytes()
    if len(data) < 20 or data[:4] != b"glTF" or u32(data, 4) != 2 or u32(data, 8) != len(data):
        raise ValueError(f"{path}: expected glTF 2.0 binary")
    offset = 12
    chunks = []
    while offset < len(data):
        if offset + 8 > len(data):
            raise ValueError(f"{path}: truncated GLB chunk")
        size, kind = struct.unpack_from("<II", data, offset)
        offset += 8
        if size % 4 or offset + size > len(data):
            raise ValueError(f"{path}: invalid GLB chunk size")
        chunks.append((kind, data[offset:offset + size]))
        offset += size
    if not chunks or chunks[0][0] != GLB_JSON or len(chunks) > 2:
        raise ValueError(f"{path}: unexpected GLB chunks")
    doc = json.loads(chunks[0][1])
    binary = chunks[1][1] if len(chunks) == 2 and chunks[1][0] == GLB_BIN else b""
    if len(doc.get("buffers", [])) != 1 or "uri" in doc["buffers"][0]:
        raise ValueError(f"{path}: expected one embedded glTF buffer")
    if doc["buffers"][0]["byteLength"] > len(binary):
        raise ValueError(f"{path}: GLB binary chunk is too short")
    return doc, binary


def write_glb(path, doc, binary):
    if len(doc.get("buffers", [])) != 1:
        raise ValueError(f"{path}: expected one glTF buffer")
    doc["buffers"][0].pop("uri", None)
    if doc["buffers"][0]["byteLength"] > len(binary):
        raise ValueError(f"{path}: glTF buffer is too short")
    encoded = json.dumps(doc, indent=2).encode("utf-8")
    encoded += b" " * (-len(encoded) % 4)
    binary += b"\0" * (-len(binary) % 4)
    data = (struct.pack("<4sII", b"glTF", 2, 12 + 8 + len(encoded) + 8 + len(binary))
            + struct.pack("<II", len(encoded), GLB_JSON) + encoded
            + struct.pack("<II", len(binary), GLB_BIN) + binary)
    path.write_bytes(data)


def canonical_texture_name(filename, names):
    stem = Path(filename).stem
    if stem in names:
        return filename
    matches = [name for name in names if name.replace(".", "_") == stem]
    if len(matches) != 1:
        raise ValueError(f"cannot identify Nitro texture for {filename}")
    return matches[0] + ".png"


def prop_section(chunk):
    if chunk[:2] not in PROP_OFFSETS:
        raise ValueError("unknown field map chunk type")
    offset_field, end_field = PROP_OFFSETS[chunk[:2]]
    offset, end = u32(chunk, offset_field), u32(chunk, end_field)
    if end != len(chunk) or offset + 4 > end:
        raise ValueError("invalid field prop section bounds")
    count = u32(chunk, offset)
    if offset + 4 + 16 * count != end:
        raise ValueError("field prop section size differs from count")
    return offset, end_field, count


def add_prop_nodes(doc, chunk):
    offset, _, count = prop_section(chunk)
    roots = doc["scenes"][doc.get("scene", 0)].setdefault("nodes", [])
    nodes = doc.setdefault("nodes", [])
    for index in range(count):
        x, y, z, rotation, high, low = struct.unpack_from("<iiiHBB", chunk, offset + 4 + 16 * index)
        angle = rotation * math.tau / 65536
        nodes.append({"name": f"bw2_prop_{index:04d}_id_{(high << 8) | low:05d}",
                      "translation": [x / 4096, y / 4096, -z / 4096],
                      "rotation": [0, math.sin(angle / 2), 0, math.cos(angle / 2)]})
        roots.append(len(nodes) - 1)


def patch_props(chunk, doc):
    offset, end_field, _ = prop_section(chunk)
    props = []
    roots = set(doc["scenes"][doc.get("scene", 0)].get("nodes", []))
    for node_index, node in enumerate(doc.get("nodes", [])):
        name = node.get("name", "")
        if not name.startswith("bw2_prop_"):
            continue
        match = PROP_NAME.fullmatch(name)
        if match is None:
            raise ValueError(f"invalid field prop node name: {name}")
        prop_id = int(match[2])
        if prop_id > 65535:
            raise ValueError(f"field prop ID exceeds 16 bits: {prop_id}")
        if node_index not in roots:
            raise ValueError(f"{name}: field props must stay at the scene root")
        if any(abs(value - 1) > 0.0001 for value in node.get("scale", [1, 1, 1])):
            raise ValueError(f"{name}: field props cannot be scaled")
        position = node.get("translation", [0, 0, 0])
        if len(position) != 3:
            raise ValueError(f"{name}: expected a 3D translation")
        coords = [round(value * 4096) for value in (position[0], position[1], -position[2])]
        if any(not -(1 << 31) <= value < (1 << 31) for value in coords):
            raise ValueError(f"{name}: position exceeds Nitro fixed point range")
        qx, qy, qz, qw = node.get("rotation", [0, 0, 0, 1])
        if abs(qx) > 0.0001 or abs(qz) > 0.0001 or abs(qx*qx + qy*qy + qz*qz + qw*qw - 1) > 0.0001:
            raise ValueError(f"{name}: prop supports only rotation around Y")
        angle = 2 * math.atan2(qy, qw)
        rotation = round(angle * 65536 / math.tau) & 0xffff
        props.append((int(match[1]), struct.pack("<iiiHBB", *coords, rotation, prop_id >> 8, prop_id & 255)))
    props.sort(key=lambda item: item[0])
    section = struct.pack("<I", len(props)) + b"".join(data for _, data in props)
    result = bytearray(chunk[:offset] + section)
    struct.pack_into("<I", result, end_field, len(result))
    return bytes(result)


def blended(weights):
    """Whether a vertex's WEIGHTS_0 blends joints, rather than giving its first joint all the weight, whether the GLB
    stores weights as normalized bytes or shorts or as floats."""
    return any(weight != 0 for weight in weights[1:]) or not weights[0]


def accessor_values(doc, binary, accessor_index):
    accessor = doc["accessors"][accessor_index]
    if "sparse" in accessor:
        raise ValueError("sparse glTF accessors are unsupported")
    fmt = {(5121, "SCALAR"): "B", (5123, "SCALAR"): "H",
           (5125, "SCALAR"): "I", (5126, "VEC2"): "2f", (5126, "VEC3"): "3f",
           (5121, "VEC3"): "3B", (5123, "VEC3"): "3H",
           (5121, "VEC4"): "4B", (5123, "VEC4"): "4H", (5126, "VEC4"): "4f"}.get(
               (accessor["componentType"], accessor["type"]))
    if fmt is None:
        raise ValueError("unsupported glTF accessor format")
    view = doc["bufferViews"][accessor["bufferView"]]
    if view["buffer"] != 0:
        raise ValueError("expected an accessor in the embedded GLB buffer")
    size = struct.calcsize("<" + fmt)
    stride = view.get("byteStride", size)
    offset = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    if stride < size or offset + stride * (accessor["count"] - 1) + size > doc["buffers"][0]["byteLength"]:
        raise ValueError("glTF accessor exceeds its buffer")
    return [struct.unpack_from("<" + fmt, binary, offset + stride * i)
            for i in range(accessor["count"])]


def mesh_primitives(directory, name):
    doc, binary = read_glb(directory / "model.glb")
    if doc["asset"]["version"] != "2.0":
        raise ValueError("expected glTF 2.0")
    meshes = [mesh for mesh in doc["meshes"] if mesh.get("name") == name]
    if len(meshes) != 1:
        raise ValueError(f"expected one {name} mesh")
    if len(doc["meshes"]) != 1:
        raise ValueError(f"{name}: extra GLB meshes cannot be packed into this Nitro model")
    mesh_nodes = [node for node in doc.get("nodes", []) if node.get("mesh") == 0]
    if len(mesh_nodes) != 1:
        raise ValueError(f"{name}: expected one instance of the GLB mesh")
    node = mesh_nodes[0]
    if ("matrix" in node or any(abs(value) > 0.00001 for value in node.get("translation", (0, 0, 0)))
            or any(abs(value - target) > 0.00001 for value, target in zip(
                node.get("rotation", (0, 0, 0, 1)), (0, 0, 0, 1)))
            or any(abs(value - 1) > 0.00001 for value in node.get("scale", (1, 1, 1)))):
        raise ValueError(f"{name}: apply the mesh object's transforms in Blender before exporting")
    primitives = meshes[0]["primitives"]
    if any(prim.get("mode", 4) != 4 for prim in primitives):
        raise ValueError("only triangle primitives are supported")
    return doc, primitives, binary


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


def forward_joint_position(doc, joint, position):
    """Recreate Apicula's baked vertex position from a Nitro local position."""
    nodes = doc["nodes"]
    parents = {child: parent for parent, node in enumerate(nodes)
               for child in node.get("children", [])}
    index = doc["skins"][0]["joints"][joint]
    point = list(position)
    while True:
        node = nodes[index]
        if "matrix" in node:
            raise ValueError("glTF matrix node transforms are unsupported")
        scale = node.get("scale", (1, 1, 1))
        point = [point[i] * scale[i] for i in range(3)]
        x, y, z, w = node.get("rotation", (0, 0, 0, 1))
        cross = (y * point[2] - z * point[1], z * point[0] - x * point[2],
                 x * point[1] - y * point[0])
        doubled = [2 * item for item in cross]
        cross_again = (y * doubled[2] - z * doubled[1],
                       z * doubled[0] - x * doubled[2],
                       x * doubled[1] - y * doubled[0])
        translation = node.get("translation", (0, 0, 0))
        point = [point[i] + w * doubled[i] + cross_again[i] + translation[i]
                 for i in range(3)]
        if index not in parents:
            return tuple(point)
        index = parents[index]


def positions_from_gltf(directory, name, nitro, mapping):
    block = u32(nitro, 16)
    model = block + dictionary_offsets(nitro, block + 8)[0]
    shape_dict = model + u32(nitro, model + 12)
    shapes = [shape_dict + offset for offset in dictionary_offsets(nitro, shape_dict)]
    order = draw_order(nitro, model)
    vertex_count, original_index_sets = canonical_indices(nitro, shapes, order)
    raw_positions = decoded_positions(nitro, shapes, order, u32(nitro, model + 28) / 4096)
    baseline = raw_positions if mapping["direct"] else mapping.get("positions")
    if vertex_count != len(raw_positions) or (baseline is not None and vertex_count != len(baseline)):
        raise ValueError(f"{name}: vertex count differs from Nitro")
    doc, primitives, binary = mesh_primitives(directory, name)
    if len(primitives) != len(original_index_sets):
        raise ValueError(f"{name}: primitive count changed")
    positions = [None] * vertex_count
    joints = [None] * vertex_count
    for prim, original_indices in zip(primitives, original_index_sets):
        current_indices = [item[0] for item in accessor_values(doc, binary, prim["indices"])]
        current_positions = accessor_values(doc, binary, prim["attributes"]["POSITION"])
        current_joints = (accessor_values(doc, binary, prim["attributes"]["JOINTS_0"])
                          if not mapping["direct"] and "JOINTS_0" in prim["attributes"] else None)
        current_weights = (accessor_values(doc, binary, prim["attributes"]["WEIGHTS_0"])
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
                if blended(current_weights[current]):
                    raise ValueError(f"{name}: blended skin weights are unsupported")
                joint = current_joints[current][0]
                if joints[original] is not None and joints[original] != joint:
                    raise ValueError(f"{name}: shared vertex has multiple joints")
                joints[original] = joint
    if baseline is None:
        baseline = [forward_joint_position(doc, joint, raw) if joint is not None else raw
                    for joint, raw in zip(joints, raw_positions)]
    result = [value if value is not None else original
              for value, original in zip(positions, baseline)]
    if mapping["direct"]:
        return result, True, (doc, primitives, binary)
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
                return adjusted, True, (doc, primitives, binary)
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
                return result, False, (doc, primitives, binary)
            translated.append(tuple(result[item][axis] - delta[axis] for axis in range(3)))
        cursor += count
    return translated, True, (doc, primitives, binary)


def draw_order(data, model):
    return [shape for shape, _ in draw_calls(data, model)]


def draw_calls(data, model):
    cursor = model + u32(data, model + 4)
    calls = []
    material = 0
    while True:
        op = data[cursor]
        cursor += 1
        if op == 0x09:
            length = 2 + 3 * data[cursor + 1]
        elif op in SBC_PARAMS:
            length = SBC_PARAMS[op]
        else:
            raise ValueError(f"unsupported Nitro render opcode {op:#x}")
        params = data[cursor:cursor + length]
        cursor += len(params)
        if op in (0x04, 0x24, 0x44):
            material = params[0]
        if op == 0x05:
            calls.append((params[0], material))
        if op == 0x01:
            return calls


def material_uv_transform(data, model, material_index):
    material_dict = model + u32(data, model + 8)
    offsets = dictionary_offsets(data, material_dict + 4)
    if material_index >= len(offsets):
        raise ValueError("Nitro draw call uses an invalid material")
    entry = material_dict + offsets[material_index]
    width, height = struct.unpack_from("<HH", data, entry + 32)
    if not width or not height:
        raise ValueError("Nitro material has zero texture dimensions")
    flags = struct.unpack_from("<H", data, entry + 30)[0] & 15
    # Apicula applies the two stored scale components for this material form.
    # Its other matrix forms currently export with an identity texture matrix.
    if flags == 13:
        scale = tuple(value / 4096 for value in struct.unpack_from("<ii", data, entry + 44))
    else:
        scale = (1.0, 1.0)
    return width, height, scale


def color5(word):
    return [(word >> shift & 31) / 31 for shift in (0, 5, 10)]


def pack_color5(values, name):
    if len(values) != 3 or any(not math.isfinite(value) or not 0 <= value <= 1 for value in values):
        raise ValueError(f"{name}: material color must have three values from 0 to 1")
    return sum(round(value * 31) << shift for value, shift in zip(values, (0, 5, 10)))


def patch_materials(chunk, doc, primitives, name):
    data = bytearray(chunk)
    model_start = u32(data, 4)
    block = model_start + u32(data, model_start + 16)
    model = block + dictionary_offsets(data, block + 8)[0]
    material_dict = model + u32(data, model + 8)
    entries = [material_dict + offset for offset in dictionary_offsets(data, material_dict + 4)]
    calls = draw_calls(data, model)
    if len(calls) != len(primitives):
        raise ValueError(f"{name}: Nitro and GLB draw call counts differ")
    mapping = {}
    for (_, original), primitive in zip(calls, primitives):
        current = primitive.get("material")
        if current is None or current >= len(doc.get("materials", [])):
            raise ValueError(f"{name}: GLB material is missing")
        if original in mapping and mapping[original] != current:
            raise ValueError(f"{name}: one Nitro material maps to multiple GLB materials")
        mapping[original] = current
    for original, current in mapping.items():
        entry = entries[original]
        material = doc["materials"][current]
        dif_amb = u32(data, entry + 4)
        spe_emi = u32(data, entry + 8)
        polygon = u32(data, entry + 12)
        pbr = material.get("pbrMetallicRoughness", {})
        factor = pbr.get("baseColorFactor", [1, 1, 1, 1])
        if len(factor) != 4 or any(not math.isfinite(value) or not 0 <= value <= 1 for value in factor):
            raise ValueError(f"{name}: invalid GLB base color factor")
        baseline_rgb = [1, 1, 1] if dif_amb & 0x8000 else color5(dif_amb)
        if any(abs(a - b) > 0.00001 for a, b in zip(factor[:3], baseline_rgb)):
            dif_amb = (dif_amb & 0xffff0000) | pack_color5(factor[:3], name)
            struct.pack_into("<I", data, entry + 4, dif_amb)
        baseline_alpha = ((polygon >> 16) & 31) / 31
        if abs(factor[3] - baseline_alpha) > 0.00001:
            polygon = (polygon & ~(31 << 16)) | (round(factor[3] * 31) << 16)
        double_sided = material.get("doubleSided", False)
        if not isinstance(double_sided, bool):
            raise ValueError(f"{name}: GLB doubleSided must be a boolean")
        if double_sided != bool(polygon & (1 << 6)):
            polygon = (polygon | (1 << 6)) if double_sided else (polygon & ~(1 << 6))
        struct.pack_into("<I", data, entry + 12, polygon)
        if "emissiveFactor" in material:
            emission = material["emissiveFactor"]
            if len(emission) != 3:
                raise ValueError(f"{name}: invalid GLB emissive color")
            if any(abs(a - b) > 0.00001 for a, b in zip(emission, color5(spe_emi >> 16))):
                spe_emi = (spe_emi & 0x8000ffff) | (pack_color5(emission, name) << 16)
            struct.pack_into("<I", data, entry + 8, spe_emi)
    return bytes(data)


def pairing_entries(data, material_dict, field):
    dictionary = material_dict + struct.unpack_from("<H", data, material_dict + field)[0]
    count = data[dictionary + 1]
    header = dictionary + 12 + 4 * count
    if struct.unpack_from("<H", data, header)[0] != 4:
        raise ValueError("expected four-byte material binding records")
    names = header + 4 + 4 * count
    entries = []
    for index in range(count):
        offset, size, flag = struct.unpack_from("<HBB", data, header + 4 + 4 * index)
        name = data[names + 16 * index:names + 16 * (index + 1)].split(b"\0")[0].decode("ascii")
        entries.append((name, list(data[material_dict + offset:material_dict + offset + size]),
                        header + 4 + 4 * index, offset, flag))
    return dictionary, entries


def patch_pairings(data, material_dict, field, requested, name):
    dictionary, entries = pairing_entries(data, material_dict, field)
    owners = {material: binding for binding, ids, _, _, _ in entries for material in ids}
    changes = {material: binding for material, binding in requested.items()
               if owners.get(material) != binding}
    if not changes:
        return set()
    if any(binding is None for binding in changes.values()):
        raise ValueError(f"{name}: binding removal needs a rebuilt table")
    if any(material not in owners for material in changes):
        raise ValueError(f"{name}: cannot add a binding to a previously unbound material")
    grouped = {binding: [material for material in ids if material not in changes]
               for binding, ids, _, _, _ in entries}
    for material, binding in changes.items():
        grouped.setdefault(binding, []).append(material)
    names = [entry[0] for entry in entries]
    for binding in changes.values():
        if binding in names:
            continue
        available = next((index for index, old in enumerate(names) if not grouped[old]
                          and old not in changes.values()), None)
        if available is None:
            raise ValueError(f"{name}: no free Nitro binding entry for {binding}")
        names[available] = binding
    used = sorted((offset for _, ids, _, offset, _ in entries if ids))
    if not used:
        raise ValueError(f"{name}: model has no material binding list")
    base = used[0]
    cursor = base
    for binding, (_, original_ids, record, _, flag) in zip(names, entries):
        ids = sorted(grouped[binding])
        if len(ids) > 255 or cursor > 65535:
            raise ValueError(f"{name}: Nitro binding list exceeds its field range")
        struct.pack_into("<HBB", data, record, cursor, len(ids), flag)
        data[material_dict + cursor:material_dict + cursor + len(ids)] = bytes(ids)
        cursor += len(ids)
    if cursor != base + sum(len(ids) for _, ids, _, _, _ in entries):
        raise ValueError(f"{name}: material binding count changed")
    if names != [entry[0] for entry in entries]:
        replace_names(data, dictionary, names)
    return set(changes)


def rebuild_pairing_tables(chunk, requested_textures, requested_palettes, name):
    data = bytearray(chunk)
    model_start = u32(data, 4)
    nitro_length = u32(data, model_start + 8)
    block = model_start + u32(data, model_start + 16)
    model = block + dictionary_offsets(data, block + 8)[0]
    material_dict = model + u32(data, model + 8)
    insertion = model + u32(data, model + 12)
    tables = []
    changed = []
    for field, requested in ((0, requested_textures), (2, requested_palettes)):
        _, entries = pairing_entries(data, material_dict, field)
        owners = {material: binding for binding, ids, _, _, _ in entries for material in ids}
        changes = {material for material, binding in requested.items() if owners.get(material) != binding}
        groups = {binding: [material for material in ids if material not in changes]
                  for binding, ids, _, _, _ in entries}
        for material in changes:
            if requested[material] is not None:
                groups.setdefault(requested[material], []).append(material)
        names = [entry[0] for entry in entries if groups[entry[0]]]
        names += [binding for binding in requested.values() if binding not in names and groups.get(binding)]
        names = list(dict.fromkeys(names))
        tables.append((names, groups))
        changed.append(changes)
    relative = insertion - material_dict
    dictionary_sizes = [16 + 24 * len(names) for names, _ in tables]
    list_cursor = relative + sum(dictionary_sizes)
    dictionaries = []
    lists = bytearray()
    for names, groups in tables:
        records = []
        for binding in names:
            ids = sorted(groups[binding])
            if len(ids) > 255 or list_cursor > 65535:
                raise ValueError(f"{name}: material binding table exceeds Nitro offset range")
            records.append(struct.pack("<HBB", list_cursor, len(ids), 0))
            lists.extend(ids)
            list_cursor += len(ids)
        dictionaries.append(build_dictionary(names, records))
    payload = b"".join(dictionaries) + lists
    payload += b"\0" * (-len(payload) % 4)
    if relative + len(dictionaries[0]) > 65535:
        raise ValueError(f"{name}: material binding dictionary exceeds Nitro offset range")
    struct.pack_into("<HH", data, material_dict, relative, relative + len(dictionaries[0]))
    delta = len(payload)
    for field in (4, 8, 12, 16):
        offset = u32(data, model + field)
        if offset and model + offset >= insertion:
            struct.pack_into("<I", data, model + field, offset + delta)
    struct.pack_into("<I", data, model, u32(data, model) + delta)
    struct.pack_into("<I", data, block + 4, u32(data, block + 4) + delta)
    struct.pack_into("<I", data, model_start + 8, nitro_length + delta)
    for field in {b"WB": (8, 12, 16), b"GC": (8, 12, 16, 20),
                  b"NG": (8, 12), b"RD": (8, 12, 16)}[bytes(data[:2])]:
        struct.pack_into("<I", data, field, u32(data, field) + delta)
    return bytearray(data[:insertion] + payload + data[insertion:]), changed[0], changed[1]


def patch_material_bindings(chunk, doc, primitives, texture_blob, name, binary=None,
                            texture_directory=None):
    if texture_blob is None:
        if any("baseColorTexture" in material.get("pbrMetallicRoughness", {})
               for material in doc.get("materials", [])):
            raise ValueError(f"{name}: map has no selected texture set for GLB texture assignment")
        return chunk, False
    textures, palettes = resources(texture_blob)
    by_name = {texture["name"]: (index, texture) for index, texture in enumerate(textures)}
    data = bytearray(chunk)
    model_start = u32(data, 4)
    block = model_start + u32(data, model_start + 16)
    model = block + dictionary_offsets(data, block + 8)[0]
    material_dict = model + u32(data, model + 8)
    entries = [material_dict + offset for offset in dictionary_offsets(data, material_dict + 4)]
    original_textures = {material: binding for binding, ids, _, _, _ in
                         pairing_entries(data, material_dict, 0)[1] for material in ids}
    calls = draw_calls(data, model)
    if len(calls) != len(primitives):
        raise ValueError(f"{name}: GLB draw call count changed")
    wanted_textures = {}
    wanted_palettes = {}
    for (_, material_id), primitive in zip(calls, primitives):
        current = primitive.get("material")
        if current is None or current >= len(doc.get("materials", [])):
            raise ValueError(f"{name}: GLB material is missing")
        pbr = doc["materials"][current].get("pbrMetallicRoughness", {})
        binding = pbr.get("baseColorTexture")
        if binding is None:
            if original_textures.get(material_id) in by_name:
                wanted_textures[material_id] = None
                wanted_palettes[material_id] = None
            continue
        texture_index = binding["index"]
        image_index = doc["textures"][texture_index]["source"]
        image = doc["images"][image_index]
        image_name = Path(image["uri"]).name if "uri" in image else image.get("name")
        if not image_name:
            raise ValueError(f"{name}: GLB texture needs an image name or PNG URI")
        filename = image_name if image_name.endswith(".png") else image_name + ".png"
        texture_name = Path(canonical_texture_name(filename, by_name)).stem
        index, texture = by_name[texture_name]
        palette = palette_for(texture, palettes, index)
        if binary is not None and "bufferView" in image:
            if image.get("mimeType") != "image/png":
                raise ValueError(f"{name}: embedded texture {texture_name} must be a PNG")
            view = doc["bufferViews"][image["bufferView"]]
            if view["buffer"] != 0:
                raise ValueError(f"{name}: embedded texture is outside the GLB buffer")
            start = view.get("byteOffset", 0)
            end = start + view["byteLength"]
            embedded = decode_png(binary[start:end], texture["width"], texture["height"],
                                  f"{name}: {texture_name}")
            original_pixels = decode(texture_blob, texture, palette)
            if embedded != original_pixels:
                shared = (texture_directory / f"{texture_name}.png"
                          if texture_directory is not None else None)
                if shared is None or not shared.exists() or embedded != read_png(
                        shared, texture["width"], texture["height"]):
                    raise ValueError(f"{name}: embedded {texture_name} image changed; save edits to its shared PNG")
        if material_id in wanted_textures and wanted_textures[material_id] != texture_name:
            raise ValueError(f"{name}: one Nitro material has conflicting GLB textures")
        wanted_textures[material_id] = texture_name
        wanted_palettes[material_id] = palette["name"]
    try:
        changed_tex = patch_pairings(data, material_dict, 0, wanted_textures, name)
        changed_pal = patch_pairings(data, material_dict, 2, wanted_palettes, name)
    except ValueError as error:
        if not any(reason in str(error) for reason in
                   ("no free Nitro binding entry", "previously unbound material", "no material binding list",
                    "binding removal needs a rebuilt table")):
            raise
        data, changed_tex, changed_pal = rebuild_pairing_tables(
            chunk, wanted_textures, wanted_palettes, name)
    if changed_tex:
        for material_id in changed_tex:
            texture_name = wanted_textures[material_id]
            entry = entries[material_id]
            original = u32(data, entry + 20)
            if texture_name is None:
                struct.pack_into("<I", data, entry + 20, original & ~(7 << 26))
            else:
                texture = by_name[texture_name][1]
                struct.pack_into("<HH", data, entry + 32, texture["width"], texture["height"])
                struct.pack_into("<I", data, entry + 20,
                                 (original & ~0x3ff00000) | (texture["params"] & 0x3ff00000))
    return bytes(data), bool(changed_tex or changed_pal)


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


def patch_uvs(chunk, doc, primitives, binary, name):
    data = bytearray(chunk)
    model_start = u32(data, 4)
    block = model_start + u32(data, model_start + 16)
    model = block + dictionary_offsets(data, block + 8)[0]
    shape_dict = model + u32(data, model + 12)
    shapes = [shape_dict + offset for offset in dictionary_offsets(data, shape_dict)]
    calls = draw_calls(data, model)
    if len(calls) != len(primitives):
        raise ValueError(f"{name}: Nitro and GLB draw call counts differ")
    _, index_sets = canonical_indices(data, shapes, [shape for shape, _ in calls])
    first_vertex = 0
    for (shape_index, material_index), primitive, original_indices in zip(calls, primitives, index_sets):
        shape = shapes[shape_index]
        start = shape + u32(data, shape + 8)
        end = start + u32(data, shape + 12)
        current = (0, 0)
        command = None
        vertices = []
        for op, cursor in gpu_commands(data, start, end):
            if op == 0x22:
                word = u32(data, cursor)
                current = (signed(word & 65535, 16), signed(word >> 16, 16))
                command = cursor
            elif op in (0x23, 0x24, 0x25, 0x26, 0x27, 0x28):
                vertices.append((current, command))
        if "TEXCOORD_0" not in primitive["attributes"]:
            if any(cursor is not None for _, cursor in vertices):
                raise ValueError(f"{name}: GLB UV accessor is missing")
            first_vertex += len(vertices)
            continue
        glb_uvs = accessor_values(doc, binary, primitive["attributes"]["TEXCOORD_0"])
        indices = [item[0] for item in accessor_values(doc, binary, primitive["indices"])]
        if len(indices) != len(original_indices):
            raise ValueError(f"{name}: GLB triangle count changed")
        desired = {}
        for original, current_index in zip(original_indices, indices):
            if current_index >= len(glb_uvs):
                raise ValueError(f"{name}: GLB UV index out of range")
            local = original - first_vertex
            value = glb_uvs[current_index]
            if local in desired and any(abs(a - b) > 0.00001 for a, b in zip(desired[local], value)):
                raise ValueError(f"{name}: one Nitro vertex has multiple edited UVs")
            desired[local] = value
        if all(cursor is None for _, cursor in vertices):
            if any(abs(value[0]) > 0.00001 or abs(value[1] - 1) > 0.00001
                   for value in desired.values()):
                raise ValueError(f"{name}: this shape has no Nitro texture commands")
            first_vertex += len(vertices)
            continue
        width, height, scale = material_uv_transform(data, model, material_index)
        writes = {}
        for local, value in desired.items():
            raw, cursor = vertices[local]
            baseline = ((raw[0] * scale[0] / (16 * width), raw[1] * scale[1] / (16 * height))
                        if cursor is not None else (0, 1))
            if all(abs(a - b) <= 0.00001 for a, b in zip(value, baseline)):
                target = raw
            else:
                if cursor is None or not all(scale):
                    raise ValueError(f"{name}: this UV has no editable Nitro texture command")
                target = (round(value[0] * width * 16 / scale[0]),
                          round(value[1] * height * 16 / scale[1]))
                if any(not -32768 <= coordinate <= 32767 for coordinate in target):
                    raise ValueError(f"{name}: edited UV exceeds Nitro coordinate range")
            if cursor is not None:
                if cursor in writes and writes[cursor] != target:
                    raise ValueError(f"{name}: vertices sharing a Nitro UV command need the same UV")
                writes[cursor] = target
            elif target != raw:
                raise ValueError(f"{name}: UV before the first Nitro texture command cannot change")
        for cursor, (u, v) in writes.items():
            if (u, v) != (signed(u32(data, cursor) & 65535, 16), signed(u32(data, cursor) >> 16, 16)):
                struct.pack_into("<I", data, cursor, (u & 65535) | ((v & 65535) << 16))
        first_vertex += len(vertices)
    return bytes(data)


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


def nitro_command(opcode, *parameters):
    return struct.pack("<I", opcode) + struct.pack("<" + "I" * len(parameters), *parameters)


def gltf_color(doc, accessor_index, values, vertex, name):
    accessor = doc["accessors"][accessor_index]
    components = values[vertex]
    if accessor["componentType"] == 5126:
        rgb = components[:3]
        alpha = components[3] if len(components) == 4 else 1
    else:
        maximum = {5121: 255, 5123: 65535}[accessor["componentType"]]
        rgb = [value / maximum for value in components[:3]]
        alpha = components[3] / maximum if len(components) == 4 else 1
    if abs(alpha - 1) > 0.00001:
        raise ValueError(f"{name}: Nitro vertex colors cannot carry per-vertex alpha")
    # Apicula exports Nitro RGB555 vertex colors as linear glTF colors.
    rgb = [value * 12.92 if value <= 0.0031308 else 1.055 * value ** (1 / 2.4) - 0.055
           for value in rgb]
    return pack_color5(rgb, name)


def topology_display_list(data, model, material_index, doc, primitive, binary, scale, direct, name):
    attributes = primitive["attributes"]
    indices = [value[0] for value in accessor_values(doc, binary, primitive["indices"])]
    if len(indices) % 3:
        raise ValueError(f"{name}: triangle index count is not divisible by three")
    if not indices:
        raise ValueError(f"{name}: a Nitro shape needs at least one triangle")
    positions = accessor_values(doc, binary, attributes["POSITION"])
    uvs = accessor_values(doc, binary, attributes["TEXCOORD_0"]) if "TEXCOORD_0" in attributes else None
    colors = accessor_values(doc, binary, attributes["COLOR_0"]) if "COLOR_0" in attributes else None
    normals = accessor_values(doc, binary, attributes["NORMAL"]) if "NORMAL" in attributes else None
    joints = accessor_values(doc, binary, attributes["JOINTS_0"]) if not direct else None
    weights = accessor_values(doc, binary, attributes["WEIGHTS_0"]) if not direct else None
    if not direct and (joints is None or weights is None):
        raise ValueError(f"{name}: transformed model needs single-joint skin attributes")
    width, height, uv_scale = material_uv_transform(data, model, material_index)
    if uvs is not None and not all(uv_scale):
        raise ValueError(f"{name}: material has a zero texture scale")
    commands = bytearray(nitro_command(0x40, 0))
    for vertex in indices:
        if vertex >= len(positions) or any(vertex >= len(values) for values in (uvs, colors, normals, joints, weights)
                                               if values is not None):
            raise ValueError(f"{name}: triangle index exceeds a vertex attribute")
        point = positions[vertex]
        if not direct:
            if blended(weights[vertex]):
                raise ValueError(f"{name}: blended skin weights are unsupported")
            point = inverse_joint_position(doc, joints[vertex][0], point)
        coords = [round(value * 4096 / scale) for value in point]
        if any(not -32768 <= value <= 32767 for value in coords):
            raise ValueError(f"{name}: edited vertex exceeds Nitro VTX_16 range")
        if colors is not None:
            commands.extend(nitro_command(0x20, gltf_color(doc, attributes["COLOR_0"], colors, vertex, name)))
        if normals is not None:
            normal = normals[vertex]
            packed = sum((max(-512, min(511, round(value * 512))) & 0x3ff) << (axis * 10)
                         for axis, value in enumerate(normal))
            commands.extend(nitro_command(0x21, packed))
        if uvs is not None:
            u, v = uvs[vertex]
            raw = (round(u * width * 16 / uv_scale[0]), round(v * height * 16 / uv_scale[1]))
            if any(not -32768 <= value <= 32767 for value in raw):
                raise ValueError(f"{name}: edited UV exceeds Nitro coordinate range")
            commands.extend(nitro_command(0x22, (raw[0] & 0xffff) | ((raw[1] & 0xffff) << 16)))
        commands.extend(nitro_command(0x23, (coords[0] & 0xffff) | ((coords[1] & 0xffff) << 16),
                                      coords[2] & 0xffff))
    commands.extend(nitro_command(0x41))
    return bytes(commands)


def rebuild_topology(chunk, doc, primitives, binary, mapping, name):
    data = bytearray(chunk)
    model_start = u32(data, 4)
    nitro_length = u32(data, model_start + 8)
    block = model_start + u32(data, model_start + 16)
    model = block + dictionary_offsets(data, block + 8)[0]
    shape_dict = model + u32(data, model + 12)
    shapes = [shape_dict + offset for offset in dictionary_offsets(data, shape_dict)]
    calls = draw_calls(data, model)
    if len(calls) != len(primitives) or sorted(shape for shape, _ in calls) != list(range(len(shapes))):
        raise ValueError(f"{name}: GLB must keep the original material draw calls")
    scale = u32(data, model + 28) / 4096
    appended = bytearray()
    triangle_count = 0
    for (shape_index, material_index), primitive in zip(calls, primitives):
        # The new display list only draws, so a shape that restores or scales matrices as it draws can't be rebuilt
        start = shapes[shape_index] + u32(data, shapes[shape_index] + 8)
        if any(op in (0x14, 0x1b) for op, _ in gpu_commands(data, start, start + u32(data, shapes[shape_index] + 12))):
            raise ValueError(f"{name}: shape {shape_index} sets matrices as it draws, so its triangles can't be "
                             "re-encoded yet; keep its topology")
        triangle_count += doc["accessors"][primitive["indices"]]["count"] // 3
        display_list = topology_display_list(data, model, material_index, doc, primitive, binary,
                                             scale, mapping["direct"], name)
        shape = shapes[shape_index]
        struct.pack_into("<II", data, shape + 8,
                         model_start + nitro_length + len(appended) - shape, len(display_list))
        appended.extend(display_list)
    delta = len(appended)
    if triangle_count > 65535 or triangle_count * 3 > 65535:
        raise ValueError(f"{name}: edited mesh exceeds Nitro model polygon or vertex count")
    struct.pack_into("<4H", data, model + 36, triangle_count * 3,
                     triangle_count, triangle_count, 0)
    struct.pack_into("<I", data, model + 0, u32(data, model) + delta)
    struct.pack_into("<I", data, block + 4, u32(data, block + 4) + delta)
    struct.pack_into("<I", data, model_start + 8, nitro_length + delta)
    for field in {b"WB": (8, 12, 16), b"GC": (8, 12, 16, 20),
                  b"NG": (8, 12), b"RD": (8, 12, 16)}[bytes(data[:2])]:
        struct.pack_into("<I", data, field, u32(data, field) + delta)
    insertion = model_start + nitro_length
    result = bytearray(data[:insertion] + appended + data[insertion:])
    nitro = model_bytes(result)
    shapes = [shape_dict - model_start + offset
              for offset in dictionary_offsets(nitro, shape_dict - model_start)]
    # The model box is used for visibility checks, so expand it with the edited mesh.
    local = decoded_positions(nitro, shapes, draw_order(nitro, model - model_start), scale)
    box_scale = u32(result, model + 56) / 4096
    while True:
        origins = [math.floor(min(point[axis] for point in local) * 4096 / box_scale)
                   for axis in range(3)]
        extents = [math.ceil(max(point[axis] for point in local) * 4096 / box_scale) - origins[axis]
                   for axis in range(3)]
        if all(-32768 <= value <= 32767 for value in (*origins, *extents)):
            break
        box_scale *= 2
        if box_scale * 4096 >= 1 << 32:
            raise ValueError(f"{name}: edited model exceeds Nitro bounding-box range")
    struct.pack_into("<6hII", result, model + 44, *origins, *extents,
                     round(box_scale * 4096), round(4096 / box_scale))
    return bytes(result)


def export(apicula, output, selected=None):
    archive = read_narc((ROOT / "extract/b2_us/files/a/0/0/8").read_bytes())
    textures = read_narc((ROOT / "extract/b2_us/files/a/0/1/4").read_bytes())
    output.mkdir(parents=True, exist_ok=True)
    texture_dirs = directories("a/0/1/4", output.parent / "map_texture_sets")
    for chunk_id, ((source, data), model_dir) in enumerate(zip(models(ROOT / "data/map_models", len(archive)),
                                                                 directories("a/0/0/8", output))):
        if selected is not None and chunk_id not in selected:
            continue
        texture_id = texture_set(data, str(source))
        nitro = model_bytes(archive[chunk_id])
        name = model_name(nitro)
        texture_names = ({item["name"]: item for item in resources(textures[texture_id])[0]}
                         if texture_id is not None else {})
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
                    texture_dir = texture_dirs[texture_id]
                    texture_dir.mkdir(parents=True, exist_ok=True)
                    canonical = canonical_texture_name(path.name, texture_names)
                    target = texture_dir / canonical
                    if target.exists():
                        texture = texture_names[Path(canonical).stem]
                        if read_png(target, texture["width"], texture["height"]) != read_png(
                                path, texture["width"], texture["height"]):
                            raise ValueError(f"{chunk_id}: {target} differs across models")
                    else:
                        shutil.copyfile(path, target)
                else:
                    target = "model.gltf" if path.suffix == ".gltf" else "model.bin" if path.suffix == ".bin" else path.name
                    shutil.copyfile(path, model_dir / target)
        document = json.loads((model_dir / "model.gltf").read_text())
        if texture_id is not None:
            for picture in document.get("images", []):
                if "uri" in picture:
                    canonical = canonical_texture_name(picture["uri"], texture_names)
                    picture["uri"] = image_uri(texture_dirs[texture_id], canonical)
        add_prop_nodes(document, archive[chunk_id])
        write_glb(model_dir / "model.glb", document, (model_dir / "model.bin").read_bytes())
        (model_dir / "model.gltf").unlink()
        (model_dir / "model.bin").unlink()
        doc, primitives, binary = mesh_primitives(model_dir, name)
        positions = accessor_values(doc, binary, primitives[0]["attributes"]["POSITION"])
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
        data = {key: item for key, item in data.items() if key != "transformed"}
        if not direct:
            data["transformed"] = True
        write(model_dir / "data.json", data)
        export_terrains(archive[chunk_id], model_dir)


def relink(directory):
    count = len(read_narc((ROOT / "extract/b2_us/files/a/0/0/8").read_bytes()))
    texture_dirs = directories("a/0/1/4")
    for model_dir, data in models(directory, count):
        texture_id = texture_set(data, str(model_dir))
        path = model_dir / "model.glb"
        doc, binary = read_glb(path)
        images = [picture for picture in doc.get("images", []) if "uri" in picture]
        if texture_id is None or not images:
            continue
        uris = [image_uri(texture_dirs[texture_id], Path(picture["uri"]).name) for picture in images]
        if [picture["uri"] for picture in images] != uris:
            for picture, uri in zip(images, uris):
                picture["uri"] = uri
            write_glb(path, doc, binary)


def pack(directory, game, output):
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
    source = ROOT / "extract" / version / "files/a/0/0/8"
    archive = read_narc(source.read_bytes())
    texture_archive = read_narc((ROOT / "extract" / version / "files/a/0/1/4").read_bytes())
    texture_dirs = directories("a/0/1/4")
    for chunk_id, (model_dir, data) in enumerate(models(directory, len(archive))):
        original = archive[chunk_id]
        name = model_name(model_bytes(original))
        mapping = {"direct": not data.get("transformed", False)}
        doc, primitives, binary = mesh_primitives(model_dir, name)
        texture_id = texture_set(data, str(model_dir))
        bound, _ = patch_material_bindings(
            original, doc, primitives, texture_archive[texture_id] if texture_id is not None else None,
            name, binary, texture_dirs[texture_id] if texture_id is not None else None)
        if data["texture_set"] is None and bound != original:
            raise ValueError(f"{name}: texture bindings need an unambiguous map texture set")
        try:
            positions, direct, _ = positions_from_gltf(model_dir, name, model_bytes(original), mapping)
            edited = patch_model(bound, positions, name) if direct else bound
            edited = patch_uvs(edited, doc, primitives, binary, name)
        except ValueError as error:
            if not any(reason in str(error) for reason in FALLBACK_ERRORS):
                raise
            edited = rebuild_topology(bound, doc, primitives, binary, mapping, name)
        archive[chunk_id] = edited
        archive[chunk_id] = patch_materials(archive[chunk_id], doc, primitives, name)
        archive[chunk_id] = patch_terrains(archive[chunk_id], model_dir)
        archive[chunk_id] = patch_props(archive[chunk_id], doc)
    dest = Path(output)
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.unlink(missing_ok=True)
    dest.write_bytes(write_narc(archive))


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == "extract":
        export(sys.argv[2], Path(sys.argv[3]),
               set(map(int, sys.argv[4:])) if len(sys.argv) > 4 else None)
        return
    if len(sys.argv) == 3 and sys.argv[1] == "relink":
        relink(Path(sys.argv[2]))
        return
    if len(sys.argv) != 5 or sys.argv[1] != "pack":
        raise SystemExit(__doc__)
    _, _, directory, game, output = sys.argv
    try:
        pack(Path(directory), game, output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
