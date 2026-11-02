#!/usr/bin/env python3
"""The areas' texture pattern animations in res/field/maps/pattern_animations/: dump writes them from the ROM once,
and pack builds the archive from them, which the build does.

    map_pattern_animation_data.py dump extract/b2_us/files res/field/maps/pattern_animations
    map_pattern_animation_data.py pack res/field/maps/pattern_animations AREA_ANIME_PAT

ARCID_AREA_ANIME_PAT (a/0/6/9) has a pack of animations for each area's pattern_animation (res/field/areas/), which
FieldTerrainAnimator_LoadTexPat loads (field_ground_anime.c): its count of animations, an offset to each one's
Game Freak texture pattern animation (GFBTP, GFBTPController_LoadFile) and to its texture set (BTX0), and the pack's
size, then the animations' files. Each texture set holds the frames of the animation's targets, which upload over the
textures of the area's texture set in VRAM as the animation plays.

Each pack is a directory named after its number, <n>/, with animations.json (pattern_animation.schema.json): each
animation's length in frames, its textures and palettes as the texture set lists them, and the key frames of each
target, which name the textures. Each texture is an indexed PNG named after it, whose pixels are the indices of its
palette, a JASC palette named after the texture set's palette, whose colors are the ones the game shows; the PNG's own
colors are only a preview. The texture sets are written again from these, byte for byte, as the game's are. Both
versions have the same archive.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.nitro_2d import (color5, palette_indices, read_indexed_png, read_palette,  # noqa: E402
                                 write_indexed_png, write_palette)
from tools.data.nitro_btx import TEX0_HEADER, texture_set  # noqa: E402

ARCHIVE = "a/0/6/9"
ANIMATION_MAX = 16  # FIELD_TERRAIN_TEXPAT_ANIMATION_MAX
# The texture formats (GXTexFmt) the animations have: their colors and bits per texel
FORMATS = {"palette4": (2, 4, 2), "palette16": (3, 16, 4)}
COLOR0 = 1 << 29  # TEXIMAGE_PARAM's color 0 transparent
PLTT4_FLAG = 0x8000  # the texture set's palette info: it has 4-color palettes
PLTT4_RECORD = 1  # a palette's dictionary record: it is a 4-color palette


def read_dictionary(blob: bytes, offset: int) -> list[tuple[str, int]]:
    """Returns a Nitro dictionary's names and the offsets of their records, in its order."""
    count = blob[offset + 1]
    datum = offset + 12 + 4 * count
    size, = struct.unpack_from("<H", blob, datum)
    names = datum + 4 + size * count
    return [(blob[names + 16 * i:names + 16 * (i + 1)].split(b"\0")[0].decode("ascii"), datum + 4 + size * i)
            for i in range(count)]


def align4(size: int) -> int:
    return (size + 3) & ~3


def padded(data: bytes) -> bytes:
    return data + bytes(align4(len(data)) - len(data))


# GFBTP: the key frames' count and frames, each one's texture and palette, the targets' first key frames and the
# animation's length, each array padded to four bytes
def read_gfbtp(data: bytes, offset: int) -> tuple[list, list, list, list, int, int]:
    count, = struct.unpack_from("<I", data, offset)
    cursor = offset + 4
    frames = list(struct.unpack_from(f"<{count}H", data, cursor))
    cursor += align4(2 * count)
    textures = list(data[cursor:cursor + count])
    cursor += align4(count)
    palettes = list(data[cursor:cursor + count])
    cursor += align4(count)
    target_count, = struct.unpack_from("<I", data, cursor)
    targets = list(data[cursor + 4:cursor + 4 + target_count])
    cursor += 4 + align4(target_count)
    length, = struct.unpack_from("<I", data, cursor)
    return frames, textures, palettes, targets, length, cursor + 4


def gfbtp_bytes(frames: list, textures: list, palettes: list, targets: list, length: int) -> bytes:
    return (struct.pack("<I", len(frames)) + padded(struct.pack(f"<{len(frames)}H", *frames)) + padded(bytes(textures))
            + padded(bytes(palettes)) + struct.pack("<I", len(targets)) + padded(bytes(targets))
            + struct.pack("<I", length))


def unpack_texels(data: bytes, bits: int) -> bytes:
    mask = (1 << bits) - 1
    return bytes(byte >> shift & mask for byte in data for shift in range(0, 8, bits))


def pack_texels(pixels: bytes, bits: int) -> bytes:
    per = 8 // bits
    return bytes(sum(pixels[i + k] << (k * bits) for k in range(per)) for i in range(0, len(pixels), per))


