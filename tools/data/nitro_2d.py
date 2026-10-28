"""Small Nitro 2D graphics helpers shared by the graphics packers."""

import struct
import sys
import zlib


def decompress(blob):
    if not blob or blob[0] not in (0x10, 0x11):
        return blob
    kind = blob[0]
    size = int.from_bytes(blob[1:4], "little")
    cursor = 4
    if size == 0:
        size = struct.unpack_from("<I", blob, cursor)[0]
        cursor += 4
    output = bytearray()
    while len(output) < size:
        flags = blob[cursor]
        cursor += 1
        for bit in range(7, -1, -1):
            if len(output) >= size:
                break
            if not flags & (1 << bit):
                output.append(blob[cursor])
                cursor += 1
                continue
            first = blob[cursor]
            second = blob[cursor + 1]
            cursor += 2
            if kind == 0x10:
                length = (first >> 4) + 3
                distance = ((first & 15) << 8 | second) + 1
            elif first >> 4 == 0:
                third = blob[cursor]
                cursor += 1
                length = ((first & 15) << 4 | second >> 4) + 0x11
                distance = ((second & 15) << 8 | third) + 1
            elif first >> 4 == 1:
                third, fourth = blob[cursor:cursor + 2]
                cursor += 2
                length = ((first & 15) << 12 | second << 4 | third >> 4) + 0x111
                distance = ((third & 15) << 8 | fourth) + 1
            else:
                length = (first >> 4) + 1
                distance = ((first & 15) << 8 | second) + 1
            if distance > len(output):
                raise ValueError("invalid LZ back-reference")
            for _ in range(length):
                output.append(output[-distance])
    if len(output) != size:
        raise ValueError("LZ stream exceeds declared size")
    return bytes(output)


def compress_literals(blob, kind):
    """Valid Nitro LZ stream; used only for edited data."""
    if kind not in (0x10, 0x11) or len(blob) >= 1 << 24:
        raise ValueError("unsupported Nitro LZ output")
    output = bytearray([kind]) + len(blob).to_bytes(3, "little")
    for offset in range(0, len(blob), 8):
        output.append(0)
        output.extend(blob[offset:offset + 8])
    return bytes(output)


def resource_data(blob, magic, section):
    raw = decompress(blob)
    if raw[:4] != magic or raw[16:20] != section:
        raise ValueError(f"expected {magic!r}/{section!r} Nitro resource")
    if struct.unpack_from("<I", raw, 8)[0] < len(raw):
        raise ValueError("Nitro resource is larger than its declared size")
    return raw


