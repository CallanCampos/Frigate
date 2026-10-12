#!/usr/bin/env python3
"""Export field prop models as GLB and pack edited models into their area bundles.

Usage: field_prop_model_data.py extract APICULA OUTPUT_DIR [BUNDLE [MODEL]]
       field_prop_model_data.py pack SOURCE_DIR black2|white2 OUTPUT

The bundle's resource information and animations stay in their original Nitro
encoding. Each model's editable geometry and materials live in one GLB.
"""

import hashlib
import json
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from narc import read_narc, write_narc
from map_model_data import (accessor_values, canonical_texture_name, decoded_positions,
                            dictionary_offsets, draw_order, mesh_primitives, model_bytes,
                            patch_material_bindings, patch_materials, patch_model,
                            patch_uvs, positions_from_gltf, rebuild_topology,
                            u32, write_glb)
from map_texture_data import read_png, resources
from graphics_paths import asset_dir
from graphics_names import prop_bundle_name


ROOT = Path(__file__).resolve().parents[2]
ARCHIVES = {"exterior": ("a/2/2/5", "a/1/7/4"),
            "interior": ("a/2/2/6", "a/1/7/5")}
FALLBACK_ERRORS = ("triangle count changed", "triangle topology or shared vertex changed",
                   "one Nitro vertex has multiple edited UVs", "vertices sharing a Nitro UV command",
                   "this shape has no Nitro texture commands", "this UV has no editable Nitro texture command",
                   "UV before the first Nitro texture command", "changes an axis omitted by Nitro opcode",
                   "vertex coordinate is not representable", "vertex coordinate exceeds Nitro command range")


def bundle_models(bundle):
    if bundle[:2] != b"AB" or bundle[2] & 1:
        raise ValueError("invalid field prop resource bundle")
    count = bundle[2] >> 1
    if count > 127:
        raise ValueError("field prop bundle has too many models")
    offsets = [u32(bundle, 4 + 4 * i) for i in range(2 * count)]
    if offsets != sorted(offsets) or (offsets and offsets[0] < 4 + 8 * count):
        raise ValueError("invalid field prop bundle offsets")
    result = []
    for index in range(count):
        start = offsets[count + index]
        end = offsets[count + index + 1] if index + 1 < count else len(bundle)
        model = bundle[start:end]
        if model[:4] != b"BMD0" or u32(model, 8) != len(model):
            raise ValueError(f"field prop model {index} has an invalid BMD0 range")
        block = u32(model, 16)
        if len(dictionary_offsets(model, block + 8)) != 1:
            raise ValueError(f"field prop model {index} has multiple models")
        name = model[block + 32:block + 48].split(b"\0")[0].decode("ascii")
        res_id = struct.unpack_from("<H", bundle, offsets[index])[0]
        result.append((index, res_id, name, model))
    return result


def wrapped(model):
    end = 16 + len(model)
    return struct.pack("<2sHIII", b"NG", 0, 16, end, end) + model


def export_one(apicula, directory, kind, bundle_id, entry, texture_blob):
    index, res_id, name, nitro = entry
    bundle_dir = asset_dir(directory, bundle_id, 3)
    model_dir = asset_dir(bundle_dir, index, 3, name)
    model_dir.mkdir(parents=True, exist_ok=True)
    texture_names = {item["name"]: item for item in resources(texture_blob)[0]}
    with tempfile.TemporaryDirectory() as temporary:
        temp = Path(temporary)
        (temp / "model.nsbmd").write_bytes(nitro)
        (temp / "textures.nsbtx").write_bytes(texture_blob)
        converted = temp / "converted"
        subprocess.run([apicula, "convert", str(temp / "model.nsbmd"),
                        str(temp / "textures.nsbtx"), "-o", str(converted), "-f", "gltf"],
                       check=True, capture_output=True)
        gltfs = list(converted.glob("*.gltf"))
        if len(gltfs) != 1:
            raise ValueError(f"{kind} {bundle_id}/{index}: expected one exported glTF")
        document = json.loads(gltfs[0].read_text())
        for picture in document.get("images", []):
            if "uri" not in picture:
                continue
            canonical = canonical_texture_name(picture["uri"], texture_names)
            texture = texture_names[Path(canonical).stem]
            preview = converted / picture["uri"]
            shared = asset_dir(ROOT / "data/field_prop_textures" / kind, bundle_id, 3) / canonical
            if read_png(preview, texture["width"], texture["height"]) != read_png(
                    shared, texture["width"], texture["height"]):
                raise ValueError(f"{kind} {bundle_id}/{index}: {canonical} differs from shared PNG")
            picture["uri"] = f"../../../../field_prop_textures/{kind}/{shared.parent.name}/{canonical}"
        binary = gltfs[0].with_suffix(".bin").read_bytes()
    write_glb(model_dir / "model.glb", document, binary)
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
            direct = patch_model(wrapped(nitro), positions, name) == wrapped(nitro)
        except ValueError:
            direct = False
    mapping = {"direct": direct,
               "glb_sha256": hashlib.sha256((model_dir / "model.glb").read_bytes()).hexdigest()}
    if not direct:
        mapping["positions"] = positions
    return mapping


