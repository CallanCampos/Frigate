#!/usr/bin/env python3
"""Geonet's places on the globe in res/geonet/ (ARCID_GEONET_PLACE, a/0/5/2): dump writes them from the ROM once, and
pack builds the archive from them, which the build does.

    geonet_place_data.py dump extract/b2_us/files res/geonet
    geonet_place_data.py pack res/geonet ARCHIVE

- countries.json is member 18 (NARC_GEONET_PLACE_COUNTRIES), a GeonetCountryPlace for each country by its number,
  whose names are the messages of the text bank of sSubdividedCountries[0] in the same order.
- regions.json is members 0 to 17, a GeonetRegionPlace for each region of the countries with regions, by their
  placeDataId in sSubdividedCountries (country_region.c), whose names are the messages of the country's text bank
  there in the same order. Geonet_LoadPlaces shows each region instead of its country.

Each entry names its message by ID, which the packer checks against the text and doesn't pack, null past the text's
last; the first entry of each file isn't a place. x and y are the globe's turn about its X and Y axes that brings the
place to the front (Geonet_AddPlace), in 65536ths of a turn.

Both versions have the same file. countries.schema.json and regions.schema.json describe the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = "a/0/5/2"
COUNTRY = struct.Struct("<Hhh")  # GeonetCountryPlace
REGION = struct.Struct("<hh")  # GeonetRegionPlace
COUNTRIES_MEMBER = 18  # NARC_GEONET_PLACE_COUNTRIES
COUNTRIES_BANK = 0x1B6  # sSubdividedCountries[0]'s text bank
# sSubdividedCountries from 1, by placeDataId: each country's number and the text bank of its regions' names
SUBDIVIDED = [(9, 0x1A3), (12, 0x1A4), (28, 0x1A5), (36, 0x1A6), (43, 0x1A7), (79, 0x1A8), (195, 0x1A9), (72, 0x1AA),
              (73, 0x1AB), (218, 0x1AC), (95, 0x1AD), (102, 0x1AE), (105, 0x1AF), (155, 0x1B0), (166, 0x1B1),
              (174, 0x1B2), (200, 0x1B3), (220, 0x1B5)]


def bank_messages(bank: int) -> list[str]:
    """The message IDs of a system text bank, whose file is named after its TEXT_BANK_* constant."""
    constant = name("TEXT_BANK_", bank)
    if not isinstance(constant, str):
        raise DataError(f"text bank {bank:#x} has no constant")
    path = ROOT / "res" / "text" / "system" / f"{constant.removeprefix('TEXT_BANK_').lower()}.txt"
    return [message for message, _ in message_numbers(path)]


def unpack(layout: struct.Struct, data: bytes, what: str) -> list[tuple]:
    if len(data) % layout.size:
        raise ValueError(f"{what} isn't whole entries")
    return [layout.unpack_from(data, offset) for offset in range(0, len(data), layout.size)]


def check_names(entries: list[dict], names: list[str], where: str):
    for i, entry in enumerate(entries):
        expected = names[i] if i < len(names) else None
        if entry["name"] != expected:
            raise DataError(f"{where}: entry {i} is named {entry['name']}, not {expected}")


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != COUNTRIES_MEMBER + 1:
        raise ValueError(f"{len(members)} members, not {COUNTRIES_MEMBER + 1}")
    names = bank_messages(COUNTRIES_BANK)
    countries = [{"name": names[i] if i < len(names) else None, "state": state, "x": x, "y": y}
                 for i, (state, x, y) in enumerate(unpack(COUNTRY, members[COUNTRIES_MEMBER], "the countries"))]
    write(output / "countries.json", {"$schema": "countries.schema.json", "countries": countries})
    lists = []
    for member, (country, bank) in enumerate(SUBDIVIDED):
        region_names = bank_messages(bank)
        lists.append({"country": names[country], "regions": [
            {"name": region_names[i] if i < len(region_names) else None, "x": x, "y": y}
            for i, (x, y) in enumerate(unpack(REGION, members[member], f"member {member}"))]})
    write(output / "regions.json", {"$schema": "regions.schema.json", "countries": lists})
    print(f"wrote {len(countries)} countries and {len(lists)} countries' regions to {output}")


def pack(root: Path, output: Path):
    countries_path, regions_path = root / "countries.json", root / "regions.json"
    countries = load(countries_path, load_schema(root / "countries.schema.json"))["countries"]
    lists = load(regions_path, load_schema(root / "regions.schema.json"))["countries"]
    names = bank_messages(COUNTRIES_BANK)
    check_names(countries, names, label(countries_path))
    if len(lists) != len(SUBDIVIDED):
        raise DataError(f"{label(regions_path)}: {len(lists)} countries, not sSubdividedCountries' {len(SUBDIVIDED)}")
    members = []
    for i, (entry, (country, bank)) in enumerate(zip(lists, SUBDIVIDED)):
        if entry["country"] != names[country]:
            raise DataError(f"{label(regions_path)}: list {i} is {entry['country']}, but sSubdividedCountries has "
                            f"{names[country]} there")
        check_names(entry["regions"], bank_messages(bank), f"{label(regions_path)}: {entry['country']}")
        members.append(b"".join(REGION.pack(region["x"], region["y"]) for region in entry["regions"]))
    members.append(b"".join(COUNTRY.pack(c["state"], c["x"], c["y"]) for c in countries))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/geonet/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build Geonet's places' archive")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
