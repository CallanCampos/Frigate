#!/usr/bin/env python3
"""The battle facilities' trainers and Pokémon in data/facilities/<facility>/, as pokeplatinum's res/trainers/frontier/:
dump writes them from the ROM once, and pack builds a facility's archives from them, which the build does.

    facility_data.py dump extract/b2_us/files data/facilities battle_subway
    facility_data.py pack data/facilities/battle_subway TRAINERS POKEMON [SINGLE_SETS]

A facility has a trainers archive, each file a trainer class and the Pokémon sets the trainer picks from, and a
Pokémon archive, each file a set (BSubwayPokemonData in include/field/battle_facility.h); FACILITIES says which archives
and message files are a facility's. Its directory holds:

- trainers/<trainer>.json: a trainer's class, its sets by name, and its name and messages, which the text takes with
  \\from{facilities.<facility>.names} and \\from{facilities.<facility>.messages};
- pokemon/<set>.json: a set, named after its species and numbered from 1;
- order.json: the trainers and sets in archive order, which the files' names don't give.

facility.schema.json and set.schema.json, in data/facilities/, describe the fields.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import ROOT, DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.make_constants import identifier  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_data import message_lines  # noqa: E402
from tools.text.text_sources import to_json  # noqa: E402

SET = struct.Struct("<5HBBHH")
STATS = ["hp", "attack", "defense", "speed", "special_attack", "special_defense"]
DATA_DIR = ROOT / "data/facilities"
# Each facility: its trainers' and Pokémon archives, the system message files of its trainers' names and messages, how
# many messages a trainer has (0 for none), and whether the names are stored compressed. The Battle Subway's are also
# the Trial House's (func_ov012_02162864); the Black Tower's, White Treehollow in White 2, are the trainers' archive
# that func_ov127_021efeec reads, 0x106, and the Pokémon's, 0x105, with names from file 0x35
# The Pokémon World Tournament has three pools of trainers, which its tournaments pick by WbtTournamentInfo's unk5 from
# the base archives 248, 251 and 254 (wbt_tool.c): the base archive gives each trainer a single set, which
# func_ov134_021f0258 uses when the entrant's unk0_7 is set, the next one the sets to pick from, and the one after the
# sets. The pools' trainers have no names in the text, and their first field isn't a trainer class: it is 2 for all but
# the first, so it is kept as unk0 (classes False), and the trainers are numbered
FACILITIES = {
    "battle_subway": {"trainers": "a/2/1/2", "pokemon": "a/2/1/1", "names": 15, "messages": 376, "per_trainer": 3,
                      "compressed_names": False},
    "black_tower": {"trainers": "a/2/6/2", "pokemon": "a/2/6/1", "names": 53, "messages": None, "per_trainer": 0,
                    "compressed_names": True},
    # Driftveil, Download, Rental and Mix
    "pwt_regular": {"trainers": "a/2/4/9", "pokemon": "a/2/5/0", "single_sets": "a/2/4/8", "names": None,
                    "messages": None, "per_trainer": 0, "compressed_names": False, "classes": False},
    # Unova, Kanto, Johto, Hoenn and Sinnoh Leaders
    "pwt_leaders": {"trainers": "a/2/5/2", "pokemon": "a/2/5/3", "single_sets": "a/2/5/1", "names": None,
                    "messages": None, "per_trainer": 0, "compressed_names": False, "classes": False},
    # The rental Pokémon its Rental tournaments let the player pick from (wbt_party.c), sets without trainers
    "pwt_rental": {"trainers": None, "pokemon": "a/2/5/7", "names": None, "messages": None, "per_trainer": 0,
                   "compressed_names": False},
    # Champions, World Leaders, Type Expert, Rental Master and Mix Master
    "pwt_masters": {"trainers": "a/2/5/5", "pokemon": "a/2/5/6", "single_sets": "a/2/5/4", "names": None,
                    "messages": None, "per_trainer": 0, "compressed_names": False, "classes": False},
}


# Dumping


def set_json(data: bytes) -> dict:
    species, *moves, ev_flags, nature, item, form = SET.unpack(data)
    return {
        "$schema": "../../set.schema.json",
        "species": name("SPECIES_", species),
        "moves": [name("MOVE_", move) for move in moves],
        "ev_flags": {stat: bool(ev_flags >> i & 1) for i, stat in enumerate(STATS)},
        "nature": name("NATURE_", nature),
        "item": name("ITEM_", item),
        "form": form,
    }


def dump(files: Path, output: Path, facility: str):
    config = FACILITIES[facility]
    trainers = read_narc((files / config["trainers"]).read_bytes()) if config["trainers"] else []
    sets = read_narc((files / config["pokemon"]).read_bytes())
    root = output / facility
    set_names = []
    count: dict[str, int] = {}
    for data in sets:
        if data[10] >> len(STATS):
            sys.exit("a set has EV flags beyond the six stats")
        stem = identifier(name("SPECIES_", SET.unpack(data)[0]).removeprefix("SPECIES_")).lower()
        count[stem] = count.get(stem, 0) + 1
        set_names.append(f"{stem}_{count[stem]}")
        write(root / "pokemon" / f"{set_names[-1]}.json", set_json(data))
    names = ([line.removeprefix("\\c") for line in message_lines(files / "a/0/0/2", config["names"])]
             if config["names"] is not None else [])
    singles = read_narc((files / config["single_sets"]).read_bytes()) if config.get("single_sets") else None
    messages = message_lines(files / "a/0/0/2", config["messages"]) if config["per_trainer"] else []
    per = config["per_trainer"]
    trainer_names = []
    count = {}
    for index, data in enumerate(trainers):
        trainer_class, set_count, *picks = struct.unpack(f"<{len(data) // 2}H", data)
        if len(picks) != set_count:
            sys.exit(f"trainer {index} doesn't list its count of sets")
        classes = config.get("classes", True)
        class_name = name("TRAINER_CLASS_", trainer_class) if classes else trainer_class
        trainer = {"$schema": "../../facility.schema.json", "class" if classes else "unk0": class_name,
                   "pokemon": [set_names[pick] for pick in picks]}
        if singles is not None:
            single_class, single_count, single = struct.unpack("<3H", singles[index])
            if single_class != trainer_class or single_count != 1:
                sys.exit(f"trainer {index}'s single set isn't one set of its class")
            trainer["single_set"] = set_names[single]
        if index < len(names):
            trainer = {"$schema": trainer.pop("$schema"), "name": to_json(names[index]), **trainer}
            if per:
                trainer["messages"] = [to_json(m) for m in messages[per * index:per * (index + 1)]]
        class_stem = str(class_name).removeprefix("TRAINER_CLASS_").lower() if classes else "trainer"
        if names:
            base = f"{class_stem}_{identifier(names[index]).lower() if index < len(names) else index}"
            count[base] = count.get(base, 0) + 1
            trainer_names.append(base if count[base] == 1 else f"{base}_{count[base]}")
        else:
            # Without names, the trainers of a class are numbered from 1
            count[class_stem] = count.get(class_stem, 0) + 1
            trainer_names.append(f"{class_stem}_{count[class_stem]}")
        write(root / "trainers" / f"{trainer_names[-1]}.json", trainer)
    write(root / "order.json", {"trainers": trainer_names, "pokemon": set_names})
    print(f"wrote {len(trainers)} trainers and {len(sets)} sets to {root}")


# Packing


def load_facility(facility: str) -> tuple[list[tuple[str, dict]], list[tuple[str, dict]]]:
    """Returns the facility's trainers and sets in archive order, by name, validated."""
    root = DATA_DIR / facility
    order = json.loads((root / "order.json").read_text())
    trainer_schema = load_schema(DATA_DIR / "facility.schema.json")
    set_schema = load_schema(DATA_DIR / "set.schema.json")
    trainers = [(n, load(root / "trainers" / f"{n}.json", trainer_schema)) for n in order["trainers"]]
    sets = [(n, load(root / "pokemon" / f"{n}.json", set_schema)) for n in order["pokemon"]]
    return trainers, sets


