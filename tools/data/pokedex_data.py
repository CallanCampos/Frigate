#!/usr/bin/env python3
"""The Pokédex's tables: each species' search data and area map in res/pokemon/<species>/pokedex.json, and the lists
in res/pokedex/. dump writes them from the ROM once, and pack builds a version's three archives from them, which the
build does.

    pokedex_data.py dump extract res/pokedex    # reads extract/b2_us and extract/w2_us
    pokedex_data.py pack res/pokedex black2 DATA AREA HABITAT

- ARCID_POKEDEX_DATA (a/0/9/7) is the search's tables (pokedex_search_engine.c): file 0 links each species to its
  other forms and each form to the next, by their places in file 2; file 1 is each species' initial, file 2 the shape
  of each species and then of every other form; files 3 to 9 are the lists of the species in the search's orders,
  res/pokedex/orders.json. A species' "initial" and "shapes" in its pokedex.json make files 0 to 2.
- ARCID_POKEDEX_AREA (a/1/7/6) is where each species lives, a file for each species but SPECIES_NONE
  (PokedexHabitat, which pokedex_detail_map.c draws on the town map): its pokedex.json's "area". A species without
  one lives nowhere.
- ARCID_POKEDEX_HABITAT (a/2/9/6) is the habitat list, a file for each of its areas (PokedexHabitatFile,
  pokedex_common.c): res/pokedex/habitats/<zone>.json, named after the zone that sPokedexHabitats gives the file, and
  <n>.json for a file that it gives no zone.

The species are in the order of generated/species.txt, and zones are their ZONE_* constants. Black 2 and White 2
differ in some species' areas and the habitat list's species: a value that differs is an object with one for each,
{ "black2": ..., "white2": ... }. res/pokemon/pokedex.schema.json, res/pokedex/orders.schema.json and
res/pokedex/habitat.schema.json describe the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.species_data import directory  # noqa: E402

VERSIONS = ["black2", "white2"]
EXTRACTS = {"black2": "b2_us", "white2": "w2_us"}
DATA, AREA, HABITAT = "a/0/9/7", "a/1/7/6", "a/2/9/6"
SEASONS = ["spring", "summer", "autumn", "winter"]

# ARCID_POKEDEX_DATA's files: the forms' links, the initials and the shapes, then the orders' lists. The search reads
# the orders by sPokedexSearchOrderFiles; the national list is unread, as the search counts 1 to 649 itself
FILE_FORMS, FILE_INITIALS, FILE_SHAPES = 0, 1, 2
ORDERS = ["national", "regional", "alphabetical", "tallest", "shortest", "heaviest", "lightest"]
FIRST_ORDER_FILE = 3
# The orders by size list forms too: an entry's species is in its low 10 bits and its form above them
ENTRY_SPECIES_MASK, ENTRY_FORM_SHIFT = 0x3FF, 10
INITIALS = [chr(ord("A") + i) for i in range(26)]
NO_INITIAL = 26

# PokedexHabitat: whether the area is the same all year, and for each season whether the species lives nowhere and the
# kinds of habitat at each place, the bits of the encounter groups (HABITAT_LAND, HABITAT_WATER and HABITAT_FISHING
# group them)
KINDS = ["grass", "dark_grass", "shaking_grass", "surf", "rippling_surf", "fishing", "rippling_fishing"]
# sPokedexDetailMapZones in pokedex_detail_map.c: the zones of the town map's places, in the order of the area files
AREA_ZONES = [
    0x185, 0x18D, 0x006, 0x010, 0x01C, 0x03E, 0x060, 0x06B, 0x071, 0x078, 0x088, 0x196, 0x19C, 0x1A2, 0x000,
    0x1A8, 0x093, 0x117, 0x17D, 0x098, 0x09A, 0x09D, 0x0A0, 0x0BF, 0x0C2, 0x0C6, 0x0CD, 0x0E6, 0x0EB, 0x0EE,
    0x144, 0x14D, 0x152, 0x15A, 0x178, 0x0F0, 0x181, 0x13D, 0x13F, 0x141, 0x146, 0x149, 0x14B, 0x151, 0x159,
    0x15C, 0x16D, 0x170, 0x172, 0x176, 0x17A, 0x17F, 0x1A7, 0x183, 0x0F9, 0x0FD, 0x0FE, 0x0FF, 0x107, 0x1AB,
    0x1B7, 0x1C0, 0x1CA, 0x1D1, 0x236, 0x1EA, 0x1B5, 0x1BE, 0x1CF, 0x1DA, 0x1EF, 0x1BC, 0x1C8, 0x1CD, 0x1CE,
    0x1DB, 0x23D, 0x1F7, 0x203, 0x1FA, 0x205, 0x1D0, 0x108, 0x228, 0x0F1,
]
AREA_SIZE = 1 + len(SEASONS) * (1 + len(AREA_ZONES))

# PokedexHabitatFile: by kind of habitat (TYPE_LAND, TYPE_WATER and TYPE_FISHING in pokedex_habitat.c), whether its
# species differ by season and whether the area has it; the land's message; the count of species; and each species
HABITAT_KINDS = ["land", "water", "fishing"]
HABITAT_HEADER = struct.Struct("<3B3BHH")
HABITAT_ENTRY = struct.Struct("<H12B12BH")
HABITAT_MAX = 30
# PokedexHabitat_GetHabitatMessage's messages for the land: PokedexHabitat_Text_PokemonLiving*
LANDS = ["tall_grass", "sand", "swamp", "place", "place_2", "land", "none"]
# sPokedexHabitats in pokedex_common.c: each area's zone and its file
HABITAT_FILES = {
    "ZONE_ASPERTIA_CITY": 24, "ZONE_ROUTE_19": 23, "ZONE_ROUTE_20": 28, "ZONE_FLOCCESY_RANCH": 26,
    "ZONE_VIRBANK_CITY": 29, "ZONE_VIRBANK_COMPLEX": 32, "ZONE_CASTELIA_CITY": 34, "ZONE_CASTELIA_SEWERS": 30,
    "ZONE_ROUTE_4": 0, "ZONE_DESERT_RESORT": 41, "ZONE_RELIC_CASTLE": 21, "ZONE_ROUTE_5": 9, "ZONE_ROUTE_16": 8,
    "ZONE_LOSTLORN_FOREST": 55, "ZONE_DRIFTVEIL_DRAWBRIDGE": 53, "ZONE_ROUTE_6": 13, "ZONE_RELIC_PASSAGE": 7,
    "ZONE_CLAY_TUNNEL": 3, "ZONE_MISTRALTON_CAVE": 49, "ZONE_CHARGESTONE_CAVE": 57, "ZONE_ROUTE_7": 19,
    "ZONE_CELESTIAL_TOWER_2": 39, "ZONE_REVERSAL_MOUNTAIN": 37, "ZONE_STRANGE_HOUSE": 27, "ZONE_UNDELLA_TOWN": 12,
    "ZONE_ROUTE_13": 50, "ZONE_UNDELLA_BAY": 14, "ZONE_ROUTE_14": 56, "ZONE_ABUNDANT_SHRINE": 11, "ZONE_ROUTE_12": 47,
    "ZONE_VILLAGE_BRIDGE": 5, "ZONE_ROUTE_11": 42, "ZONE_ROUTE_9": 31, "ZONE_SEASIDE_CAVE": 54, "ZONE_ROUTE_21": 2,
    "ZONE_ROUTE_15": 1, "ZONE_MARVELOUS_BRIDGE": 10, "ZONE_HUMILAU_CITY": 36, "ZONE_ROUTE_22": 40,
    "ZONE_GIANT_CHASM": 18, "ZONE_ROUTE_23": 45, "ZONE_VICTORY_ROAD": 20, "ZONE_ROUTE_8": 25,
    "ZONE_MOOR_OF_ICIRRUS": 46, "ZONE_ICIRRUS_CITY": 52, "ZONE_DRAGONSPIRAL_TOWER": 6, "ZONE_TWIST_MOUNTAIN": 4,
    "ZONE_PINWHEEL_FOREST": 35, "ZONE_ROUTE_3": 51, "ZONE_WELLSPRING_CAVE": 15, "ZONE_STRIATON_CITY": 22,
    "ZONE_DREAMYARD": 38, "ZONE_ROUTE_2": 48, "ZONE_ROUTE_1": 44, "ZONE_ROUTE_17": 43, "ZONE_ROUTE_18": 17,
    "ZONE_P2_LABORATORY": 33,
}


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def species_list() -> list[str]:
    listed = sorted(load_list("species").items(), key=lambda item: item[1])
    check([v for _, v in listed] == list(range(len(listed))), "species.txt doesn't number the species from 0")
    return [species for species, _ in listed]


def pokedex_path(pokemon: Path, species: str) -> Path:
    return pokemon / directory(species) / "pokedex.json"


def schema_ref(species: str) -> str:
    return "../" * directory(species).count("/") + "../pokedex.schema.json"


def habitat_names(count: int) -> list[str]:
    """Returns the habitat files' names, in archive order: their zones', or their numbers for the files that
    sPokedexHabitats gives no zone."""
    by_file = {file: zone for zone, file in HABITAT_FILES.items()}
    return [by_file[i].removeprefix("ZONE_").lower() if i in by_file else str(i) for i in range(count)]


def versioned(black, white):
    return black if black == white else {"black2": black, "white2": white}


def for_version(item, version: str):
    """Returns the version's part of a value that is either the same for both versions or split between them."""
    if isinstance(item, dict) and set(item) == set(VERSIONS):
        return item[version]
    return item


