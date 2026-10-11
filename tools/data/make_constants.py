#!/usr/bin/env python3
"""Write the constant lists in data/constants/ for moves, abilities, items, species and types, named after the game's
own text, for sound sequences, named after the sound archive's symbols, and for trainer classes. This bootstraps the
lists from the ROM once; after that the committed lists are the source of truth, edited by hand, and the build
generates the headers from them with gen_constants.py.

    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --sdat extract/b2_us/files/swan_sound_data.sdat
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --trainer-classes extract/b2_us
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --trainers extract/b2_us
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --zones extract/b2_us
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --encounters extract/b2_us
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --trades extract/b2_us
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --places
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --files
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --flags
    make_constants.py extract/b2_us/files/a/0/0/2 data/constants --map-matrices extract/b2_us

Names are the English names in upper case, with words split at spaces, hyphens and capitals inside a word, so that
"ThunderPunch" becomes MOVE_THUNDER_PUNCH. Items named "???" are unused, and are named after their ID, as
ITEM_UNUSED_113, so that each has a constant and a data file.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.text.msgdata import read_archive_file  # noqa: E402

# (list, prefix, file in the system message archive, number of IDs or None for all lines, description)
TABLES = [
    ("moves.txt", "MOVE", 403, None, "Moves"),
    ("abilities.txt", "ABILITY", 374, None, "Abilities"),
    ("items.txt", "ITEM", 64, None, "Items"),
    # The lines after the national Pokédex name eggs and other things that are not species
    ("species.txt", "SPECIES", 90, 650, "Species, by national Pokédex number"),
    ("types.txt", "TYPE", 398, None, "Types"),
    ("natures.txt", "NATURE", 27, None, "Natures"),
]

# Names for the IDs whose text is not a name, or is shared with another ID. The item descriptions tell them apart.
OVERRIDES = {
    "MOVE": {0: "NONE"},
    "ABILITY": {0: "NONE"},
    "ITEM": {
        0: "NONE",
        621: "XTRANSCEIVER_MALE",
        626: "XTRANSCEIVER_FEMALE",
        628: "DNA_SPLICERS_FUSE",
        629: "DNA_SPLICERS_SEPARATE",
        636: "DROPPED_ITEM_MALE",
        637: "DROPPED_ITEM_FEMALE",
    },
    "SPECIES": {0: "NONE"},
}

# The font draws the female and male signs with these characters
CHARACTERS = {"é": "e", "⑮": "_F", "⑭": "_M"}


def identifier(name: str) -> str:
    for char, replacement in CHARACTERS.items():
        name = name.replace(char, replacement)
    name = re.sub(r"(?<=[a-z])(?=[A-Z])", "_", name)
    name = re.sub(r"['.]", "", name)
    name = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_")
    return name.upper()


def write_list(path: Path, names: dict[int, str], description: str, hexadecimal: bool = False):
    """Writes a gen_constants.py list: a value is only written where it does not follow the previous name's."""
    lines = [f"# {description} (bootstrapped from the ROM by make_constants.py)"]
    previous = None
    for i, name in sorted(names.items()):
        value = f" = {i:#x}" if hexadecimal else f" = {i}"
        lines.append(f"{name}{'' if previous is not None and i == previous + 1 else value}")
        previous = i
    path.write_text("\n".join(lines) + "\n")


def sound_sequence_names(sdat: bytes) -> list[str | None]:
    """Returns the names of the sequences in a sound archive's symbol block, by sequence number."""
    symb_offset, symb_size = struct.unpack_from("<II", sdat, 0x10)
    symb = sdat[symb_offset:symb_offset + symb_size]
    record = struct.unpack_from("<I", symb, 8)[0]
    names = []
    for i in range(struct.unpack_from("<I", symb, record)[0]):
        offset = struct.unpack_from("<I", symb, record + 4 + 4 * i)[0]
        names.append(symb[offset:symb.index(b"\0", offset)].decode() if offset else None)
    return names


def write_sound_list(path: Path, sdat: bytes):
    names = {i: name for i, name in enumerate(sound_sequence_names(sdat)) if name}
    write_list(path, names, "Sound sequences, with the names that the sound archive's symbols give them",
               hexadecimal=True)