def pack(facility: str, trainers_output: Path, pokemon_output: Path | None = None, singles_output: Path | None = None):
    trainers, sets = load_facility(facility)
    set_index = {set_name: i for i, (set_name, _) in enumerate(sets)}
    trainer_files = []
    single_files = []
    for trainer_name, data in trainers:
        where = label(DATA_DIR / facility / "trainers" / f"{trainer_name}.json")
        picks = []
        for set_name in data["pokemon"]:
            if set_name not in set_index:
                raise DataError(f"{where}: {set_name} is not a set of order.json")
            picks.append(set_index[set_name])
        if ("class" in data) == ("unk0" in data):
            raise DataError(f"{where}: a facility trainer has a class or, in the Pokémon World Tournament, unk0")
        first = value(data["class"], where) if "class" in data else data["unk0"]
        trainer_files.append(struct.pack(f"<{2 + len(picks)}H", first, len(picks), *picks))
        if singles_output is not None:
            if data.get("single_set") not in set_index:
                raise DataError(f"{where}: single_set is missing or not a set of order.json")
            single_files.append(struct.pack("<3H", first, 1, set_index[data["single_set"]]))
    set_files = []
    for set_name, data in sets:
        where = label(DATA_DIR / facility / "pokemon" / f"{set_name}.json")
        moves = [value(move, where) for move in data["moves"]]
        ev_flags = sum(1 << i for i, stat in enumerate(STATS) if data["ev_flags"][stat])
        set_files.append(SET.pack(value(data["species"], where), *moves, ev_flags, value(data["nature"], where),
                                  value(data["item"], where), data["form"]))
    if FACILITIES[facility]["trainers"] is None:
        # A facility of sets only: the one archive given is the sets'
        outputs = [(trainers_output, set_files)]
    else:
        outputs = [(trainers_output, trainer_files), (pokemon_output, set_files)]
    if singles_output is not None:
        outputs.append((singles_output, single_files))
    for output, members in outputs:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def text_lines(facility: str, kind: str) -> list[str]:
    """The names or messages of a facility's trainers that have them, which come first, for the text."""
    trainers, _ = load_facility(facility)
    prefix = "\\c" if FACILITIES[facility]["compressed_names"] and kind == "names" else ""
    lines = []
    for trainer_name, data in trainers:
        if "name" not in data:
            break
        lines += [data["name"]] if kind == "names" else data["messages"]
    return [prefix + line for line in lines]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write a facility's data from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    dump_parser.add_argument("facility", choices=FACILITIES)
    pack_parser = commands.add_parser("pack", help="build a facility's archives")
    pack_parser.add_argument("root", type=Path, help="the facility's directory, such as data/facilities/battle_subway")
    pack_parser.add_argument("outputs", type=Path, nargs="+", metavar="ARCHIVE",
                             help="the trainers and Pokémon archives, and the single sets' for a facility that has "
                                  "them; only the sets' for a facility of sets only")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output, args.facility)
        return
    try:
        pack(args.root.name, *args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
