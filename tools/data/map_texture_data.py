#!/usr/bin/env python3
"""Export field map NSBTX textures as PNG and pack edited pixels into Nitro."""

import json
import hashlib
import struct
import sys
import zlib
from pathlib import Path

from narc import read_narc, write_narc


ROOT = Path(__file__).resolve().parents[2]


def number(data, offset, fmt="I"):
    return struct.unpack_from("<" + fmt, data, offset)[0]


def dictionary(data, offset, datum_size):
    if data[offset] != 0:
        raise ValueError("invalid Nitro dictionary")
    count = data[offset + 1]
    datum = offset + 12 + 4 * count
    if number(data, datum, "H") != datum_size:
        raise ValueError("unexpected Nitro dictionary datum size")
    names = datum + 4 + datum_size * count
    return [(data[names + 16 * i:names + 16 * (i + 1)].split(b"\0")[0].decode("ascii"),
             datum + 4 + datum_size * i) for i in range(count)]


def resources(blob):
    if blob[:4] != b"BTX0":
        raise ValueError("expected an NSBTX texture set")
    base = number(blob, 16)
    if blob[base:base + 4] != b"TEX0":
        raise ValueError("missing TEX0 block")
    texture_data = base + number(blob, base + 20)
    compressed_data = base + number(blob, base + 36)
    compressed_info = base + number(blob, base + 40)
    palette_data = base + number(blob, base + 56)
    textures = []
    for name, entry in dictionary(blob, base + number(blob, base + 14, "H"), 8):
        params = number(blob, entry)
        width = 8 << ((params >> 20) & 7)
        height = 8 << ((params >> 23) & 7)
        kind = (params >> 26) & 7
        bits = {1: 8, 2: 2, 3: 4, 4: 8, 5: 2, 6: 8, 7: 16}[kind]
        offset = (params & 0xffff) << 3
        size = width * height * bits // 8
        texels = compressed_data if kind == 5 else texture_data
        textures.append({"name": name, "width": width, "height": height,
                         "kind": kind, "params": params,
                         "offset": texels + offset, "length": size,
                         "info_offset": compressed_info + offset // 2 if kind == 5 else None})
    palettes = []
    for name, entry in dictionary(blob, base + number(blob, base + 52), 4):
        palettes.append({"name": name, "offset": palette_data + (number(blob, entry, "H") << 3)})
    return textures, palettes


def palette_for(texture, palettes, index):
    by_name = {item["name"]: item for item in palettes}
    name = texture["name"]
    exceptions = {
        "h04_stair": "stair", "tg_stair": "stair", "warp01": "gtswarp_r",
        "c3_bayblock3": "bayblock", "c3test_block3_b": "c34",
        "c3test_block4_b": "block4", "c3test_block6_b": "c3_block6",
        "c3test_block_b": "c3test_block_pl", "pc_kabe1": "pckabe1",
        "pc_logo": "pclogo", "pc_win1": "pcwin", "m_fh01_07f": "m_fs01_07f",
        "clsm01": "m_pc17", "clsm02": "m_pc21", "clsm03": "m_pc16",
        "clsm04": "m_pc15",
    }
    return (by_name.get(name + "_pl") or by_name.get(name)
            or by_name.get(exceptions.get(name)) or palettes[index])


def rgba555(color, alpha=31):
    channels = [((color >> shift) & 31) for shift in (0, 5, 10)] + [alpha]
    return tuple((value << 3) | (value >> 2) for value in channels)


def decode(blob, texture, palette):
    kind = texture["kind"]
    pixels = []
    start = texture["offset"]
    raw = blob[start:start + texture["length"]]
    color0 = bool(texture["params"] & (1 << 29))
    palette_colors = [number(blob, palette["offset"] + 2 * i, "H")
                      if palette["offset"] + 2 * i + 2 <= len(blob) else 0
                      for i in range({1: 32, 2: 4, 3: 16, 4: 256, 6: 8}[kind])]
    count = texture["width"] * texture["height"]
    for pixel in range(count):
        if kind in (1, 4, 6):
            value = raw[pixel]
        elif kind == 2:
            value = (raw[pixel // 4] >> (2 * (pixel % 4))) & 3
        elif kind == 3:
            value = (raw[pixel // 2] >> (4 * (pixel % 2))) & 15
        else:
            raise ValueError(f"unsupported Nitro texture format {kind}")
        if kind == 1:
            index = value & 31
            alpha3 = value >> 5
            alpha = (alpha3 << 2) | (alpha3 >> 1)
        elif kind == 6:
            index = value & 7
            alpha = value >> 3
        else:
            index = value
            alpha = 0 if index == 0 and color0 else 31
        pixels.extend(rgba555(palette_colors[index], alpha))
    return bytes(pixels)


PNG_MAGIC = b"\x89PNG\r\n\x1a\n"


def png_chunk(kind, payload):
    body = kind + payload
    return struct.pack(">I", len(payload)) + body + struct.pack(">I", zlib.crc32(body))


def write_png(width, height, pixels):
    stride = width * 4
    scanlines = b"".join(b"\0" + pixels[y * stride:(y + 1) * stride] for y in range(height))
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return (PNG_MAGIC + png_chunk(b"IHDR", header)
            + png_chunk(b"IDAT", zlib.compress(scanlines, 9)) + png_chunk(b"IEND", b""))


def read_png(path, width, height):
    data = path.read_bytes()
    if not data.startswith(PNG_MAGIC):
        raise ValueError(f"{path}: expected a PNG")
    offset = 8
    compressed = bytearray()
    dimensions = None
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        if zlib.crc32(kind + payload) != struct.unpack_from(">I", data, offset + 8 + length)[0]:
            raise ValueError(f"{path}: invalid PNG checksum")
        offset += 12 + length
        if kind == b"IHDR":
            dimensions = struct.unpack(">IIBBBBB", payload)
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break
    if dimensions not in ((width, height, 8, 2, 0, 0, 0), (width, height, 8, 6, 0, 0, 0)):
        raise ValueError(f"{path}: expected {width}x{height} RGB or RGBA PNG")
    channels = 3 if dimensions[3] == 2 else 4
    scanlines = zlib.decompress(compressed)
    stride = width * channels
    pixels = bytearray()
    previous = bytearray(stride)
    cursor = 0
    for _ in range(height):
        mode = scanlines[cursor]
        line = bytearray(scanlines[cursor + 1:cursor + 1 + stride])
        cursor += stride + 1
        if len(line) != stride:
            raise ValueError(f"{path}: truncated PNG")
        for i in range(stride):
            left = line[i - channels] if i >= channels else 0
            above = previous[i]
            upper_left = previous[i - channels] if i >= channels else 0
            if mode == 1:
                predictor = left
            elif mode == 2:
                predictor = above
            elif mode == 3:
                predictor = (left + above) // 2
            elif mode == 4:
                predictor = min((left, above, upper_left),
                                key=lambda value: abs(left + above - upper_left - value))
            elif mode == 0:
                predictor = 0
            else:
                raise ValueError(f"{path}: unsupported PNG filter {mode}")
            line[i] = (line[i] + predictor) & 255
        if channels == 3:
            pixels.extend(value for i in range(0, stride, 3)
                          for value in (*line[i:i + 3], 255))
        else:
            pixels.extend(line)
        previous = line
    return bytes(pixels)


def encode_pixels(source, kind):
    if kind in (1, 6):
        index_bits, alpha_bits = (5, 3) if kind == 1 else (3, 5)
        if any(index >= 1 << index_bits or alpha >= 1 << alpha_bits
               for index, alpha in zip(source[::2], source[1::2])):
            raise ValueError("texture index or alpha exceeds its Nitro format")
        return bytes(index | (alpha << index_bits)
                     for index, alpha in zip(source[::2], source[1::2]))
    limit = {2: 4, 3: 16, 4: 256}[kind]
    if any(index >= limit for index in source):
        raise ValueError("texture index exceeds its Nitro format")
    if kind == 2:
        return bytes(sum(source[i + j] << (2 * j) for j in range(4))
                     for i in range(0, len(source), 4))
    if kind == 3:
        return bytes(source[i] | source[i + 1] << 4 for i in range(0, len(source), 2))
    return source


def palette_segments(blob, palettes):
    base = number(blob, 16)
    end = base + number(blob, base + 56) + (number(blob, base + 48, "H") << 3)
    offsets = sorted({item["offset"] for item in palettes})
    if not offsets or end > len(blob) or offsets[0] < 0:
        raise ValueError("invalid Nitro palette block")
    return [(offset, following, [item["name"] for item in palettes if item["offset"] == offset])
            for offset, following in zip(offsets, offsets[1:] + [end])]


def export(directory):
    archive = read_narc((ROOT / "extract/b2_us/files/a/0/1/4").read_bytes())
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "manifest.json").write_text(json.dumps({"archive": "a/0/1/4", "sets": len(archive)}, indent=2) + "\n")
    for set_id, blob in enumerate(archive):
        textures, palettes = resources(blob)
        folder = directory / f"{set_id:03d}"
        folder.mkdir(parents=True, exist_ok=True)
        mapping = {"nitro_sha256": hashlib.sha256(blob).hexdigest(), "textures": []}
        for index, texture in enumerate(textures):
            palette = palette_for(texture, palettes, index)
            pixels = decode(blob, texture, palette)
            (folder / f"{texture['name']}.png").write_bytes(
                write_png(texture["width"], texture["height"], pixels))
            mapping["textures"].append({"name": texture["name"], "kind": texture["kind"],
                                        "palette_offset": palette["offset"]})
        (folder / "nitro.json").write_text(json.dumps(mapping, indent=2) + "\n")


def quantized_pixels(pixels, texture):
    kind = texture["kind"]
    color0 = bool(texture["params"] & (1 << 29))
    result = []
    for offset in range(0, len(pixels), 4):
        r, g, b, alpha = pixels[offset:offset + 4]
        color = sum(round(channel * 31 / 255) << shift
                    for channel, shift in zip((r, g, b), (0, 5, 10)))
        if kind in (1, 6):
            bits = 3 if kind == 1 else 5
            result.append((color, round(alpha * ((1 << bits) - 1) / 255)))
        elif color0 and alpha < 128:
            result.append((None, 0))
        else:
            if alpha < 128 and not color0:
                raise ValueError(f"{texture['name']}: this Nitro texture cannot contain transparency")
            result.append((color, 0))
    return result


def encode_palette_group(blob, members, start, end):
    """Assign colors across all images that share one Nitro palette."""
    capacity = (end - start) // 2
    original = [number(blob, start + 2 * i, "H") & 0x7fff for i in range(capacity)]
    requests = []
    for texture, pixels in members:
        values = quantized_pixels(pixels, texture)
        limit = min(capacity, {1: 32, 2: 4, 3: 16, 4: 256, 6: 8}[texture["kind"]])
        requests.append((texture, values, limit))
    transparent = any(value[0] is None for _, values, _ in requests for value in values)
    by_color = {}
    colors = {}
    for _, values, limit in requests:
        for color, _ in values:
            if color is not None:
                colors[color] = min(limit, colors.get(color, limit))
    used = {0} if transparent else set()
    for color, limit in sorted(colors.items(), key=lambda item: (item[1], item[0])):
        choices = [index for index in range(limit) if index not in used]
        if not choices:
            raise ValueError("edited textures exceed their shared Nitro palette capacity")
        preferred = next((index for index in choices if original[index] == color), choices[0])
        by_color[color] = preferred
        used.add(preferred)
    for color, index in by_color.items():
        old = number(blob, start + 2 * index, "H")
        struct.pack_into("<H", blob, start + 2 * index, (old & 0x8000) | color)
    for texture, values, _ in requests:
        kind = texture["kind"]
        if kind in (1, 6):
            source = bytes(value for color, alpha in values
                           for value in (by_color[color], alpha))
        else:
            source = bytes(0 if color is None else by_color[color] for color, _ in values)
        encoded = encode_pixels(source, kind)
        yield texture, encoded


def pack(directory, game, output):
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
    archive_path = ROOT / "extract" / version / "files/a/0/1/4"
    archive = read_narc(archive_path.read_bytes())
    manifest = json.loads((directory / "manifest.json").read_text())
    if manifest != {"archive": "a/0/1/4", "sets": len(archive)}:
        raise ValueError("map texture set manifest differs from the Nitro archive")
    result = []
    for set_id, original in enumerate(archive):
        blob = bytearray(original)
        textures, palettes = resources(blob)
        folder = directory / f"{set_id:03d}"
        mapping = json.loads((folder / "nitro.json").read_text())
        if mapping["nitro_sha256"] != hashlib.sha256(original).hexdigest():
            raise ValueError(f"texture set {set_id}: original Nitro data changed")
        expected = [{"name": item["name"], "kind": item["kind"],
                     "palette_offset": palette_for(item, palettes, i)["offset"]}
                    for i, item in enumerate(textures)]
        if mapping["textures"] != expected:
            raise ValueError(f"texture set {set_id}: texture order or palette mapping changed")
        pixels = []
        changed = set()
        for index, texture in enumerate(textures):
            path = folder / f"{texture['name']}.png"
            image = read_png(path, texture["width"], texture["height"])
            pixels.append(image)
            if image != decode(original, texture, palette_for(texture, palettes, index)):
                changed.add(index)
        if changed:
            segments = {start: end for start, end, _ in palette_segments(blob, palettes)}
            groups = {}
            for index, texture in enumerate(textures):
                palette = palette_for(texture, palettes, index)
                groups.setdefault(palette["offset"], []).append(index)
            writes = {}
            for start, indices in groups.items():
                if not changed.intersection(indices):
                    continue
                members = [(textures[index], pixels[index]) for index in indices]
                for texture, encoded in encode_palette_group(blob, members, start, segments[start]):
                    key = (texture["offset"], texture["length"])
                    if key in writes and writes[key] != encoded:
                        raise ValueError(f"texture set {set_id}: shared texel data needs conflicting edits")
                    writes[key] = encoded
            for (start, length), encoded in writes.items():
                if len(encoded) != length:
                    raise ValueError(f"texture set {set_id}: texture byte length changed")
                blob[start:start + length] = encoded
            for index, texture in enumerate(textures):
                palette = palette_for(texture, palettes, index)
                actual = quantized_pixels(decode(blob, texture, palette), texture)
                desired = quantized_pixels(pixels[index], texture)
                if actual != desired:
                    raise ValueError(f"texture set {set_id}/{texture['name']}: "
                                     "shared Nitro data cannot represent the edited PNGs together")
        result.append(bytes(blob))
    destination = Path(output)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.unlink(missing_ok=True)
    destination.write_bytes(write_narc(result))


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "extract":
        export(Path(sys.argv[2]))
    elif len(sys.argv) == 5 and sys.argv[1] == "pack":
        pack(Path(sys.argv[2]), sys.argv[3], sys.argv[4])
    else:
        raise SystemExit(__doc__ + "\nextract OUTPUT_DIR\npack SOURCE_DIR black2|white2 OUTPUT")


if __name__ == "__main__":
    main()
