#!/usr/bin/env python3
"""The Medal Rally's medals in res/medals/, a JSON file per archive: dump writes them from the ROM once, and pack builds
the two archives from them, which the build does.

    medal_data.py dump extract/b2_us/files res/medals
    medal_data.py pack res/medals MEDALS TYPE_SPECIES

- medals.json (ARCID_MEDAL_DEF, a/2/3/5): the MEDAL_COUNT medals, a file each (MedalDef in scrcmd_pokemon_center.c,
  MedalData in field/medal.h), in the order of their names in res/text/system/medal_names.txt, whose message each entry
  names: what earns it, a record that reaches a threshold or one of the special checks, and its type, kind and rank.
- type_species.json (ARCID_MEDAL_TYPE_SPECIES, a/2/3/9, one file of MedalTypeSpecies): the Pokémon whose types the
  type medals count, which PokemonCenter_CheckTypes reads, species and types by their constants.

Both versions have the same files. medals.schema.json and type_species.schema.json describe the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ARCHIVES = {"medals": "a/2/3/5", "type_species": "a/2/3/9"}
NAMES = ROOT / "res" / "text" / "system" / "medal_names.txt"

MEDAL_COUNT = 255
MEDAL = struct.Struct("<Ii4B")  # MedalDef
MEDAL_CHECK_FIRST = 10000  # a check from this is an index into sMedalChecks, below it a record
SPECIAL_CHECK_COUNT = 120  # sMedalChecks
TYPE_SPECIES_COUNT = 301  # MEDAL_TYPE_SPECIES_COUNT
TYPE_SPECIES = struct.Struct("<HBBB3x")  # MedalTypeSpecies, with its unused unk5[3]


def medal_names() -> list[str]:
    return [message for message, _ in message_numbers(NAMES)][:MEDAL_COUNT]


def medal_json(data: bytes, message: str) -> dict:
    if len(data) != MEDAL.size:
        raise ValueError(f"a medal is {len(data)} bytes, not {MEDAL.size}")
    threshold, check, unk8, medal_type, kind, rank = MEDAL.unpack(data)
    if not 0 <= check < MEDAL_CHECK_FIRST + SPECIAL_CHECK_COUNT:
        raise ValueError(f"a medal's check {check} is out of range")
    earned = {"record": check} if check < MEDAL_CHECK_FIRST else {"special": check - MEDAL_CHECK_FIRST}
    return {"name": message, **earned, "threshold": threshold, "type": medal_type, "kind": kind, "rank": rank,
            "unk8": unk8}


def medal_bytes(entry: dict, where: str) -> bytes:
    if ("record" in entry) == ("special" in entry):
        raise DataError(f"{where}: a medal has a record or a special check, not both or neither")
    check = entry["record"] if "record" in entry else MEDAL_CHECK_FIRST + entry["special"]
    return MEDAL.pack(entry["threshold"], check, entry["unk8"], entry["type"], entry["kind"], entry["rank"])


def type_species_json(data: bytes) -> list[dict]:
    if len(data) != TYPE_SPECIES.size * TYPE_SPECIES_COUNT:
        raise ValueError(f"the type medals' species are {len(data)} bytes")
    entries = []
    for offset in range(0, len(data), TYPE_SPECIES.size):
        species, type1, type2, counted = TYPE_SPECIES.unpack_from(data, offset)
        if counted > 1 or data[offset + 5:offset + TYPE_SPECIES.size] != bytes(3):
            raise ValueError("a type medal species' flag isn't a boolean or its padding isn't 0")
        entries.append({"species": name("SPECIES_", species), "types": [name("TYPE_", type1), name("TYPE_", type2)],
                        "counted": bool(counted)})
    return entries


def type_species_bytes(entries: list[dict], where: str) -> bytes:
    return b"".join(TYPE_SPECIES.pack(value(entry["species"], f"{where}: entry {i}"),
                                      *(value(t, f"{where}: entry {i}") for t in entry["types"]), entry["counted"])
                    for i, entry in enumerate(entries))


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVES["medals"]).read_bytes())
    if len(members) != MEDAL_COUNT:
        raise ValueError(f"{len(members)} medals, not {MEDAL_COUNT}")
    medals = [medal_json(data, message) for data, message in zip(members, medal_names())]
    write(output / "medals.json", {"$schema": "medals.schema.json", "medals": medals})
    (species,) = read_narc((files / ARCHIVES["type_species"]).read_bytes())
    write(output / "type_species.json", {"$schema": "type_species.schema.json",
                                          "species": type_species_json(species)})
    print(f"wrote {len(medals)} medals and the type medals' species to {output}")


def pack(root: Path, medals_out: Path, species_out: Path):
    path = root / "medals.json"
    medals = load(path, load_schema(root / "medals.schema.json"))["medals"]
    names = medal_names()
    for i, entry in enumerate(medals):
        if entry["name"] != names[i]:
            raise DataError(f"{label(path)}: medal {i} is named {entry['name']}, but its name in "
                            f"{label(NAMES)} is {names[i]}")
    path_species = root / "type_species.json"
    species = load(path_species, load_schema(root / "type_species.schema.json"))["species"]
    for output in (medals_out, species_out):
        output.parent.mkdir(parents=True, exist_ok=True)
    medals_out.write_bytes(write_narc([medal_bytes(entry, f"{label(path)}: medal {i}")
                                       for i, entry in enumerate(medals)]))
    species_out.write_bytes(write_narc([type_species_bytes(species, label(path_species))]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/medals/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the medals' archives")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("medals", type=Path)
    pack_parser.add_argument("type_species", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.medals, args.type_species)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
