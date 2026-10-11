#!/usr/bin/env python3
"""The species data in data/pokemon/, one JSON file per species, form and extra record, as pokeplatinum's res/pokemon/:
dump writes them from the ROM once, and pack builds the archives from them, which the build does.

    species_data.py dump extract/b2_us/files data/pokemon
    species_data.py pack data/pokemon PERSONAL LEVELUP EVOLUTIONS BABIES GROWTH EGG_MOVES

A record holds what the game keeps in four archives: the species data (a/0/1/6), its level-up moves (a/0/1/8), its
evolutions (a/0/1/9) and its baby species (a/0/2/0), and a species' file its egg moves too (a/1/2/4).
species.schema.json describes each field. The records are, in order:

- the species of data/constants/species.txt, in data/pokemon/<species>/data.json, from SPECIES_NONE in none/;
- the extra records, which no species' forms point at, in data/pokemon/extra/<record>.json, in order of their numbers;
- the forms that have records of their own, in data/pokemon/<species>/form_<n>.json, in the order of
  data/pokemon/forms.json. A species' first form record is set from where its forms are.

The species data archive ends with the Unova Pokédex numbers of the species and extra records, which their files hold
as regional_dex_number; the baby species archive has no entries for the forms.

A species' egg moves (a/1/2/4, one entry per species, a count and the moves) are its learnset's egg_moves;
SPECIES_NONE's entry is empty, so its file has none, and forms and extra records have none.

The experience tables of the growth rates (a/0/1/7) are data/pokemon/growth_rates.csv: a row per level from 0 to 100
and a column per table, in archive order, each headed by its GROWTH_* constant. The two tables after the growth rates
are copies of GROWTH_MEDIUM_FAST's that nothing names, headed EXTRA.
"""
import argparse
import csv
import io
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_data import message_lines  # noqa: E402
from tools.text.text_sources import article, to_json  # noqa: E402

GROWTH_RATES = "growth_rates.csv"
# Files of the system messages (a/0/0/2) with the species' names, names with an article, categories and Pokédex
# entries, one line per species from SPECIES_NONE
NAMES, NAMES_WITH_ARTICLE, CATEGORIES, POKEDEX_ENTRIES = 90, 483, 464, 442
# The Pokédex's heights and weights as it shows them, a line per species, an empty one, then the alternate forms'
HEIGHTS, WEIGHTS = 451, 471
# The Pokédex text the game keeps in other languages, for the species up to Arceus (LANGUAGE_SPECIES), which a Pokémon
# from a game in that language shows: each language's files of names, categories, entries, heights and weights. The
# heights and weights end with LANGUAGE_FORMS' first alternate forms; the Japanese categories are of every species
LANGUAGES = {
    "en": (458, 465, 444, 452, 472), "fr": (459, 466, 445, 453, 473), "de": (460, 467, 446, 454, 474),
    "it": (461, 468, 447, 455, 475), "ko": (462, 469, 448, 456, 476), "es": (463, 470, 449, 457, 477),
    "ja": (492, 493, 490, 491, 494),
}
LANGUAGE_SPECIES = 494
LANGUAGE_FORMS = ["SPECIES_GIRATINA", "SPECIES_SHAYMIN"]
RECORD = struct.Struct("<6B2BBBH3H10BHHBBHHH4I5I")
assert RECORD.size == 0x4C
STATS = ["hp", "attack", "defense", "speed", "special_attack", "special_defense"]
EVOLUTIONS = 7
NOT_IN_DEX = 999
# The kind of each evolution method's parameter, for the constant it is written with; the rest are numbers
PARAM_PREFIXES = {
    "ITEM_": {6, 8, 17, 18, 19, 20},
    "MOVE_": {21},
    "SPECIES_": {7, 22},
}