# Files of the system message archive with the trainers' names and the trainer classes' names
TRAINER_NAMES = 382
TRAINER_CLASS_NAMES = 383
# Black 2's table of trainer classes that TrainerClass_GetSex reads, 4 bytes per class with the sex in the second
TRAINER_CLASS_TABLE = 0x02092394


def trainer_class_names(extract: Path) -> dict[int, str]:
    """Returns the name of each trainer class. A class name the game uses for several classes gets the name of the one
    trainer of the class, then the sex the game gives the class (_M or _F), then the class ID, until it is unique."""
    import yaml
    from tools.data.narc import read_narc

    files = extract / "files"
    classes = [identifier(n.replace("⒆⒇", "Pkmn")) or "NONE" for n in
               read_archive_file(files / "a/0/0/2", TRAINER_CLASS_NAMES)]
    trainer_names = read_archive_file(files / "a/0/0/2", TRAINER_NAMES)
    users: dict[int, set[str]] = {}
    for i, trainer in enumerate(read_narc((files / "a/0/9/1").read_bytes())):
        if len(trainer) == 20:
            if identifier(trainer_names[i]):
                users.setdefault(trainer[1], set()).add(identifier(trainer_names[i]))
    arm9 = (extract / "arm9" / "arm9.bin").read_bytes()
    base = yaml.safe_load((extract / "arm9" / "arm9.yaml").read_text())["base_address"]
    sexes = [arm9[TRAINER_CLASS_TABLE - base + 4 * c + 1] for c in range(len(classes))]

    names = dict(enumerate(classes))

    def duplicates():
        groups: dict[str, list[int]] = {}
        for c, name in names.items():
            groups.setdefault(name, []).append(c)
        return [group for group in groups.values() if len(group) > 1]

    for group in duplicates():
        for c in group:
            if len(users.get(c, ())) == 1:
                names[c] += "_" + next(iter(users[c]))
    for group in duplicates():
        if len({sexes[c] for c in group}) > 1:
            for c in group:
                names[c] += "_F" if sexes[c] else "_M"
    for group in duplicates():
        for c in group:
            names[c] += f"_{c}"
    return names


# The class of the rivals and other story characters, which their names say enough without
GENERIC_CLASS = "PKMN_TRAINER"


def trainer_names(extract: Path) -> dict[int, str]:
    """Returns the name of each trainer: its class and its name, as TRAINER_YOUNGSTER_JIMMY, without the class for
    story characters (TRAINER_CHEREN), and with the ID for one without a name. Trainers with the same class and name,
    such as rematches, get _2, _3 and so on in ID order."""
    from tools.data.narc import read_narc

    files = extract / "files"
    classes = [identifier(n.replace("⒆⒇", "Pkmn")) for n in read_archive_file(files / "a/0/0/2", TRAINER_CLASS_NAMES)]
    texts = read_archive_file(files / "a/0/0/2", TRAINER_NAMES)
    names: dict[int, str] = {}
    count: dict[str, int] = {}
    for i, trainer in enumerate(read_narc((files / "a/0/9/1").read_bytes())):
        if i == 0:
            names[i] = "TRAINER_NONE"
            continue
        trainer_class = classes[trainer[1]]
        name = identifier(texts[i])
        parts = [] if trainer_class == GENERIC_CLASS and name else [trainer_class]
        parts.append(name or str(i))
        base = "TRAINER_" + "_".join(parts)
        count[base] = count.get(base, 0) + 1
        names[i] = base if count[base] == 1 else f"{base}_{count[base]}"
    if len(set(names.values())) != len(names):
        sys.exit("two trainers have the same name")
    return names


# Zones whose names the code shows, which the zone headers alone don't: the Victory Road that Escape Rope leads out
# of (the zones from 214 are Black and White's), and the Union Room, whose place name is a placeholder
ZONE_OVERRIDES = {0x1A6: "UNION_ROOM", 0x23D: "VICTORY_ROAD"}
# Music that tells what a zone is, for the zones that share a place name
ZONE_KINDS = {"SEQ_BGM_POKECEN": "POKEMON_CENTER", "SEQ_BGM_GATE": "GATE", "SEQ_BGM_LABO": "LAB"}
ZONE_HEADER_SIZE = 0x30
PLACE_NAMES = 109


