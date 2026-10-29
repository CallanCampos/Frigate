"""The game's particle effects, in the resource format of NitroSystem's particle library (SPA, magic " APS", which
lib/spl reads), as their emitters' fields and their textures' PNGs.

    nitro_particles.py dump FILE DIRECTORY        # writes a resource's particles.json and textures
    nitro_particles.py pack DIRECTORY OUT [FILE]  # and builds it from them, FILE's textures where they are unchanged

graphics_archive.py dumps and packs ARCID_BTLV_EFFVM_PARTICLES's resources with this, and the other archives' particle
members, each a directory of:

- particles.json (particles.schema.json in res/particles/): the resource's emitters, in order, which the game creates by
  their index (SPLManager_CreateEmitter), and its textures, in order. An emitter has the fields of the resource header
  (SPLResourceHeader) under lib/spl's names, then the blocks it has of the ones that may follow, under the names of
  SPLResource's pointers: the scale, color, alpha and texture animations, the child resource and the six behaviors.
  Which blocks follow is the header's flags (hasScaleAnim and the rest), which the packer sets from the blocks there
  are, as SPLManager_LoadResources reads them. Fixed-point fields are the integers the game keeps, the schema saying
  in what units; colors are {red, green, blue}, 0 to 31 each; the enumerations are names, lib/spl's in lower case;
  textures are named. A block's "unused", where it has one, holds the fields and bits that the library never reads
  but that the game's files don't leave zero, so that they build again byte for byte.
- a PNG for each texture, named after it, texture00.png and on in the resource's order: RGBA pixels as the texture's
  format shows them, decoded and encoded as map_texture_data.py does a field texture's (A3I5, A5I3 and 4, 16 and 256
  colors), or direct color (RGB555 and an alpha bit). An unchanged texture packs as the game's bytes; an edited one is
  encoded again in its format and palette, which may grow to the format's colors, and its size is the PNG's, a power of
  two from 8 to 1024. A texture of another format (4x4 compressed) is its data as it is, texture00.bin, and its entry
  gives its width and height.

The rest is computed: the file header's counts, sizes and offsets, the flags, a texture animation's count, and each
texture's header but for its format, repeat, flip, color 0 and sharing. A resource that this doesn't read, or wouldn't
build again byte for byte, stays its .spa file, as graphics_archive.py's dump says.

Only the standard library, since the build runs the packer.
"""
import json
import os
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
if str(ROOT / "tools" / "data") not in sys.path:
    sys.path.append(str(ROOT / "tools" / "data"))
from tools.data.datajson import DataError, load, load_schema  # noqa: E402
from map_texture_data import decode, decode_png, encode_palette_group, visible, write_png  # noqa: E402

SCHEMA = ROOT / "res" / "particles" / "particles.schema.json"
JSON_NAME = "particles.json"
SIGNATURE, VERSION, TEXTURE_SIGNATURE = b" APS", b"12_1", b" TPS"

# The enumerations' names, by value, as lib/spl's enums give them
EMISSION_TYPES = ["point", "sphere_surface", "circle_border", "circle_border_uniform", "sphere", "circle",
                  "cylinder_surface", "cylinder", "hemisphere_surface", "hemisphere"]
DRAW_TYPES = ["billboard", "directional_billboard", "polygon", "directional_polygon", "directional_polygon_center"]
AXES = ["z", "y", "x", "emitter"]
# SPLDraw's sRotateFuncs: about y, or the diagonal (1, 1, 1)
ROTATION_AXES = ["y", "xyz"]
PLANES = ["xy", "xz"]
SCALE_AXES = ["xy", "x", "y"]
CHILD_ROTATIONS = ["none", "angle", "angle_and_velocity"]
# SPLBehavior_ApplySpin's cases
SPIN_AXES = ["x", "y", "z"]
COLLISIONS = ["kill", "bounce"]
# GXTexFmt, GXTexRepeat and GXTexFlip
TEXTURE_FORMATS = ["none", "a3i5", "palette4", "palette16", "palette256", "compressed4x4", "a5i3", "direct"]
REPEATS = ["none", "s", "t", "st"]

