#!/usr/bin/env python3
"""The Entralink's tables in res/field/entralink/: dump writes them from the ROM once, and pack builds the luminance
tables' archive (ARCID_FIELD_PALACE_SYS, a/2/1/9) and the Entree Forest's levels (ARCID_ENTREE_FOREST, a/2/2/3) from
them, which the build does.

    entralink_data.py dump extract/b2_us/files res/field/entralink
    entralink_data.py pack res/field/entralink LUMINANCE LEVELS

- luminance.json: the grayscale that the Entralink's worlds of another player draw the field in (field_palace_sys.c).
  FieldPalaceSys_InitPostFX picks a file by the area owner's version class (Intrude_GetAreaOwnerVersionClass) and
  whether the player has been to the world's town (FieldPalaceSys_CheckEventFlag): files 0 and 1 for class 2, 2 and 3
  for class 1, the second of each once the player has. A file is a list of PalaceLuminanceEntry, of which
  FieldPalaceSys_LoadLuminanceTable takes the first from the second on that matches the area and the season, or else
  the first; ColorFilter_ApplyLUT maps each color's luminance, 0 to 31, through its table to a gray of 0 to 31.
- forest_levels.json: the level of each species in the Entree Forest, which EntreeForest_CreateSymbolMon gives the
  Pokémon it makes (entree_forest.c), by its SPECIES_* constant.

Both versions have the same tables. entralink.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVES = {"luminance": "a/2/1/9", "forest_levels": "a/2/2/3"}
LUMINANCE = struct.Struct("<BBH32B")  # PalaceLuminanceEntry
ANY = 0xff  # PALACE_LUMINANCE_ANY
SEASONS = ["spring", "summer", "autumn", "winter"]


def luminance_json(data: bytes) -> list:
    if len(data) % LUMINANCE.size:
        raise ValueError("a luminance file isn't whole PalaceLuminanceEntry")
    entries = []
    for area, season, unk2, *table in LUMINANCE.iter_unpack(data):
        if season != ANY and season >= len(SEASONS) or any(gray > 31 for gray in table):
            raise ValueError("unexpected luminance entry values")
        entries.append({"area": None if area == ANY else area, "season": None if season == ANY else SEASONS[season],
                        "unk2": unk2, "table": table})
    return entries


def luminance_bytes(entries: list) -> bytes:
    return b"".join(LUMINANCE.pack(ANY if entry["area"] is None else entry["area"],
                                   ANY if entry["season"] is None else SEASONS.index(entry["season"]), entry["unk2"],
                                   *entry["table"]) for entry in entries)


def dump(files: Path, output: Path):
    files_luminance = read_narc((files / ARCHIVES["luminance"]).read_bytes())
    write(output / "luminance.json", {"$schema": "entralink.schema.json",
                                      "luminance": [luminance_json(data) for data in files_luminance]})
    levels = read_narc((files / ARCHIVES["forest_levels"]).read_bytes())
    if len(levels) != 1:
        raise ValueError("the Entree Forest's levels aren't one file")
    species = {number: name("SPECIES_", number) for number in range(len(levels[0]))}
    if any(not isinstance(constant, str) for constant in species.values()):
        raise ValueError("a species of the Entree Forest's levels has no constant")
    write(output / "forest_levels.json", {"$schema": "entralink.schema.json",
                                          "forest_levels": {species[i]: level for i, level in enumerate(levels[0])}})
    print(f"wrote {len(files_luminance)} luminance files and {len(levels[0])} species' levels to {output}")


def levels_bytes(levels: dict, where: str) -> bytes:
    numbers = {value(species, where): level for species, level in levels.items()}
    for species, level in levels.items():
        if not isinstance(level, int) or isinstance(level, bool) or not 0 <= level <= 100:
            raise DataError(f"{where}: {species}: {level!r} is not a level from 0 to 100")
    if sorted(numbers) != list(range(len(numbers))):
        raise DataError(f"{where}: the species are SPECIES_NONE and every one after it up to the last, once each")
    return bytes(numbers[i] for i in range(len(numbers)))


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "entralink.schema.json")
    luminance, levels = (load(root / f"{kind}.json", schema)[kind] for kind in ARCHIVES)
    for output, members in zip(outputs, ([luminance_bytes(entries) for entries in luminance],
                                         [levels_bytes(levels, label(root / "forest_levels.json"))])):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/entralink/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the Entralink's archives from res/field/entralink/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE",
                             help="the luminance tables' and the Entree Forest levels' archives")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