# The moves of the type move tutor, by their bit in the record's first tutor word: PokeParty_GetTutorMoveID tests bits 0
# to 5 for the pledges and the starters' ultimate moves, and bit 6, Draco Meteor, is set for dragons only
TYPE_TUTOR_MOVES = ["MOVE_GRASS_PLEDGE", "MOVE_FIRE_PLEDGE", "MOVE_WATER_PLEDGE", "MOVE_FRENZY_PLANT",
                    "MOVE_BLAST_BURN", "MOVE_HYDRO_CANNON", "MOVE_DRACO_METEOR"]
# The four move tutors' moves are their shop tables in this file, in the order of MOVE_TUTOR_SHOP_ITEMS: a move's bit
# in its tutor's word is its place in the table (ShopUI_LoadMoveTutorItems, PML_UtilCheckMoveTutorPaid)
TUTOR_SHOPS = Path(__file__).parents[2] / "src/ov036/scrcmd_shop.c"


def tutor_moves() -> list[list[str]]:
    """Returns the moves of each of the record's five tutor words, by bit: the type tutor's, then the four tutors'."""
    source = TUTOR_SHOPS.read_text()
    tables = {match[1]: re.findall(r"\{ \{ (MOVE_\w+), \d+ \}, \d+ \}", match[2]) for match in
              re.finditer(r"static const MoveTutorShopItem (MOVE_TUTOR_ITEMS_\w+)\[\] = \{(.*?)\n\};", source, re.S)}
    order = re.search(r"MOVE_TUTOR_SHOP_ITEMS\[\] = \{(.*?)\};", source, re.S)[1]
    return [TYPE_TUTOR_MOVES] + [tables[name.strip()] for name in order.split(",") if name.strip()]


def machine_bit(machine: str) -> int:
    return int(machine[2:]) - 1 if machine.startswith("TM") else int(machine[2:]) + 94


def machine_name(bit: int) -> str:
    return f"TM{bit + 1:02d}" if bit < 95 else f"HM{bit - 94:02d}"


def directory(species: str) -> str:
    return species.removeprefix("SPECIES_").lower()


# Dumping


def record_json(record: bytes, levelup: bytes, evolutions: bytes, tutors_by_bit: list[list[str]]) -> dict:
    fields = RECORD.unpack(record)
    stats, (type1, type2, catch_rate, stage, evs) = fields[0:6], fields[6:11]
    items = fields[11:14]
    gender, hatch, friendship, growth, egg1, egg2, ability1, ability2, hidden, flee = fields[14:24]
    _, sprite_offset, form_count, color, base_exp, height, weight = fields[24:31]
    machine_words, tutors = fields[31:35], fields[35:40]
    if evs >> 13 or color >> 8:
        raise ValueError("unknown bits set")
    machine_bits = sum(word << (32 * i) for i, word in enumerate(machine_words))

    by_level = []
    for move, level in (struct.unpack_from("<HH", levelup, i) for i in range(0, len(levelup) - 4, 4)):
        by_level.append([level, name("MOVE_", move)])
    if levelup[-4:] != b"\xff\xff\xff\xff":
        raise ValueError("level-up moves don't end in 0xffffffff")

    evolution_list = []
    for method, param, species in (struct.unpack_from("<3H", evolutions, 6 * i) for i in range(EVOLUTIONS)):
        if method or param or species:
            prefix = next((p for p, methods in PARAM_PREFIXES.items() if method in methods), None)
            evolution_list.append({
                "method": name("EVO_METHOD_", method),
                "param": name(prefix, param) if prefix else param,
                "species": name("SPECIES_", species),
            })
    ratio_names = {0: "GENDER_RATIO_MALE_ONLY", 254: "GENDER_RATIO_FEMALE_ONLY", 255: "GENDER_RATIO_GENDERLESS"}
    return {
        "$schema": "../species.schema.json",
        "base_stats": dict(zip(STATS, stats)),
        "types": [name("TYPE_", type1), name("TYPE_", type2)],
        "catch_rate": catch_rate,
        "evolution_stage": stage,
        "ev_yields": {stat: evs >> (2 * i) & 3 for i, stat in enumerate(STATS)},
        "underground": bool(evs >> 12 & 1),
        "held_items": dict(zip(["common", "rare", "very_rare"], (name("ITEM_", item) for item in items))),
        "gender_ratio": ratio_names.get(gender, gender),
        "hatch_cycles": hatch,
        "base_friendship": friendship,
        "growth_rate": name("GROWTH_", growth),
        "egg_groups": [name("EGG_GROUP_", egg1), name("EGG_GROUP_", egg2)],
        "abilities": [name("ABILITY_", ability1), name("ABILITY_", ability2)],
        "hidden_ability": name("ABILITY_", hidden),
        "flee_rate": flee,
        "forms": {"count": form_count, "sprite_offset": sprite_offset},
        "color": name("COLOR_", color & 0x3F),
        "asymmetric": bool(color >> 6 & 1),
        "palette_forms": bool(color >> 7 & 1),
        "base_exp": base_exp,
        "height": height,
        "weight": weight,
        "learnset": {
            "by_level": by_level,
            "by_tm": [machine_name(bit) for bit in range(128) if machine_bits >> bit & 1],
            "by_tutor": [moves[bit] for moves, word in zip(tutors_by_bit, tutors) for bit in range(32)
                         if word >> bit & 1],
        },
        "evolutions": evolution_list,
    }