def export(apicula, directory, selected_bundle=None, selected_model=None):
    kind = directory.name
    if kind not in ARCHIVES:
        raise ValueError("output directory must end in exterior or interior")
    archive_name, texture_name = ARCHIVES[kind]
    archive = read_narc((ROOT / "extract/b2_us/files" / archive_name).read_bytes())
    textures = read_narc((ROOT / "extract/b2_us/files" / texture_name).read_bytes())
    if len(archive) != len(textures):
        raise ValueError("field prop bundle and texture set counts differ")
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "manifest.json").write_text(json.dumps(
        {"archive": archive_name, "bundles": len(archive)}, indent=2) + "\n")
    for bundle_id, bundle in enumerate(archive):
        if selected_bundle is not None and bundle_id != selected_bundle:
            continue
        entries = bundle_models(bundle)
        folder = asset_dir(directory, bundle_id, 3,
                           prop_bundle_name(kind, bundle_id, entries[0][2] if entries else "empty"))
        folder.mkdir(parents=True, exist_ok=True)
        manifest_path = folder / "manifest.json"
        metadata = (json.loads(manifest_path.read_text()) if selected_model is not None
                    and manifest_path.exists() else
                    {"bundle_sha256": hashlib.sha256(bundle).hexdigest(),
                     "models": [{"index": index, "res_id": res_id, "name": name}
                                for index, res_id, name, _ in entries]})
        selected = [entry for entry in entries
                    if selected_model is None or entry[0] == selected_model]
        with ThreadPoolExecutor(max_workers=4) as executor:
            mappings = list(executor.map(lambda entry: export_one(apicula, directory, kind, bundle_id,
                                                                  entry, textures[bundle_id]), selected))
        for entry, mapping in zip(selected, mappings):
            item = metadata["models"][entry[0]]
            item["glb_sha256"] = mapping.pop("glb_sha256")
            if mapping["direct"]:
                item.pop("conversion", None)
            else:
                item["conversion"] = mapping
        (folder / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n")
        print(f"{kind} {bundle_id:03d}: {len(selected)} GLBs", flush=True)


def edit_model(model_dir, name, original, texture_blob, metadata, texture_directory=None):
    glb = (model_dir / "model.glb").read_bytes()
    if hashlib.sha256(glb).hexdigest() == metadata["glb_sha256"]:
        return original
    mapping = metadata.get("conversion", {"direct": True})
    doc, primitives, binary = mesh_primitives(model_dir, name)
    wrapped_original = wrapped(original)
    bound, _ = patch_material_bindings(wrapped_original, doc, primitives, texture_blob, name,
                                       binary, texture_directory)
    try:
        positions, direct, _ = positions_from_gltf(model_dir, name, original, mapping)
        edited = patch_model(bound, positions, name) if direct else bound
        edited = patch_uvs(edited, doc, primitives, binary, name)
    except ValueError as error:
        if not any(reason in str(error) for reason in FALLBACK_ERRORS):
            raise
        edited = rebuild_topology(bound, doc, primitives, binary, mapping, name)
    edited = patch_materials(edited, doc, primitives, name)
    return model_bytes(edited)


def pack(directory, game, output):
    kind = directory.name
    if kind not in ARCHIVES:
        raise ValueError("source directory must end in exterior or interior")
    archive_name, texture_name = ARCHIVES[kind]
    if not Path(output).as_posix().endswith("/" + archive_name):
        raise ValueError(f"output {output} does not name {archive_name}")
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
    archive = read_narc((ROOT / "extract" / version / "files" / archive_name).read_bytes())
    textures = read_narc((ROOT / "extract" / version / "files" / texture_name).read_bytes())
    if json.loads((directory / "manifest.json").read_text()) != {
            "archive": archive_name, "bundles": len(archive)}:
        raise ValueError("field prop model manifest differs from original archive")
    result = []
    for bundle_id, bundle in enumerate(archive):
        entries = bundle_models(bundle)
        folder = asset_dir(directory, bundle_id, 3)
        metadata = json.loads((folder / "manifest.json").read_text())
        if metadata["bundle_sha256"] != hashlib.sha256(bundle).hexdigest() or [
                {key: value for key, value in item.items()
                 if key not in ("conversion", "glb_sha256")}
                for item in metadata["models"]] != [
                    {"index": i, "res_id": r, "name": n} for i, r, n, _ in entries]:
            raise ValueError(f"{kind} bundle {bundle_id}: model manifest changed")
        texture_directory = asset_dir(ROOT / "data/field_prop_textures" / kind, bundle_id, 3)
        edited = [edit_model(asset_dir(folder, index, 3), name, model, textures[bundle_id],
                             metadata["models"][index], texture_directory)
                  for index, _, name, model in entries]
        if all(model == original[3] for model, original in zip(edited, entries)):
            result.append(bundle)
            continue
        count = len(entries)
        first = u32(bundle, 4 + 4 * count)
        rebuilt = bytearray(bundle[:first])
        for index, model in enumerate(edited):
            struct.pack_into("<I", rebuilt, 4 + 4 * (count + index), len(rebuilt))
            rebuilt.extend(model)
        result.append(bytes(rebuilt))
    dest = Path(output)
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.unlink(missing_ok=True)
    dest.write_bytes(write_narc(result))


def main():
    if len(sys.argv) in (4, 5, 6) and sys.argv[1] == "extract":
        export(sys.argv[2], Path(sys.argv[3]),
               int(sys.argv[4]) if len(sys.argv) >= 5 else None,
               int(sys.argv[5]) if len(sys.argv) == 6 else None)
    elif len(sys.argv) == 5 and sys.argv[1] == "pack":
        pack(Path(sys.argv[2]), sys.argv[3], sys.argv[4])
    else:
        raise SystemExit(__doc__)


if __name__ == "__main__":
    main()