# Kinds of field besides int, bool and a list of names: the ones the packer computes, such as the presence flags,
# which the blocks there are set; the fields of "unused"; vectors; colors; a texture's index, or a texture animation's
# eight
COMPUTED, UNUSED, VEC32, VEC16, RGB = "computed", "unused", "vec32", "vec16", "rgb"
TEXTURE, TEXTURES = "texture", "textures"

# A block's layout, as lib/spl's structs give it: (name, format, kind) for a field, or (format, [(name, bits, kind),
# ...]) for a word of bitfields from the lowest bit. The bits a struct leaves unnamed are named unk and the offset of
# their word, as lib/spl names its unknown fields
HEADER = [
    ("I", [("emissionType", 4, EMISSION_TYPES), ("drawType", 2, DRAW_TYPES), ("emissionAxis", 2, AXES),
           ("hasScaleAnim", 1, COMPUTED), ("hasColorAnim", 1, COMPUTED), ("hasAlphaAnim", 1, COMPUTED),
           ("hasTextureAnim", 1, COMPUTED), ("hasRotation", 1, bool), ("randomInitAngle", 1, bool),
           ("autoTerminate", 1, bool), ("followEmitter", 1, bool), ("hasChildResource", 1, COMPUTED),
           ("polygonRotAxis", 2, ROTATION_AXES), ("polygonReferencePlane", 1, PLANES), ("randomLoopOffset", 1, bool),
           ("drawChildrenFirst", 1, bool), ("hideParent", 1, bool), ("relativeToBasePos", 1, bool),
           ("hasGravityBehavior", 1, COMPUTED), ("hasRandomBehavior", 1, COMPUTED), ("hasMagnetBehavior", 1, COMPUTED),
           ("hasSpinBehavior", 1, COMPUTED), ("hasCollisionPlaneBehavior", 1, COMPUTED),
           ("hasConvergenceBehavior", 1, COMPUTED), ("fixedPolygonID", 1, bool), ("childFixedPolygonID", 1, bool)]),
    ("emitterBasePos", "3i", VEC32), ("emissionCount", "i", int), ("radius", "i", int), ("length", "i", int),
    ("axis", "3h", VEC16), ("color", "H", RGB), ("initVelPositionAmplifier", "i", int),
    ("initVelAxisAmplifier", "i", int), ("baseScale", "i", int), ("aspectRatio", "h", int), ("startDelay", "H", int),
    ("minRotation", "h", int), ("maxRotation", "h", int), ("initAngle", "H", int), ("unk3a", "H", UNUSED),
    ("emitterLifeTime", "H", int), ("particleLifeTime", "H", int), ("scaleVariance", "B", int),
    ("lifeTimeVariance", "B", int), ("velocityVariance", "B", int), ("unk43", "B", UNUSED),
    ("emissionInterval", "B", int), ("baseAlpha", "B", int), ("airResistance", "B", int), ("texture", "B", TEXTURE),
    ("I", [("loopTime", 8, int), ("dirStretch", 16, int), ("textureRepeatShiftS", 2, int),
           ("textureRepeatShiftT", 2, int), ("scaleAnimDir", 3, SCALE_AXES), ("dirFromPosition", 1, bool)]),
    ("I", [("flipTextureS", 1, bool), ("flipTextureT", 1, bool), ("unk4c", 30, UNUSED)]),
    ("polygonX", "h", int), ("polygonY", "h", int), ("unk54", "I", UNUSED),
]
SCALE_ANIM = [("start", "h", int), ("mid", "h", int), ("end", "h", int), ("in", "B", int), ("out", "B", int),
              ("H", [("loop", 1, bool), ("unk8", 15, UNUSED)]), ("padding", "H", UNUSED)]
COLOR_ANIM = [("start", "H", RGB), ("end", "H", RGB), ("in", "B", int), ("peak", "B", int), ("out", "B", int),
              ("padding", "B", UNUSED),
              ("H", [("randomStart", 1, bool), ("loop", 1, bool), ("interpolate", 1, bool), ("unk8", 13, UNUSED)]),
              ("padding2", "H", UNUSED)]
