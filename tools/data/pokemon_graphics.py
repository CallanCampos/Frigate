#!/usr/bin/env python3
"""Export and pack Pokémon battle sprites, icons, and footprints.

    pokemon_graphics.py extract data/pokemon
    pokemon_graphics.py pack data/pokemon black2|white2 OUTPUT...

Images are indexed PNGs: their pixel indices select colors in the adjacent
JASC palette. Nitro cell, animation, and other binary resources remain separate
files so every archive member has a source without inventing lossy metadata.
"""

import json
import re
import sys
from functools import lru_cache
from pathlib import Path

from narc import read_narc, write_narc
from nitro_2d import (character_pixels, palette_colors, patch_characters, patch_palette,
                      read_indexed_png, read_palette, write_indexed_png, write_palette)


ROOT = Path(__file__).resolve().parents[2]
ARCHIVES = {"a/0/0/4": "battle", "a/0/0/7": "icons", "a/0/5/1": "musical",
            "a/1/6/5": "footprints", "a/2/2/9": "footprint_forms"}
SLOT_NAMES = ("front_single", "front_single_female", "front", "front_female",
              "front_cells", "front_animations", "front_multi_cells", "front_multi_animations", "front_info",
              "back_single", "back_single_female", "back", "back_female",
              "back_cells", "back_animations", "back_multi_cells", "back_multi_animations", "back_info",
              "normal", "shiny")
CHAR_SLOTS = {0, 1, 2, 3, 9, 10, 11, 12}
PALETTE_SLOTS = {18, 19}
FORM_FOOTPRINTS = (("giratina", 1), ("shaymin", 1), ("kyurem", 1), ("kyurem", 2),
                   ("keldeo", 1), ("tornadus", 1), ("thundurus", 1), ("landorus", 1))


@lru_cache(maxsize=1)
def species_names():
    return [re.match(r"SPECIES_([A-Z0-9_]+)", line).group(1).lower()
            for line in (ROOT / "data/constants/species.txt").read_text().splitlines()
            if line.startswith("SPECIES_")]


@lru_cache(maxsize=1)
def form_names():
    result = {}
    for species in species_names():
        path = ROOT / "data/pokemon" / species / "data.json"
        if not path.exists():
            continue
        record = json.loads(path.read_text())
        forms = record.get("forms", {})
        if forms.get("count", 1) <= 1 or record.get("palette_forms"):
            continue
        for form in range(1, forms["count"]):
            result[forms["sprite_offset"] + form - 1] = (species, form)
    return result


def group_folder(root, group, musical=False):
    names = species_names()
    if group < len(names):
        path = root / names[group]
    elif group == (650 if musical else 683):
        path = root / "egg"
    elif group == (651 if musical else 684):
        path = root / "manaphy" / "egg"
    elif not musical and group < 683:
        path = root / "extra_graphics" / f"battle_group_{group}"
    elif group >= (652 if musical else 685):
        offset = group - (652 if musical else 685)
        owner = form_names().get(offset) if not musical else None
        path = root / owner[0] / f"form_{owner[1]}" if owner else root / "extra_graphics" / f"form_group_{group}"
    else:
        path = root / "extra_graphics" / f"group_{group}"
    return path / "musical" if musical else path


def battle_file(root, index, musical):
    end = 14240 if musical else 15020
    if index >= end:
        palette_start = 14253 if musical else 15033
        if index >= palette_start:
            offset = index - palette_start
            folder = root / "arceus" / f"form_{offset // 2 + 1}"
            if musical:
                folder /= "musical"
            return folder / ("shiny.pal" if offset % 2 else "normal.pal"), "palette", None
        return root / "extra_graphics" / ("musical" if musical else "battle") / f"tail_{index:05d}.bin", "raw", None
    group, slot = divmod(index, 20)
    folder = group_folder(root, group, musical)
    name = SLOT_NAMES[slot]
    if slot in CHAR_SLOTS:
        return folder / f"{name}.png", "char", {
            "single_cell": slot in (0, 1, 9, 10), "linear": slot in (2, 3, 11, 12)}
    if slot in PALETTE_SLOTS:
        return folder / f"{name}.pal", "palette", None
    extension = {4: ".ncer", 5: ".nanr.lz11", 6: ".nmcr", 7: ".nmar",
                 13: ".ncer", 14: ".nanr.lz11", 15: ".nmcr", 16: ".nmar"}.get(slot, ".bin")
    return folder / f"{name}{extension}", "raw", None


@lru_cache(maxsize=1)
def icon_palettes():
    source = (ROOT / "src/system/poke_icon.c").read_text()
    table = source.split("static const u8 sPokeIconPalettes[] = {", 1)[1].split("};", 1)[0]
    return [int(value, 16) for value in re.findall(r"0x[0-9a-fA-F]+", table)]


def icon_file(root, index):
    if index == 0:
        return root / "shared" / "icon_palettes.pal", "palette", None
    if index == 1:
        return root / "shared" / "icon_palettes_alt.pal", "palette", None
    if index < 8:
        return root / "shared" / f"icon_{index}.{'nanr' if index % 2 == 0 else 'ncer'}", "raw", None
    group, female = divmod(index - 8, 2)
    path = group_folder(root, group) / ("icon_female.png" if female else "icon.png")
    palette_index = group if group < 683 else group + 2
    table = icon_palettes()
    bank = (table[palette_index] >> (4 if female else 0)) & 15 if palette_index < len(table) else 0
    return path, "char", {"size": (32, 64), "palette_bank": bank}


