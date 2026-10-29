#!/usr/bin/env python3
"""Dump and pack the archives of Nitro resources: the screens, windows, icons and sprites of the game's menus and apps,
the 3D models of its demos, gimmicks and effects, and its fonts.

    graphics_archive.py dump ARCID_TITLE            # writes res/graphics/title/ from both versions' archives
    graphics_archive.py dump ARCID_X res/demo       # or under another of the archive roots (ARCHIVE_ROOTS)
    graphics_archive.py dump ARCID_X res/demo --direct-color 256x256    # members that are 256x256 bitmaps as PNGs
    graphics_archive.py dump ARCID_X res/demo --apicula PATH    # Apicula, for models, where it isn't on PATH
    graphics_archive.py pack res/graphics/title black2 OUT

Each archive is a directory named after its ARCID_* constant in include/constants/arc.h, whose number gives its file
(ARCID_TITLE, 26, is a/0/2/6), in the root for what it holds (ARCHIVE_ROOTS: res/graphics/ for the menus and apps,
res/demo/, res/field/gimmicks/ and effects/, res/particles/ and res/fonts/). It holds the archive's members in order,
each a file named after its number in the archive, or after its constant where generated/narc_<archive>.txt names it, as
narc_intro.txt does:

- a character resource (NCGR) is an indexed PNG, `03.png`, 4bpp or 8bpp as the resource is. Its pixels are the palette
  indices the game draws, so edit it in indexed mode and keep the palette's order: the packer reads the indices, not
  the colors. The PNG's colors are only a preview, from the archive's nearest palette, because which palette a
  screen or sprite uses is the code's to say. A sized resource is as wide as the game's; OBJ characters, which have
  no size, are laid out in rows of their tiles.
- a palette resource (NCLR) is a JASC palette, `02.pal`, which sets the colors.
- a 3D model (BMD0) is a directory named as the member is, `05/`, as a field prop's model is (field_prop_model_data.py):
  model.glb, which dump has Apicula convert it to, its data.json, whose glb_sha256 is the GLB's hash as extracted
  (MODEL_SCHEMA), and a PNG of each texture that the model embeds (a TEX0 block), named after it, which the GLB's
  images name. A GLB that still has its hash packs the game's model; an edited one goes back into it as the props' do,
  and edited PNGs as a field texture set's do (map_texture_data.py). Each of the GLB's draw calls has a material of its
  own, a copy where the game's share one, since Blender joins the draw calls of one material on export; edit the copies
  alike. A model that those converters don't take stays its .nsbmd file, as dump says: one with textures of a format
  that map_texture_data.py doesn't decode, one without a mesh, or one whose materials pair a texture with another
  palette than map_texture_data.py gives it. A new model is added as its .nsbmd file.
- a texture set (BTX0) is a directory of its textures' PNGs, `07/`, packed as a field texture set's are.
- a font (NFTR) is a directory named as the member is, `00/`, which nitro_font.py dumps and builds: glyphs.png, the
  glyphs in the font's order, a cell each, whose pixels are the glyphs' values (0 clear, then the letter, its shadow
  and the background), and font.json (res/fonts/font.schema.json), the font's metrics, its character maps' ranges and
  methods, and each glyph's character, left offset, width and advance. The packer computes the rest; a new glyph is an
  entry at the end of the glyphs and the next cell, which the map whose range holds its character takes. A font that
  nitro_font.py doesn't read, or wouldn't build again byte for byte, stays its .nftr file, as dump says.
- anything else is the member itself, decompressed, named by its kind: `.nscr` (screens), `.ncer` (cells), `.nanr`
  (cell animations), `.nmcr`/`.nmar` (multi-cells), `.nsbca`/`.nsbta`/`.nsbma`/`.nsbtp`/`.nsbva` (3D animations),
  `.nftr` (fonts), or `.bin`.

archive.json, where there is one, lists the members the game stores LZ-compressed, `{"compressed": {"03.nscr": "lz10"}}`;
the packer compresses them again, an edited one as a valid stream of literals that is an eighth larger than its data.
A member that differs between Black 2 and White 2 is in black2/ and white2/ instead, as the title screen's are. A
member that is unchanged packs as the game's own bytes. An edited one keeps the game's header with the new data and
sizes: an image may change size, which a sized image's character position section follows, and a palette may change
its number of colors unless it has a PCMP section. Add a member by adding the next number's file: a PNG becomes a
sized character resource, 8bpp where its palette has more than 16 colors, a .pal a palette, and the rest as they are.
dump writes an archive's directory again from scratch, so it replaces any edits. pack needs only Python.
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

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
# The field models' converters, which import their neighbors as narc and map_texture_data, also when configure.py
# imports this as tools.data.graphics_archive
if str(ROOT / "tools" / "data") not in sys.path:
    sys.path.append(str(ROOT / "tools" / "data"))
from tools.data.datajson import DataError, load, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.nitro_2d import (bitmap_png, build_bitmap, compress_literals, decompress, palette_colors,  # noqa: E402
                                 palette_resource, read_indexed_png, read_palette, write_indexed_png, write_palette)
from tools.data.nitro_font import FontError, build_font_directory, export_font  # noqa: E402
from field_prop_model_data import edit_model, glb_fields  # noqa: E402
from map_model_data import (canonical_texture_name, dictionary_offsets, model_name, pairing_entries, u32,  # noqa: E402
                            write_glb)
from map_texture_data import decode, pack_textures, palette_for, read_png, resources, write_textures  # noqa: E402

GRAPHICS = ROOT / "res" / "graphics"
# The directories that hold archive directories, by what the archives are: the menus' and apps' 2D graphics, the
# demos and cutscenes, the field's gimmicks and effects, the particles and the fonts
ARCHIVE_ROOTS = [GRAPHICS, ROOT / "res" / "demo", ROOT / "res" / "field" / "gimmicks", ROOT / "res" / "field" / "effects",
                 ROOT / "res" / "particles", ROOT / "res" / "fonts"]
VERSIONS = {"black2": "b2_us", "white2": "w2_us"}
EXTENSIONS = {b"RGCN": ".png", b"RLCN": ".pal", b"RCSN": ".nscr", b"RECN": ".ncer", b"RNAN": ".nanr",
              b"RCMN": ".nmcr", b"RAMN": ".nmar", b"BMD0": ".nsbmd", b"BCA0": ".nsbca", b"BTA0": ".nsbta",
              b"BMA0": ".nsbma", b"BTP0": ".nsbtp", b"BTX0": ".nsbtx", b"BVA0": ".nsbva", b"RTFN": ".nftr"}
LZ = {0x10: "lz10", 0x11: "lz11"}


class GraphicsError(Exception):
    pass


def archive_ids() -> dict[str, int]:
    """Returns the ARCID_* constants of include/constants/arc.h and their numbers."""
    text = (ROOT / "include" / "constants" / "arc.h").read_text()
    return {name: int(value, 0) for name, value in re.findall(r"#define (ARCID_\w+) (\w+)", text)}


def archive_path(number: int) -> str:
    digits = f"{number:03d}"
    return f"a/{digits[0]}/{digits[1]}/{digits[2]}"


def archive_directories() -> list[Path]:
    """Every archive directory under ARCHIVE_ROOTS; a name, and so an ARCID_, may be in one root only."""
    found = {}
    for root in ARCHIVE_ROOTS:
        if root.is_dir():
            for directory in sorted(root.iterdir()):
                if directory.is_dir():
                    if directory.name in found:
                        raise GraphicsError(f"{directory} and {found[directory.name]} both name ARCID_{directory.name.upper()}")
                    found[directory.name] = directory
    return sorted(found.values())


def directory_archive(directory: Path) -> str:
    """Returns the archive file of an archive directory, from its ARCID_* constant."""
    name = "ARCID_" + directory.name.upper()
    ids = archive_ids()
    if name not in ids:
        raise GraphicsError(f"{directory}: no {name} in include/constants/arc.h names its archive")
    return archive_path(ids[name])


KIND_EXTENSIONS = {"NCGR": ".png", "NCLR": ".pal", "NSCR": ".nscr", "NCER": ".ncer", "NANR": ".nanr", "NMCR": ".nmcr",
                   "NMAR": ".nmar", "BIN": ".bin", "NSBMD": ".nsbmd", "NSBCA": ".nsbca", "NSBTA": ".nsbta",
                   "NSBMA": ".nsbma", "NSBTP": ".nsbtp", "NSBTX": ".nsbtx", "NSBVA": ".nsbva", "NFTR": ".nftr"}


def member_names(directory: Path) -> dict[int, str]:
    """Returns the file names that generated/narc_<archive>.txt gives an archive's members, by number: the
    constant's name after the archive's, with the extension of its kind (professor.png for NARC_INTRO_PROFESSOR_NCGR)."""
    path = ROOT / "generated" / f"narc_{directory.name}.txt"
    names = {}
    if path.exists():
        prefix = f"NARC_{directory.name.upper()}_"
        for line in path.read_text().splitlines():
            match = re.match(r"(NARC_\w+) = (\d+)", line)
            if match:
                stem, _, kind = match.group(1).removeprefix(prefix).rpartition("_")
                if kind not in KIND_EXTENSIONS:
                    raise GraphicsError(f"{path}: {match.group(1)} doesn't end in a kind of file, such as _NCGR")
                names[int(match.group(2))] = stem.lower() + KIND_EXTENSIONS[kind]
    return names


def unpack(blob: bytes) -> tuple[bytes, str | None]:
    """Returns a member's content and its compression: decompressed where it is an LZ stream of a whole resource."""
    if blob and blob[0] in LZ:
        try:
            data = decompress(blob)
        except (ValueError, IndexError):
            return blob, None
        if data[:4] in EXTENSIONS or len(blob) < len(data):
            return data, LZ[blob[0]]
    return blob, None


