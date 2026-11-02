#!/usr/bin/env python3
"""Which walking Pokémon's object code each species, sex and form uses (ARCID_TPOKE, a/2/0/8), in the object codes'
directories of res/field/actors/pokemon/: dump writes each code's pokemon.json from the ROM once, and pack builds the
archive from them, which the build does.

    walking_pokemon_data.py dump extract/b2_us/files res/field/actors
    walking_pokemon_data.py pack res/field/actors ARCHIVE

The archive's one file is a table (TPokeEntry in tpoke_data.c) of species, sex and form, each with the object code
whose actor shows it, which GetPokemonFieldOBJCODE looks up for the Entree Forest, the Hidden Grottoes and Game Sync:
the first entry of the species and form whose sex is the Pokémon's, or either. pokemon/<code>/pokemon.json is the entry
of OBJ_CODE_POKEMON_<CODE>, and the table has the codes that have one in the order of generated/obj_codes.txt. Both
versions have the same table. walking_pokemon.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.field_actor_data import directory  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/0/8"
ENTRY = struct.Struct("<4H")
# An entry's sex: GENDER_MALE, GENDER_FEMALE, or TPOKE_SEX_ANY for either, which the file writes as null
SEXES = ["GENDER_MALE", "GENDER_FEMALE", None]
FILE_NAME = "pokemon.json"
CODE_PREFIX = "OBJ_CODE_POKEMON_"


def walking_codes() -> list[str]:
    codes = sorted(load_list("obj_codes").items(), key=lambda item: item[1])
    return [code for code, _ in codes if code.startswith(CODE_PREFIX)]


def dump(files: Path, root: Path):
    data = read_narc((files / ARCHIVE).read_bytes())
    if len(data) != 1 or len(data[0]) % ENTRY.size:
        sys.exit(f"{ARCHIVE} isn't one table of {ENTRY.size}-byte entries")
    entries = [ENTRY.unpack_from(data[0], i) for i in range(0, len(data[0]), ENTRY.size)]
    codes = [name("OBJ_CODE_", code) for *_, code in entries]
    if codes != [code for code in walking_codes() if code in codes]:
        sys.exit("the table's object codes aren't walking Pokémon's, once each in the list's order")
    for (species, sex, form, _), code in zip(entries, codes):
        if sex >= len(SEXES):
            sys.exit(f"{code}'s entry has an unknown sex, {sex}")
        write(directory(root, code) / FILE_NAME,
              {"$schema": "../../walking_pokemon.schema.json", "species": name("SPECIES_", species), "sex": SEXES[sex],
               "form": form})
    print(f"wrote {len(entries)} walking Pokémon's entries to {root / 'pokemon'}")


def pack(root: Path, output: Path):
    schema = load_schema(root / "walking_pokemon.schema.json")
    table = b""
    for code in walking_codes():
        path = directory(root, code) / FILE_NAME
        if path.exists():
            entry, where = load(path, schema), label(path)
            table += ENTRY.pack(value(entry["species"], where), SEXES.index(entry["sex"]), entry["form"],
                                value(code, where))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([table]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write the walking Pokémon's pokemon.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("root", type=Path, help="res/field/actors")
    pack_parser = commands.add_parser("pack", help="build the walking Pokémon's table")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.root)
        return
    try:
        pack(args.root, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
