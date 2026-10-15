"""Small Nitro 2D graphics helpers shared by Pokémon asset packers."""

import struct
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


def single_cell_reorder(chars, reverse=False):
    if len(chars) != 0x1200:
        raise ValueError("single-cell Pokémon sprite must be 96 by 96")
    output = bytearray(len(chars))
    # Four OBJ blocks (64x64, 32x64, 64x32, 32x32) become one 96x96 tiled image.
    spans = ([(i * 0x100, i * 0x180, 0x100) for i in range(8)]
             + [(0x800 + i * 0x80, i * 0x180 + 0x100, 0x80) for i in range(8)]
             + [(0xc00 + i * 0x100, 0xc00 + i * 0x180, 0x100) for i in range(4)]
             + [(0x1000 + i * 0x80, 0xc00 + i * 0x180 + 0x100, 0x80) for i in range(4)])
    for source, target, size in spans:
        if reverse:
            output[source:source + size] = chars[target:target + size]
        else:
            output[target:target + size] = chars[source:source + size]
    return bytes(output)


def character_pixels(blob, width=None, height=None, single_cell=False, linear=False):
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


def patch_characters(blob, width, height, pixels, single_cell=False, linear=False):
    raw = bytearray(resource_data(blob, b"RGCN", b"RAHC"))
    old_width, old_height, _ = character_pixels(blob, width, height, single_cell, linear)
    if (width, height) != (old_width, old_height) or len(pixels) != width * height:
        raise ValueError("Nitro character dimensions changed")
    chars = bytearray(width * height // 2)
    cursor = 0
    if linear:
        for left, right in zip(pixels[::2], pixels[1::2]):
            if left > 15 or right > 15:
                raise ValueError("Nitro character index exceeds 4bpp")
            chars[cursor] = left | right << 4
            cursor += 1
    else:
        for ty in range(0, height, 8):
            for tx in range(0, width, 8):
                for y in range(8):
                    for x in range(0, 8, 2):
                        base = (ty + y) * width + tx + x
                        left, right = pixels[base:base + 2]
                        if left > 15 or right > 15:
                            raise ValueError("Nitro character index exceeds 4bpp")
                        chars[cursor] = left | right << 4
                        cursor += 1
    raw[48:48 + len(chars)] = single_cell_reorder(chars, reverse=True) if single_cell else chars
    return compress_literals(bytes(raw), blob[0]) if blob[0] in (0x10, 0x11) else bytes(raw)


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
