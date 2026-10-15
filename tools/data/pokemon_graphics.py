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
import struct
import sys
from functools import lru_cache
from pathlib import Path

from narc import read_narc, write_narc
from species_data import directory
from nitro_2d import (character_pixels, palette_colors, patch_characters, patch_palette,
                      read_indexed_png, read_palette, write_indexed_png, write_palette)


ROOT = Path(__file__).resolve().parents[2]
# The extracted files of the version being packed, which hold the originals that the sources are compared with.
# Exporting reads Black 2's; pack() switches to the version it packs, the only one extracted in its CI job
EXTRACTED = ROOT / "extract/b2_us/files"
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
    """Returns the directories of the species of data/constants/species.txt, in order: the national Pokédex's, then
    the egg, the bad egg and the Pokéstar Studios species."""
    return [directory(re.match(r"SPECIES_[A-Z0-9_]+", line).group(0))
            for line in (ROOT / "data/constants/species.txt").read_text().splitlines()
            if line.startswith("SPECIES_")]


@lru_cache(maxsize=1)
def national_species():
    """Returns the number of species up to the national Pokédex's last, the index of SPECIES_EGG."""
    return species_names().index("egg")


@lru_cache(maxsize=2)
def form_names(musical=False):
    """Returns the species and form of each form sprite group, from its species' forms (sprite_offset and count) in
    data/pokemon. The musical sprites follow Black and White's species data instead (ARCID_PERSONAL_BW1, a/3/0/2,
    which MusicalMcss_GetDataIDBase reads), whose offsets differ and which has no source. Forms told apart only by
    their palette (palette_forms, Arceus) have no groups."""
    records = read_narc((EXTRACTED / "a/3/0/2").read_bytes()) if musical else None
    result = {}
    for number, species in enumerate(species_names()[:national_species()]):
        if musical:
            _, sprite_offset, count = struct.unpack_from("<HHB", records[number], 0x1C)
            palette_only = records[number][0x21] >> 7 & 1
        else:
            data = json.loads((ROOT / "data/pokemon" / species / "data.json").read_text())
            sprite_offset, count = data["forms"]["sprite_offset"], data["forms"]["count"]
            palette_only = data["palette_forms"]
        if count <= 1 or palette_only:
            continue
        for form in range(1, count):
            result[sprite_offset + form - 1] = (species, form)
    return result


# Each sprite archive's groups of files (20 to a battle or musical group, 2 to an icon group): the egg's, the next
# Manaphy's egg's, and the first form's. Before them, a group is a species' (in a/0/0/4, of the species two after it
# past the bad egg, which has no sprites of its own, GetPokemonDataIDBase)
GROUPS = {"battle": (683, 685), "icons": (683, 685), "musical": (650, 652)}


def group_folder(root, group, kind):
    egg, forms = GROUPS[kind]
    names = species_names()
    if group == egg:
        path = root / "egg"
    elif group == egg + 1:
        path = root / "manaphy" / "egg"
    elif group >= forms:
        if group - forms not in form_names(kind == "musical"):
            raise ValueError(f"{kind} group {group} is no species' form")
        species, form = form_names(kind == "musical")[group - forms]
        path = root / species / f"form_{form}"
    else:
        path = root / names[group + 2 if kind == "battle" and group >= national_species() else group]
    return path / "musical" if kind == "musical" else path


# The Substitute doll's files after the sprite groups, as BtlvMcss_SetSubstitute loads them: the front's and the
# back's, then the palette. The musical archive keeps Black and White's, which nothing in Black 2 and White 2 loads
SUBSTITUTE_SLOTS = ("front", "front_cells", "front_animations", "front_multi_cells", "front_multi_animations",
                    "front_info", "back", "back_cells", "back_animations", "back_multi_cells",
                    "back_multi_animations", "back_info", "normal")
EXTENSIONS = {"cells": ".ncer", "animations": ".nanr.lz11", "multi_cells": ".nmcr", "multi_animations": ".nmar",
              "info": ".bin"}


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
        folder = root / "shared" / "substitute"
        if musical:
            folder /= "musical"
        name = SUBSTITUTE_SLOTS[index - end]
        if name == "normal":
            return folder / "normal.pal", "palette", None
        if name in ("front", "back"):
            # The musical copy's sheets are single 96 by 96 images, as the musical sprites' are
            return (folder / f"{name}_single.png" if musical else folder / f"{name}.png"), "char", {
                "single_cell": musical, "linear": not musical}
        return folder / (name + EXTENSIONS[name.split("_", 1)[1]]), "raw", None
    group, slot = divmod(index, 20)
    folder = group_folder(root, group, "musical" if musical else "battle")
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


# The icons' shared files (poke_icon.c): the palettes, the second the PC boxes', and the animations and cells for 1D
# OBJ mappings of 32K, 64K and 128K
ICON_FILES = ("icon_palettes.pal", "icon_palettes_box.pal", "icon_animations_32k.nanr", "icon_cells_32k.ncer",
              "icon_animations_64k.nanr", "icon_cells_64k.ncer", "icon_animations_128k.nanr", "icon_cells_128k.ncer")


