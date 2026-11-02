#!/usr/bin/env python3
"""Black and White's species data (ARCID_PERSONAL_BW1, a/3/0/2), which poke_personal.c reads beside this game's, in
res/pokemon/black_white.json as the ways it differs from this game's records in res/pokemon/: dump writes it from the
ROM once, and pack builds the archive from it and the species' records, which the build does.

    personal_bw1_data.py dump extract/b2_us/files res/pokemon
    personal_bw1_data.py pack res/pokemon ARCHIVE

The archive is Black and White's records (PersonalData, PML_PersonalFetchBW1), which the musical reads the forms of
(musical_mcss.c), in their order:
- the species from SPECIES_NONE to Genesect, each the record of its data.json, or the species' "differences" in
  black_white.json; Black and White had one tutor, the type tutor, so a record ends after its first word of tutor
  moves, and SPECIES_NONE's before it;
- the forms of "form_records", each its form_<n>.json's record and its "differences", whose places give each species'
  first form record;
- Black and White's Unova Pokédex numbers of the species from SPECIES_NONE to Genesect, "regional_dex", the species in
  the Pokédex's order from #000, 999 for any other.

A difference is a field of data.json (species.schema.json describes them) with Black and White's value, or "by_tm",
the TMs and HMs that the species learns; a species' record changes with its data.json in what has no difference.
Both versions have the same archive. black_white.schema.json describes the fields.
"""
import argparse
import copy
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, validate, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.species_data import (RECORD, directory, national_species, record_bytes, record_json,  # noqa: E402
                                     tutor_moves)

ARCHIVE = "a/3/0/2"
# A record of Black and White's: this game's without the four words of the tutors after the type tutor's
RECORD_SIZE = RECORD.size - 16
NONE_RECORD_SIZE = RECORD_SIZE - 4
NOT_IN_DEX = 999
# The fields of a record, as data.json has them; by_tm is learnset's
FIELDS = ["base_stats", "types", "catch_rate", "evolution_stage", "ev_yields", "underground", "held_items",
          "gender_ratio", "hatch_cycles", "base_friendship", "growth_rate", "egg_groups", "abilities", "hidden_ability",
          "flee_rate", "forms", "color", "asymmetric", "palette_forms", "base_exp", "height", "weight", "by_tm"]


def record_fields(record: dict) -> dict:
    """Returns a record's fields, as differences name them."""
    fields = {field: record[field] for field in FIELDS if field != "by_tm"}
    fields["by_tm"] = record["learnset"]["by_tm"]
    return fields


def with_differences(data: dict, differences: dict) -> dict:
    merged = copy.deepcopy(data)
    for field, item in differences.items():
        if field == "by_tm":
            merged["learnset"]["by_tm"] = item
        else:
            merged[field] = item
    return merged


def bw1_record(data: dict, first_form: int, size: int, tutors: list[list[str]], where: str) -> bytes:
    """Returns Black and White's record of a species' data: this game's record, cut after the type tutor's word."""
    return record_bytes(data, first_form, tutors, where)[:size]


def first_forms(form_records: list[list], species_count: int) -> dict[str, int]:
    first: dict[str, int] = {}
    for i, (species, form) in enumerate(form_records):
        first.setdefault(species, species_count + i)
        if species_count + i != first[species] + form - 1:
            raise DataError(f"black_white.json: form_records: the forms of {species} aren't one after the other, "
                            "from 1")
    return first


def form_path(root: Path, species: str, form: int) -> Path:
    return root / directory(species) / f"form_{form}.json"


def species_names(count: int) -> list[str]:
    listed = sorted(load_list("species").items(), key=lambda item: item[1])
    return [species for species, _ in listed[:count]]


def dump(files: Path, root: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    count = national_species()
    tutors = tutor_moves()
    schema = load_schema(root / "species.schema.json")
    species = species_names(count)
    # The forms: the first of this game's form records, as many as there are records between the species' and the
    # Pokédex numbers
    form_records = json.loads((root / "forms.json").read_text())[:len(members) - count - 1]
    first = first_forms(form_records, count)
    entries = [(s, load(root / directory(s) / "data.json", schema), first.get(s, 0)) for s in species]
    entries += [(f"{s} form {f}", load(form_path(root, s, f), schema), 0) for s, f in form_records]
    differences, form_list = {}, []
    for i, ((what, data, first_form), member) in enumerate(zip(entries, members)):
        size = NONE_RECORD_SIZE if i == 0 else RECORD_SIZE
        if len(member) != size:
            sys.exit(f"{what}'s record is {len(member)} bytes, not {size}")
        original = record_fields(record_json(member + bytes(RECORD.size - len(member)), b"\xff" * 4, bytes(42),
                                             tutors))
        ours = record_fields(data)
        found = {field: original[field] for field in FIELDS if original[field] != ours[field]}
        if bw1_record(with_differences(data, found), first_form, size, tutors, what) != member:
            sys.exit(f"{what}'s record isn't this game's with its differences: its first form or tutor moves differ")
        if i < count:
            if found:
                differences[what] = found
        else:
            species_name, form = form_records[i - count]
            form_list.append({"species": species_name, "form": form, **({"differences": found} if found else {})})
    numbers = struct.unpack(f"<{count}H", members[-1])
    numbered = sorted((n, s) for n, s in zip(numbers, species) if n != NOT_IN_DEX)
    if [n for n, _ in numbered] != list(range(len(numbered))):
        sys.exit("Black and White's Pokédex numbers aren't 0 and on, one each")
    write(root / "black_white.json", {"$schema": "black_white.schema.json", "species": differences,
                                      "form_records": form_list, "regional_dex": [s for _, s in numbered]})
    print(f"wrote {len(differences)} species' and {len(form_list)} forms' differences to {root / 'black_white.json'}")


def pack(root: Path, output: Path):
    path = root / "black_white.json"
    table = load(path, load_schema(root / "black_white.schema.json"))
    where = label(path)
    schema = load_schema(root / "species.schema.json")
    count = national_species()
    tutors = tutor_moves()
    species = species_names(count)
    unknown = [s for s in table["species"] if s not in species]
    if unknown:
        raise DataError(f"{where}: species: {unknown[0]} is not a species before SPECIES_EGG")
    form_records = [[f["species"], f["form"]] for f in table["form_records"]]
    first = first_forms(form_records, count)
    members = []
    sources = [(s, root / directory(s) / "data.json", table["species"].get(s, {}), first.get(s, 0)) for s in species]
    sources += [(f"{f['species']} form {f['form']}", form_path(root, f["species"], f["form"]), f.get("differences", {}),
                 0) for f in table["form_records"]]
    for i, (what, data_path, differences, first_form) in enumerate(sources):
        data = with_differences(load(data_path, schema), differences)
        try:
            validate(data, schema, "")
        except DataError as error:
            raise DataError(f"{where}: {what}'s differences: {error}") from None
        members.append(bw1_record(data, first_form, NONE_RECORD_SIZE if i == 0 else RECORD_SIZE, tutors,
                                  f"{where}: {what}"))
    numbers = [NOT_IN_DEX] * count
    for number, species_name in enumerate(table["regional_dex"]):
        if species_name not in species or numbers[species.index(species_name)] != NOT_IN_DEX:
            raise DataError(f"{where}: regional_dex: {species_name} is not a species before SPECIES_EGG, or twice")
        numbers[species.index(species_name)] = number
    members.append(struct.pack(f"<{count}H", *numbers))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/pokemon/black_white.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("root", type=Path, help="res/pokemon, whose records it compares the archive's with")
    pack_parser = commands.add_parser("pack", help="build Black and White's species data")
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