ALPHA_ANIM = [("H", [("start", 5, int), ("mid", 5, int), ("end", 5, int), ("unk0", 1, UNUSED)]),
              ("H", [("randomRange", 8, int), ("loop", 1, bool), ("unk2", 7, UNUSED)]),
              ("in", "B", int), ("out", "B", int), ("padding2", "H", UNUSED)]
TEXTURE_ANIM = [("textures", "8B", TEXTURES),
                ("I", [("count", 8, COMPUTED), ("step", 8, int), ("randomStart", 1, bool), ("loop", 1, bool),
                       ("unk8", 14, UNUSED)])]
CHILD_RESOURCE = [
    ("H", [("hasBehaviors", 1, bool), ("hasScaleAnim", 1, bool), ("hasAlphaAnim", 1, bool),
           ("rotationType", 2, CHILD_ROTATIONS), ("followEmitter", 1, bool), ("useChildColor", 1, bool),
           ("drawType", 2, DRAW_TYPES), ("polygonRotAxis", 2, ROTATION_AXES), ("polygonReferencePlane", 1, PLANES),
           ("unk0", 4, UNUSED)]),
    ("randomInitVelMag", "h", int), ("endScale", "h", int), ("lifeTime", "H", int), ("velocityRatio", "B", int),
    ("scaleRatio", "B", int), ("color", "H", RGB), ("emissionCount", "B", int), ("emissionDelay", "B", int),
    ("emissionInterval", "B", int), ("texture", "B", TEXTURE),
    ("I", [("textureRepeatShiftS", 2, int), ("textureRepeatShiftT", 2, int), ("flipTextureS", 1, bool),
           ("flipTextureT", 1, bool), ("dirFromPosition", 1, bool), ("unk10", 25, UNUSED)]),
]
GRAVITY = [("magnitude", "3h", VEC16), ("padding", "H", UNUSED)]
RANDOM = [("magnitude", "3h", VEC16), ("applyInterval", "H", int)]
MAGNET = [("target", "3i", VEC32), ("force", "h", int), ("padding", "H", UNUSED)]
SPIN = [("angle", "H", int), ("axis", "H", SPIN_AXES)]
COLLISION_PLANE = [("y", "i", int), ("elasticity", "h", int), ("H", [("type", 2, COLLISIONS), ("unk6", 14, UNUSED)])]
CONVERGENCE = [("target", "3i", VEC32), ("force", "h", int), ("padding", "H", UNUSED)]
# The blocks that may follow the header, in order, each with its flag (SPLManager_LoadResources)
BLOCKS = [("hasScaleAnim", "scaleAnim", SCALE_ANIM), ("hasColorAnim", "colorAnim", COLOR_ANIM),
          ("hasAlphaAnim", "alphaAnim", ALPHA_ANIM), ("hasTextureAnim", "textureAnim", TEXTURE_ANIM),
          ("hasChildResource", "childResource", CHILD_RESOURCE), ("hasGravityBehavior", "gravityBehavior", GRAVITY),
          ("hasRandomBehavior", "randomBehavior", RANDOM), ("hasMagnetBehavior", "magnetBehavior", MAGNET),
          ("hasSpinBehavior", "spinBehavior", SPIN),
          ("hasCollisionPlaneBehavior", "collisionPlaneBehavior", COLLISION_PLANE),
          ("hasConvergenceBehavior", "convergenceBehavior", CONVERGENCE)]
# SPLTextureResource and its SPLTextureParam
TEXTURE_HEADER = "<4sIIIIIII"
TEXTURE_PARAM = [("format", 4, TEXTURE_FORMATS), ("s", 4, COMPUTED), ("t", 4, COMPUTED), ("repeat", 2, REPEATS),
                 ("flip", 2, REPEATS), ("palColor0", 1, bool), ("useSharedTexture", 1, COMPUTED),
                 ("sharedTexID", 8, COMPUTED),
                 ("unk4", 6, UNUSED)]
