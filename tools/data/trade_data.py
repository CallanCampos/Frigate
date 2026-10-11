#!/usr/bin/env python3
"""The in-game trades in data/trades/, one JSON file per trade: dump writes them from the ROM once, and pack builds the
archive (a/1/6/3) from them, which the build does.

    trade_data.py dump extract/b2_us/files data/trades
    trade_data.py pack data/trades ARCHIVE

Each trade is data/trades/<trade>.json, named after its constant (petilil.json for TRADE_PETILIL), and the archive has
them in the order of data/constants/trades.txt. A trade's file also has the Pokémon's nickname and its trainer's name,
which the text file of the trades' names (system message file 37) takes with \\from{trades.names}, two lines per
trade, in the order the packer gives their message IDs. trade.schema.json describes each field.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_data import message_lines  # noqa: E402
from tools.text.text_sources import ordered, to_json  # noqa: E402

OFFER = struct.Struct("<27I")
RANDOM = 0xFF
STATS = ["hp", "attack", "defense", "speed", "special_attack", "special_defense"]
CONTEST = ["cool", "beauty", "cute", "smart", "tough"]
# The file of the system messages (a/0/0/2) with the trades' nicknames and trainers' names
NAMES = 37


def file_name(trade: str) -> str:
    return trade.removeprefix("TRADE_").lower() + ".json"


def random_or(number: int):
    return None if number == RANDOM else number


def trade_json(offer: bytes, index: int, names: list[str]) -> dict:
    (own_index, species, form, level, *rest) = OFFER.unpack(offer)
    ivs, (ability, nature, sex, trainer_id), rest = rest[0:6], rest[6:10], rest[10:]
    contest, (item, gender, unk54, region, wanted, wanted_sex, nickname, trainer_name) = rest[0:5], rest[5:]
    if own_index != index or (nickname, trainer_name) != (2 * index, 2 * index + 1):
        raise ValueError(f"trade {index} isn't numbered as its place in the archive")
    return {
        "$schema": "trade.schema.json",
        "nickname": to_json(names[nickname]),
        "trainer_name": to_json(names[trainer_name]),
        "species": name("SPECIES_", species),
        "form": form,
        "level": level,
        "ivs": {stat: random_or(iv) for stat, iv in zip(STATS, ivs)},
        "ability_slot": ability,
        "nature": None if nature == RANDOM else name("NATURE_", nature),
        "sex": None if sex == RANDOM else name("GENDER_", sex),
        "trainer_id": trainer_id,
        "contest": dict(zip(CONTEST, contest)),
        "held_item": name("ITEM_", item),
        "trainer_gender": name("GENDER_", gender),
        "unk54": unk54,
        "region": region,
        "wanted_species": name("SPECIES_", wanted),
        "wanted_sex": name("GENDER_", wanted_sex),
    }


def dump(files: Path, output: Path):
    offers = read_narc((files / "a/1/6/3").read_bytes())
    trades = ordered("trades")
    if len(offers) != len(trades):
        sys.exit(f"{len(offers)} trades in the archive and {len(trades)} in trades.txt")
    names = message_lines(files / "a/0/0/2", NAMES)
    for index, (trade, offer) in enumerate(zip(trades, offers)):
        write(output / file_name(trade), trade_json(offer, index, names))
    print(f"wrote {len(offers)} trades to {output}")


def trade_bytes(data: dict, index: int, where: str) -> bytes:
    def or_random(number):
        return RANDOM if number is None else number

    return OFFER.pack(
        index, value(data["species"], where), data["form"], data["level"],
        *(or_random(data["ivs"][stat]) for stat in STATS), data["ability_slot"],
        RANDOM if data["nature"] is None else value(data["nature"], where),
        RANDOM if data["sex"] is None else value(data["sex"], where),
        data["trainer_id"], *(data["contest"][c] for c in CONTEST), value(data["held_item"], where),
        value(data["trainer_gender"], where), data["unk54"], data["region"], value(data["wanted_species"], where),
        value(data["wanted_sex"], where), 2 * index, 2 * index + 1,
    )


def pack(root: Path, output: Path):
    schema = load_schema(root / "trade.schema.json")
    members = []
    for index, trade in enumerate(ordered("trades")):
        path = root / file_name(trade)
        members.append(trade_bytes(load(path, schema), index, label(path)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/trades/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the trade archive from data/trades/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
