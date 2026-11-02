#!/usr/bin/env python3
"""The battle regulations (ARCID_REGULATION, a/1/0/6) in res/battle/regulations/, a JSON file for each: dump writes them
from the ROM once, and pack builds the archive from them, which the build does.

    regulation_data.py dump extract/b2_us/files res/battle/regulations
    regulation_data.py pack res/battle/regulations ARCHIVE

<n>.json is regulation n (Regulation in regulation.h), which PokeRegulation_Load and PokeRegulation_LoadNew read by
number for the Union Room, the Wi-Fi and wireless battles, the Battle Subway, Pokéstar Studios and the field's
battles: the rules a team must meet, the level it is set to, and the battle it is for. Its name is a message of
res/text/system/load_battle_type_text_to_strbuf.txt, species and items are their SPECIES_* and ITEM_* constants, and the
bit arrays are lists. Both versions have the same regulations. regulation.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import ROOT, DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

ARCHIVE = "a/1/0/6"
NAMES = ROOT / "res/text/system/load_battle_type_text_to_strbuf.txt"
SPECIES_BYTES, ITEM_BYTES, AC_BYTES = 0x52, 0x4C, 7
REGULATION = struct.Struct(f"<6BH2B{SPECIES_BYTES}s{ITEM_BYTES}sHBB{AC_BYTES}s9B")
assert REGULATION.size == 0xBC
# levelMode, as getLevel applies it: not at all, as the least or the most level allowed, as a cap that the level is set
# down to, as the level of all, or as the least level that the level is raised to
LEVEL_MODES = ["none", "minimum", "maximum", "cap", "all", "raise"]
# battleType: the battles that wifibattlematch_sys.c sets up for it, and the multi battle
BATTLE_TYPES = ["single", "double", "triple", "rotation", "multi"]
BOOLEANS = ["allow_same_species", "allow_same_item", "allow_nicknames", "show_partners"]
TAIL = ["unkb3", "unkb4", "allow_nicknames", "unkb6", "unkb7", "show_partners", "time_limit", "battle_type",
        "max_battles"]


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def bits(data: bytes) -> list[int]:
    return [bit for bit in range(8 * len(data)) if data[bit // 8] >> bit % 8 & 1]


def bit_bytes(numbers: list[int], size: int, where: str) -> bytes:
    data = bytearray(size)
    for number in numbers:
        if not 0 <= number < 8 * size:
            raise DataError(f"{where}: {number} doesn't fit in {size} bytes of bits")
        data[number // 8] |= 1 << number % 8
    return bytes(data)


def messages() -> list[str]:
    return [message_id for message_id, _ in sorted(message_numbers(NAMES), key=lambda item: item[1])]


def regulation_json(data: bytes, names: list[str]) -> dict:
    (name_id, unk1, min_count, max_count, level, level_mode, max_level_total, same_species, same_item, species, items,
     required, form, launcher, unkac, *tail) = REGULATION.unpack(data)
    check(name_id < len(names), "a regulation's name isn't a message")
    check(level_mode < len(LEVEL_MODES), "a regulation's level mode isn't known")
    entry = {"$schema": "regulation.schema.json", "name": names[name_id], "unk1": unk1, "min_count": min_count,
             "max_count": max_count, "level": level, "level_mode": LEVEL_MODES[level_mode],
             "max_level_total": max_level_total, "allow_same_species": same_species, "allow_same_item": same_item,
             "banned_species": [name("SPECIES_", s) for s in bits(species)],
             "banned_items": [name("ITEM_", i) for i in bits(items)],
             "required_species": name("SPECIES_", required), "required_form": form, "launcher": launcher,
             "unkac": bits(unkac), **dict(zip(TAIL, tail))}
    check(entry["battle_type"] < len(BATTLE_TYPES), "a regulation's battle type isn't known")
    entry["battle_type"] = BATTLE_TYPES[entry["battle_type"]]
    for key in BOOLEANS:
        check(entry[key] in (0, 1), f"a regulation's {key} isn't 0 or 1")
        entry[key] = bool(entry[key])
    return entry


def regulation_bytes(entry: dict, names: list[str], where: str) -> bytes:
    if entry["name"] not in names:
        raise DataError(f"{where}: {entry['name']} isn't a message of {label(NAMES)}")
    tail = [entry[key] for key in TAIL]
    tail[TAIL.index("battle_type")] = BATTLE_TYPES.index(entry["battle_type"])
    return REGULATION.pack(
        names.index(entry["name"]), entry["unk1"], entry["min_count"], entry["max_count"], entry["level"],
        LEVEL_MODES.index(entry["level_mode"]), entry["max_level_total"], int(entry["allow_same_species"]),
        int(entry["allow_same_item"]),
        bit_bytes([value(s, f"{where}: banned_species") for s in entry["banned_species"]], SPECIES_BYTES, where),
        bit_bytes([value(i, f"{where}: banned_items") for i in entry["banned_items"]], ITEM_BYTES, where),
        value(entry["required_species"], where), entry["required_form"], entry["launcher"],
        bit_bytes(entry["unkac"], AC_BYTES, where), *(int(item) for item in tail))


def dump(files: Path, output: Path):
    names = messages()
    members = read_narc((files / ARCHIVE).read_bytes())
    for i, data in enumerate(members):
        check(len(data) == REGULATION.size, f"regulation {i} is {len(data)} bytes, not {REGULATION.size}")
        write(output / f"{i}.json", regulation_json(data, names))
    print(f"wrote {len(members)} regulations to {output}")


def pack(root: Path, output: Path):
    schema = load_schema(root / "regulation.schema.json")
    names = messages()
    members = []
    while (root / f"{len(members)}.json").exists():
        path = root / f"{len(members)}.json"
        members.append(regulation_bytes(load(path, schema), names, label(path)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/battle/regulations/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the regulations' archive")
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
