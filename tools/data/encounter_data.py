#!/usr/bin/env python3
"""The wild encounters in data/encounters/, one JSON file per encounter table: dump writes them from the ROM once, and
pack builds a version's archive (a/1/2/7) from them, which the build does.

    encounter_data.py dump extract data/encounters    # reads extract/b2_us and extract/w2_us
    encounter_data.py pack data/encounters black2 ARCHIVE

Each table is data/encounters/<table>.json, named after its constant (castelia_city.json for
ENCOUNTERS_CASTELIA_CITY), and the archive has them in the order of data/constants/encounters.txt, which zone headers
name them by. Black 2 and White 2 have different encounters in many places: a table's rates, or a group of its slots,
is either the same for both versions or an object with one for each, { "black2": ..., "white2": ... }.
encounters.schema.json describes each field.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

TABLE_SIZE = 0xE8
GROUPS = [("grass", 12), ("dark_grass", 12), ("shaking_grass", 12), ("surf", 5), ("rippling_surf", 5),
          ("fishing", 5), ("rippling_fishing", 5)]
SEASONS = ["spring", "summer", "autumn", "winter"]
VERSIONS = ["black2", "white2"]
EXTRACTS = {"black2": "b2_us", "white2": "w2_us"}


def file_name(table: str) -> str:
    return table.removeprefix("ENCOUNTERS_").lower() + ".json"


def ordered_tables() -> list[str]:
    tables = sorted(load_list("encounters").items(), key=lambda item: item[1])
    if [v for _, v in tables] != list(range(len(tables))):
        sys.exit("encounters.txt doesn't number the tables from 0 in order")
    return [table for table, _ in tables]


# Dumping


def rates_json(table: bytes) -> dict:
    return {**{group: rate for (group, _), rate in zip(GROUPS, table[:7])}, "flags": table[7]}


def slots_json(table: bytes, offset: int, count: int) -> list:
    # The slots after the last one used are empty
    while count and not any(table[offset + 4 * (count - 1):offset + 4 * count]):
        count -= 1
    slots = []
    for i in range(count):
        species_form, low, high = struct.unpack_from("<HBB", table, offset + 4 * i)
        slot = [name("SPECIES_", species_form & 0x7FF), low, high]
        slots.append(slot + [species_form >> 11] if species_form >> 11 else slot)
    return slots


def versioned(black, white):
    return black if black == white else {"black2": black, "white2": white}


def season_json(black: bytes, white: bytes) -> dict:
    season = {"rates": versioned(rates_json(black), rates_json(white))}
    offset = 8
    for group, count in GROUPS:
        season[group] = versioned(slots_json(black, offset, count), slots_json(white, offset, count))
        offset += 4 * count
    return season


def table_json(black: bytes, white: bytes) -> dict:
    if len(black) != len(white) or len(black) not in (TABLE_SIZE, 4 * TABLE_SIZE):
        raise ValueError("the versions have different numbers of seasons")
    data = {"$schema": "encounters.schema.json"}
    if len(black) == TABLE_SIZE:
        data["all_year"] = season_json(black, white)
    else:
        for i, season in enumerate(SEASONS):
            part = slice(TABLE_SIZE * i, TABLE_SIZE * (i + 1))
            data[season] = season_json(black[part], white[part])
    return data


def dump(extract: Path, output: Path):
    archives = {v: read_narc((extract / EXTRACTS[v] / "files/a/1/2/7").read_bytes()) for v in VERSIONS}
    tables = ordered_tables()
    if not len(archives["black2"]) == len(archives["white2"]) == len(tables):
        sys.exit("the archives don't have one entry per table of encounters.txt")
    for table, black, white in zip(tables, archives["black2"], archives["white2"]):
        write(output / file_name(table), table_json(black, white))
    print(f"wrote {len(tables)} tables to {output}")


# Packing


def for_version(item, version: str):
    """Returns the version's part of a value that is either the same for both versions or split between them."""
    if isinstance(item, dict) and set(item) == set(VERSIONS):
        return item[version]
    return item


def season_bytes(season: dict, version: str, where: str) -> bytes:
    rates = for_version(season["rates"], version)
    data = bytes([rates[group] for group, _ in GROUPS] + [rates["flags"]])
    for group, count in GROUPS:
        slots = for_version(season[group], version)
        if len(slots) > count:
            raise DataError(f"{where}: {group} has {len(slots)} slots, at most {count}")
        for slot in slots:
            species, low, high, *form = slot
            data += struct.pack("<HBB", value(species, where) | (form[0] if form else 0) << 11, low, high)
        data += bytes(4 * (count - len(slots)))
    return data


def pack(root: Path, version: str, output: Path):
    schema = load_schema(root / "encounters.schema.json")
    members = []
    for table in ordered_tables():
        path = root / file_name(table)
        data, where = load(path, schema), label(path)
        seasons = ["all_year"] if "all_year" in data else SEASONS
        if any(season not in data for season in seasons) or ("all_year" in data and any(s in data for s in SEASONS)):
            raise DataError(f"{where}: a table has all_year, or spring, summer, autumn and winter")
        members.append(b"".join(season_bytes(data[season], version, f"{where}: {season}") for season in seasons))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/encounters/ from both extracted versions")
    dump_parser.add_argument("extract", type=Path, help="the directory with the extracted versions, b2_us and w2_us")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build a version's encounter archive from data/encounters/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("version", choices=VERSIONS)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.extract, args.output)
        return
    try:
        pack(args.root, args.version, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