def u16s(data: bytes) -> list[int]:
    check(len(data) % 2 == 0, "a list of halfwords has an odd size")
    return list(struct.unpack(f"<{len(data) // 2}H", data))


# Dumping


def form_counts(links: list[int]) -> list[int]:
    """Returns each species' count of other forms from the forms' links, checking that each species' forms follow the
    species before them in order, as pack writes them. The species are the places before the first form's."""
    first_form = min((link for link in links if link), default=len(links))
    counts = []
    for species in range(first_form):
        count, place = 0, species
        while links[place] and count < len(links):
            place = links[place]
            count += 1
        counts.append(count)
    check(links == pack_links(counts, first_form), "the forms' links aren't in species order")
    return counts


def pack_links(counts: list[int], first_form: int) -> list[int]:
    links = [0] * (first_form + sum(counts))
    place = first_form
    for species, count in enumerate(counts):
        previous = species
        for _ in range(count):
            links[previous] = place
            previous = place
            place += 1
    return links


def area_json(data: bytes) -> dict:
    check(len(data) == AREA_SIZE, f"an area file is {len(data)} bytes, not {AREA_SIZE}")
    all_year = data[0]
    check(all_year in (0, 1), "an area's all-year flag isn't 0 or 1")
    seasons = []
    for i in range(len(SEASONS)):
        start = 1 + i * (1 + len(AREA_ZONES))
        nowhere, places = data[start], data[start + 1:start + 1 + len(AREA_ZONES)]
        check(nowhere == (not any(places)), "an area's season is nowhere but has places, or the other way")
        check(all(kinds < 1 << len(KINDS) for kinds in places), "a place has a kind of habitat that isn't known")
        seasons.append({name("ZONE_", zone): [kind for bit, kind in enumerate(KINDS) if kinds >> bit & 1]
                        for zone, kinds in zip(AREA_ZONES, places) if kinds})
    if all_year:
        check(all(season == seasons[0] for season in seasons), "an all-year area differs by season")
        return {"all_year": True, "places": seasons[0]}
    return {"all_year": False, "seasons": dict(zip(SEASONS, seasons))}