FILE_HEADER = "<4s4sHHIIIII"
# A palette format's colors, and its bits a pixel; the texel bits of the others
PALETTE_COLORS = {1: 32, 2: 4, 3: 16, 4: 256, 6: 8}
TEXEL_BITS = {0: 0, 1: 8, 2: 2, 3: 4, 4: 8, 5: 2, 6: 8, 7: 16}
DIRECT, COMPRESSED = 7, 5


class ParticleError(Exception):
    pass


def layout_format(layout) -> str:
    return "<" + "".join(entry[1] if len(entry) == 3 else entry[0] for entry in layout)


def layout_fields(layout):
    """Each field of a layout, (name, kind), the bitfields' too."""
    for entry in layout:
        if len(entry) == 3:
            yield entry[0], entry[2]
        else:
            yield from ((name, kind) for name, _, kind in entry[1])


def read_block(layout, data: bytes, offset: int) -> tuple[dict, int]:
    """Returns a block's fields as numbers, by name, and the offset after it."""
    fmt = layout_format(layout)
    if offset + struct.calcsize(fmt) > len(data):
        raise ParticleError(f"a block at {offset:#x} runs past the end")
    values = list(struct.unpack_from(fmt, data, offset))
    fields = {}
    for entry in layout:
        if len(entry) == 3:
            count = len(struct.unpack("<" + entry[1], bytes(struct.calcsize("<" + entry[1]))))
            fields[entry[0]] = values[:count] if count > 1 else values[0]
            del values[:count]
        else:
            word, shift = values.pop(0), 0
            for name, bits, _ in entry[1]:
                fields[name] = word >> shift & (1 << bits) - 1
                shift += bits
    return fields, offset + struct.calcsize(fmt)


def write_block(layout, fields: dict, where: str = "a block") -> bytes:
    values = []
    for entry in layout:
        if len(entry) == 3:
            value = fields[entry[0]]
            values += value if isinstance(value, list) else [value]
        else:
            word, shift = 0, 0
            for name, bits, _ in entry[1]:
                if not 0 <= fields[name] < 1 << bits:
                    raise ParticleError(f"{where}: {name} is {fields[name]}, past its {bits} bits")
                word |= fields[name] << shift
                shift += bits
            values.append(word)
    try:
        return struct.pack(layout_format(layout), *values)
    except struct.error as error:
        raise ParticleError(f"{where}: a field past its size ({error})") from None


# Fields and their JSON

def color_json(value: int) -> dict:
    if value & 0x8000:
        raise ParticleError(f"a color, {value:#06x}, with its top bit set")
    return {"red": value & 31, "green": value >> 5 & 31, "blue": value >> 10 & 31}


def block_json(layout, fields: dict, names: list[str]) -> dict:
    """A block's JSON: its fields under their names, but the computed ones, and its unused ones that aren't zero."""
    result, unused = {}, {}
    for name, kind in layout_fields(layout):
        value = fields[name]
        if kind == COMPUTED:
            continue
        if kind == UNUSED:
            if value:
                unused[name] = value
        elif kind is bool:
            result[name] = bool(value)
        elif isinstance(kind, list):
            if value >= len(kind):
                raise ParticleError(f"{name} is {value}, which has no name")
            result[name] = kind[value]
        elif kind in (VEC32, VEC16):
            result[name] = dict(zip("xyz", value))
        elif kind == RGB:
            result[name] = color_json(value)
        elif kind == TEXTURE:
            if value >= len(names):
                raise ParticleError(f"{name} is texture {value}, of {len(names)}")
            result[name] = names[value]
        elif kind == TEXTURES:
            count = fields["count"]
            if count > 8 or any(value[count:]) or any(index >= len(names) for index in value[:count]):
                raise ParticleError("a texture animation of textures past its count or the resource's")
            result[name] = [names[index] for index in value[:count]]
        else:
            result[name] = value
    if unused:
        result["unused"] = unused
    return result


