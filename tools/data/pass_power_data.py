#!/usr/bin/env python3
"""The Pass Powers, res/pass_powers/pass_powers.json (ARCID_PASS_POWER, a/0/3/7): dump writes it from the ROM once, and
pack builds the archive from it, which the build does.

    pass_power_data.py dump extract/b2_us/files res/pass_powers
    pass_power_data.py pack res/pass_powers ARCHIVE

The archive's one file is a PassPowerData (save/high_link.h) for each Pass Power, by its number to PASS_POWER_MAX,
which PassPowerData_Create reads: the levels that unlock it, its price, its name and description, messages of
res/text/system/pass_powers.txt and monolith_power_info.txt by ID, its PASS_POWER_EFFECT_*, how strong the effect is
and how long it lasts.

Both versions have the same file. pass_powers.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
TEXT = ROOT / "res" / "text" / "system"
NAMES = TEXT / "pass_powers.txt"  # TEXT_BANK_PASS_POWERS
INFO = TEXT / "monolith_power_info.txt"  # TEXT_BANK_MONOLITH_POWER_INFO
POWER = struct.Struct("<BBHHHHBB")  # PassPowerData
# The effects of save/high_link.h
EFFECTS = ["PASS_POWER_EFFECT_ENCOUNTER_UP", "PASS_POWER_EFFECT_ENCOUNTER_DOWN", "PASS_POWER_EFFECT_HATCH",
           "PASS_POWER_EFFECT_BEFRIEND", "PASS_POWER_EFFECT_BARGAIN", "PASS_POWER_EFFECT_HP_RESTORE",
           "PASS_POWER_EFFECT_PP_RESTORE", "PASS_POWER_EFFECT_EXP_UP", "PASS_POWER_EFFECT_EXP_DOWN",
           "PASS_POWER_EFFECT_PRIZE_MONEY", "PASS_POWER_EFFECT_CAPTURE", "PASS_POWER_EFFECT_11",
           "PASS_POWER_EFFECT_12", "PASS_POWER_EFFECT_EXPLORING", "PASS_POWER_EFFECT_GROTTO", "PASS_POWER_EFFECT_LUCKY"]


def messages(path: Path) -> list[str]:
    return [message for message, _ in message_numbers(path)]


def number(texts: list[str], message: str, path: Path, where: str) -> int:
    if message not in texts:
        raise DataError(f"{where}: {message} is not a message of {label(path)}")
    return texts.index(message)


def dump(files: Path, output: Path):
    (data,) = read_narc((files / "a/0/3/7").read_bytes())
    if len(data) % POWER.size:
        raise ValueError("the Pass Powers aren't whole PassPowerData")
    names, info = messages(NAMES), messages(INFO)
    powers = []
    for offset in range(0, len(data), POWER.size):
        level_white, level_black, price, name_id, info_id, strength, effect, minutes = POWER.unpack_from(data, offset)
        if effect >= len(EFFECTS):
            raise ValueError(f"a Pass Power's effect {effect} is out of range")
        powers.append({"name": names[name_id], "description": info[info_id], "effect": EFFECTS[effect],
                       "value": strength, "minutes": minutes, "level_white": level_white,
                       "level_black": level_black, "price": price})
    write(output / "pass_powers.json", {"$schema": "pass_powers.schema.json", "pass_powers": powers})
    print(f"wrote {len(powers)} Pass Powers to {output}")


def pack(root: Path, output: Path):
    path = root / "pass_powers.json"
    powers = load(path, load_schema(root / "pass_powers.schema.json"))["pass_powers"]
    names, info = messages(NAMES), messages(INFO)
    data = b""
    for i, power in enumerate(powers):
        where = f"{label(path)}: Pass Power {i}"
        data += POWER.pack(power["level_white"], power["level_black"], power["price"],
                           number(names, power["name"], NAMES, where), number(info, power["description"], INFO, where),
                           power["value"], EFFECTS.index(power["effect"]), power["minutes"])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([data]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/pass_powers/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the Pass Powers' archive")
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