def dump(files: Path, output: Path):
    personal = read_narc((files / "a/0/1/6").read_bytes())
    levelup = read_narc((files / "a/0/1/8").read_bytes())
    evolutions = read_narc((files / "a/0/1/9").read_bytes())
    babies = read_narc((files / "a/0/2/0").read_bytes())
    egg_moves = read_narc((files / "a/1/2/4").read_bytes())
    *records, table = personal
    dex = struct.unpack(f"<{len(table) // 2}H", table)
    species = sorted(load_list("species").items(), key=lambda item: item[1])
    if [v for _, v in species] != list(range(len(species))):
        sys.exit("species.txt doesn't number the species from 0 in order")

    # The form records, by the species whose forms field points at them
    forms = []
    for name_, index in species:
        first, _, count = struct.unpack_from("<HHB", records[index], 0x1C)
        if first:
            forms += [(first + n, name_, n + 1) for n in range(count - 1)]
    forms.sort()
    form_start = forms[0][0]
    if [record for record, _, _ in forms] != list(range(form_start, len(records))):
        sys.exit("the form records are not the last records, one after the other")

    tutors_by_bit = tutor_moves()

    def entry(index: int) -> dict:
        data = record_json(records[index], levelup[index], evolutions[index], tutors_by_bit)
        if index < len(babies):
            data["regional_dex_number"] = None if dex[index] == NOT_IN_DEX else dex[index]
            data["baby_species"] = name("SPECIES_", struct.unpack("<H", babies[index])[0])
        return data

    text = files / "a/0/0/2"
    names, with_article, categories, entries, heights, weights = (
        message_lines(text, n) for n in (NAMES, NAMES_WITH_ARTICLE, CATEGORIES, POKEDEX_ENTRIES, HEIGHTS, WEIGHTS))
    # After the species' entries, heights and weights, an empty one and then the alternate forms', in species order
    form_lines = len(species) + 1
    languages = {language: [message_lines(text, n) for n in files_] for language, files_ in LANGUAGES.items()}
    language_forms = {name_: i for i, name_ in enumerate(LANGUAGE_FORMS)}
    for name_, index in species:
        data = entry(index)
        if egg_moves[index]:
            count = struct.unpack_from("<H", egg_moves[index])[0]
            moves = struct.unpack_from(f"<{count}H", egg_moves[index], 2)
            data["learnset"]["egg_moves"] = [name("MOVE_", move) for move in moves]
        species_text = {"name": to_json(names[index])}
        # SPECIES_NONE's line is empty
        if index and f"{{bd01}}{article(species_text)} {{ff00:255}}{names[index]}" != with_article[index]:
            species_text["name_article"] = with_article[index].removeprefix("{bd01}").split(" ")[0]
        pokedex = {"category": to_json(categories[index]), "entry": to_json(entries[index]),
                   "height": to_json(heights[index]), "weight": to_json(weights[index])}
        if data["forms"]["count"] > 1:
            pokedex["forms"] = []
            for _ in range(data["forms"]["count"] - 1):
                pokedex["forms"].append({"entry": to_json(entries[form_lines]), "height": to_json(heights[form_lines]),
                                         "weight": to_json(weights[form_lines])})
                form_lines += 1
        pokedex["languages"] = {}
        for language, (l_names, l_categories, l_entries, l_heights, l_weights) in languages.items():
            if index < LANGUAGE_SPECIES:
                block = {"name": to_json(l_names[index]), "category": to_json(l_categories[index]),
                         "entry": to_json(l_entries[index]), "height": to_json(l_heights[index]),
                         "weight": to_json(l_weights[index])}
                if name_ in language_forms:
                    line = LANGUAGE_SPECIES + language_forms[name_]
                    block["forms"] = [{"height": to_json(l_heights[line]), "weight": to_json(l_weights[line])}]
                pokedex["languages"][language] = block
            elif index < len(l_categories):
                pokedex["languages"][language] = {"category": to_json(l_categories[index])}
        species_text["pokedex"] = pokedex
        schema = {"$schema": data.pop("$schema")}
        write(output / directory(name_) / "data.json", {**schema, **species_text, **data})
    for index in range(len(species), form_start):
        write(output / "extra" / f"{index}.json", entry(index))
    for record, name_, form in forms:
        write(output / directory(name_) / f"form_{form}.json", entry(record))
    write(output / "forms.json", [[name_, form] for _, name_, form in forms])

    tables = [struct.unpack("<101I", table) for table in read_narc((files / "a/0/1/7").read_bytes())]
    text = io.StringIO()
    writer = csv.writer(text, lineterminator="\n")
    writer.writerow(["level", *(name("GROWTH_", i) if isinstance(name("GROWTH_", i), str) else "EXTRA"
                                for i in range(len(tables)))])
    for level in range(101):
        writer.writerow([level, *(table[level] for table in tables)])
    (output / GROWTH_RATES).write_text(text.getvalue())
    print(f"wrote {len(records)} records and {len(tables)} experience tables to {output}")