def block_fields(layout, item: dict, names: dict[str, int], where: str) -> dict:
    """A block's fields from its JSON, the computed ones 0 where the JSON doesn't give them."""
    fields = {}
    unused = item.get("unused", {})
    unknown = set(unused) - {name for name, kind in layout_fields(layout) if kind == UNUSED}
    if unknown:
        raise ParticleError(f"{where}: no unused field {min(unknown)} here")
    for name, kind in layout_fields(layout):
        if kind == COMPUTED:
            fields.setdefault(name, 0)
        elif kind == UNUSED:
            fields[name] = unused.get(name, 0)
        elif kind == TEXTURES:
            if name == "textures":
                if len(item["textures"]) > 8:
                    raise ParticleError(f"{where}: a texture animation of {len(item['textures'])} textures, not 8 "
                                        "or fewer")
                indices = [texture_index(texture, names, where) for texture in item["textures"]]
                fields["textures"] = indices + [0] * (8 - len(indices))
                fields["count"] = len(indices)
        elif kind is bool:
            fields[name] = int(item[name])
        elif isinstance(kind, list):
            fields[name] = kind.index(item[name])
        elif kind in (VEC32, VEC16):
            fields[name] = [item[name][axis] for axis in "xyz"]
        elif kind == RGB:
            color = item[name]
            fields[name] = color["red"] | color["green"] << 5 | color["blue"] << 10
        elif kind == TEXTURE:
            fields[name] = texture_index(item[name], names, where)
        else:
            fields[name] = item[name]
    return fields


def texture_index(name: str, names: dict[str, int], where: str) -> int:
    if name not in names:
        raise ParticleError(f"{where}: no texture {name!r} in the resource's textures")
    return names[name]


# The resource

def texture_name(index: int) -> str:
    return f"texture{index:02d}"


def read_particles(data: bytes) -> tuple[dict, list[dict]]:
    """Returns a resource's JSON, its emitters and its textures' entries, and its textures, each its format, size,
    texel data and palette."""
    if len(data) < 0x20:
        raise ParticleError("too short for a particle resource's header")
    signature, version, resource_count, texture_count, unk_c, resource_size, texture_size, texture_offset, unk_1c = \
        struct.unpack_from(FILE_HEADER, data)
    if signature != SIGNATURE or version != VERSION or unk_c or unk_1c:
        raise ParticleError(f"not a version {VERSION[::-1].decode()} particle resource with its header's unknowns 0")
    names = [texture_name(index) for index in range(texture_count)]
    emitters, offset = [], 0x20
    for _ in range(resource_count):
        header, offset = read_block(HEADER, data, offset)
        emitter = block_json(HEADER, header, names)
        for flag, name, layout in BLOCKS:
            if header[flag]:
                fields, offset = read_block(layout, data, offset)
                emitter[name] = block_json(layout, fields, names)
        emitters.append(emitter)
    if offset != 0x20 + resource_size or offset != texture_offset:
        raise ParticleError("its emitters don't end where its header says its textures start")
    entries, textures = [], []
    for index in range(texture_count):
        if offset + 0x20 > len(data):
            raise ParticleError(f"texture {index} runs past the end")
        signature, param, size, palette_offset, palette_size, unk14, unk18, total = \
            struct.unpack_from(TEXTURE_HEADER, data, offset)
        fields, _ = read_block([("I", TEXTURE_PARAM)], param.to_bytes(4, "little"), 0)
        if signature != TEXTURE_SIGNATURE or unk14 != total or unk18 or palette_offset != 0x20 + size or \
                total != palette_offset + palette_size or offset + total > len(data):
            raise ParticleError(f"texture {index}'s header isn't one of texture data and then a palette")
        width, height = 8 << fields["s"], 8 << fields["t"]
        if size != texel_size(fields["format"], width, height):
            raise ParticleError(f"texture {index} has {size} bytes of data for a {width} by {height} texture")
        entry = {"name": names[index]} | block_json([("I", TEXTURE_PARAM)], fields, names)
        if fields["useSharedTexture"]:
            if fields["sharedTexID"] >= texture_count:
                raise ParticleError(f"texture {index} shares texture {fields['sharedTexID']}, of {texture_count}")
            entry["sharedTexture"] = names[fields["sharedTexID"]]
        elif fields["sharedTexID"]:
            raise ParticleError(f"texture {index} shares no texture but names one")
        if not decodable(fields["format"]):
            entry |= {"width": width, "height": height}
        entries.append(entry)
        start = offset + 0x20
        textures.append({"format": fields["format"], "width": width, "height": height,
                         "texels": data[start:start + size], "palette": data[start + size:start + size + palette_size]})
        offset += total
    if offset != len(data) or offset != texture_offset + texture_size:
        raise ParticleError("its textures don't end at the end of the file")
    return {"emitters": emitters, "textures": entries}, textures


