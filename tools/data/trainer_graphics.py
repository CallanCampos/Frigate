#!/usr/bin/env python3
"""The trainer sprites in data/trainer_sprites/: dump writes them from the ROM once, and pack builds the archives from
them, which the build does.

    trainer_graphics.py dump extract/b2_us/files data/trainer_sprites
    trainer_graphics.py pack data/trainer_sprites black2|white2 FRONT BACK BACK_COPY

A sprite is a directory, front/<sprite>/ or back/<sprite>/, named after its constant in trainer_sprites.txt or
trainer_back_sprites.txt, and the archives (a/0/7/1, a/0/7/2) have the sprites in the order of those lists, eight
files each. a/0/6/3 is a copy of the back sprites' archive. A sprite's files:

- single.png, the 80 by 80 picture shown outside battle, and sheet.png, the parts that the cells put together in
  battle: indexed PNGs whose indices select the colors of normal.pal, a JASC palette of 16 colors, 0 transparent.
  Only the sprites that screens outside battle show have a single.png; without one the packer writes an empty
  picture, as the game has for the others.
- cells.ncer, animations.nanr, multi_cells.nmcr and multi_animations.nmar: the Nitro cells and animations of the
  battle sprite, uncompressed, as Nitro editors open them.
- mcss.bin: the battle sprite system's data (mcss.c), which the game reads as it is stored.

To add a sprite, add its constant and its directory; tr_tool.c's TRAINER_CLASS_RESOURCES gives a class its sprite.
The packer builds each file from its source, and where that gives the same picture, palette or data as one of the
original archive's files, it takes the original's bytes, which keeps the ROM's compression and headers.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.gen_constants import ordered_names  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.nitro_2d import (SINGLE_CELL_80, character_pixels, character_resource, color5,  # noqa: E402
                                 compress_literals, decompress, palette_colors, palette_indices, palette_resource,
                                 read_indexed_png, read_palette, write_indexed_png, write_palette)

ROOT = Path(__file__).resolve().parents[2]
# Each sprite's files, in archive order: its file name and how it is stored. "single" and "sheet" are pictures, the
# one in the single-cell OBJ layout and the other in rows with a character position section; "lz" files are stored
# compressed and "raw" ones as they are
FILES = [("single.png", "single"), ("sheet.png", "sheet"), ("cells.ncer", "raw"), ("animations.nanr", "lz"),
         ("multi_cells.nmcr", "raw"), ("multi_animations.nmar", "raw"), ("mcss.bin", "raw"),
         ("normal.pal", "palette")]
PALETTE = len(FILES) - 1
SHEET_POSITION = (32, 32)
SINGLE_SIZE = 80
SHEET_SIZE = (256, 128)
COLORS = 16
# The magic of each Nitro file, which the packer checks, so that a wrong file stops the build instead of the game
MAGIC = {"cells.ncer": b"RECN", "animations.nanr": b"RNAN", "multi_cells.nmcr": b"RCMN",
         "multi_animations.nmar": b"RAMN"}
# TRAINER_CLASS_RESOURCES's sprite field is a byte
MAX_FRONT_SPRITES = 256
# Each side's constant list, its prefix and its archive
SIDES = {"front": ("trainer_sprites", "TRAINER_SPRITE_", "a/0/7/1"),
         "back": ("trainer_back_sprites", "TRAINER_BACK_SPRITE_", "a/0/7/2")}
BACK_COPY = "a/0/6/3"


def sprite_dirs(root: Path, side: str) -> list[Path]:
    """Returns the side's sprite directories, in the order of its constant list."""
    name, prefix, _ = SIDES[side]
    constants = ordered_names(name)
    if side == "front" and len(constants) > MAX_FRONT_SPRITES:
        raise SystemExit(f"{name}.txt has {len(constants)} sprites, more than the {MAX_FRONT_SPRITES} that "
                         "TRAINER_CLASS_RESOURCES's sprite field can hold")
    return [root / side / constant.removeprefix(prefix).lower() for constant in constants]


def picture(blob: bytes, kind: str) -> tuple[int, int, bytes]:
    return character_pixels(blob, linear=True) if kind == "sheet" else character_pixels(blob, objs=SINGLE_CELL_80)


