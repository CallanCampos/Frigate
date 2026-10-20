#!/usr/bin/env python3
"""The item icons, in each item's directory of res/items/: dump writes them from the ROM once, and pack builds the
archive (a/0/2/5) from them, which the build does.

    item_icon_data.py dump extract/b2_us/files res/items
    item_icon_data.py pack res/items black2|white2 ARCHIVE

The archive's members are the constants of generated/item_icons.txt, in its order. An icon (ITEM_ICON_POTION) is
icon.png, 32 by 32, and a palette (ITEM_ICON_PALETTE_POTION) icon.pal, a JASC palette of 16 colors, 0 transparent, in
the directory of the item they are named after, res/items/potion/; the Wonder Launcher's list icons
(ITEM_ICON_POTION_LAUNCHER) are launcher_icon.png and launcher_icon.pal. res/items/shared/ has the icons' Nitro
animations and cells, the return arrow's icon, and the icons that no item shows, in unused_icons/. pml_item.c's tables
give each item its icon and palette, and an icon can go with several palettes, as the TMs' disc does, so an item can
have only its icon.pal. An icon's PNG shows it with the palette of the first item that has it, but only its indices
count, so keep them in an image editor's indexed mode; the palette files set the colors.

To add an icon, add its constants to the end of the list and its files, and give an item them in pml_item.c. The
packer builds each member from its file, and where that gives the same picture, palette or data as one of the
original archive's members, it takes the original's bytes, which keeps the ROM's headers and the 240 unused colors
some palettes keep after their 16.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.gen_constants import ordered_names  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.nitro_2d import (character_pixels, character_resource, color5, palette_colors,  # noqa: E402
                                 palette_resource, read_indexed_png, read_palette, write_indexed_png, write_palette)

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = "a/0/2/5"
SIZE = 32
COLORS = 16
# The palettes' GXTexFmt (GX_TEXFMT_PLTT16) and the colors they keep, of which an icon shows the first 16
PALETTE_FORMAT = 3
PALETTE_ENTRIES = 256
# The characters' OBJ VRAM mapping, GX_OBJVRAMMODE_CHAR_1D_32K
CHAR_MAPPING = 0x10
NITRO_FILES = {"ITEM_ICON_ANIMATIONS": ("icon_animations.nanr", b"RNAN"), "ITEM_ICON_CELLS": ("icon_cells.ncer", b"RECN")}


def member_path(root: Path, constant: str) -> Path:
    """Returns the file of a member of the list."""
    if constant in NITRO_FILES:
        return root / "shared" / NITRO_FILES[constant][0]
    name = constant.removeprefix("ITEM_ICON_")
    suffix = ".pal" if name.startswith("PALETTE_") else ".png"
    name = name.removeprefix("PALETTE_")
    if name == "RETURN":
        return root / "shared" / f"return_icon{suffix}"
    if name.startswith("UNUSED_MEMBER_"):
        return root / "shared" / "unused_icons" / (name.removeprefix("UNUSED_MEMBER_") + suffix)
    if name.endswith("_LAUNCHER"):
        return root / name.removesuffix("_LAUNCHER").lower() / f"launcher_icon{suffix}"
    return root / name.lower() / f"icon{suffix}"


def members(root: Path) -> list[tuple[str, Path, str]]:
    """Returns each member's constant, file and kind (icon, palette or raw), in archive order."""
    result = []
    for constant in ordered_names("item_icons"):
        path = member_path(root, constant)
        result.append((constant, path, {".pal": "palette", ".png": "icon"}.get(path.suffix, "raw")))
    return result