def read_texture_set(blob: bytes) -> tuple[list, list]:
    """Returns a texture set's textures, (name, width, height, format, color0, texels), and its palettes, (names,
    colors): the names of each palette's data in the order of the dictionary, which runs over each data's in turn."""
    tex0 = struct.unpack_from("<I", blob, 16)[0]
    texel_start = tex0 + struct.unpack_from("<I", blob, tex0 + 20)[0]
    palette_size = struct.unpack_from("<H", blob, tex0 + 48)[0] << 3
    palette_start = tex0 + struct.unpack_from("<I", blob, tex0 + 56)[0]
    textures = []
    for name, record in read_dictionary(blob, tex0 + TEX0_HEADER):
        params, = struct.unpack_from("<I", blob, record)
        fmt = next(key for key, (value, *_) in FORMATS.items() if value == params >> 26 & 7)
        width, height = 8 << (params >> 20 & 7), 8 << (params >> 23 & 7)
        start = texel_start + ((params & 0xFFFF) << 3)
        size = width * height * FORMATS[fmt][2] // 8
        textures.append((name, width, height, fmt, bool(params & COLOR0),
                         unpack_texels(blob[start:start + size], FORMATS[fmt][2])))
    entries = read_dictionary(blob, tex0 + struct.unpack_from("<I", blob, tex0 + 52)[0])
    offsets = [struct.unpack_from("<H", blob, record)[0] << 3 for _, record in entries]
    palettes = []
    for (name, _), offset in zip(entries, offsets):
        if palettes and palettes[-1][2] == offset:
            palettes[-1][0].append(name)
        else:
            palettes.append(([name], None, offset))
    if [offset for *_, offset in palettes] != sorted({*offsets}):
        raise ValueError("a texture set's palettes aren't in the order of their data")
    ends = [offset for *_, offset in palettes[1:]] + [palette_size]
    return textures, [(names, [tuple(value >> shift & 31 for shift in (0, 5, 10)) for (value,) in
                               struct.iter_unpack("<H", blob[palette_start + offset:palette_start + end])])
                      for (names, _, offset), end in zip(palettes, ends)]


def texture_set_bytes(textures: list, palettes: list) -> bytes:
    """Returns the texture set (BTX0) of textures and palettes as read_texture_set gives them, as the game's
    converter writes it: palettes that share data named in turn, and the flags of 4-color palettes."""
    pltt4 = {name for names, _ in palettes for name in names
             if any(t[3] == "palette4" and t[0] + "_pl" == name for t in textures)}
    entries = [(name, b"" if i < len(names) - 1 else b"".join(struct.pack("<H", r | g << 5 | b << 10)
                                                                for r, g, b in colors))
               for names, colors in palettes for i, name in enumerate(names)]
    blob = bytearray(texture_set([(name, width, height, FORMATS[fmt][0], color0, pack_texels(pixels, FORMATS[fmt][2]))
                                  for name, width, height, fmt, color0, pixels in textures], entries))
    if pltt4:
        tex0 = struct.unpack_from("<I", blob, 16)[0]
        struct.pack_into("<H", blob, tex0 + 50, PLTT4_FLAG)
        dictionary = tex0 + struct.unpack_from("<I", blob, tex0 + 52)[0]
        for name, record in read_dictionary(blob, dictionary):
            if name in pltt4:
                struct.pack_into("<H", blob, record + 2, PLTT4_RECORD)
    return bytes(blob)