def footprint_file(root, index, forms=False):
    if forms:
        species, form = FORM_FOOTPRINTS[index]
        return root / species / f"form_{form}" / "footprint.png", "char", {"size": (16, 32)}
    if index == 0:
        return root / "shared" / "footprint_palette.pal", "palette", None
    if index < 5:
        return root / "shared" / f"footprint_{index}.{'nanr' if index % 2 else 'ncer'}", "raw", None
    names = species_names()
    species = index - 5
    folder = root / (names[species] if species < len(names) else "extra_graphics")
    return folder / ("footprint.png" if species < len(names) else f"footprint_{species}.png"), "char", {"size": (16, 32)}


def source_file(root, archive_name, index):
    kind = ARCHIVES[archive_name]
    if kind in ("battle", "musical"):
        return battle_file(root, index, kind == "musical")
    if kind == "icons":
        return icon_file(root, index)
    return footprint_file(root, index, kind == "footprint_forms")


def colors_for(root, archive_name, archive, index, options):
    kind = ARCHIVES[archive_name]
    if kind in ("battle", "musical"):
        group = index // 20
        return palette_colors(archive[group * 20 + 18])[:16]
    if kind == "icons":
        colors = palette_colors(archive[0])
        bank = options["palette_bank"]
        return colors[bank * 16:(bank + 1) * 16]
    if kind == "footprint_forms":
        archive = read_narc((ROOT / "extract/b2_us/files/a/1/6/5").read_bytes())
    return palette_colors(archive[0])[:16]


def export(root):
    for archive_name in ARCHIVES:
        path = ROOT / "extract/b2_us/files" / archive_name
        archive = read_narc(path.read_bytes())
        for index, blob in enumerate(archive):
            if not blob:
                continue
            target, kind, options = source_file(root, archive_name, index)
            target.parent.mkdir(parents=True, exist_ok=True)
            if kind == "raw":
                target.write_bytes(blob)
            elif kind == "palette":
                target.write_text(write_palette(palette_colors(blob)))
            else:
                width, height, pixels = character_pixels(blob, *(options.get("size", (None, None)) if options else
                                                          (None, None)),
                                                          single_cell=options.get("single_cell", False) if options else False,
                                                          linear=options.get("linear", False) if options else False)
                target.write_bytes(write_indexed_png(width, height, pixels,
                                                    colors_for(root, archive_name, archive, index, options)))
        print(f"{archive_name}: {len(archive)} entries", flush=True)
    musical = read_narc((ROOT / "extract/b2_us/files/a/0/5/1").read_bytes())
    for index, blob in enumerate(musical):
        if not blob:
            continue
        path, _, _ = battle_file(root, index, True)
        if path.parent.name != "musical":
            continue
        shared = path.parent.parent / path.name
        if shared.is_file() and path.read_bytes() == shared.read_bytes():
            path.unlink()


def pack(root, game, output):
    archive_name = Path(output).as_posix().split("/files/", 1)[-1]
    if archive_name not in ARCHIVES:
        raise ValueError(f"unexpected Pokémon graphics archive {archive_name}")
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
    archive = read_narc((ROOT / "extract" / version / "files" / archive_name).read_bytes())
    result = []
    for index, original in enumerate(archive):
        if not original:
            result.append(original)
            continue
        path, kind, options = source_file(root, archive_name, index)
        if archive_name == "a/0/5/1" and not path.exists() and path.parent.name == "musical":
            path = path.parent.parent / path.name
        if kind == "raw":
            edited = path.read_bytes()
        elif kind == "palette":
            edited_colors = read_palette(path)
            original_colors = palette_colors(original)
            edited = (original if all(round(a * 31 / 255) == round(b * 31 / 255)
                                      for left, right in zip(edited_colors, original_colors)
                                      for a, b in zip(left, right)) and len(edited_colors) == len(original_colors)
                      else patch_palette(original, edited_colors))
        else:
            width, height, pixels = read_indexed_png(path)
            expected_width, expected_height, original_pixels = character_pixels(
                original, *(options.get("size", (None, None)) if options else (None, None)),
                single_cell=options.get("single_cell", False) if options else False,
                linear=options.get("linear", False) if options else False)
            if (width, height) != (expected_width, expected_height):
                raise ValueError(f"{path}: expected {expected_width} by {expected_height} pixels")
            edited = (original if pixels == original_pixels else
                      patch_characters(original, width, height, pixels,
                                       single_cell=options.get("single_cell", False) if options else False,
                                       linear=options.get("linear", False) if options else False))
        result.append(edited)
    target = Path(output)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.unlink(missing_ok=True)
    target.write_bytes(write_narc(result))


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "extract":
        export(Path(sys.argv[2]))
    elif len(sys.argv) == 4 + len(ARCHIVES) and sys.argv[1] == "pack":
        for output in sys.argv[4:]:
            pack(Path(sys.argv[2]), sys.argv[3], output)
    else:
        raise SystemExit(__doc__)