NOWHERE = {"all_year": True, "places": {}}


def habitat_entry_json(data: bytes, offset: int) -> dict:
    species, *rest = HABITAT_ENTRY.unpack_from(data, offset)
    present, forms, unk1a = rest[:12], rest[12:24], rest[24]
    check(all(p in (0, 1) for p in present), "a habitat's species has a presence that isn't 0 or 1")
    entry = {"species": name("SPECIES_", species),
             "present": {kind: [season for s, season in enumerate(SEASONS) if present[s * 3 + k]]
                         for k, kind in enumerate(HABITAT_KINDS)},
             "forms": {kind: [forms[s * 3 + k] for s in range(len(SEASONS))]
                       for k, kind in enumerate(HABITAT_KINDS) if any(forms[s * 3 + k] for s in range(len(SEASONS)))}}
    if unk1a:
        entry["unk1a"] = unk1a
    return entry


def habitat_json(black: bytes, white: bytes) -> dict:
    files = []
    for data in (black, white):
        *header, land, count = HABITAT_HEADER.unpack_from(data)
        check(len(data) == HABITAT_HEADER.size + count * HABITAT_ENTRY.size, "a habitat file isn't its species' size")
        check(count <= HABITAT_MAX, f"a habitat file has more than {HABITAT_MAX} species")
        check(all(flag in (0, 1) for flag in header), "a habitat's flags aren't 0 or 1")
        check(land < len(LANDS), "a habitat's land message isn't known")
        files.append({"kinds": [kind for k, kind in enumerate(HABITAT_KINDS) if header[3 + k]],
                      "by_season": [kind for k, kind in enumerate(HABITAT_KINDS) if header[k]],
                      "land": LANDS[land],
                      "species": [habitat_entry_json(data, HABITAT_HEADER.size + i * HABITAT_ENTRY.size)
                                  for i in range(count)]})
    black_file, white_file = files
    check(all(black_file[key] == white_file[key] for key in ("kinds", "by_season", "land"))
          and len(black_file["species"]) == len(white_file["species"]),
          "the versions' habitat files differ in more than their species")
    entries = []
    for b, w in zip(black_file["species"], white_file["species"]):
        entry = {key: versioned(b.get(key, {}), w.get(key, {})) for key in ("species", "present", "forms")}
        if entry["forms"] == {}:
            del entry["forms"]
        check(b.get("unk1a") == w.get("unk1a"), "the versions' habitat files differ in unk1a")
        if "unk1a" in b:
            entry["unk1a"] = b["unk1a"]
        entries.append(entry)
    return {"$schema": "habitat.schema.json", "kinds": black_file["kinds"], "by_season": black_file["by_season"],
            "land": black_file["land"], "species": entries}