def texel_size(kind: int, width: int, height: int) -> int:
    """A texture's bytes of data: its texels, and a 4x4 compressed one's palette indices too."""
    return width * height * TEXEL_BITS[kind] // 8 + (width * height // 8 if kind == COMPRESSED else 0)


def decodable(kind: int) -> bool:
    return kind in PALETTE_COLORS or kind == DIRECT


def build_particles(document: dict, textures: list[dict], where: str = "particles") -> bytes:
    """Returns the resource of a JSON document and its textures, each as read_particles gives them."""
    names = {entry["name"]: index for index, entry in enumerate(document["textures"])}
    if len(names) != len(document["textures"]):
        raise ParticleError(f"{where}: two textures have one name")
    if len(document["emitters"]) > 0xFFFF or len(names) > 0xFFFF:
        raise ParticleError(f"{where}: more emitters or textures than a resource counts")
    resources = bytearray()
    for number, emitter in enumerate(document["emitters"]):
        at = f"{where}: emitter {number}"
        header = block_fields(HEADER, emitter, names, at)
        blocks = b""
        for flag, name, layout in BLOCKS:
            if name in emitter:
                header[flag] = 1
                blocks += write_block(layout, block_fields(layout, emitter[name], names, f"{at}'s {name}"),
                                      f"{at}'s {name}")
        resources += write_block(HEADER, header, at) + blocks
    texture_data = bytearray()
    for index, (entry, texture) in enumerate(zip(document["textures"], textures)):
        fields = block_fields([("I", TEXTURE_PARAM)], entry, names, f"{where}: texture {entry['name']}")
        fields["s"], fields["t"] = (side.bit_length() - 4 for side in (texture["width"], texture["height"]))
        if "sharedTexture" in entry:
            fields["useSharedTexture"] = 1
            fields["sharedTexID"] = texture_index(entry["sharedTexture"], names, f"{where}: texture {entry['name']}")
        else:
            fields["useSharedTexture"] = fields["sharedTexID"] = 0
        size = len(texture["texels"]) + len(texture["palette"])
        texture_data += struct.pack(TEXTURE_HEADER, TEXTURE_SIGNATURE,
                                    struct.unpack("<I", write_block([("I", TEXTURE_PARAM)], fields))[0],
                                    len(texture["texels"]), 0x20 + len(texture["texels"]), len(texture["palette"]),
                                    0x20 + size, 0, 0x20 + size) + texture["texels"] + texture["palette"]
    header = struct.pack(FILE_HEADER, SIGNATURE, VERSION, len(document["emitters"]), len(textures), 0, len(resources),
                         len(texture_data), 0x20 + len(resources), 0)
    return header + resources + texture_data


# Textures as PNGs

def texture_view(texture: dict, color0: bool) -> tuple[bytes, dict, dict]:
    """Returns a texture as map_texture_data.py decodes and encodes one: a texture set's bytes of its texels and its
    palette, as many colors as its format has, and the entries of the texture and its palette there."""
    full = max(len(texture["palette"]), 2 * PALETTE_COLORS[texture["format"]])
    blob = texture["texels"] + texture["palette"].ljust(full, b"\0")
    entry = {"name": "texture", "width": texture["width"], "height": texture["height"], "kind": texture["format"],
             "params": int(color0) << 29, "offset": 0, "length": len(texture["texels"])}
    return blob, entry, {"offset": len(texture["texels"])}