def growth_tables(path: Path) -> list[bytes]:
    rows = list(csv.reader(path.read_text().splitlines()))
    header, rows = rows[0], rows[1:]
    where = label(path)
    if header[0] != "level" or [int(row[0]) for row in rows] != list(range(101)):
        raise DataError(f"{where}: the first column is the level, from 0 to 100")
    for column, heading in enumerate(header[1:]):
        if heading != "EXTRA" and value(heading, where) != column:
            raise DataError(f"{where}: column {column + 1} is {heading}, whose value is not {column}")
    return [struct.pack("<101I", *(int(row[column]) for row in rows)) for column in range(1, len(header))]


# Packing


def record_files(root: Path) -> tuple[list[Path], int, dict[str, int]]:
    """Returns the files of the records in order, the number of records with a baby species (the species and extra
    records), and the first form record of each species with forms."""
    species = sorted(load_list("species").items(), key=lambda item: item[1])
    files = [root / directory(name_) / "data.json" for name_, _ in species]
    files += sorted((root / "extra").glob("*.json"), key=lambda path: int(path.stem))
    with_babies = len(files)
    first_forms: dict[str, int] = {}
    for species_name, form in json.loads((root / "forms.json").read_text()):
        if species_name in first_forms and first_forms[species_name] + form - 1 != len(files):
            raise DataError(f"forms.json: the forms of {species_name} are not one after the other, from 1")
        first_forms.setdefault(species_name, len(files))
        files.append(root / directory(species_name) / f"form_{form}.json")
    return files, with_babies, first_forms