def palette_colors(blob):
    raw = resource_data(blob, b"RLCN", b"TTLP")
    size = struct.unpack_from("<I", raw, 32)[0]
    if 40 + size > len(raw) or size % 2:
        raise ValueError("invalid Nitro palette data size")
    return [tuple(((value >> shift) & 31) * 255 // 31 for shift in (0, 5, 10))
            for (value,) in struct.iter_unpack("<H", raw[40:40 + size])]


def patch_palette(blob, colors):
    raw = bytearray(resource_data(blob, b"RLCN", b"TTLP"))
    original = palette_colors(blob)
    if len(colors) != len(original):
        raise ValueError("Nitro palette size changed")
    for index, color in enumerate(colors):
        old = struct.unpack_from("<H", raw, 40 + 2 * index)[0]
        value = old & 0x8000
        for axis, shift in enumerate((0, 5, 10)):
            value |= round(color[axis] * 31 / 255) << shift
        struct.pack_into("<H", raw, 40 + 2 * index, value)
    return bytes(raw)


def color5(color):
    """Returns an 8-bit color as the 5-bit one the DS shows."""
    return tuple(round(axis * 31 / 255) for axis in color)


def palette_resource(colors, fmt, count=None):
    """Returns a Nitro palette resource (NCLR) of the colors, padded with black to count, whose header gives fmt, the
    palette's GXTexFmt as the game's file of that kind has it."""
    colors = list(colors) + [(0, 0, 0)] * ((count or len(colors)) - len(colors))
    data = b"".join(struct.pack("<H", r | g << 5 | b << 10) for r, g, b in map(color5, colors))
    section = struct.pack("<4sIIIII", b"TTLP", 0x18 + len(data), fmt, 0, len(data), 0x10) + data
    return struct.pack("<4sHHIHH", b"RLCN", 0xFEFF, 0x0100, 0x10 + len(section), 0x10, 1) + section


def write_palette(colors):
    return "JASC-PAL\n0100\n" + str(len(colors)) + "\n" + "".join(
        f"{r} {g} {b}\n" for r, g, b in colors)


def read_palette(path):
    lines = path.read_text().splitlines()
    if lines[:2] != ["JASC-PAL", "0100"] or int(lines[2]) != len(lines) - 3:
        raise ValueError(f"{path}: invalid JASC palette")
    colors = [tuple(map(int, line.split())) for line in lines[3:]]
    if any(len(color) != 3 or any(not 0 <= axis <= 255 for axis in color) for color in colors):
        raise ValueError(f"{path}: invalid palette color")
    return colors


# The OBJs of a single-cell sprite, in the order its characters store them, as (x, y, width, height) in pixels: a
# 96 by 96 Pokémon sprite and an 80 by 80 trainer sprite, each a square OBJ with the strips right of and below it
SINGLE_CELL_96 = ((0, 0, 64, 64), (64, 0, 32, 64), (0, 64, 64, 32), (64, 64, 32, 32))
SINGLE_CELL_80 = ((0, 0, 64, 64), (64, 0, 16, 64), (0, 64, 64, 16), (64, 64, 16, 16))


def obj_reorder(chars, width, objs, reverse=False):
    """Returns characters stored OBJ by OBJ, each OBJ's tiles in rows, as the tiles of one image in rows, or with
    reverse the other way round."""
    tiles = [chars[i:i + 32] for i in range(0, len(chars), 32)]
    order = [(y // 8) * (width // 8) + x // 8
             for ox, oy, ow, oh in objs for y in range(oy, oy + oh, 8) for x in range(ox, ox + ow, 8)]
    if sorted(order) != list(range(len(tiles))):
        raise ValueError("the OBJs don't cover the sprite's characters")
    output = [b""] * len(tiles)
    for stored, tiled in enumerate(order):
        if reverse:
            output[stored] = tiles[tiled]
        else:
            output[tiled] = tiles[stored]
    return b"".join(output)


def single_cell_reorder(chars, reverse=False):
    if len(chars) != 0x1200:
        raise ValueError("single-cell Pokémon sprite must be 96 by 96")
    return obj_reorder(chars, 96, SINGLE_CELL_96, reverse)


def character_pixels(blob, width=None, height=None, single_cell=False, linear=False, objs=None):
    """Returns a character resource's width, height and pixels, from tiles in rows, OBJs in the order of objs, the
    96 by 96 single-cell layout with single_cell, or rows of pixels with linear."""
    raw = resource_data(blob, b"RGCN", b"RAHC")
    tile_height, tile_width = struct.unpack_from("<HH", raw, 24)
    size = struct.unpack_from("<I", raw, 40)[0]
    if tile_width != 0xffff and tile_height != 0xffff:
        width, height = tile_width * 8, tile_height * 8
    if width is None or height is None or width * height // 2 != size:
        raise ValueError("Nitro character dimensions do not match data size")
    chars = raw[48:48 + size]
    if len(chars) != size or width % 8 or height % 8:
        raise ValueError("invalid Nitro character data")
    if single_cell:
        chars = single_cell_reorder(chars)
    if objs:
        chars = obj_reorder(chars, width, objs)
    if linear:
        return width, height, bytes(value for byte in chars for value in (byte & 15, byte >> 4))
    pixels = bytearray(width * height)
    cursor = 0
    for ty in range(0, height, 8):
        for tx in range(0, width, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    value = chars[cursor]
                    cursor += 1
                    base = (ty + y) * width + tx + x
                    pixels[base] = value & 15
                    pixels[base + 1] = value >> 4
    return width, height, bytes(pixels)


def encode_characters(width, height, pixels, linear=False):
    """Returns 4bpp pixels as character data: in 8 by 8 tiles, or with linear in rows."""
    if len(pixels) != width * height or width % 8 or height % 8:
        raise ValueError("invalid Nitro character dimensions")
    if any(value > 15 for value in pixels):
        raise ValueError("Nitro character index exceeds 4bpp")
    if linear:
        return bytes(left | right << 4 for left, right in zip(pixels[::2], pixels[1::2]))
    chars = bytearray(width * height // 2)
    cursor = 0
    for ty in range(0, height, 8):
        for tx in range(0, width, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    base = (ty + y) * width + tx + x
                    chars[cursor] = pixels[base] | pixels[base + 1] << 4
                    cursor += 1
    return bytes(chars)


def character_resource(width, height, pixels, linear=False, position=None, objs=None, mapping=0, sized=True):
    """Returns a Nitro character resource (NCGR) of 4bpp pixels, tiled, in the OBJs of objs or with linear in rows:
    with position, a (width, height) in tiles, the character position section (SOPC) that some resources add, the OBJ
    VRAM mapping, and without sized the 0xFFFF size that OBJ characters can have."""
    chars = encode_characters(width, height, pixels, linear)
    if objs:
        chars = obj_reorder(chars, width, objs, reverse=True)
    tiles = (height // 8, width // 8) if sized else (0xFFFF, 0xFFFF)
    sections = [struct.pack("<4sIHHIIIII", b"RAHC", 0x20 + len(chars), *tiles, 3, mapping, int(linear), len(chars),
                            0x18) + chars]
    if position:
        sections.append(struct.pack("<4sIIHH", b"SOPC", 0x10, 0, *position))
    body = b"".join(sections)
    return struct.pack("<4sHHIHH", b"RGCN", 0xFEFF, 0x0101, 0x10 + len(body), 0x10, len(sections)) + body


def patch_characters(blob, width, height, pixels, single_cell=False, linear=False):
    raw = bytearray(resource_data(blob, b"RGCN", b"RAHC"))
    old_width, old_height, _ = character_pixels(blob, width, height, single_cell, linear)
    if (width, height) != (old_width, old_height) or len(pixels) != width * height:
        raise ValueError("Nitro character dimensions changed")
    chars = encode_characters(width, height, pixels, linear)
    raw[48:48 + len(chars)] = single_cell_reorder(chars, reverse=True) if single_cell else chars
    return compress_literals(bytes(raw), blob[0]) if blob[0] in (0x10, 0x11) else bytes(raw)


def palette_indices(path, pixels, png_colors, colors):
    """Returns a PNG's pixels as indices of the sprite's palette, whose .pal file sets the colors: its own indices
    where the PNG's palette agrees with the sprite's, as exported, the sprite's index of the same color where an
    editor reordered or compacted the PNG's palette on saving, and its own index where the color is nowhere in the
    sprite's palette, as after the .pal file's colors were changed. Index 0 is transparent and stays 0."""
    target = [color5(color) for color in colors]
    mapping = {0: 0}
    for i in sorted(set(pixels) - {0}):
        if i >= len(png_colors):
            raise ValueError(f"{path}: a pixel's index {i} is outside the PNG's palette")
        color = color5(png_colors[i])
        matches = [j for j, other in enumerate(target) if j and other == color]
        if (i < len(target) and target[i] == color) or not matches:
            if i >= len(target):
                raise ValueError(f"{path}: a pixel's index {i} is outside the sprite's {len(target)} colors")
            if target[i] != color:
                print(f"{path}: the PNG's color {png_colors[i]} at index {i} is not in the sprite's palette; the "
                      ".pal file's color is used, so recolor a sprite in its .pal file", file=sys.stderr)
            mapping[i] = i
            continue
        mapping[i] = matches[0]
    if all(i == j for i, j in mapping.items()):
        return pixels
    return bytes(mapping[i] for i in pixels)


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def write_indexed_png(width, height, pixels, colors):
    if len(pixels) != width * height or len(colors) > 256:
        raise ValueError("invalid indexed image")
    raw = b"".join(b"\0" + pixels[y * width:(y + 1) * width] for y in range(height))
    palette = bytes(axis for color in colors for axis in color)
    return (b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0))
            + png_chunk(b"PLTE", palette) + png_chunk(b"tRNS", b"\0")
            + png_chunk(b"IDAT", zlib.compress(raw, 9)) + png_chunk(b"IEND", b""))


def read_indexed_png(path):
    """Returns an indexed PNG's width, height, pixels (palette indices) and its palette's colors."""
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError(f"{path}: expected PNG")
    cursor, image = 8, bytearray()
    width = height = depth = None
    colors = []
    while cursor < len(data):
        size = struct.unpack_from(">I", data, cursor)[0]
        kind = data[cursor + 4:cursor + 8]
        payload = data[cursor + 8:cursor + 8 + size]
        if zlib.crc32(kind + payload) != struct.unpack_from(">I", data, cursor + 8 + size)[0]:
            raise ValueError(f"{path}: PNG CRC mismatch")
        cursor += 12 + size
        if kind == b"IHDR":
            width, height, depth, color, comp, filt, interlace = struct.unpack(">IIBBBBB", payload)
            if color != 3 or depth not in (4, 8) or comp or filt or interlace:
                raise ValueError(f"{path}: expected noninterlaced indexed PNG")
        elif kind == b"PLTE":
            colors = [tuple(payload[i:i + 3]) for i in range(0, len(payload) - 2, 3)]
        elif kind == b"IDAT":
            image.extend(payload)
        elif kind == b"IEND":
            break
    raw = zlib.decompress(image)
    stride = (width * depth + 7) // 8
    pixels = bytearray()
    previous = bytearray(stride)
    cursor = 0
    for _ in range(height):
        filter_type = raw[cursor]
        row = bytearray(raw[cursor + 1:cursor + 1 + stride])
        cursor += 1 + stride
        for x in range(stride):
            left = row[x - 1] if x else 0
            above = previous[x]
            upper_left = previous[x - 1] if x else 0
            if filter_type == 1:
                row[x] = (row[x] + left) & 255
            elif filter_type == 2:
                row[x] = (row[x] + above) & 255
            elif filter_type == 3:
                row[x] = (row[x] + (left + above) // 2) & 255
            elif filter_type == 4:
                prediction = left + above - upper_left
                distances = (abs(prediction - left), abs(prediction - above), abs(prediction - upper_left))
                predictor = (left, above, upper_left)[distances.index(min(distances))]
                row[x] = (row[x] + predictor) & 255
            elif filter_type != 0:
                raise ValueError(f"{path}: unsupported PNG filter")
        if depth == 8:
            pixels.extend(row[:width])
        else:
            unpacked = bytes(value for byte in row for value in (byte >> 4, byte & 15))
            pixels.extend(unpacked[:width])
        previous = row
    return width, height, bytes(pixels), colors


def unfilter_png(raw, width, height, pixel_bytes, label):
    """Returns a non-interlaced PNG's rows without their filters, as one run of bytes."""
    stride = width * pixel_bytes
    out = bytearray()
    previous = bytearray(stride)
    cursor = 0
    for _ in range(height):
        filter_type = raw[cursor]
        row = bytearray(raw[cursor + 1:cursor + 1 + stride])
        cursor += 1 + stride
        for x in range(stride):
            left = row[x - pixel_bytes] if x >= pixel_bytes else 0
            above = previous[x]
            upper_left = previous[x - pixel_bytes] if x >= pixel_bytes else 0
            if filter_type == 1:
                row[x] = (row[x] + left) & 255
            elif filter_type == 2:
                row[x] = (row[x] + above) & 255
            elif filter_type == 3:
                row[x] = (row[x] + (left + above) // 2) & 255
            elif filter_type == 4:
                prediction = left + above - upper_left
                distances = (abs(prediction - left), abs(prediction - above), abs(prediction - upper_left))
                row[x] = (row[x] + (left, above, upper_left)[distances.index(min(distances))]) & 255
            elif filter_type != 0:
                raise ValueError(f"{label}: unsupported PNG filter")
        out.extend(row)
        previous = row
    return bytes(out)


def write_rgba_png(width, height, pixels):
    if len(pixels) != width * height * 4:
        raise ValueError("invalid RGBA image")
    raw = b"".join(b"\0" + pixels[y * width * 4:(y + 1) * width * 4] for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + png_chunk(b"IDAT", zlib.compress(raw, 9)) + png_chunk(b"IEND", b""))


def read_rgba_png(path):
    """Returns an 8-bit RGB or RGBA PNG's width, height and its pixels as RGBA."""
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError(f"{path}: expected PNG")
    cursor, image = 8, bytearray()
    width = height = color = None
    while cursor < len(data):
        size = struct.unpack_from(">I", data, cursor)[0]
        kind = data[cursor + 4:cursor + 8]
        payload = data[cursor + 8:cursor + 8 + size]
        if zlib.crc32(kind + payload) != struct.unpack_from(">I", data, cursor + 8 + size)[0]:
            raise ValueError(f"{path}: PNG CRC mismatch")
        cursor += 12 + size
        if kind == b"IHDR":
            width, height, depth, color, comp, filt, interlace = struct.unpack(">IIBBBBB", payload)
            if color not in (2, 6) or depth != 8 or comp or filt or interlace:
                raise ValueError(f"{path}: expected a noninterlaced 8-bit RGB or RGBA PNG")
        elif kind == b"IDAT":
            image.extend(payload)
        elif kind == b"IEND":
            break
    rows = unfilter_png(zlib.decompress(image), width, height, 4 if color == 6 else 3, path)
    if color == 2:
        rows = b"".join(rows[i:i + 3] + b"\xff" for i in range(0, len(rows), 3))
    return width, height, rows


def bitmap_png(data, width, height):
    """A direct color bitmap (RGB555, bit 15 set where the pixel shows) as an RGBA PNG: each channel's five bits
    widened to eight, and alpha 255 where the pixel shows and 0 where it doesn't, keeping its color."""
    if len(data) != width * height * 2:
        raise ValueError(f"a {width}x{height} direct color bitmap is {width * height * 2} bytes, not {len(data)}")
    pixels = bytearray()
    for (value,) in struct.iter_unpack("<H", data):
        for shift in (0, 5, 10):
            channel = value >> shift & 31
            pixels.append(channel << 3 | channel >> 2)
        pixels.append(255 if value & 0x8000 else 0)
    return write_rgba_png(width, height, bytes(pixels))


def build_bitmap(path, width, height):
    """An RGBA PNG as a direct color bitmap: each channel's top five bits, and bit 15 where alpha is at least 128."""
    png_width, png_height, pixels = read_rgba_png(path)
    if (png_width, png_height) != (width, height):
        raise ValueError(f"{path}: a frame is {width}x{height}, not {png_width}x{png_height}")
    return b"".join(struct.pack("<H", pixels[i] >> 3 | (pixels[i + 1] >> 3) << 5 | (pixels[i + 2] >> 3) << 10
                                | (0x8000 if pixels[i + 3] >= 128 else 0)) for i in range(0, len(pixels), 4))
