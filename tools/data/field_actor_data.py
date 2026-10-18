#!/usr/bin/env python3
"""The field's actor models in res/field/actors/: dump writes them from the ROM once, and pack builds the model table
(a/0/4/7) and the models' graphics (a/0/4/8) from them, which the build does.

    field_actor_data.py dump extract/b2_us/files res/field/actors
    field_actor_data.py pack res/field/actors TABLE GRAPHICS

Each object code of generated/obj_codes.txt, which zone events and scripts give an actor, is a directory named
after it, hero/ for OBJ_CODE_HERO, pokemon/bulbasaur/ for the walking Pokémon's OBJ_CODE_POKEMON_BULBASAUR and
models/0/ for the 3D models' OBJ_CODE_MODEL_BIGROCK, with its record of the table in data.json, described by
field_actor.schema.json, and its graphics:

- a sprite (type "sprite") has sprite.png, its frames side by side, each a square 32 or 64 pixels wide, and
  sprite.pal, its 16 colors in JASC format, 0 transparent. The PNG's own palette only puts back the indices that an
  image editor reordered on saving; the .pal file sets the colors.
- a 3D model (type "model") has model.nsbmd, a Nitro model, unless data.json's "model" names the object code whose
  model it shows.
- an actor of type "none" has no graphics.

The table has the codes in the list's order. The graphics archive has the models, then the sprites, each in the order
of the codes that show them, and the packer writes each code's place in it into the table, so a new code is a
constant and a directory. The sprites' texture sets are written from the PNGs, as the game's are, byte for byte.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.nitro_2d import (color5, encode_characters, palette_indices, read_indexed_png,  # noqa: E402
                                 read_palette, write_indexed_png, write_palette)
from tools.data.nitro_btx import texture_set  # noqa: E402

TABLE = "a/0/4/7"
GRAPHICS = "a/0/4/8"
# A record of the table: FieldActorConfig in include/field/field_actor.h
RECORD = struct.Struct("<HBBBBBBBBBBBbbbHHHHHH")
TYPES = {0: "none", 1: "sprite", 2: "model"}
# A 3D model's resources other than its model, and a sprite's or no model's
NO_RESOURCE = 0xFFFF
COLORS = 16
FRAME_SIZES = (32, 64)
# The sprites' textures: GX_TEXFMT_PLTT16, color 0 transparent
TEXTURE_FORMAT = 3
GENDERS = {0: "GENDER_MALE", 1: "GENDER_FEMALE", 2: None}


def codes() -> list[tuple[str, int]]:
    """Returns the object codes and their values, in value order: the table's order."""
    found: dict[int, str] = {}
    for constant, number in load_list("obj_codes").items():
        found.setdefault(number, constant)
    return [(found[number], number) for number in sorted(found)]


GROUPS = (("POKEMON_", "pokemon"), ("MODEL_", "models"))


def directory(root: Path, code: str) -> Path:
    """Returns an object code's directory: pokemon/ for the walking Pokémon's, models/ for the 3D models'."""
    name = code.removeprefix("OBJ_CODE_")
    if name.lower() in [group for _, group in GROUPS]:
        raise DataError(f"{code} would be named as the directory of a group of object codes")
    for prefix, group in GROUPS:
        if name.startswith(prefix):
            return root / group / name.removeprefix(prefix).lower()
    return root / name.lower()