def zone_names(extract: Path) -> dict[int, str]:
    """Returns the name of each zone, after its place name. Of the zones that share a place name, the one the player
    can fly from, or else the first, gets the place name alone; a Pokémon Center, gate, lab or gym, told by its music,
    gets the place name and what it is; and the rest are numbered from _2 in ID order. A zone whose place name is a
    placeholder is named after its ID."""
    from tools.data.narc import read_narc
    from tools.data.gen_constants import constant_names

    files = extract / "files"
    (data,) = read_narc((files / "a/0/1/2").read_bytes())
    places = read_archive_file(files / "a/0/0/2", PLACE_NAMES)
    sequences = constant_names("sound.h", "SEQ_")
    zones = []
    for zone in range(len(data) // ZONE_HEADER_SIZE):
        fields = struct.unpack_from("<BBHHHHH4HHHHHHHHHiii", data, ZONE_HEADER_SIZE * zone)
        spring, place, flags = fields[7], fields[14] & 0x3FF, fields[16]
        text = places[place]
        zones.append((zone, identifier(text) if "[" not in text else "", sequences.get(spring, ""), flags >> 13 & 1))

    names = {zone: f"ZONE_{name}" for zone, name in ZONE_OVERRIDES.items()}
    groups: dict[str, list] = {}
    for zone in zones:
        if zone[0] not in names:
            if zone[1]:
                groups.setdefault(zone[1], []).append(zone)
            else:
                names[zone[0]] = f"ZONE_{zone[0]}"
    taken = set(names.values())
    for place, group in groups.items():
        rest = list(group)
        if f"ZONE_{place}" not in taken:
            flying = [zone for zone in group if zone[3]]
            main = flying[0] if len(flying) == 1 else group[0]
            names[main[0]] = f"ZONE_{place}"
            rest.remove(main)
        count: dict[str, int] = {}
        for zone, _, sequence, _ in rest:
            kind = ZONE_KINDS.get(sequence) or ("GYM" if "_GYM" in sequence else "")
            base = f"ZONE_{place}_{kind}" if kind else f"ZONE_{place}"
            count[base] = count.get(base, 0) + 1
            # The place name alone is the main zone's, so the other zones start at _2
            number = count[base] + (0 if kind else 1)
            names[zone] = base if number == 1 else f"{base}_{number}"
    if len(set(names.values())) != len(names):
        sys.exit("two zones have the same name")
    return dict(sorted(names.items()))


def encounter_names(extract: Path) -> dict[int, str]:
    """Returns the name of each wild encounter table, after the place of the first zone whose header names it,
    numbered from _2 where places repeat."""
    from tools.data.narc import read_narc

    files = extract / "files"
    (data,) = read_narc((files / "a/0/1/2").read_bytes())
    places = read_archive_file(files / "a/0/0/2", PLACE_NAMES)
    tables = len(read_narc((files / "a/1/2/7").read_bytes()))
    table_places: dict[int, str] = {}
    for zone in range(len(data) // ZONE_HEADER_SIZE):
        fields = struct.unpack_from("<BBHHHHH4HHHHHHHHHiii", data, ZONE_HEADER_SIZE * zone)
        table, place = fields[11] & 0x1FFF, fields[14] & 0x3FF
        if table < tables:
            table_places.setdefault(table, identifier(places[place]))
    names: dict[int, str] = {}
    count: dict[str, int] = {}
    for table in range(tables):
        base = f"ENCOUNTERS_{table_places.get(table) or 'UNUSED'}"
        count[base] = count.get(base, 0) + 1
        names[table] = base if count[base] == 1 else f"{base}_{count[base]}"
    return names


def trade_names(extract: Path) -> dict[int, str]:
    """Returns the name of each in-game trade, after the species it offers, numbered from _2 where they repeat."""
    from tools.data.gen_constants import load
    from tools.data.narc import read_narc

    species = {number: name for name, number in reversed(load("species").items())}
    names: dict[int, str] = {}
    count: dict[str, int] = {}
    for i, offer in enumerate(read_narc((extract / "files/a/1/6/3").read_bytes())):
        base = "TRADE_" + species[struct.unpack_from("<I", offer, 4)[0]].removeprefix("SPECIES_")
        count[base] = count.get(base, 0) + 1
        names[i] = base if count[base] == 1 else f"{base}_{count[base]}"
    return names


def place_names(archive: Path) -> dict[int, str]:
    """Returns the name of each place, after its name in the text, numbered from _2 where names repeat; a place whose
    name is a placeholder or made of codes is named after its number, but the first is PLACE_NONE."""
    names: dict[int, str] = {0: "PLACE_NONE"}
    count: dict[str, int] = {}
    for i, text in enumerate(read_archive_file(archive, PLACE_NAMES)):
        if i == 0:
            continue
        base = f"PLACE_{identifier(text)}" if re.fullmatch(r"[\w .'’é-]+", text) else f"PLACE_{i}"
        count[base] = count.get(base, 0) + 1
        names[i] = base if count[base] == 1 else f"{base}_{count[base]}"
    return names


def file_names() -> dict[str, dict[int, str]]:
    """Returns the names of the field script files and of the system and script text banks, from what uses them: a
    zone's scripts and map scripts (init) file and its text bank after the zone, a global script file and its text bank
    after the first script ID of its range, and the rest after the names of their text files, or their numbers."""
    import json

    root = Path(__file__).parents[2]
    zone_files = {}
    for path in (root / "data/zones").glob("*.json"):
        if path.name.endswith("schema.json"):
            continue
        zone = json.loads(path.read_text())
        zone_files[path.stem] = (zone["scripts"], zone.get("init_scripts", zone["scripts"] + 1), zone["text"])
    scripts: dict[int, str] = {}
    script_text: dict[int, str] = {}
    for zone, (file, init, text) in zone_files.items():
        scripts[file] = f"SCRIPTS_{zone.upper()}"
        scripts[init] = f"SCRIPTS_INIT_{zone.upper()}"
        script_text[text] = f"SCRIPT_TEXT_{zone.upper()}"
    for entry in json.loads((root / "tools/script/field_commands.json").read_text())["global_scripts"]:
        scripts[entry["file"]] = f"SCRIPTS_GLOBAL_{entry['first']}"
        script_text.setdefault(entry["text_file"], f"SCRIPT_TEXT_GLOBAL_{entry['first']}")
    field_scripts = sorted(int(path.stem[:4]) for path in (root / "data/field_scripts").glob("*.s"))
    for number in field_scripts:
        scripts.setdefault(number, f"SCRIPTS_{number:04d}")

    def by_file_names(directory: Path, prefix: str, names: dict[int, str]) -> dict[int, str]:
        count: dict[str, int] = {}
        for name_ in names.values():
            count[name_] = 1
        for path in sorted(directory.glob("*.txt")):
            number = int(path.name[:4])
            if number in names:
                continue
            stem = path.stem[5:]
            base = f"{prefix}{stem.upper()}" if stem else f"{prefix}{number:04d}"
            count[base] = count.get(base, 0) + 1
            names[number] = base if count[base] == 1 else f"{base}_{count[base]}"
        return dict(sorted(names.items()))

    return {
        "field_scripts": dict(sorted(scripts.items())),
        "script_text_banks": by_file_names(root / "data/text/script", "SCRIPT_TEXT_", script_text),
        "text_banks": by_file_names(root / "data/text/system", "TEXT_BANK_", {}),
    }


# The ranges of the event flags and variables, from EventWork_GetFlagBytePtr (0x17f bytes of saved flags, then 8 of
# temporary ones from 0x4000), TrainerFlagGet (0x5f0 + the trainer), EventWork_ResetDailyFlags (0xaa0 to 0xb03),
# HIDDEN_ITEM_FLAG_START (0xb04) and EventWork_GetWkPtr (the variables from 0x4000 before the flags, at 0x35e)
FLAGS_END, TRAINER_FLAGS, DAILY_FLAGS, HIDDEN_ITEM_FLAGS = 0xBF8, 0x5F0, (0xAA0, 0xB03), 0xB04
TEMP_FLAGS, TEMP_FLAG_COUNT, VARS, VAR_COUNT = 0x4000, 64, 0x4000, 0x35E // 2


def flag_names(named: dict[int, str]) -> dict[int, str]:
    """Returns a name for every event flag: the named ones, the trainers' (EVENT_FLAG_TRAINER_<trainer>), the hidden
    items' and the temporary ones by their number, and the rest, the daily ones among them, by their value."""
    from tools.data.gen_constants import load

    trainers = {number: trainer for trainer, number in reversed(load("trainers").items())}
    names = {}
    for flag in range(1, FLAGS_END):
        if flag in named:
            names[flag] = named[flag]
        elif TRAINER_FLAGS <= flag < TRAINER_FLAGS + len(trainers):
            names[flag] = "EVENT_FLAG_" + trainers[flag - TRAINER_FLAGS]
        elif DAILY_FLAGS[0] <= flag <= DAILY_FLAGS[1]:
            names[flag] = f"EVENT_FLAG_DAILY_{flag:#06x}"
        elif flag >= HIDDEN_ITEM_FLAGS:
            names[flag] = f"EVENT_FLAG_HIDDEN_ITEM_{flag - HIDDEN_ITEM_FLAGS}"
        else:
            names[flag] = f"EVENT_FLAG_{flag:#06x}"
    for flag in range(TEMP_FLAGS, TEMP_FLAGS + TEMP_FLAG_COUNT):
        names[flag] = named.get(flag, f"EVENT_FLAG_TEMP_{flag - TEMP_FLAGS}")
    return names


def var_names(named: dict[int, str]) -> dict[int, str]:
    return {var: named.get(var, f"EVENT_WORK_{var:#06x}") for var in range(VARS, VARS + VAR_COUNT)}


def map_matrix_names(extract: Path) -> dict[int, str]:
    """Returns the name of each map matrix: the overworld's, which give each cell's zone, MAP_MATRIX_OVERWORLD, and
    the others after the first zone whose header names them, or else their number."""
    from tools.data.narc import read_narc

    files = extract / "files"
    (headers,) = read_narc((files / "a/0/1/2").read_bytes())
    count = len(read_narc((files / "a/0/0/9").read_bytes()))
    zones = {number: zone for zone, number in reversed(load_zone_list().items())}
    first: dict[int, str] = {}
    for z in range(len(headers) // ZONE_HEADER_SIZE):
        matrix = struct.unpack_from("<H", headers, ZONE_HEADER_SIZE * z + 4)[0]
        # A zone named after its number keeps its prefix, so as not to read as a matrix's number
        zone = zones[z].removeprefix("ZONE_")
        first.setdefault(matrix, f"ZONE_{zone}" if zone.isdigit() else zone)
    names = {m: f"MAP_MATRIX_{first[m]}" if m in first else f"MAP_MATRIX_{m}" for m in range(count)}
    # The matrices that give each cell's zone are the overworld's, which many zones share
    overworld = [m for m, data in enumerate(read_narc((files / "a/0/0/9").read_bytes())) if data[0] == 1]
    for i, m in enumerate(overworld):
        names[m] = "MAP_MATRIX_OVERWORLD" if i == 0 else f"MAP_MATRIX_OVERWORLD_{i + 1}"
    return names


def load_zone_list() -> dict[str, int]:
    from tools.data.gen_constants import load

    return load("zones")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("archive", type=Path, help="the system message archive, files/a/0/0/2")
    parser.add_argument("output", type=Path, help="the directory for the lists, data/constants")
    parser.add_argument("--sdat", type=Path, help="the sound archive, files/swan_sound_data.sdat, for sound.txt")
    parser.add_argument("--trainer-classes", type=Path, metavar="EXTRACT",
                        help="write only trainer_classes.txt, from an extracted version such as extract/b2_us")
    parser.add_argument("--trainers", type=Path, metavar="EXTRACT",
                        help="write only trainers.txt, from an extracted version such as extract/b2_us")
    parser.add_argument("--only", metavar="LIST",
                        help="write only this list of the game's text, such as natures, since the lists are edited "
                             "by hand after")
    parser.add_argument("--places", action="store_true", help="write only places.txt, from the place names")
    parser.add_argument("--map-matrices", type=Path, metavar="EXTRACT",
                        help="write only map_matrices.txt, from an extracted version such as extract/b2_us")
    parser.add_argument("--flags", action="store_true",
                        help="write only flags.txt and vars.txt, every flag and variable, keeping the names they have")
    parser.add_argument("--files", action="store_true",
                        help="write only field_scripts.txt, script_text_banks.txt and text_banks.txt, from the zones, "
                             "the global scripts and the names of the files in data/")
    parser.add_argument("--trades", type=Path, metavar="EXTRACT",
                        help="write only trades.txt, from an extracted version such as extract/b2_us")
    parser.add_argument("--encounters", type=Path, metavar="EXTRACT",
                        help="write only encounters.txt, from an extracted version such as extract/b2_us")
    parser.add_argument("--zones", type=Path, metavar="EXTRACT",
                        help="write only zones.txt, from an extracted version such as extract/b2_us")
    args = parser.parse_args()
    if args.trainer_classes:
        names = {c: f"TRAINER_CLASS_{name}" for c, name in trainer_class_names(args.trainer_classes).items()}
        write_list(args.output / "trainer_classes.txt", names, "Trainer classes, told apart by trainer, sex or ID")
    if args.trainers:
        write_list(args.output / "trainers.txt", trainer_names(args.trainers),
                   "Trainers, by class and name, numbered where they repeat")
    if args.zones:
        write_list(args.output / "zones.txt", zone_names(args.zones),
                   "Zones, by place name, then by what they are or numbered where places repeat")
    if args.encounters:
        write_list(args.output / "encounters.txt", encounter_names(args.encounters),
                   "Wild encounter tables, by the place of the first zone that has them, numbered where places repeat")
    if args.map_matrices:
        write_list(args.output / "map_matrices.txt", map_matrix_names(args.map_matrices),
                   "Map matrices: the overworld's, and the others by the first zone whose header names them, or their number")
        return
    if args.flags:
        from tools.data.gen_constants import load

        named_flags = {v: k for k, v in load("flags").items() if not k.startswith(("EVENT_FLAG_0x", "EVENT_FLAG_TRAINER_",
                       "EVENT_FLAG_DAILY_0x", "EVENT_FLAG_HIDDEN_ITEM_", "EVENT_FLAG_TEMP_"))}
        named_vars = {v: k for k, v in load("vars").items() if not k.startswith("EVENT_WORK_0x")}
        write_list(args.output / "flags.txt", flag_names(named_flags),
                   "Event flags: the saved ones from 1, with the trainers' from 0x5f0, the daily ones from 0xaa0 and "
                   "the hidden items' from 0xb04, then the temporary ones from 0x4000. Named ones keep their names; "
                   "rename the others when the code or a script shows what they are", hexadecimal=True)
        write_list(args.output / "vars.txt", var_names(named_vars),
                   "Event variables, from 0x4000; rename one when the code or a script shows what it is",
                   hexadecimal=True)
        return
    if args.files:
        descriptions = {
            "field_scripts": "Field script files: each zone's scripts and map scripts, by the zone, and the global "
                             "scripts, by the first script ID of their range",
            "script_text_banks": "Script message files (text archive a/0/0/3): each zone's, by the zone, the global "
                                 "scripts', and the others by their names or numbers",
            "text_banks": "System message files (text archive a/0/0/2), by what they hold, or by their numbers until "
                          "that is known",
        }
        for list_name, names in file_names().items():
            write_list(args.output / f"{list_name}.txt", names, descriptions[list_name])
        return
    if args.places:
        write_list(args.output / "places.txt", place_names(args.archive),
                   "Places, the place names that zone headers name, numbered where names repeat")
        return
    if args.trades:
        write_list(args.output / "trades.txt", trade_names(args.trades),
                   "In-game trades, by the species offered, numbered where they repeat")
    if args.trainer_classes or args.trainers or args.zones or args.encounters or args.trades:
        return
    if args.sdat and not args.only:
        write_sound_list(args.output / "sound.txt", args.sdat.read_bytes())

    for list_file, prefix, file, count, description in TABLES:
        if args.only and list_file != f"{args.only}.txt":
            continue
        lines = read_archive_file(args.archive, file)[:count]
        overrides = OVERRIDES.get(prefix, {})
        names = {}
        for i, text in enumerate(lines):
            if i in overrides:
                names[i] = f"{prefix}_{overrides[i]}"
            elif text == "???":
                names[i] = f"{prefix}_UNUSED_{i}"
            else:
                names[i] = f"{prefix}_{identifier(text)}"
        seen = {}
        for i, name in names.items():
            if name in seen:
                sys.exit(f"{name} is both {seen[name]} and {i}, add an override")
            seen[name] = i
        write_list(args.output / list_file, names, description)


if __name__ == "__main__":
    main()