def dump(extract: Path, output: Path):
    archives = {path: {v: read_narc((extract / EXTRACTS[v] / "files" / path).read_bytes()) for v in VERSIONS}
                for path in (DATA, AREA, HABITAT)}
    data = archives[DATA]["black2"]
    if data != archives[DATA]["white2"]:
        sys.exit(f"the versions' {DATA} differ")
    check(len(data) == FIRST_ORDER_FILE + len(ORDERS), f"{DATA} doesn't have {FIRST_ORDER_FILE + len(ORDERS)} files")
    species = species_list()
    links, initials, shapes = u16s(data[FILE_FORMS]), data[FILE_INITIALS], data[FILE_SHAPES]
    check(len(links) == len(shapes), "the forms' links and the shapes have different counts")
    counts = form_counts(links)
    check(len(initials) <= len(counts) <= len(species), "the initials or the shapes are for more than the species")
    areas = {v: archives[AREA][v] for v in VERSIONS}
    check(len(areas["black2"]) == len(areas["white2"]) == len(species) - 1, "the areas aren't one per species")
    pokemon = output.parent / "pokemon"
    form_shapes = iter(shapes[len(counts):])
    for number, species_name in enumerate(species):
        area = None
        if number:
            area = versioned(area_json(areas["black2"][number - 1]), area_json(areas["white2"][number - 1]))
        if number >= len(counts):
            check(area == NOWHERE, f"{species_name} has no Pokédex data but lives somewhere")
            continue
        entry = {"$schema": schema_ref(species_name)}
        if number < len(initials):
            check(initials[number] <= NO_INITIAL, f"{species_name}'s initial isn't a letter")
            entry["initial"] = None if initials[number] == NO_INITIAL else INITIALS[initials[number]]
        entry["shapes"] = [shapes[number]] + [next(form_shapes) for _ in range(counts[number])]
        if area is not None and (number < len(initials) or area != NOWHERE):
            entry["area"] = area
        write(pokedex_path(pokemon, species_name), entry)
    orders = {"$schema": "orders.schema.json"}
    for i, order in enumerate(ORDERS):
        orders[order] = [name("SPECIES_", e & ENTRY_SPECIES_MASK) if not e >> ENTRY_FORM_SHIFT
                         else [name("SPECIES_", e & ENTRY_SPECIES_MASK), e >> ENTRY_FORM_SHIFT]
                         for e in u16s(data[FIRST_ORDER_FILE + i])]
    write(output / "orders.json", orders)
    habitats = archives[HABITAT]
    check(len(habitats["black2"]) == len(habitats["white2"]), "the versions have different numbers of habitat files")
    names = habitat_names(len(habitats["black2"]))
    check(len(set(names)) == len(names), "two habitat files have one name")
    for file_name, black, white in zip(names, habitats["black2"], habitats["white2"]):
        write(output / "habitats" / f"{file_name}.json", habitat_json(black, white))
    print(f"wrote {len(counts)} species' Pokédex data to {pokemon}, and orders.json and {len(names)} habitat files to "
          f"{output}")


# Packing


def area_bytes(area: dict, where: str) -> bytes:
    if area["all_year"] != ("places" in area) or ("places" in area) == ("seasons" in area):
        raise DataError(f"{where}: an area has all_year true and places, or all_year false and seasons")
    seasons = [area["places"]] * len(SEASONS) if area["all_year"] else [area["seasons"][s] for s in SEASONS]
    data = bytearray([int(area["all_year"])])
    for places in seasons:
        for zone, found in places.items():
            if any(kind not in KINDS for kind in found):
                raise DataError(f"{where}: {zone} has a kind of habitat that isn't one of {KINDS}")
        kinds = {value(zone, where): sum(1 << KINDS.index(kind) for kind in set(found))
                 for zone, found in places.items()}
        unknown = [zone for zone in kinds if zone not in AREA_ZONES]
        if unknown:
            raise DataError(f"{where}: {name('ZONE_', unknown[0])} is not a place of the town map")
        row = [kinds.get(zone, 0) for zone in AREA_ZONES]
        data += bytes([int(not any(row))] + row)
    return bytes(data)