def content_key(blob: bytes, kind: str):
    """Returns what a file shows, apart from how it is stored: a picture's size and pixels, a palette's colors as the
    DS shows them, or the data, decompressed."""
    if kind in ("single", "sheet"):
        return kind, picture(blob, kind)
    if kind == "palette":
        return kind, tuple(color5(color) for color in palette_colors(blob))
    return kind, decompress(blob) if kind == "lz" else blob


def dump(files: Path, root: Path):
    for side, (_, _, archive_name) in SIDES.items():
        archive = read_narc((files / archive_name).read_bytes())
        directories = sprite_dirs(root, side)
        if len(archive) != len(directories) * len(FILES):
            raise SystemExit(f"{archive_name} has {len(archive)} files, not {len(FILES)} for each of "
                             f"{len(directories)} sprites")
        for directory, start in zip(directories, range(0, len(archive), len(FILES))):
            directory.mkdir(parents=True, exist_ok=True)
            colors = palette_colors(archive[start + PALETTE])
            for (file_name, kind), blob in zip(FILES, archive[start:start + len(FILES)]):
                path = directory / file_name
                if kind == "single" and not any(picture(blob, kind)[2]):
                    continue
                if kind in ("single", "sheet"):
                    path.write_bytes(write_indexed_png(*picture(blob, kind), colors))
                elif kind == "palette":
                    path.write_text(write_palette(colors))
                else:
                    path.write_bytes(decompress(blob) if kind == "lz" else blob)


def check(condition: bool, path: Path, message: str):
    if not condition:
        raise SystemExit(f"{path}: {message}")


def build(path: Path, kind: str, colors: list) -> bytes:
    """Returns a sprite's file built from its source."""
    if kind == "palette":
        return palette_resource(colors)
    if kind == "single" and not path.exists():
        width = height = SINGLE_SIZE
        pixels = bytes(width * height)
    else:
        check(path.is_file(), path, "missing; every sprite needs this file (only single.png is optional)")
        if kind not in ("single", "sheet"):
            data = path.read_bytes()
            if path.name in MAGIC:
                check(data[:4] == MAGIC[path.name], path, f"not a Nitro {path.suffix.lstrip('.').upper()} file")
            check(bool(data), path, "empty")
            return compress_literals(data, 0x11) if kind == "lz" else data
        width, height, pixels, png_colors = read_indexed_png(path)
        size = SHEET_SIZE if kind == "sheet" else (SINGLE_SIZE, SINGLE_SIZE)
        check((width, height) == size, path, f"must be {size[0]} by {size[1]} pixels, not {width} by {height}")
        pixels = palette_indices(path, pixels, png_colors, colors)
    if kind == "sheet":
        resource = character_resource(width, height, pixels, linear=True, position=SHEET_POSITION)
    else:
        resource = character_resource(width, height, pixels, objs=SINGLE_CELL_80)
    return compress_literals(resource, 0x11)


def pack_side(root: Path, side: str, originals: list[bytes]) -> bytes:
    stored = {}
    for index, blob in enumerate(originals):
        stored.setdefault(content_key(blob, FILES[index % len(FILES)][1]), blob)
    result = []
    for directory in sprite_dirs(root, side):
        if not directory.is_dir():
            raise SystemExit(f"{directory}: no such sprite directory")
        palette = directory / "normal.pal"
        check(palette.is_file(), palette, "missing; every sprite needs its palette")
        colors = read_palette(palette)
        check(len(colors) == COLORS, palette, f"must have {COLORS} colors, not {len(colors)}")
        for file_name, kind in FILES:
            built = build(directory / file_name, kind, colors)
            result.append(stored.get(content_key(built, kind), built))
    return write_narc(result)


def pack(root: Path, game: str, outputs: list[Path]):
    files = ROOT / "extract" / {"black2": "b2_us", "white2": "w2_us"}[game] / "files"
    front, back, back_copy = outputs
    for side, output in (("front", front), ("back", back)):
        archive = pack_side(root, side, read_narc((files / SIDES[side][2]).read_bytes()))
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(archive)
        if side == "back":
            back_copy.parent.mkdir(parents=True, exist_ok=True)
            back_copy.write_bytes(archive)


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "dump":
        dump(Path(sys.argv[2]), Path(sys.argv[3]))
    elif len(sys.argv) == 7 and sys.argv[1] == "pack":
        pack(Path(sys.argv[2]), sys.argv[3], [Path(arg) for arg in sys.argv[4:]])
    else:
        raise SystemExit(__doc__)
