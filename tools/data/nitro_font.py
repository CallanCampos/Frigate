"""The game's fonts, in Nitro's font format (NFTR, magic RTFN), as a sheet of glyphs and their metrics.

    nitro_font.py dump FILE DIRECTORY        # writes a font's glyphs.png and font.json
    nitro_font.py pack DIRECTORY OUT         # and builds the font from them

graphics_archive.py dumps and packs ARCID_FONT's fonts with this, each a directory of:

- glyphs.png, an indexed PNG of every glyph in the font's order, a cell each, left to right in rows, with a line of
  the guide color around each cell. Its pixels are the glyph's values, 0 to 3 for the game's 2 bits a pixel: 0 is
  clear, and the game draws 1 in the letter color, 2 in the shadow's and 3 in the background's
  (GFL_TextRndUpdateColorIndexLUT), so edit it in indexed mode; its colors are only a preview. The packer reads the
  cells and ignores the guide lines, so a row of cells may be added for new glyphs.
- font.json (font.schema.json in res/fonts/): the font information (FINF) and the glyph cells' size (the start of CGLP),
  with gf_font.c's names; the character maps (CMAP), in order, each its characters' range and method; and the glyphs, in
  order, each its character and its left offset, width and advance, which the game keeps at the start of each cell.

Everything else is computed: the blocks' offsets and sizes, each glyph's cell, the maps' contents and the widths
block. That block isn't the format's usual per-glyph widths but Game Freak's (GFL_FontGetCharWidthDynamic): the three
commonest advances, a 2-bit class for each glyph, the first three meaning those, and for the rest a hash table of
advances, with buckets of glyphs and advances where several of the rest share a hash. A map takes the characters of its
range that no earlier map's range holds, since the game stops at the first range that holds a character; a direct map's
characters must be its first glyph's and those after it, a table's missing characters are 0xFFFF, and a scan map lists
its characters in order. The game's fonts all end in a scan map of every character, which takes a new glyph's.
The header's file size counts the widths block as 16 bytes, as the game's fonts do.

Only the standard library, since the build runs the packer.
"""
import json
import os
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.data.datajson import DataError, dumps, load, load_schema  # noqa: E402
from tools.data.nitro_2d import read_indexed_png, write_indexed_png  # noqa: E402

SCHEMA = ROOT / "res" / "fonts" / "font.schema.json"
ENCODINGS = ["UTF-8", "UTF-16", "Shift-JIS", "CP1252"]
METHODS = ["direct", "table", "scan"]
# The glyphs' preview colors by value: clear (white), the letter and its shadow (the archive's palette's colors 1 and 2,
# as the game draws them by default), the background (pale blue), then the guide lines (magenta)
PREVIEW = [(255, 255, 255), (90, 82, 82), (164, 164, 172), (189, 206, 238)]
GUIDE = (255, 0, 255)
NO_GLYPH = 0xFFFF


class FontError(Exception):
    pass


def code_name(code: int) -> str:
    return f"U+{code:04X}"


def code_value(text: str) -> int:
    return int(text.removeprefix("U+"), 16)


def glyph_code(glyph: dict, where: str) -> int:
    """A glyph's character code: its code where it has one, which its character must then be, or its character."""
    if "code" in glyph:
        code = code_value(glyph["code"])
        if "char" in glyph and glyph["char"] != chr(code):
            raise FontError(f"{where}: its char {glyph['char']!r} isn't its code {glyph['code']}")
        return code
    if "char" not in glyph:
        raise FontError(f"{where}: has neither a char nor a code")
    if len(glyph["char"]) != 1 or ord(glyph["char"]) > 0xFFFF:
        raise FontError(f"{where}: {glyph['char']!r} isn't one character of the Basic Multilingual Plane")
    return ord(glyph["char"])


def glyph_entry(code: int, left: int, width: int, advance: int) -> dict:
    """A glyph's entry: its character, and its code too where the character doesn't print (or can't be written,
    a surrogate's half)."""
    character = chr(code)
    entry = {} if 0xD800 <= code <= 0xDFFF else {"char": character}
    if not character.isprintable():
        entry["code"] = code_name(code)
    return entry | {"left": left, "width": width, "advance": advance}


# Reading a font