def icon_file(root, index):
    if index < len(ICON_FILES):
        path = root / "shared" / ICON_FILES[index]
        return (path, "palette", None) if path.suffix == ".pal" else (path, "raw", None)
    group, female = divmod(index - len(ICON_FILES), 2)
    path = group_folder(root, group, "icons") / ("icon_female.png" if female else "icon.png")
    if group == national_species():
        # SPECIES_EGG's own icon, a copy of SPECIES_NONE's: an egg shows the egg's, icon.png (PokeParty_GetIconIndex)
        path = path.with_name("species_icon_female.png" if female else "species_icon.png")
    palette_index = group if group < GROUPS["icons"][0] else group + 2
    table = icon_palettes()
    bank = (table[palette_index] >> (4 if female else 0)) & 15 if palette_index < len(table) else 0
    return path, "char", {"size": (32, 64), "palette_bank": bank}


# The footprints' shared files (zukan_info.c): the palette, then the animations and cells of the species up to
# Arceus, then those of the species from Victini
FOOTPRINT_FILES = ("footprint_palette.pal", "footprint_animations_old.nanr", "footprint_cells_old.ncer",
                   "footprint_animations.nanr", "footprint_cells.ncer")


def footprint_file(root, index, forms=False):
    if forms:
        species, form = FORM_FOOTPRINTS[index]
        return root / species / f"form_{form}" / "footprint.png", "char", {"size": (16, 32)}
    if index < len(FOOTPRINT_FILES):
        path = root / "shared" / FOOTPRINT_FILES[index]
        return (path, "palette", None) if path.suffix == ".pal" else (path, "raw", None)
    return root / species_names()[index - len(FOOTPRINT_FILES)] / "footprint.png", "char", {"size": (16, 32)}


def source_file(root, archive_name, index):
    kind = ARCHIVES[archive_name]
    if kind in ("battle", "musical"):
        return battle_file(root, index, kind == "musical")
    if kind == "icons":
        return icon_file(root, index)
    return footprint_file(root, index, kind == "footprint_forms")


def palette_member(kind, index):
    """Returns the file of a battle or musical sprite's palette: its group's normal.pal, or the Substitute's."""
    end = 14240 if kind == "musical" else 15020
    return end + SUBSTITUTE_SLOTS.index("normal") if index >= end else index // 20 * 20 + 18


def colors_for(root, archive_name, archive, index, options):
    kind = ARCHIVES[archive_name]
    if kind in ("battle", "musical"):
        return palette_colors(archive[palette_member(kind, index)])[:16]
    if kind == "icons":
        colors = palette_colors(archive[0])
        bank = options["palette_bank"]
        return colors[bank * 16:(bank + 1) * 16]
    if kind == "footprint_forms":
        archive = read_narc((EXTRACTED / "a/1/6/5").read_bytes())
    return palette_colors(archive[0])[:16]


def current_colors(root, archive_name, archive, index, options):
    """Returns the colors the game shows a sprite with: its palette's source file, as edited, or else the original's."""
    kind = ARCHIVES[archive_name]
    member, palette_archive = ((palette_member(kind, index), archive_name) if kind in ("battle", "musical") else
                               (0, "a/1/6/5") if kind == "footprint_forms" else (0, archive_name))
    path, source_kind, _ = source_file(root, palette_archive, member)
    if palette_archive == "a/0/5/1" and not path.exists() and path.parent.name == "musical":
        path = path.parent.parent / path.name
    if source_kind != "palette" or not path.exists():
        return colors_for(root, archive_name, archive, index, options)
    colors = read_palette(path)
    bank = options["palette_bank"] if kind == "icons" else 0
    return colors[bank * 16:(bank + 1) * 16]


def palette_indices(path, pixels, png_colors, colors):
    """Returns a PNG's pixels as indices of the sprite's palette: its own indices where the PNG's palette agrees with
    the sprite's, as exported, or else the sprite's index of the same color, where an editor reordered or compacted
    the PNG's palette on saving. Index 0 is transparent and stays 0."""
    def color5(color):
        return tuple(round(axis * 31 / 255) for axis in color)

    target = [color5(color) for color in colors]
    mapping = {0: 0}
    for i in sorted(set(pixels) - {0}):
        if i >= len(png_colors):
            raise ValueError(f"{path}: a pixel's index {i} is outside the PNG's palette")
        color = color5(png_colors[i])
        if i < len(target) and target[i] == color:
            mapping[i] = i
            continue
        matches = [j for j, other in enumerate(target) if j and other == color]
        if not matches:
            raise ValueError(f"{path}: the PNG's color {png_colors[i]} is not in the sprite's palette; change the "
                             "palette's colors in its .pal file")
        mapping[i] = matches[0]
    if all(i == j for i, j in mapping.items()):
        return pixels
    return bytes(mapping[i] for i in pixels)


def export(root):
    for archive_name in ARCHIVES:
        path = EXTRACTED / archive_name
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
    musical = read_narc((EXTRACTED / "a/0/5/1").read_bytes())
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
    global EXTRACTED
    EXTRACTED = ROOT / "extract" / {"black2": "b2_us", "white2": "w2_us"}[game] / "files"
    archive = read_narc((EXTRACTED / archive_name).read_bytes())
    result = []
    for index, original in enumerate(archive):
        path, kind, options = source_file(root, archive_name, index)
        if not original:
            # The game has no file here, such as a female sprite of a species that has none
            if path.exists():
                raise ValueError(f"{path}: the archive has no file for this, so it can't be packed")
            result.append(original)
            continue
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
            width, height, pixels, png_colors = read_indexed_png(path)
            pixels = palette_indices(path, pixels, png_colors, current_colors(root, archive_name, archive, index, options))
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
