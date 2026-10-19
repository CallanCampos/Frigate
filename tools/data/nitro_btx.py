"""Write Nitro texture sets (NSBTX, BTX0) of plain textures and palettes, as the game's field sprites have them."""

import struct

from nitro_dictionary import build_dictionary

TEX0_HEADER = 0x3C
# Texture formats (GXTexFmt) and their bits per texel
BITS = {2: 2, 3: 4, 4: 8, 7: 16}


def texture_params(offset, width, height, fmt, color0):
    """Returns a texture's TEXIMAGE_PARAM: its data's offset in 8-byte units, size, format and whether color 0 is
    transparent."""
    return ((offset >> 3) | ((width // 8).bit_length() - 1) << 20 | ((height // 8).bit_length() - 1) << 23
            | fmt << 26 | int(color0) << 29)


def texture_set(textures, palettes):
    """Returns a BTX0 of textures, (name, width, height, fmt, color0, data), and palettes, (name, data)."""
    texel_data = b"".join(data for *_, data in textures)
    palette_data = b"".join(data for _, data in palettes)
    records, offset = [], 0
    for name, width, height, fmt, color0, data in textures:
        if len(data) != width * height * BITS[fmt] // 8:
            raise ValueError(f"texture {name}: {len(data)} bytes of data for {width} by {height}")
        records.append(struct.pack("<II", texture_params(offset, width, height, fmt, color0),
                                   width | height << 11 | 0x80000000))
        offset += len(data)
    texture_dictionary = build_dictionary([name for name, *_ in textures], records)
    palette_records, offset = [], 0
    for _, data in palettes:
        palette_records.append(struct.pack("<HH", offset >> 3, 0))
        offset += len(data)
    palette_dictionary = build_dictionary([name for name, _ in palettes], palette_records)
    palette_info = TEX0_HEADER + len(texture_dictionary)
    texel_start = palette_info + len(palette_dictionary)
    palette_start = texel_start + len(texel_data)
    size = palette_start + len(palette_data)
    header = struct.pack("<4sIIHHIIIHHIIIIIII", b"TEX0", size, 0, len(texel_data) >> 3, TEX0_HEADER, 0, texel_start,
                         0, 0, TEX0_HEADER, 0, palette_start, palette_start, 0, len(palette_data) >> 3, palette_info,
                         palette_start)
    block = header + texture_dictionary + palette_dictionary + texel_data + palette_data
    return struct.pack("<4sHHIHHI", b"BTX0", 0xFEFF, 1, 0x14 + len(block), 0x10, 1, 0x14) + block