def blocks(data: bytes) -> list[tuple[bytes, int, int]]:
    """Returns a font's blocks, (magic, offset, size), each block after the last's size rounded up to 4 bytes."""
    magic, bom, version, _, header_size, count = struct.unpack_from("<4sHHIHH", data)
    if magic != b"RTFN" or bom != 0xFEFF or version != 0x0101 or header_size != 16:
        raise FontError("not a version 1.1 Nitro font")
    found, offset = [], header_size
    for _ in range(count):
        if offset + 8 > len(data):
            raise FontError(f"block at {offset:#x} is past the end")
        kind, size = struct.unpack_from("<4sI", data, offset)
        found.append((kind[::-1], offset, size))
        offset += (size + 3) & ~3
    if offset != len(data):
        raise FontError(f"the blocks end at {offset:#x}, the file at {len(data):#x}")
    return found


def unpack_bits(cell: bytes, count: int, bpp: int) -> bytes:
    """A glyph's pixels from its bitmap, the first pixel in the top bits of the first byte."""
    value = int.from_bytes(cell, "big")
    shift = len(cell) * 8
    mask = (1 << bpp) - 1
    return bytes((value >> (shift - (i + 1) * bpp)) & mask for i in range(count))


def pack_bits(pixels: bytes, size: int, bpp: int) -> bytes:
    value = 0
    for pixel in pixels:
        value = value << bpp | pixel
    return (value << (size * 8 - len(pixels) * bpp)).to_bytes(size, "big")


def read_font(data: bytes) -> tuple[dict, list[bytes]]:
    """Returns a font's font.json and its glyphs' pixels. Raises FontError for a layout that this doesn't write."""
    found = blocks(data)
    kinds = [kind for kind, _, _ in found]
    if kinds[:3] != [b"FINF", b"CGLP", b"CWDH"] or any(kind != b"CMAP" for kind in kinds[3:]) or len(kinds) < 4:
        raise FontError(f"blocks {[kind.decode() for kind in kinds]}, not FINF, CGLP, CWDH and character maps")
    (_, info_at, info_size), (_, glyphs_at, glyphs_size), (_, widths_at, _) = found[:3]
    if info_size != 28:
        raise FontError(f"a font information block of {info_size} bytes")
    (font_type, line_feed, alter, left, glyph_width, char_width, encoding, glyph_offset, width_offset,
     map_offset) = struct.unpack_from("<BBHBBBBIII", data, info_at + 8)
    if (glyph_offset, width_offset, map_offset) != (glyphs_at + 8, widths_at + 8, found[3][1] + 8):
        raise FontError("the font information's offsets aren't its blocks'")
    if encoding >= len(ENCODINGS):
        raise FontError(f"encoding {encoding}")
    cell_width, cell_height, cell_size, baseline, max_width, bpp, flags = struct.unpack_from("<BBHbBBB", data,
                                                                                             glyph_offset)
    if bpp not in (1, 2, 4) or cell_size != 3 + (cell_width * cell_height * bpp + 7) // 8 or \
            (glyphs_size - 16) % cell_size:
        raise FontError(f"cells of {cell_size} bytes for {cell_width}x{cell_height} at {bpp} bits a pixel")
    count = (glyphs_size - 16) // cell_size
    cells = [data[glyph_offset + 8 + i * cell_size:glyph_offset + 8 + (i + 1) * cell_size] for i in range(count)]
    pixels = [unpack_bits(cell[3:], cell_width * cell_height, bpp) for cell in cells]
    codes: dict[int, int] = {}
    maps = []
    for index, (_, at, size) in enumerate(found[3:]):
        first, last, method, reserved, following = struct.unpack_from("<HHHHI", data, at + 8)
        if method >= len(METHODS) or reserved or first > last:
            raise FontError(f"character map {index}: method {method}, reserved {reserved}, {first:#x} to {last:#x}")
        body = data[at + 20:at + size]
        if method == 0:
            start = struct.unpack_from("<H", body)[0]
            pairs = [(code, start + code - first) for code in range(first, last + 1)]
        elif method == 1:
            table = struct.unpack_from(f"<{last - first + 1}H", body)
            pairs = [(first + i, glyph) for i, glyph in enumerate(table) if glyph != NO_GLYPH]
        else:
            pairs = list(struct.iter_unpack("<HH", body[2:2 + 4 * struct.unpack_from("<H", body)[0]]))
        for code, glyph in pairs:
            if code in codes or glyph >= count or glyph in codes.values():
                raise FontError(f"character map {index}: {code_name(code)} maps to glyph {glyph} again or past the end")
            codes[code] = glyph
        maps.append({"method": METHODS[method], "first": code_name(first), "last": code_name(last)})
    by_glyph = {glyph: code for code, glyph in codes.items()}
    if len(by_glyph) != count:
        raise FontError(f"{count - len(by_glyph)} glyphs have no character")
    widths = data[widths_at + 8:found[3][1]]
    classes_at = struct.unpack_from("<I", widths, 4)[0]
    classes_end = classes_at + (count * 2 + 7) // 8
    padding = list(widths[classes_end:struct.unpack_from("<I", widths, 8)[0]])
    font = {
        "fontType": font_type,
        "lineFeed": line_feed,
        "alterCharIndex": alter,
        "defaultLeft": left,
        "defaultGlyphWidth": glyph_width,
        "defaultCharWidth": char_width,
        "encoding": ENCODINGS[encoding],
        "cellWidth": cell_width,
        "cellHeight": cell_height,
        "baseline": baseline,
        "maxCharWidth": max_width,
        "bpp": bpp,
        "flags": flags,
    }
    if any(padding):
        font["classPadding"] = padding
    font["maps"] = maps
    font["glyphs"] = [glyph_entry(by_glyph[i], *cells[i][:3]) for i in range(count)]
    return font, pixels