def texture_pixels(texture: dict, color0: bool) -> bytes:
    """A texture's RGBA pixels."""
    if texture["format"] == DIRECT:
        values = struct.unpack(f"<{len(texture['texels']) // 2}H", texture["texels"])
        return bytes(channel for value in values for channel in (
            *((value >> shift & 31) << 3 | (value >> shift & 31) >> 2 for shift in (0, 5, 10)),
            255 if value & 0x8000 else 0))
    blob, entry, palette = texture_view(texture, color0)
    return decode(blob, entry, palette)


def encode_texture(pixels: bytes, width: int, height: int, kind: int, color0: bool, base: dict | None,
                   where: str) -> dict:
    """Returns a texture of pixels in a format: in base's palette where there is one, which may grow to the format's
    colors, and else in a new one."""
    if kind == DIRECT:
        texels = b"".join(struct.pack("<H", (0x8000 if pixels[i + 3] >= 128 else 0) | sum(
            round(pixels[i + axis] * 31 / 255) << 5 * axis for axis in range(3))) for i in range(0, len(pixels), 4))
        return {"format": kind, "width": width, "height": height, "texels": texels, "palette": b""}
    palette = base["palette"] if base is not None and base["format"] == kind else b""
    texture = {"format": kind, "width": width, "height": height, "texels": bytes(texel_size(kind, width, height)),
               "palette": palette}
    blob, entry, palette_entry = texture_view(texture, color0)
    blob = bytearray(blob)
    try:
        ((_, texels),) = encode_palette_group(blob, [(entry, pixels)], palette_entry["offset"], len(blob))
    except ValueError as error:
        raise ParticleError(f"{where}: {error}") from None
    indices = texels if kind in (1, 4, 6) else bytes(byte >> shift & (1 << TEXEL_BITS[kind]) - 1 for byte in texels
                                                      for shift in range(0, 8, TEXEL_BITS[kind]))
    mask = {1: 31, 6: 7}.get(kind, 255)
    used = 2 * (max((index & mask for index in indices), default=0) + 1)
    size = max(len(palette), used + -used % 4)
    texture["texels"] = bytes(texels)
    texture["palette"] = bytes(blob[len(texels):len(texels) + size])
    entry["params"] = int(color0) << 29
    if visible(texture_pixels(texture, color0), entry) != visible(pixels, entry):
        raise ParticleError(f"{where}: the texture's format and palette can't show the PNG's pixels")
    return texture


def png_size(path: Path) -> tuple[int, int]:
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[12:16] != b"IHDR":
        raise ParticleError(f"{path}: not a PNG")
    return struct.unpack(">II", data[16:24])


def read_texture(directory: Path, entry: dict, original: dict | None, where: str) -> dict:
    """Returns a texture from its file: the original's where its pixels are unchanged, else encoded again."""
    kind, color0 = TEXTURE_FORMATS.index(entry["format"]), entry["palColor0"]
    if not decodable(kind):
        path = directory / f"{entry['name']}.bin"
        data = path.read_bytes()
        size = texel_size(kind, entry["width"], entry["height"])
        if len(data) < size:
            raise ParticleError(f"{path}: {len(data)} bytes, short of a {entry['width']} by {entry['height']} "
                                f"texture's {size}")
        return {"format": kind, "width": entry["width"], "height": entry["height"], "texels": data[:size],
                "palette": data[size:]}
    path = directory / f"{entry['name']}.png"
    width, height = png_size(path)
    if any(side not in (8, 16, 32, 64, 128, 256, 512, 1024) for side in (width, height)):
        raise ParticleError(f"{path}: {width} by {height}; a texture's sides are powers of two from 8 to 1024")
    try:
        pixels = decode_png(path.read_bytes(), width, height, str(path))
    except ValueError as error:
        raise ParticleError(str(error)) from None
    same = original is not None and (original["format"], original["width"], original["height"]) == (kind, width, height)
    if same and kind != DIRECT:
        decoded = texture_pixels(original, color0)
        view = {"name": entry["name"], "kind": kind, "params": int(color0) << 29}
        if pixels == decoded or visible(pixels, view) == visible(decoded, view):
            return original
    try:
        texture = encode_texture(pixels, width, height, kind, color0, original, str(path))
    except ValueError as error:
        raise ParticleError(f"{path}: {error}") from None
    return original if same and texture["texels"] == original["texels"] else texture