def compress(data: bytes, kind: str | None) -> bytes:
    return compress_literals(data, {"lz10": 0x10, "lz11": 0x11}[kind]) if kind else data


# Character resources

def characters(data: bytes) -> tuple[int, int, bool, bytes]:
    """Returns a character resource's tile width and height (None for OBJ characters), bits a pixel, linearity and
    character data."""
    if data[16:20] != b"RAHC":
        raise GraphicsError("a character resource without its RAHC section")
    height, width, depth, _, linear, size, offset = struct.unpack_from("<HHIIIII", data, 24)
    if depth not in (3, 4):
        raise GraphicsError(f"a character resource of depth {depth}, neither 4bpp (3) nor 8bpp (4)")
    bits = {3: 4, 4: 8}[depth]
    chars = data[24 + offset:24 + offset + size]
    sized = width != 0xFFFF and height != 0xFFFF
    return (width if sized else None), (height if sized else None), bits, bool(linear), chars


def chars_to_pixels(chars: bytes, bits: int, width: int, linear: bool) -> tuple[int, bytes]:
    """Returns the height and pixels of character data drawn width pixels wide, in 8 by 8 tiles or with linear in
    rows."""
    if bits == 4:
        values = bytes(value for byte in chars for value in (byte & 15, byte >> 4))
    else:
        values = chars
    height = len(values) // width
    if linear:
        return height, values
    pixels = bytearray(len(values))
    tiles_across = width // 8
    for tile in range(len(values) // 64):
        tx, ty = tile % tiles_across * 8, tile // tiles_across * 8
        for y in range(8):
            pixels[(ty + y) * width + tx:(ty + y) * width + tx + 8] = values[tile * 64 + y * 8:tile * 64 + y * 8 + 8]
    return height, bytes(pixels)


def pixels_to_chars(pixels: bytes, width: int, height: int, bits: int, linear: bool) -> bytes:
    if width % 8 or height % 8:
        raise GraphicsError(f"is {width} by {height}; a character image is a whole number of 8 by 8 tiles")
    if max(pixels, default=0) >= 1 << bits:
        raise GraphicsError(f"has a pixel of index {max(pixels)}, past the {1 << bits} colors of a {bits}bpp image")
    if linear:
        values = pixels
    else:
        values = bytearray()
        for ty in range(0, height, 8):
            for tx in range(0, width, 8):
                for y in range(8):
                    values += pixels[(ty + y) * width + tx:(ty + y) * width + tx + 8]
    if bits == 8:
        return bytes(values)
    return bytes(left | right << 4 for left, right in zip(values[::2], values[1::2]))


def obj_width(tiles: int, bits: int) -> int:
    """Returns the width in pixels to draw OBJ characters, which have no size: rows of four tiles, as a 32 by 32 OBJ
    is stored, or fewer where the tiles don't fill them."""
    return 8 * math.gcd(tiles, 4) if tiles else 8


def character_png(data: bytes, colors: list) -> bytes:
    width, height, bits, linear, chars = characters(data)
    pixel_width = width * 8 if width else obj_width(len(chars) * 8 // bits // 64, bits)
    pixel_height, pixels = chars_to_pixels(chars, bits, pixel_width, linear)
    return write_indexed_png(pixel_width, pixel_height, pixels, colors)


def build_characters(path: Path, original: bytes | None) -> bytes:
    """Returns the character resource of a PNG: the game's resource with the new characters where it had one, its
    size, and its character position section's, the image's where it is sized; else a new sized resource, 8bpp where
    the PNG's palette has more than 16 colors and 4bpp otherwise."""
    width, height, pixels, colors = read_indexed_png(path)
    if original is None:
        bits = 8 if len(colors) > 16 else 4
        try:
            chars = pixels_to_chars(pixels, width, height, bits, False)
        except GraphicsError as error:
            raise GraphicsError(f"{path}: {error}") from None
        sections = struct.pack("<4sIHHIIIII", b"RAHC", 0x20 + len(chars), height // 8, width // 8,
                               3 if bits == 4 else 4, 0, 0, len(chars), 0x18) + chars
        sections += struct.pack("<4sIHHHH", b"SOPC", 0x10, 0, 0, width // 8, height // 8)
        return struct.pack("<4sHHIHH", b"RGCN", 0xFEFF, 0x0101, 0x10 + len(sections), 0x10, 2) + sections
    old_width, _, bits, linear, old_chars = characters(original)
    try:
        chars = pixels_to_chars(pixels, width, height, bits, linear)
    except GraphicsError as error:
        raise GraphicsError(f"{path}: {error}") from None
    if chars == old_chars and (old_width is None or old_width * 8 == width):
        return original
    data = bytearray(original)
    _, _, _, _, _, size, offset = struct.unpack_from("<HHIIIII", data, 24)
    start = 24 + offset
    data[start:start + size] = chars
    section_size = struct.unpack_from("<I", data, 20)[0] + len(chars) - size
    struct.pack_into("<I", data, 20, section_size)
    struct.pack_into("<I", data, 40, len(chars))
    if old_width is not None:
        struct.pack_into("<HH", data, 24, height // 8, width // 8)
        position = 16 + section_size
        if data[position:position + 4] == b"SOPC":
            struct.pack_into("<HH", data, position + 12, width // 8, height // 8)
    struct.pack_into("<I", data, 8, len(data))
    return bytes(data)


# Palette resources

def build_palette(path: Path, original: bytes | None) -> bytes:
    colors = read_palette(path)
    if original is None:
        return palette_resource(colors, 3 if len(colors) <= 16 else 4)
    old = palette_colors(original)
    five = [tuple(round(axis * 31 / 255) for axis in color) for color in colors]
    if five == [tuple(round(axis * 31 / 255) for axis in color) for color in old]:
        return original
    if len(colors) != len(old):
        if original[14] != 1:
            raise GraphicsError(f"{path}: has {len(colors)} colors for the game's {len(old)}; a palette with a "
                                "PCMP section keeps its number of colors")
        fmt = struct.unpack_from("<I", original, 24)[0]
        return palette_resource(colors, fmt)
    data = bytearray(original)
    for index, color in enumerate(five):
        value = struct.unpack_from("<H", data, 40 + 2 * index)[0] & 0x8000
        struct.pack_into("<H", data, 40 + 2 * index, value | color[0] | color[1] << 5 | color[2] << 10)
    return bytes(data)


# 3D models and textures

# A model's data.json, as the field props' is (field_prop_model.schema.json) without their resource ID
MODEL_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["glb_sha256"],
    "properties": {
        "transformed": {"enum": [True]},
        "positions": {"type": "array",
                      "items": {"type": "array", "minItems": 3, "maxItems": 3, "items": {"type": "number"}}},
        "glb_sha256": {"type": "string", "pattern": "^[0-9a-f]{64}$"},
    },
}


def nitro_blocks(data: bytes) -> list[bytes]:
    """Returns the blocks of a G3D file (BMD0, BTX0), which follow its header and its blocks' offsets in order."""
    count = struct.unpack_from("<H", data, 14)[0]
    offsets = [u32(data, 16 + 4 * index) for index in range(count)]
    blocks = []
    for offset, following in zip(offsets, offsets[1:] + [len(data)]):
        if offset + 8 > following or offset + u32(data, offset + 4) != following:
            raise GraphicsError("a 3D file whose blocks don't follow one another")
        blocks.append(data[offset:following])
    if u32(data, 8) != len(data) or not blocks or offsets[0] != 16 + 4 * count:
        raise GraphicsError("a 3D file whose header doesn't give its size and blocks")
    return blocks


def nitro_file(magic: bytes, version: int, blocks: list[bytes]) -> bytes:
    offsets, cursor = [], 16 + 4 * len(blocks)
    for block in blocks:
        offsets.append(cursor)
        cursor += len(block)
    return struct.pack(f"<4sHHIHH{len(blocks)}I", magic, 0xFEFF, version, cursor, 16, len(blocks), *offsets) + \
        b"".join(blocks)


def model_parts(data: bytes) -> tuple[bytes, bytes | None]:
    """Returns a BMD0 file's model, as a BMD0 file of its MDL0 block, which the field models' converters take, and the
    textures it embeds, as a BTX0 file of its TEX0 block, which map_texture_data.py's take, or None without any."""
    blocks = nitro_blocks(data)
    kinds = [block[:4] for block in blocks]
    if kinds not in ([b"MDL0"], [b"MDL0", b"TEX0"]):
        raise GraphicsError(f"a model of blocks {b' '.join(kinds).decode('ascii', 'replace')}, not MDL0 and TEX0")
    version = struct.unpack_from("<H", data, 6)[0]
    return nitro_file(b"BMD0", version, blocks[:1]), nitro_file(b"BTX0", 1, blocks[1:]) if len(blocks) == 2 else None


def joined_model(original: bytes, model: bytes, textures: bytes | None) -> bytes:
    """Returns the BMD0 file of original's header with model's MDL0 block and the TEX0 block of textures."""
    mdl0 = bytearray(nitro_blocks(model)[0])
    mdl0 += bytes(-len(mdl0) % 4)
    struct.pack_into("<I", mdl0, 4, len(mdl0))
    blocks = [bytes(mdl0)] + (nitro_blocks(textures) if textures is not None else [])
    return nitro_file(b"BMD0", struct.unpack_from("<H", original, 6)[0], blocks)


# The texture formats that map_texture_data.py decodes, which have palettes: A3I5, 4-color, 16-color, 256-color and
# A5I3, but not the 4x4 compressed or direct color ones
PALETTE_FORMATS = (1, 2, 3, 4, 6)


def texture_problem(textures: bytes, model: bytes | None = None) -> str | None:
    """Returns why a texture set's textures can't be PNGs that pack again, or None: a texture format that
    map_texture_data.py doesn't decode, or a material of model's that pairs a texture with another palette than the
    one map_texture_data.py gives it."""
    images, palettes = resources(textures)
    formats = {texture["kind"] for texture in images} - set(PALETTE_FORMATS)
    if formats:
        return f"textures of format {min(formats)}, which map_texture_data.py doesn't decode"
    chosen = {}
    try:
        for index, texture in enumerate(images):
            chosen[texture["name"]] = palette_for(texture, palettes, index)["name"]
    except ValueError as error:
        return str(error)
    if model is None:
        return None
    block = u32(model, 16)
    materials = block + dictionary_offsets(model, block + 8)[0]
    materials += u32(model, materials + 8)
    paired = {material: name for name, ids, *_ in pairing_entries(model, materials, 2)[1] for material in ids}
    for name, ids, *_ in pairing_entries(model, materials, 0)[1]:
        for material in ids:
            if name in chosen and paired.get(material, chosen[name]) != chosen[name]:
                return f"material {material} pairs {name} with palette {paired[material]}, not {chosen[name]}"
    return None


def export_textures(directory: Path, textures: bytes) -> str | None:
    """Writes a texture set's directory of PNGs, or returns why it can't."""
    problem = texture_problem(textures)
    if problem is None:
        write_textures(textures, directory)
    return problem


def separate_materials(document: dict):
    """Gives each of a glTF's primitives, the model's draw calls, a material of its own, copying a material that
    several use, since Blender joins the primitives of a material into one on export."""
    used = set()
    for mesh in document.get("meshes", []):
        for primitive in mesh["primitives"]:
            if "material" not in primitive:
                continue
            if primitive["material"] in used:
                material = dict(document["materials"][primitive["material"]])
                copies = sum(other.get("name", "").startswith(material.get("name", "") + ".")
                             for other in document["materials"])
                material["name"] = f"{material.get('name', 'material')}.{copies + 1:03d}"
                document["materials"].append(material)
                primitive["material"] = len(document["materials"]) - 1
            used.add(primitive["material"])


def export_model(directory: Path, data: bytes, apicula: str) -> str | None:
    """Writes a model's directory: model.glb, which Apicula converts the model to, its data.json and the PNGs of the
    textures it embeds, which the GLB's images name. Returns why it can't, writing nothing, where the model is one the
    converters don't take."""
    try:
        model, textures = model_parts(data)
        name = model_name(model)
        problem = texture_problem(textures, model) if textures is not None else None
    except (GraphicsError, ValueError) as error:
        return str(error)
    if problem is not None:
        return problem
    images = {texture["name"]: (index, texture) for index, texture in enumerate(resources(textures)[0])} \
        if textures is not None else {}
    with tempfile.TemporaryDirectory() as temporary:
        temp = Path(temporary)
        (temp / "model.nsbmd").write_bytes(data)
        converted = subprocess.run([apicula, "convert", str(temp / "model.nsbmd"), "-o", str(temp / "converted"), "-f",
                                    "gltf"], capture_output=True, text=True)
        if converted.returncode:
            return f"Apicula doesn't convert it: {converted.stderr.strip().splitlines()[-1:]}"
        gltfs = list((temp / "converted").glob("*.gltf"))
        if len(gltfs) != 1:
            return f"Apicula wrote {len(gltfs)} glTF files for its one model"
        document = json.loads(gltfs[0].read_text())
        separate_materials(document)
        for picture in document.get("images", []):
            if "uri" not in picture:
                continue
            try:
                canonical = canonical_texture_name(picture["uri"], images)
            except ValueError as error:
                return str(error)
            index, texture = images[Path(canonical).stem]
            pixels = decode(textures, texture, palette_for(texture, resources(textures)[1], index))
            if read_png(temp / "converted" / picture["uri"], texture["width"], texture["height"]) != pixels:
                return f"Apicula shows {texture['name']} otherwise than map_texture_data.py decodes it"
            picture["uri"] = canonical
        binary = gltfs[0].with_suffix(".bin").read_bytes()
    directory.mkdir(parents=True)
    if textures is not None:
        write_textures(textures, directory)
    write_glb(directory / "model.glb", document, binary)
    try:
        write(directory / "data.json", glb_fields(directory, name, model))
    except (KeyError, ValueError) as error:
        shutil.rmtree(directory)
        return f"its GLB has no {error}" if isinstance(error, KeyError) else str(error)
    return None


def build_model(directory: Path, original: bytes) -> bytes:
    """Returns the model of a model's directory: the original's bytes where its GLB has the hash it was extracted with
    and its PNGs are unchanged, else the original with the GLB's edits, which the field props' converter makes, and the
    PNGs' pixels."""
    data = load(directory / "data.json", MODEL_SCHEMA)
    model, textures = model_parts(original)
    edited_textures = pack_textures(textures, directory, str(directory)) if textures is not None else None
    edited = edit_model(directory, model_name(model), model, edited_textures, data, directory)
    if edited == model and edited_textures == textures:
        return original
    return joined_model(original, edited, edited_textures)


# Dumping

def preview_colors(members: list[bytes], index: int, bits: int) -> list:
    """Returns the colors to show a character image with: the nearest palette resource's, the first of its 16-color
    banks with four colors or more for a 4bpp image, or grays without one."""
    palettes = [i for i, data in enumerate(members) if data[:4] == b"RLCN"]
    if not palettes:
        return [(value * 17, value * 17, value * 17) for value in range(16)] if bits == 4 else \
            [(value, value, value) for value in range(256)]
    nearest = min(palettes, key=lambda i: (abs(i - index), i > index))
    colors = palette_colors(members[nearest])
    if bits == 8:
        return (colors + [(0, 0, 0)] * 256)[:256]
    banks = [colors[start:start + 16] for start in range(0, len(colors), 16)]
    bank = next((bank for bank in banks if len(set(bank)) >= 4), banks[0])
    return bank + [(0, 0, 0)] * (16 - len(bank))


def member_file(name: str, data: bytes, members: list[bytes], index: int,
                direct_color: tuple[int, int] | None = None) -> tuple[str, bytes]:
    extension = EXTENSIONS.get(data[:4], ".bin")
    if direct_color and extension == ".bin" and len(data) == direct_color[0] * direct_color[1] * 2:
        return name + ".png", bitmap_png(data, *direct_color)
    if extension == ".png":
        _, _, bits, _, _ = characters(data)
        return name + ".png", character_png(data, preview_colors(members, index, bits))
    if extension == ".pal":
        return name + ".pal", write_palette(palette_colors(data)).encode()
    return name + extension, data


def dump(arcid: str, root: Path = GRAPHICS, direct_color: tuple[int, int] | None = None, apicula: str | None = None):
    ids = archive_ids()
    if arcid not in ids:
        raise SystemExit(f"no {arcid} in include/constants/arc.h")
    root = (ROOT / root).resolve() if not root.is_absolute() else root
    if root not in ARCHIVE_ROOTS:
        raise SystemExit(f"{root} is not one of the archive roots: {', '.join(str(r.relative_to(ROOT)) for r in ARCHIVE_ROOTS)}")
    name = arcid.removeprefix("ARCID_").lower()
    for other in archive_directories():
        if other.name == name and other.parent != root:
            raise SystemExit(f"{other} already holds {arcid}")
    directory = root / name
    path = archive_path(ids[arcid])
    games = {game: [unpack(blob) for blob in read_narc((ROOT / "extract" / folder / "files" / path).read_bytes())]
             for game, folder in VERSIONS.items()}
    if len(games["black2"]) != len(games["white2"]):
        raise SystemExit(f"{path} has a different number of members in each version")
    if apicula is None and any(data[:4] == b"BMD0" for members in games.values() for data, _ in members):
        raise SystemExit(f"{path} has models, which Apicula converts to GLBs: install it or pass --apicula PATH")
    names = member_names(directory)
    digits = max(2, len(str(len(games["black2"]) - 1)))
    compressed = {}
    if directory.exists():
        shutil.rmtree(directory)
    directory.mkdir(parents=True)
    for index in range(len(games["black2"])):
        name = names.get(index, f"{index:0{digits}d}").split(".")[0]
        black, white = games["black2"][index], games["white2"][index]
        if black[1] != white[1]:
            raise SystemExit(f"{path}: member {index} is compressed differently in each version")
        targets = {"": black} if black == white else {"black2/": black, "white2/": white}
        for sub, (data, _) in targets.items():
            members = [content for content, _ in games["black2" if sub != "white2/" else "white2"]]
            (directory / sub).mkdir(exist_ok=True)
            file_name, converted = name, False
            if data[:4] in (b"BMD0", b"BTX0", b"RTFN"):
                problem = export_model(directory / sub / name, data, apicula) if data[:4] == b"BMD0" else \
                    export_textures(directory / sub / name, data) if data[:4] == b"BTX0" else \
                    export_font(directory / sub / name, data)
                converted = problem is None
                if problem is not None:
                    print(f"{path}: member {index} stays a Nitro file: {problem}")
            if not converted:
                file_name, content = member_file(name, data, members, index, direct_color)
                (directory / sub / file_name).write_bytes(content)
            if black[1]:
                compressed[file_name] = black[1]
    settings = {"direct_color": list(direct_color)} if direct_color else {}
    if compressed:
        settings["compressed"] = compressed
    if settings:
        (directory / "archive.json").write_text(json.dumps(settings, indent=4) + "\n")
    print(f"wrote {directory.relative_to(ROOT)} from {path}")


# Packing

def member_files(directory: Path, game: str) -> list[tuple[str, Path]]:
    """Returns an archive's members, (name, file), in order: numbered files, or named ones at their constants'
    numbers, each from the version's directory where it has one. A member's file is a directory where it is a model,
    a texture set or a font, named without an extension."""
    files = {}
    for folder in (directory, directory / game):
        for path in folder.iterdir() if folder.is_dir() else []:
            if path.is_file() and path.name != "archive.json" or path.is_dir() and not (
                    folder == directory and path.name in VERSIONS):
                files[path.name] = path
    names = {name: number for number, name in member_names(directory).items()}
    names |= {name.split(".")[0]: number for name, number in names.copy().items()
              if name.endswith((".nsbmd", ".nsbtx", ".nftr"))}
    numbered = {}
    for file_name, path in files.items():
        stem = file_name.split(".")[0]
        if file_name in names:
            number = names[file_name]
        elif stem.isdigit():
            number = int(stem)
        else:
            raise GraphicsError(f"{path}: named neither by a number nor by a constant of narc_{directory.name}.txt")
        if number in numbered:
            raise GraphicsError(f"{path}: member {number} also has {numbered[number][1].name}")
        numbered[number] = (stem, path)
    if sorted(numbered) != list(range(len(numbered))):
        missing = sorted(set(range(max(numbered, default=-1) + 1)) - set(numbered))
        raise GraphicsError(f"{directory}: members {missing} have no file; an archive's members are numbered from 0")
    return [numbered[number] for number in range(len(numbered))]


def build_member(path: Path, original: bytes | None, direct_color: list[int] | None = None) -> bytes:
    if path.is_dir():
        if (path / "font.json").exists():
            return build_font_directory(path)
        if original is not None and original[:4] == b"BMD0":
            return build_model(path, original)
        if original is not None and original[:4] == b"BTX0":
            return pack_textures(original, path, str(path))
        raise GraphicsError(f"{path}: a directory, which only the game's models and texture sets and fonts are; add a "
                            "new model or texture set as its .nsbmd or .nsbtx file")
    if path.suffix == ".png" and direct_color and not (original and original[:4] == b"RGCN"):
        return build_bitmap(path, *direct_color)
    if path.suffix == ".png":
        return build_characters(path, original if original and original[:4] == b"RGCN" else None)
    if path.suffix == ".pal":
        return build_palette(path, original if original and original[:4] == b"RLCN" else None)
    return path.read_bytes()


def pack(directory: Path, game: str, output: Path):
    path = directory_archive(directory)
    originals = read_narc((ROOT / "extract" / VERSIONS[game] / "files" / path).read_bytes())
    settings = json.loads((directory / "archive.json").read_text()) if (directory / "archive.json").exists() else {}
    compressed = settings.get("compressed", {})
    members = member_files(directory, game)
    for name, kind in compressed.items():
        if kind not in ("lz10", "lz11"):
            raise GraphicsError(f"{directory / 'archive.json'}: {name}'s compression is {kind!r}, not lz10 or lz11")
        if name not in {file.name for _, file in members}:
            raise GraphicsError(f"{directory / 'archive.json'}: {name} is compressed, but there is no such member")
    result = []
    for index, (stem, file) in enumerate(members):
        original = originals[index] if index < len(originals) else None
        content, kind = unpack(original) if original is not None else (None, None)
        built = build_member(file, content, settings.get("direct_color"))
        if original is not None and built == content and kind == compressed.get(file.name):
            result.append(original)
        else:
            result.append(compress(built, compressed.get(file.name)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(result))


def main():
    try:
        if len(sys.argv) >= 3 and sys.argv[1] == "dump":
            args = sys.argv[2:]
            direct_color = None
            if "--direct-color" in args:
                at = args.index("--direct-color")
                direct_color = tuple(int(n) for n in args[at + 1].split("x"))
                del args[at:at + 2]
            apicula = shutil.which("apicula")
            if "--apicula" in args:
                at = args.index("--apicula")
                apicula = args[at + 1]
                del args[at:at + 2]
            dump(args[0], *(Path(a) for a in args[1:]), direct_color=direct_color, apicula=apicula)
        elif len(sys.argv) == 5 and sys.argv[1] == "pack":
            pack(Path(sys.argv[2]), sys.argv[3], Path(sys.argv[4]))
        else:
            raise SystemExit(__doc__)
    except (GraphicsError, DataError, FontError, ValueError) as error:
        raise SystemExit(f"error: {error}") from None


if __name__ == "__main__":
    main()