def preview(colors: list) -> list:
    return [tuple(axis * 255 // 31 for axis in color) for color in colors]


def animation_json(data: bytes, gfbtp: int, btx: int, directory: Path) -> dict:
    frames, texture_keys, palette_keys, targets, length, end = read_gfbtp(data, gfbtp)
    if end != btx:
        raise ValueError("an animation's texture set doesn't follow its GFBTP")
    if not targets or targets[0] != 0 or targets != sorted(targets) or targets[-1] >= len(frames):
        raise ValueError("an animation's targets aren't in the order of their key frames")
    size, = struct.unpack_from("<I", data, btx + 8)
    textures, palettes = read_texture_set(data[btx:btx + size])
    palette_names = [name for names, _ in palettes for name in names]
    for name, width, height, fmt, color0, pixels in textures:
        colors = next(colors for names, colors in palettes if name + "_pl" in names)
        (directory / f"{name}.png").write_bytes(write_indexed_png(width, height, pixels, preview(colors)))
    for names, colors in palettes:
        (directory / f"{names[0]}.pal").write_text(write_palette(preview(colors)))
    keys = []
    for start, stop in zip(targets, targets[1:] + [len(frames)]):
        target = []
        for frame, texture, palette in zip(frames[start:stop], texture_keys[start:stop], palette_keys[start:stop]):
            key = {"frame": frame, "texture": textures[texture][0]}
            if palette != texture:
                key["palette"] = palette_names[palette]
            target.append(key)
        keys.append(target)
    return {"length": length,
            "textures": [{"name": name, "format": fmt, "color0_transparent": color0}
                         for name, _, _, fmt, color0, _ in textures],
            "palettes": [names for names, _ in palettes],
            "targets": keys}


def animation_bytes(entry: dict, directory: Path, where: str) -> bytes:
    textures = []
    for texture in entry["textures"]:
        name = texture["name"]
        path = directory / f"{name}.png"
        palette = next((names[0] for names in entry["palettes"] if name + "_pl" in names), None)
        if palette is None:
            raise DataError(f"{where}: texture {name} has no palette {name}_pl")
        colors = read_palette(directory / f"{palette}.pal")
        if not path.is_file():
            raise DataError(f"{where}: {label(path)} is missing")
        width, height, pixels, png_colors = read_indexed_png(path)
        pixels = palette_indices(path, pixels, png_colors, colors)
        _, capacity, _ = FORMATS[texture["format"]]
        if width not in (8, 16, 32, 64, 128, 256, 512, 1024) or height not in (8, 16, 32, 64, 128, 256, 512, 1024):
            raise DataError(f"{label(path)}: a texture is 8 to 1024 pixels wide and high, a power of two")
        if max(pixels) >= min(capacity, len(colors)):
            raise DataError(f"{label(path)}: a pixel's index is outside its {min(capacity, len(colors))} colors")
        textures.append((name, width, height, texture["format"], texture["color0_transparent"], pixels))
    palettes = []
    for names in entry["palettes"]:
        path = directory / f"{names[0]}.pal"
        if not path.is_file():
            raise DataError(f"{where}: {label(path)} is missing")
        palettes.append((names, [color5(color) for color in read_palette(path)]))
    texture_names = [texture["name"] for texture in entry["textures"]]
    palette_names = [name for names in entry["palettes"] for name in names]
    frames, texture_keys, palette_keys, targets = [], [], [], []
    for t, target in enumerate(entry["targets"]):
        targets.append(len(frames))
        for k, key in enumerate(target):
            if key["texture"] not in texture_names:
                raise DataError(f"{where}: target {t} key {k}: {key['texture']} is not one of the textures")
            index = texture_names.index(key["texture"])
            palette = key.get("palette", palette_names[index] if index < len(palette_names) else None)
            if palette not in palette_names:
                raise DataError(f"{where}: target {t} key {k}: {palette} is not one of the palettes")
            frames.append(key["frame"])
            texture_keys.append(index)
            palette_keys.append(palette_names.index(palette))
    return gfbtp_bytes(frames, texture_keys, palette_keys, targets, entry["length"]) + texture_set_bytes(textures,
                                                                                                         palettes)


def pack_bytes(entries: list[bytes]) -> bytes:
    """Returns a pack: its count of animations, each one's offsets of its GFBTP and texture set, and the pack's size,
    then the animations."""
    offset = 4 + 8 * len(entries) + 4
    table, body = struct.pack("<I", len(entries)), b""
    for data in entries:
        _, _, _, _, _, end = read_gfbtp(data, 0)
        table += struct.pack("<II", offset + len(body), offset + len(body) + end)
        body += data
    return table + struct.pack("<I", offset + len(body)) + body


def dump(files: Path, output: Path):
    packs = read_narc((files / ARCHIVE).read_bytes())
    for n, data in enumerate(packs):
        directory = output / str(n)
        directory.mkdir(parents=True, exist_ok=True)
        count, = struct.unpack_from("<I", data)
        animations = [animation_json(data, *struct.unpack_from("<II", data, 4 + 8 * i), directory)
                      for i in range(count)]
        write(directory / "animations.json", {"$schema": "../pattern_animation.schema.json", "animations": animations})
        rebuilt = pack_bytes([animation_bytes(entry, directory, str(directory)) for entry in animations])
        if rebuilt != data:
            raise ValueError(f"pack {n} doesn't build again as the game's")
    print(f"wrote {len(packs)} pattern animation packs to {output}")


def pack(root: Path, output: Path):
    schema = load_schema(root / "pattern_animation.schema.json")
    directories = sorted((path for path in root.iterdir() if path.is_dir() and path.name.isdigit()),
                         key=lambda path: int(path.name))
    if [int(path.name) for path in directories] != list(range(len(directories))):
        raise DataError(f"{label(root)}: the packs are numbered from 0 without gaps")
    members = []
    for directory in directories:
        path = directory / "animations.json"
        animations = load(path, schema)["animations"]
        if len(animations) > ANIMATION_MAX:
            raise DataError(f"{label(path)}: a pack has at most {ANIMATION_MAX} animations")
        members.append(pack_bytes([animation_bytes(entry, directory, f"{label(path)}: animation {i}")
                                   for i, entry in enumerate(animations)]))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/maps/pattern_animations/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the pattern animations' archive")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.output)
    except (DataError, ValueError) as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