# The resource's directory

def compact_dumps(data, indent: int = 0, width: int = 120) -> str:
    """JSON as datajson writes it, but with an object of plain values on one line where it fits, a vector or a
    color as one."""
    inner = " " * (indent + 4)
    if isinstance(data, dict) and data and all(not isinstance(value, (dict, list)) for value in data.values()):
        line = "{ " + ", ".join(f"{json.dumps(key)}: {json.dumps(value)}" for key, value in data.items()) + " }"
        if indent + len(line) <= width:
            return line
    if isinstance(data, list) and data and all(not isinstance(value, (dict, list)) for value in data):
        line = "[ " + ", ".join(json.dumps(value) for value in data) + " ]"
        if indent + len(line) <= width:
            return line
    if isinstance(data, dict) and data:
        return "{\n" + ",\n".join(f"{inner}{json.dumps(key)}: {compact_dumps(value, indent + 4, width)}"
                                  for key, value in data.items()) + "\n" + " " * indent + "}"
    if isinstance(data, list) and data:
        return "[\n" + ",\n".join(inner + compact_dumps(item, indent + 4, width) for item in data) + "\n" + \
            " " * indent + "]"
    return json.dumps(data)


def write_directory(directory: Path, document: dict, textures: list[dict]):
    directory.mkdir(parents=True, exist_ok=True)
    for entry, texture in zip(document["textures"], textures):
        if decodable(texture["format"]):
            pixels = texture_pixels(texture, entry["palColor0"])
            (directory / f"{entry['name']}.png").write_bytes(write_png(texture["width"], texture["height"], pixels))
        else:
            (directory / f"{entry['name']}.bin").write_bytes(texture["texels"] + texture["palette"])
    schema = os.path.relpath(SCHEMA, directory.resolve())
    (directory / JSON_NAME).write_text(compact_dumps({"$schema": schema} | document) + "\n")


def build_directory(directory: Path, original: bytes | None = None) -> bytes:
    """Returns the resource of a directory: its emitters from particles.json, and its textures from their files, the
    original's bytes of each texture whose pixels are unchanged."""
    try:
        document = load(directory / JSON_NAME, load_schema(SCHEMA))
        originals = read_particles(original)[1] if original is not None else []
    except DataError as error:
        raise ParticleError(str(error)) from None
    except ParticleError:
        originals = []
    textures = [read_texture(directory, entry, originals[index] if index < len(originals) else None,
                             f"{directory}: texture {entry['name']}")
                for index, entry in enumerate(document["textures"])]
    return build_particles(document, textures, str(directory))


def export_particles(directory: Path, data: bytes) -> str | None:
    """Writes a resource's directory, or returns why it can't be one: a layout this doesn't read, or one it wouldn't
    build again byte for byte."""
    try:
        document, textures = read_particles(data)
        if build_particles(document, textures) != data:
            return "it doesn't build again byte for byte from its emitters and textures"
        write_directory(directory, document, textures)
        if build_directory(directory, data) != data:
            raise ParticleError("its textures don't pack again from their PNGs")
    except (ParticleError, struct.error) as error:
        if directory.exists():
            for path in directory.iterdir():
                path.unlink()
            directory.rmdir()
        return str(error)
    return None


def main():
    try:
        if len(sys.argv) == 4 and sys.argv[1] == "dump":
            problem = export_particles(Path(sys.argv[3]), Path(sys.argv[2]).read_bytes())
            if problem:
                raise SystemExit(f"error: {problem}")
        elif len(sys.argv) in (4, 5) and sys.argv[1] == "pack":
            original = Path(sys.argv[4]).read_bytes() if len(sys.argv) == 5 else None
            Path(sys.argv[3]).write_bytes(build_directory(Path(sys.argv[2]), original))
        else:
            raise SystemExit(__doc__)
    except (ParticleError, ValueError) as error:
        raise SystemExit(f"error: {error}") from None


if __name__ == "__main__":
    main()