def display_palettes() -> dict[int, int]:
    """Returns the palette member that each icon member is shown with: the first item's in pml_item.c's tables."""
    source = (ROOT / "src/system/pml_item.c").read_text()
    names = {name: number for number, name in enumerate(ordered_names("item_icons"))}
    shown = {}
    for table in ("ITEM_GRAPHICS_CHARS_AND_PALETTES", "LIST_ITEM_GRAPHICS"):
        body = re.search(rf"{table}\[\]\[2\] = \{{(.*?)\n\}};", source, re.S).group(1)
        for icon, palette in re.findall(r"\{ *(\w+), *(\w+) *\}", body):
            shown.setdefault(names.get(icon, icon), names.get(palette, palette))
    return {int(icon): int(palette) for icon, palette in shown.items()}


def dump(files: Path, root: Path):
    archive = read_narc((files / ARCHIVE).read_bytes())
    entries = members(root)
    if len(archive) != len(entries):
        raise SystemExit(f"{ARCHIVE} has {len(archive)} members, item_icons.txt {len(entries)}")
    shown = display_palettes()
    for index, ((_, path, kind), blob) in enumerate(zip(entries, archive)):
        path.parent.mkdir(parents=True, exist_ok=True)
        if kind == "palette":
            path.write_text(write_palette(palette_colors(blob)[:COLORS]))
        elif kind == "raw":
            path.write_bytes(blob)
        else:
            # An icon that no item shows is shown with the palette after it, or else the placeholder's
            palette = shown.get(index, index + 1 if entries[min(index + 1, len(entries) - 1)][2] == "palette" else
                                ordered_names("item_icons").index("ITEM_ICON_PALETTE_NONE"))
            colors = palette_colors(archive[palette])[:COLORS]
            path.write_bytes(write_indexed_png(*character_pixels(blob, SIZE, SIZE), colors))


def check(condition: bool, path: Path, message: str):
    if not condition:
        raise SystemExit(f"{path}: {message}")


def build(path: Path, kind: str) -> tuple[bytes, object]:
    """Returns a member built from its file, and what it shows, to find an original that shows the same."""
    check(path.is_file(), path, "missing; every constant of item_icons.txt needs its file")
    if kind == "palette":
        colors = read_palette(path)
        check(len(colors) == COLORS, path, f"must have {COLORS} colors, not {len(colors)}")
        return palette_resource(colors, PALETTE_FORMAT, PALETTE_ENTRIES), tuple(map(color5, colors))
    if kind == "raw":
        data = path.read_bytes()
        magic = next(magic for name, magic in NITRO_FILES.values() if name == path.name)
        check(data[:4] == magic, path, f"not a Nitro {path.suffix.lstrip('.').upper()} file")
        return data, data
    width, height, pixels, _ = read_indexed_png(path)
    check((width, height) == (SIZE, SIZE), path, f"must be {SIZE} by {SIZE} pixels, not {width} by {height}")
    check(max(pixels) < COLORS, path, f"uses a color index past {COLORS - 1}; an icon has {COLORS} colors")
    return character_resource(width, height, pixels, mapping=CHAR_MAPPING, sized=False), (width, height, pixels)


def original_key(blob: bytes, kind: str):
    if kind == "palette":
        return tuple(color5(color) for color in palette_colors(blob)[:COLORS])
    if kind == "raw":
        return blob
    return character_pixels(blob, SIZE, SIZE)


def pack(root: Path, game: str, output: Path):
    files = ROOT / "extract" / {"black2": "b2_us", "white2": "w2_us"}[game] / "files"
    originals = read_narc((files / ARCHIVE).read_bytes())
    entries = members(root)
    stored = {}
    for blob in originals:
        kind = {b"RLCN": "palette", b"RGCN": "icon"}.get(blob[:4], "raw")
        stored.setdefault((kind, original_key(blob, kind)), blob)
    result = []
    for _, path, kind in entries:
        built, shows = build(path, kind)
        result.append(stored.get((kind, shows), built))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(result))


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "dump":
        dump(Path(sys.argv[2]), Path(sys.argv[3]))
    elif len(sys.argv) == 5 and sys.argv[1] == "pack":
        pack(Path(sys.argv[2]), sys.argv[3], Path(sys.argv[4]))
    else:
        raise SystemExit(__doc__)