def sprite_frames(blob: bytes) -> tuple[int, int, bytes, list]:
    """Returns a sprite's texture set as one strip of its frames: its width, height, pixels and colors."""
    tex0 = 0x14
    texel_start = tex0 + struct.unpack_from("<I", blob, tex0 + 20)[0]
    palette_start = tex0 + struct.unpack_from("<I", blob, tex0 + 56)[0]
    dictionary = tex0 + 0x3C
    count = blob[dictionary + 1]
    records = dictionary + 8 + 4 * (count + 1) + 4
    frames = []
    for index in range(count):
        params, extra = struct.unpack_from("<II", blob, records + 8 * index)
        size = extra & 0x7FF
        if size != extra >> 11 & 0x7FF or (frames and size * size != len(frames[0])):
            raise SystemExit("a sprite's frames aren't squares of one size, which sprite.png can't hold")
        start = texel_start + ((params & 0xFFFF) << 3)
        data = blob[start:start + size * size // 2]
        frames.append(bytes(value for byte in data for value in (byte & 15, byte >> 4)))
    pixels = bytes(frames[index][y * size + x] for y in range(size) for index in range(count) for x in range(size))
    colors = [tuple(((value >> shift) & 31) * 255 // 31 for shift in (0, 5, 10))
              for (value,) in struct.iter_unpack("<H", blob[palette_start:palette_start + 2 * COLORS])]
    return size * count, size, pixels, colors


def sprite_texture_set(width: int, height: int, pixels: bytes, colors: list) -> bytes:
    """Returns the texture set of a strip of frames, named by their size and count as the game's are."""
    count = width // height
    frames = [bytes(pixels[y * width + index * height + x] for y in range(height) for x in range(height))
              for index in range(count)]
    base = f"t{height // 8}x{height // 8}x{count}"
    textures = [(f"{base}.{index}", height, height, TEXTURE_FORMAT, True,
                 encode_characters(height, height, frame, linear=True)) for index, frame in enumerate(frames)]
    palette = b"".join(struct.pack("<H", r | g << 5 | b << 10) for r, g, b in map(color5, colors))
    return texture_set(textures, [(base, palette)])


def record_json(record: tuple, model_owner: str | None) -> dict:
    (_, entity, scene, shadow, footprint, reflections, billboard, atlas, controller, gender, width, height,
     x, y, z, resource, *others) = record
    data = {
        "$schema": "field_actor.schema.json",
        "type": TYPES[entity],
        "scene_node_type": scene,
        "shadow": bool(shadow),
        "footprint_type": footprint,
        "reflections": bool(reflections),
        "billboard_size": billboard,
        "sprite_atlas_size": atlas,
        "sprite_controller_type": controller,
        "gender": GENDERS[gender],
        "collision": {"width": width, "height": height},
        "offset": {"x": x, "y": y, "z": z},
    }
    if model_owner:
        data["model"] = model_owner
    return data


def dump(files: Path, root: Path):
    table = read_narc((files / TABLE).read_bytes())[0]
    graphics = read_narc((files / GRAPHICS).read_bytes())
    count = struct.unpack_from("<I", table, 0)[0]
    records = [RECORD.unpack_from(table, 4 + RECORD.size * index) for index in range(count)]
    code_list = codes()
    if [number for _, number in code_list] != [record[0] for record in records]:
        raise SystemExit("obj_codes.txt doesn't list the table's object codes in its order")
    owners: dict[int, str] = {}
    for (code, _), record in zip(code_list, records):
        entity, resource, others = record[1], record[15], record[16:20]
        expected = (NO_RESOURCE,) * 4 if entity == 2 else (0,) * 4
        if tuple(others) != expected or record[20] or (entity == 0 and resource):
            raise SystemExit(f"{code}: its record has resources the packer can't write again")
        path = directory(root, code)
        path.mkdir(parents=True, exist_ok=True)
        owner = None
        if entity == 2:
            owner = owners.get(resource)
            if owner is None:
                owners[resource] = code
                (path / "model.nsbmd").write_bytes(graphics[resource])
        elif entity == 1:
            width, height, pixels, colors = sprite_frames(graphics[resource])
            (path / "sprite.png").write_bytes(write_indexed_png(width, height, pixels, colors))
            (path / "sprite.pal").write_text(write_palette(colors))
        write(path / "data.json", record_json(record, owner))
    print(f"wrote {len(records)} object codes to {root}")


def check(condition: bool, path: Path, message: str):
    if not condition:
        raise DataError(f"{label(path)}: {message}")


def sprite(path: Path) -> bytes:
    palette = path / "sprite.pal"
    image = path / "sprite.png"
    check(palette.is_file() and image.is_file(), path, "a sprite needs sprite.png and sprite.pal")
    colors = read_palette(palette)
    check(len(colors) == COLORS, palette, f"must have {COLORS} colors, not {len(colors)}")
    width, height, pixels, png_colors = read_indexed_png(image)
    check(height in FRAME_SIZES and width % height == 0, image,
          f"must be frames side by side, each square and {' or '.join(map(str, FRAME_SIZES))} pixels wide, "
          f"not {width} by {height}")
    check(width // height <= 255, image, "has more frames than a texture set can name")
    return sprite_texture_set(width, height, palette_indices(image, pixels, png_colors, colors), colors)


def pack(root: Path, table_output: Path, graphics_output: Path):
    schema = load_schema(root / "field_actor.schema.json")
    entries = []
    for code, number in codes():
        path = directory(root, code)
        check(number <= 0xFFFF, path, f"{code} is {number:#x}, more than an object code can be")
        check(path.is_dir(), path, f"{code} has no directory")
        data = load(path / "data.json", schema)
        own_model = (path / "model.nsbmd").is_file()
        check(data["type"] == "model" or "model" not in data and not own_model, path,
              "only a 3D model (type \"model\") has a model.nsbmd or a \"model\"")
        check(not (own_model and "model" in data), path, "a model has its own model.nsbmd or a \"model\", not both")
        entries.append((code, number, path, data))
    models: list[bytes] = []
    model_index: dict[str, int] = {}
    for code, _, path, data in entries:
        if data["type"] == "model" and "model" not in data:
            check((path / "model.nsbmd").is_file(), path, "a model without \"model\" needs model.nsbmd")
            model_index[code] = len(models)
            models.append((path / "model.nsbmd").read_bytes())
    sprites: list[bytes] = []
    records = []
    for code, number, path, data in entries:
        if data["type"] == "sprite":
            resource = len(models) + len(sprites)
            sprites.append(sprite(path))
        elif data["type"] == "model":
            owner = data.get("model", code)
            check(owner in model_index, path, f"{owner} is no 3D model with a model.nsbmd of its own, which a \"model\" "
                  "must name")
            resource = model_index[owner]
        else:
            resource = 0
        others = (NO_RESOURCE,) * 4 if data["type"] == "model" else (0,) * 4
        gender = next(key for key, value in GENDERS.items() if value == data["gender"])
        records.append(RECORD.pack(
            number, {value: key for key, value in TYPES.items()}[data["type"]], data["scene_node_type"],
            data["shadow"], data["footprint_type"], data["reflections"], data["billboard_size"],
            data["sprite_atlas_size"], data["sprite_controller_type"], gender, data["collision"]["width"],
            data["collision"]["height"], data["offset"]["x"], data["offset"]["y"], data["offset"]["z"], resource,
            *others, 0))
    table_output.parent.mkdir(parents=True, exist_ok=True)
    table_output.write_bytes(write_narc([struct.pack("<I", len(records)) + b"".join(records)]))
    graphics_output.parent.mkdir(parents=True, exist_ok=True)
    graphics_output.write_bytes(write_narc(models + sprites))


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "dump":
        dump(Path(sys.argv[2]), Path(sys.argv[3]))
    elif len(sys.argv) == 5 and sys.argv[1] == "pack":
        try:
            pack(Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4]))
        except DataError as error:
            sys.exit(f"error: {error}")
    else:
        raise SystemExit(__doc__)