def tutor_words(moves: list[str], tutors_by_bit: list[list[str]], where: str) -> list[int]:
    words = [0] * len(tutors_by_bit)
    for move in moves:
        place = next(((w, b) for w, word_moves in enumerate(tutors_by_bit) for b, m in enumerate(word_moves)
                      if m == move), None)
        if place is None:
            raise DataError(f"{where}: learnset.by_tutor: {move} is not a tutor's move")
        words[place[0]] |= 1 << place[1]
    return words


def record_bytes(data: dict, first_form: int, tutors_by_bit: list[list[str]], where: str) -> bytes:
    stats = [data["base_stats"][stat] for stat in STATS]
    evs = sum(data["ev_yields"][stat] << (2 * i) for i, stat in enumerate(STATS)) | data["underground"] << 12
    items = [value(data["held_items"][key], where) for key in ("common", "rare", "very_rare")]
    color = value(data["color"], where) | data["asymmetric"] << 6 | data["palette_forms"] << 7
    machine_bits = 0
    for machine in data["learnset"]["by_tm"]:
        machine_bits |= 1 << machine_bit(machine)
    return RECORD.pack(
        *stats, *(value(t, where) for t in data["types"]), data["catch_rate"], data["evolution_stage"], evs, *items,
        value(data["gender_ratio"], where), data["hatch_cycles"], data["base_friendship"],
        value(data["growth_rate"], where), *(value(g, where) for g in data["egg_groups"]),
        *(value(a, where) for a in data["abilities"]), value(data["hidden_ability"], where), data["flee_rate"],
        first_form, data["forms"]["sprite_offset"], data["forms"]["count"], color, data["base_exp"], data["height"],
        data["weight"], *(machine_bits >> (32 * i) & 0xFFFFFFFF for i in range(4)),
        *tutor_words(data["learnset"]["by_tutor"], tutors_by_bit, where),
    )


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "species.schema.json")
    files, with_babies, first_forms = record_files(root)
    species_count = len(load_list("species"))
    personal, levelup, evolutions, babies, dex, egg_moves = [], [], [], [], [], []
    tutors_by_bit = tutor_moves()
    for index, path in enumerate(files):
        where = label(path)
        data = load(path, schema)
        if (index < with_babies) != ("baby_species" in data and "regional_dex_number" in data):
            raise DataError(f"{where}: a species or extra record has baby_species and regional_dex_number, and a form "
                            "has neither")
        species_name = f"SPECIES_{path.parent.name.upper()}"
        first_form = first_forms.get(species_name, 0) if path.name == "data.json" else 0
        personal.append(record_bytes(data, first_form, tutors_by_bit, where))
        levelup.append(b"".join(struct.pack("<HH", value(move, where), level)
                                for level, move in data["learnset"]["by_level"]) + b"\xff\xff\xff\xff")
        entries = [(value(e["method"], where), value(e["param"], where), value(e["species"], where))
                   for e in data["evolutions"]]
        evolutions.append(b"".join(struct.pack("<3H", *e) for e in entries + [(0, 0, 0)] * (EVOLUTIONS - len(entries))))
        if index < species_count:
            moves = [value(move, where) for move in data["learnset"].get("egg_moves", [])]
            empty = "egg_moves" not in data["learnset"]
            egg_moves.append(b"" if empty else struct.pack(f"<H{len(moves)}H", len(moves), *moves))
        elif "egg_moves" in data["learnset"]:
            raise DataError(f"{where}: only a species has egg_moves, not a form or an extra record")
        if index < with_babies:
            babies.append(struct.pack("<H", value(data["baby_species"], where)))
            number = data["regional_dex_number"]
            dex.append(NOT_IN_DEX if number is None else number)
    personal.append(struct.pack(f"<{len(dex)}H", *dex))
    growth = growth_tables(root / GROWTH_RATES)
    for output, members in zip(outputs, (personal, levelup, evolutions, babies, growth, egg_moves)):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/pokemon/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the six archives from data/pokemon/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=6, metavar="ARCHIVE",
                             help="the species data, level-up moves, evolutions, baby species, experience table and "
                                  "egg move archives")
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