def habitat_bytes(habitat: dict, version: str, where: str) -> bytes:
    entries = habitat["species"]
    if len(entries) > HABITAT_MAX:
        raise DataError(f"{where}: {len(entries)} species, at most {HABITAT_MAX}")
    data = HABITAT_HEADER.pack(*(int(kind in habitat["by_season"]) for kind in HABITAT_KINDS),
                               *(int(kind in habitat["kinds"]) for kind in HABITAT_KINDS),
                               LANDS.index(habitat["land"]), len(entries))
    for i, entry in enumerate(entries):
        present = for_version(entry["present"], version)
        forms = for_version(entry.get("forms", {}), version)
        data += HABITAT_ENTRY.pack(
            value(for_version(entry["species"], version), f"{where}: species {i}"),
            *(int(season in present[kind]) for season in SEASONS for kind in HABITAT_KINDS),
            *(forms.get(kind, [0] * len(SEASONS))[s] for s in range(len(SEASONS)) for kind in HABITAT_KINDS),
            entry.get("unk1a", 0))
    return data


def pack(root: Path, version: str, outputs: list[Path]):
    pokemon = root.parent / "pokemon"
    schema = load_schema(pokemon / "pokedex.schema.json")
    species = species_list()
    entries = []
    for species_name in species:
        path = pokedex_path(pokemon, species_name)
        if not path.exists():
            break
        entries.append((load(path, schema), label(path)))
    for species_name in species[len(entries):]:
        if pokedex_path(pokemon, species_name).exists():
            raise DataError(f"{label(pokedex_path(pokemon, species_name))}: the species before it have no pokedex.json")
    initials = [entry["initial"] for entry, _ in entries if "initial" in entry]
    if any("initial" not in entry for entry, _ in entries[:len(initials)]):
        raise DataError(f"{entries[len(initials)][1]}: the species before the last with an initial must have one")
    shapes = [entry["shapes"] for entry, _ in entries]
    links = pack_links([len(s) - 1 for s in shapes], len(entries))
    all_shapes = [s[0] for s in shapes] + [shape for s in shapes for shape in s[1:]]
    orders = load(root / "orders.json", load_schema(root / "orders.schema.json"))
    data = [struct.pack(f"<{len(links)}H", *links),
            bytes(NO_INITIAL if initial is None else INITIALS.index(initial) for initial in initials),
            bytes(all_shapes)]
    for order in ORDERS:
        where = f"{label(root / 'orders.json')}: {order}"
        listed = [value(item, where) if not isinstance(item, list)
                  else value(item[0], where) | item[1] << ENTRY_FORM_SHIFT for item in orders[order]]
        data.append(struct.pack(f"<{len(listed)}H", *listed))
    areas = []
    for number in range(1, len(species)):
        entry, where = entries[number] if number < len(entries) else ({}, species[number])
        areas.append(area_bytes(for_version(entry.get("area", NOWHERE), version), where))
    habitat_schema = load_schema(root / "habitat.schema.json")
    habitats = []
    for file_name in habitat_names(len(list((root / "habitats").glob("*.json")))):
        path = root / "habitats" / f"{file_name}.json"
        if not path.exists():
            raise DataError(f"{label(path)} is missing: the habitat files are each of sPokedexHabitats' zones and "
                            "numbers for the files between them")
        habitats.append(habitat_bytes(load(path, habitat_schema), version, label(path)))
    for output, members in zip(outputs, (data, areas, habitats)):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write the Pokédex's data from the extracted versions")
    dump_parser.add_argument("extract", type=Path, help="the directory with the extracted versions, b2_us and w2_us")
    dump_parser.add_argument("output", type=Path, help="res/pokedex; the species' files go in res/pokemon beside it")
    pack_parser = commands.add_parser("pack", help="build a version's Pokédex archives")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("version", choices=VERSIONS)
    pack_parser.add_argument("outputs", type=Path, nargs=3, metavar="ARCHIVE",
                             help="the search's data, the areas and the habitat list")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.extract, args.output)
        return
    try:
        pack(args.root, args.version, args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
