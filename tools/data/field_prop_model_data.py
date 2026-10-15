#!/usr/bin/env python3
"""Export field prop models as GLB and pack edited models into their area bundles.

Usage: field_prop_model_data.py extract APICULA OUTPUT_DIR [BUNDLE [MODEL]]
       field_prop_model_data.py relink SOURCE_DIR
       field_prop_model_data.py pack SOURCE_DIR black2|white2 OUTPUT

The bundle's resource information and animations stay in their original Nitro
encoding. Each model's editable geometry and materials live in one GLB, next to
its data.json (field_prop_model.schema.json). Bundles are directories named
after their constants in data/constants/field_props_{exterior,interior}.txt.
relink points every GLB's images at its bundle's texture directory again, after
a bundle is renamed.
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
from map_model_data import (FALLBACK_ERRORS, accessor_values, canonical_texture_name, decoded_positions,
                            dictionary_offsets, draw_order, mesh_primitives, model_bytes, read_glb,
                            patch_material_bindings, patch_materials, patch_model,
                            patch_uvs, positions_from_gltf, rebuild_topology,
                            u32, write_glb)
from map_texture_data import read_png, resources
from graphics_paths import directories, numbered


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.data.datajson import DataError, load, load_schema, write  # noqa: E402

ARCHIVES = {"exterior": ("a/2/2/5", "a/1/7/4"),
            "interior": ("a/2/2/6", "a/1/7/5")}


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


def needs_baseline(model_dir, name, nitro):
    """Whether a transformed model's GLB, unedited, packs back into something else than its Nitro model when the packer
    works out the exported positions from the GLB's joints, so that it needs them as exported."""
    try:
        positions, direct, _ = positions_from_gltf(model_dir, name, nitro, {"direct": False})
        return not direct or patch_model(wrapped(nitro), positions, name) != wrapped(nitro)
    except ValueError:
        return True


def export_one(apicula, model_dir, texture_dir, entry, texture_blob):
    index, res_id, name, nitro = entry
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
            raise ValueError(f"{model_dir}: expected one exported glTF")
        document = json.loads(gltfs[0].read_text())
        for picture in document.get("images", []):
            if "uri" not in picture:
                continue
            canonical = canonical_texture_name(picture["uri"], texture_names)
            texture = texture_names[Path(canonical).stem]
            preview = converted / picture["uri"]
            shared = texture_dir / canonical
            if read_png(preview, texture["width"], texture["height"]) != read_png(
                    shared, texture["width"], texture["height"]):
                raise ValueError(f"{model_dir}: {canonical} differs from shared PNG")
            picture["uri"] = image_uri(texture_dir, canonical)
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
    data = {"$schema": "../../../field_prop_model.schema.json", "res_id": res_id}
    if not direct:
        data["transformed"] = True
        if needs_baseline(model_dir, name, nitro):
            data["positions"] = positions
    data["glb_sha256"] = hashlib.sha256((model_dir / "model.glb").read_bytes()).hexdigest()
    write(model_dir / "data.json", data)


def image_uri(texture_dir, filename):
    """Returns a GLB's path to one of its bundle's textures, from data/field_prop_models/<kind>/<bundle>/<model>/."""
    return f"../../../../field_prop_textures/{texture_dir.parent.name}/{texture_dir.name}/{filename}"


def model_dirs(folder, entries):
    """Returns the directories of a bundle's models, named after them, numbered where a name repeats."""
    return [folder / name for name in numbered(name for _, _, name, _ in entries)]