# Building a font

def width_hash(index: int) -> int:
    """The hash of a glyph's index in the widths block, as GFL_FontGetCharWidthDynamic computes it."""
    parity = ((index >> 12) ^ (index >> 11) ^ (index >> 10) ^ (index >> 9)) & 1
    return (index & 0x1FF) ^ (parity << 3)


def widths_block(advances: list[int], padding: list[int]) -> bytes:
    """The widths block's data: the commonest three advances (the commonest first, the first glyph's first among
    equals, the commonest repeated where there are fewer), each glyph's class, the bytes that pad the classes to 4
    (the font's classPadding, as far as it fits, then zeros), the hash table and its buckets."""
    counts = Counter(advances)
    common = sorted(counts, key=lambda width: (-counts[width], advances.index(width)))[:3] or [0]
    common += [common[0]] * (3 - len(common))
    classes = bytearray((len(advances) * 2 + 7) // 8)
    slots: dict[int, list[tuple[int, int]]] = {}
    for index, advance in enumerate(advances):
        kind = common.index(advance) if advance in common else 3
        classes[index >> 2] |= kind << (6 - (index & 3) * 2)
        if kind == 3:
            slots.setdefault(width_hash(index), []).append((index, advance))
    classes_at = 16
    table_at = (classes_at + len(classes) + 3) & ~3
    room = table_at - classes_at - len(classes)
    padding = (list(padding) + [0] * room)[:room]
    table, buckets, buckets_count = bytearray(512), bytearray(), 0
    for slot in range(512):
        entries = slots.get(slot, [])
        if len(entries) == 1 and entries[0][1] < 128:
            table[slot] = entries[0][1]
        elif entries:
            bucket = buckets_count
            buckets_count += 1
            if bucket >= 128 or len(entries) > 255:
                raise FontError("too many glyphs of uncommon advances for the widths block's buckets")
            table[slot] = (-bucket - 1) & 0xFF
            buckets.append(len(entries))
            for index, advance in entries:
                buckets += struct.pack(">HB", index, advance)
    return (bytes(common) + b"\0" + struct.pack("<III", classes_at, table_at, table_at + 512) + classes +
            bytes(padding) + table + buckets)


def block(kind: bytes, data: bytes) -> bytes:
    """A block, padded with zeros to 4 bytes, which its size counts but for the glyphs' block."""
    padding = bytes(-len(data) % 4)
    size = 8 + len(data) + (0 if kind == b"CGLP" else len(padding))
    return kind[::-1] + struct.pack("<I", size) + data + padding


def map_contents(font: dict, codes: dict[int, int], where: str) -> list[tuple[int, int, int, bytes]]:
    """Each map's (first, last, method, data), holding the characters of its range that no earlier map's holds."""
    ranges = [(code_value(m["first"]), code_value(m["last"]), METHODS.index(m["method"])) for m in font["maps"]]
    owner = {}
    for code in codes:
        index = next((i for i, (first, last, _) in enumerate(ranges) if first <= code <= last), None)
        if index is None:
            raise FontError(f"{where}: no character map holds {code_name(code)}; extend a map's range or add one")
        owner.setdefault(index, []).append(code)
    result = []
    for index, (first, last, method) in enumerate(ranges):
        if first > last:
            raise FontError(f"{where}: map {index} runs from {code_name(first)} back to {code_name(last)}")
        mine = sorted(owner.get(index, []))
        if method == 0:
            if codes.get(first) is None:
                raise FontError(f"{where}: direct map {index} has no glyph for {code_name(first)}")
            start = codes[first]
            for code in range(first, last + 1):
                if codes.get(code) != start + code - first and not any(
                        a <= code <= z for a, z, _ in ranges[:index]):
                    raise FontError(f"{where}: direct map {index} needs glyph {start + code - first} for "
                                    f"{code_name(code)}")
            data = struct.pack("<H", start)
        elif method == 1:
            data = struct.pack(f"<{last - first + 1}H", *(codes[code] if code in mine else NO_GLYPH
                                                          for code in range(first, last + 1)))
        else:
            data = struct.pack("<H", len(mine)) + b"".join(struct.pack("<HH", code, codes[code]) for code in mine)
        result.append((first, last, method, data))
    return result


def build_font(font: dict, pixels: list[bytes], where: str = "font") -> bytes:
    """Builds a font from its font.json and its glyphs' pixels."""
    glyphs = font["glyphs"]
    if len(pixels) != len(glyphs) or len(glyphs) > 0xFFFF:
        raise FontError(f"{where}: {len(glyphs)} glyphs and {len(pixels)} cells")
    codes = {}
    for index, glyph in enumerate(glyphs):
        code = glyph_code(glyph, f"{where}: glyph {index}")
        if code in codes:
            raise FontError(f"{where}: glyphs {codes[code]} and {index} are both {code_name(code)}")
        codes[code] = index
    width, height, bpp = font["cellWidth"], font["cellHeight"], font["bpp"]
    bitmap_size = (width * height * bpp + 7) // 8
    cells = b"".join(bytes([glyph["left"], glyph["width"], glyph["advance"]]) + pack_bits(cell, bitmap_size, bpp)
                     for glyph, cell in zip(glyphs, pixels))
    maps = map_contents(font, codes, where)
    glyph_data = struct.pack("<BBHbBBB", width, height, 3 + bitmap_size, font["baseline"], font["maxCharWidth"], bpp,
                             font["flags"]) + cells
    widths = widths_block([glyph["advance"] for glyph in glyphs], font.get("classPadding", []))
    info_at = 16
    glyphs_at = info_at + 28
    widths_at = glyphs_at + len(block(b"CGLP", glyph_data))
    maps_at = widths_at + len(block(b"CWDH", widths))
    info = struct.pack("<BBHBBBBIII", font["fontType"], font["lineFeed"], font["alterCharIndex"], font["defaultLeft"],
                       font["defaultGlyphWidth"], font["defaultCharWidth"], ENCODINGS.index(font["encoding"]),
                       glyphs_at + 8, widths_at + 8, maps_at + 8)
    map_blocks, at = [], maps_at
    for first, last, method, data in maps:
        size = len(block(b"CMAP", bytes(12) + data))
        # Each map's next is where one after it would start, the last's too, which its range of every character
        # keeps the game from following
        map_blocks.append(block(b"CMAP", struct.pack("<HHHHI", first, last, method, 0, at + size + 8) + data))
        at += size
    body = block(b"FINF", info) + block(b"CGLP", glyph_data) + block(b"CWDH", widths) + b"".join(map_blocks)
    size = 16 + len(body) - len(block(b"CWDH", widths)) + 16
    return struct.pack("<4sHHIHH", b"RTFN", 0xFEFF, 0x0101, size, 16, 3 + len(maps)) + body


# The glyph sheet

def sheet_columns(count: int) -> int:
    return 16 if count <= 256 else 32


def glyph_png(font: dict, pixels: list[bytes]) -> bytes:
    """The glyph sheet: each glyph's cell in rows, with a line of the guide color around each."""
    width, height, bpp = font["cellWidth"], font["cellHeight"], font["bpp"]
    guide = 1 << bpp
    columns = sheet_columns(len(pixels))
    rows = max(1, -(-len(pixels) // columns))
    sheet_width, sheet_height = columns * (width + 1) + 1, rows * (height + 1) + 1
    image = bytearray([guide]) * (sheet_width * sheet_height)
    for index in range(columns * rows):
        cell = pixels[index] if index < len(pixels) else bytes(width * height)
        x0, y0 = index % columns * (width + 1) + 1, index // columns * (height + 1) + 1
        for y in range(height):
            image[(y0 + y) * sheet_width + x0:(y0 + y) * sheet_width + x0 + width] = cell[y * width:(y + 1) * width]
    colors = (PREVIEW + [(0, 0, 0)] * guide)[:guide] + [GUIDE]
    return write_indexed_png(sheet_width, sheet_height, bytes(image), colors)


def read_glyph_png(path: Path, font: dict) -> list[bytes]:
    width, height, bpp = font["cellWidth"], font["cellHeight"], font["bpp"]
    sheet_width, sheet_height, image, _ = read_indexed_png(path)
    columns, rows = (sheet_width - 1) // (width + 1), (sheet_height - 1) // (height + 1)
    if (sheet_width - 1) % (width + 1) or (sheet_height - 1) % (height + 1):
        raise FontError(f"{path}: {sheet_width}x{sheet_height} isn't a sheet of {width}x{height} cells, each with a "
                        "guide line around it")
    count = len(font["glyphs"])
    if columns * rows < count:
        raise FontError(f"{path}: {columns * rows} cells for {count} glyphs; add a row of cells")
    cells = []
    for index in range(count):
        x0, y0 = index % columns * (width + 1) + 1, index // columns * (height + 1) + 1
        cell = b"".join(image[(y0 + y) * sheet_width + x0:(y0 + y) * sheet_width + x0 + width] for y in range(height))
        if max(cell, default=0) >= 1 << bpp:
            raise FontError(f"{path}: glyph {index} has a pixel of value {max(cell)}, past {(1 << bpp) - 1}")
        cells.append(cell)
    return cells


# The font's directory

def write_font_json(path: Path, font: dict):
    """font.json, as datajson writes JSON, but with a line for each map and each glyph."""
    def entries(items: list[dict]) -> str:
        return ",\n".join("        { " + ", ".join(f"{json.dumps(key)}: {json.dumps(value, ensure_ascii=False)}"
                                                for key, value in item.items()) + " }" for item in items)

    head = dumps({key: value for key, value in font.items() if key not in ("maps", "glyphs")})
    path.write_text(head[:-2] + ',\n    "maps": [\n' + entries(font["maps"]) + '\n    ],\n    "glyphs": [\n' +
                    entries(font["glyphs"]) + "\n    ]\n}\n")


def export_font(directory: Path, data: bytes) -> str | None:
    """Writes a font's directory, or returns why it can't be one: a layout this doesn't read, or one it wouldn't
    build again byte for byte."""
    try:
        font, pixels = read_font(data)
        if build_font(font, pixels) != data:
            return "it doesn't build again byte for byte from its glyphs and metrics"
    except (FontError, struct.error) as error:
        return str(error)
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "glyphs.png").write_bytes(glyph_png(font, pixels))
    write_font_json(directory / "font.json", {"$schema": os.path.relpath(SCHEMA, directory.resolve())} | font)
    return None


def build_font_directory(directory: Path) -> bytes:
    try:
        font = load(directory / "font.json", load_schema(SCHEMA))
        return build_font(font, read_glyph_png(directory / "glyphs.png", font), str(directory))
    except (FontError, DataError) as error:
        raise FontError(str(error)) from None


def main():
    try:
        if len(sys.argv) == 4 and sys.argv[1] == "dump":
            problem = export_font(Path(sys.argv[3]), Path(sys.argv[2]).read_bytes())
            if problem:
                raise SystemExit(f"error: {problem}")
        elif len(sys.argv) == 4 and sys.argv[1] == "pack":
            Path(sys.argv[3]).write_bytes(build_font_directory(Path(sys.argv[2])))
        else:
            raise SystemExit(__doc__)
    except (FontError, ValueError) as error:
        raise SystemExit(f"error: {error}") from None


if __name__ == "__main__":
    main()
