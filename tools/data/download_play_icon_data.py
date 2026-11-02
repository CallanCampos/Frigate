#!/usr/bin/env python3
"""The DS Download Play icons in res/download_play/: dump writes them from the ROM once, and pack builds the files from
them, which the build does.

    download_play_icon_data.py dump extract/b2_us/files res/download_play
    download_play_icon_data.py pack res/download_play ICON_B_CHAR ICON_B_PLT ICON_W_CHAR ICON_W_PLT

dl_rom/icon_b.char and icon_b.plt are Black 2's icon, and icon_w.char and icon_w.plt White 2's, which the game's DS
Download Play parent (mb_parent_sys.c, mb_parent_dataconv_sys.c) gives the children it sends (MBGameRegistry's
iconCharPathp and iconPalettePathp) to show on their download menus: 32 by 32 pixels in 16 colors, the characters
4bpp in 8 by 8 tiles and the palette 16 RGB555 colors. Each icon is an indexed PNG, icon_b.png and icon_w.png, whose
pixels are the indices of its palette and whose palette is the icon's 16 colors, color 0 transparent, so edit it in
indexed mode and keep its palette's 16 colors in their order. Both versions have the same files.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label  # noqa: E402
from tools.data.nitro_2d import color5, encode_characters, read_indexed_png, write_indexed_png  # noqa: E402

ICONS = ["icon_b", "icon_w"]  # Black 2's and White 2's
SIZE = 32
COLORS = 16


def tile_pixels(chars: bytes) -> bytes:
    """Returns 4bpp characters in 8 by 8 tiles, SIZE by SIZE pixels, as rows of pixels: encode_characters's inverse."""
    pixels = bytearray(SIZE * SIZE)
    cursor = 0
    for ty in range(0, SIZE, 8):
        for tx in range(0, SIZE, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    byte = chars[cursor]
                    pixels[(ty + y) * SIZE + tx + x] = byte & 15
                    pixels[(ty + y) * SIZE + tx + x + 1] = byte >> 4
                    cursor += 1
    return bytes(pixels)


def dump(files: Path, output: Path):
    output.mkdir(parents=True, exist_ok=True)
    for icon in ICONS:
        chars = (files / "dl_rom" / f"{icon}.char").read_bytes()
        palette = (files / "dl_rom" / f"{icon}.plt").read_bytes()
        if len(chars) != SIZE * SIZE // 2 or len(palette) != 2 * COLORS:
            raise ValueError(f"{icon} isn't a 32 by 32 icon in 16 colors")
        colors = [tuple((value >> shift & 31) * 255 // 31 for shift in (0, 5, 10))
                  for (value,) in struct.iter_unpack("<H", palette)]
        if any(value & 0x8000 for (value,) in struct.iter_unpack("<H", palette)):
            raise ValueError(f"{icon}.plt has a color with bit 15 set, which the PNG doesn't keep")
        (output / f"{icon}.png").write_bytes(write_indexed_png(SIZE, SIZE, tile_pixels(chars), colors))
    print(f"wrote {len(ICONS)} icons to {output}")


def icon_files(path: Path) -> tuple[bytes, bytes]:
    width, height, pixels, colors = read_indexed_png(path)
    if (width, height) != (SIZE, SIZE):
        raise DataError(f"{label(path)}: an icon is {SIZE} by {SIZE} pixels, not {width} by {height}")
    if len(colors) > COLORS:
        raise DataError(f"{label(path)}: an icon has {COLORS} colors, and this PNG's palette {len(colors)}")
    if max(pixels) >= len(colors):
        raise DataError(f"{label(path)}: a pixel's index is outside the PNG's {len(colors)} colors")
    colors = colors + [(0, 0, 0)] * (COLORS - len(colors))
    palette = b"".join(struct.pack("<H", r | g << 5 | b << 10) for r, g, b in map(color5, colors))
    return encode_characters(SIZE, SIZE, pixels), palette


def pack(root: Path, outputs: list[Path]):
    if len(outputs) != 2 * len(ICONS):
        raise DataError(f"pack writes {2 * len(ICONS)} files: each icon's .char and .plt")
    for icon, (chars_out, palette_out) in zip(ICONS, zip(outputs[::2], outputs[1::2])):
        chars, palette = icon_files(root / f"{icon}.png")
        for output, data in ((chars_out, chars), (palette_out, palette)):
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/download_play/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the icons' characters and palettes")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs="+", help="icon_b.char, icon_b.plt, icon_w.char, icon_w.plt")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.outputs)
    except (DataError, ValueError) as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