def export(apicula, directory, selected_bundle=None, selected_model=None):
    kind = directory.name
    if kind not in ARCHIVES:
        raise ValueError("output directory must end in exterior or interior")
    archive_name, texture_name = ARCHIVES[kind]
    archive = read_narc((ROOT / "extract/b2_us/files" / archive_name).read_bytes())
    textures = read_narc((ROOT / "extract/b2_us/files" / texture_name).read_bytes())
    if len(archive) != len(textures):
        raise ValueError("field prop bundle and texture set counts differ")
    folders = directories(archive_name, directory)
    texture_dirs = directories(texture_name)
    if len(folders) != len(archive):
        raise ValueError(f"{archive_name} has {len(archive)} bundles and its list {len(folders)}")
    for bundle_id, (bundle, folder) in enumerate(zip(archive, folders)):
        if selected_bundle is not None and bundle_id != selected_bundle:
            continue
        entries = bundle_models(bundle)
        selected = [(entry, model_dir) for entry, model_dir in zip(entries, model_dirs(folder, entries))
                    if selected_model is None or entry[0] == selected_model]
        with ThreadPoolExecutor(max_workers=4) as executor:
            list(executor.map(lambda item: export_one(apicula, item[1], texture_dirs[bundle_id], item[0],
                                                      textures[bundle_id]), selected))
        print(f"{kind} {bundle_id:03d}: {len(selected)} GLBs", flush=True)


def relink(directory):
    kind = directory.name
    if kind not in ARCHIVES:
        raise ValueError("source directory must end in exterior or interior")
    archive_name, texture_name = ARCHIVES[kind]
    archive = read_narc((ROOT / "extract/b2_us/files" / archive_name).read_bytes())
    for bundle, folder, texture_dir in zip(archive, directories(archive_name, directory),
                                           directories(texture_name)):
        entries = bundle_models(bundle)
        for model_dir in model_dirs(folder, entries):
            path = model_dir / "model.glb"
            glb = path.read_bytes()
            doc, binary = read_glb(path)
            images = [picture for picture in doc.get("images", []) if "uri" in picture]
            uris = [image_uri(texture_dir, Path(picture["uri"]).name) for picture in images]
            if [picture["uri"] for picture in images] == uris:
                continue
            for picture, uri in zip(images, uris):
                picture["uri"] = uri
            write_glb(path, doc, binary)
            # A GLB as extracted stays one, so that it packs its original model
            data_path = model_dir / "data.json"
            data = json.loads(data_path.read_text())
            if data["glb_sha256"] == hashlib.sha256(glb).hexdigest():
                data["glb_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
                write(data_path, data)


def edit_model(model_dir, name, original, texture_blob, data, texture_directory=None):
    glb = (model_dir / "model.glb").read_bytes()
    if hashlib.sha256(glb).hexdigest() == data["glb_sha256"]:
        return original
    mapping = {"direct": not data.get("transformed", False)}
    if "positions" in data:
        mapping["positions"] = data["positions"]
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
    folders = directories(archive_name, directory)
    texture_dirs = directories(texture_name)
    if len(folders) != len(archive):
        raise ValueError(f"{archive_name} has {len(archive)} bundles and its list {len(folders)}")
    schema = load_schema(directory.parent / "field_prop_model.schema.json")
    result = []
    for bundle_id, (bundle, folder) in enumerate(zip(archive, folders)):
        entries = bundle_models(bundle)
        edited = []
        for (index, res_id, name, model), model_dir in zip(entries, model_dirs(folder, entries)):
            data = load(model_dir / "data.json", schema)
            if data["res_id"] != res_id:
                raise DataError(f"{model_dir}: the bundle gives this model the resource ID {res_id}")
            edited.append(edit_model(model_dir, name, model, textures[bundle_id], data, texture_dirs[bundle_id]))
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
    elif len(sys.argv) == 3 and sys.argv[1] == "relink":
        relink(Path(sys.argv[2]))
    elif len(sys.argv) == 5 and sys.argv[1] == "pack":
        try:
            pack(Path(sys.argv[2]), sys.argv[3], sys.argv[4])
        except DataError as error:
            sys.exit(f"error: {error}")
    else:
        raise SystemExit(__doc__)


if __name__ == "__main__":
    main()
