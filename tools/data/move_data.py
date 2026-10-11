#!/usr/bin/env python3
"""The move data in data/moves/, one JSON file per move, as pokeplatinum's res/moves/: dump writes them from the ROM
once, and pack builds the archive (a/0/2/1) from them, which the build does.

    move_data.py dump extract/b2_us/files data/moves
    move_data.py pack data/moves ARCHIVE

Each move is data/moves/<move>/data.json, named after its constant (pound/ for MOVE_POUND), and the archive has them
in the order of data/constants/moves.txt. move.schema.json describes each field.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, constants, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_data import message_lines  # noqa: E402
from tools.text.text_sources import to_json  # noqa: E402

RECORD = struct.Struct("<6BbBH6BHbbB3B3b3B2sI")
assert RECORD.size == 0x24
MARKER = b"SS"
# Files of the system messages (a/0/0/2) with the moves' names and descriptions, one line per move
NAMES, DESCRIPTIONS = 403, 402


def directory(move: str) -> str:
    return move.removeprefix("MOVE_").lower()


def ordered_moves() -> list[str]:
    moves = sorted(load_list("moves").items(), key=lambda item: item[1])
    if [v for _, v in moves] != list(range(len(moves))):
        sys.exit("moves.txt doesn't number the moves from 0 in order")
    return [move for move, _ in moves]


def stat_names() -> dict[int, str]:
    return {v: k for k, v in reversed(constants().items()) if k.startswith("BATTLEMON_") and k.endswith("_STAGE")}


def move_json(record: bytes) -> dict:
    (type_, quality, category, power, accuracy, pp, priority, hits, condition, chance, duration, min_turns, max_turns,
     crit, flinch, effect, drain, heal, target, *rest) = RECORD.unpack(record)
    stats, stages, chances, marker, flags = rest[0:3], rest[3:6], rest[6:9], rest[9], rest[10]
    if marker != MARKER:
        raise ValueError("no SS marker")
    changes = [{"stat": stat_names().get(stats[i], stats[i]), "stages": stages[i], "chance": chances[i]}
               for i in range(3)]
    while changes and not (stats[len(changes) - 1] or stages[len(changes) - 1] or chances[len(changes) - 1]):
        changes.pop()
    return {
        "$schema": "../move.schema.json",
        "type": name("TYPE_", type_),
        "quality": name("MOVE_QUALITY_", quality),
        "category": name("MOVE_CATEGORY_", category),
        "power": power,
        "accuracy": accuracy,
        "pp": pp,
        "priority": priority,
        "hits": {"min": hits & 0xF, "max": hits >> 4},
        "inflicts": {"condition": name("CONDITION_", condition), "chance": chance, "duration": duration,
                     "min_turns": min_turns, "max_turns": max_turns},
        "crit_stage": crit,
        "flinch_chance": flinch,
        "effect": name("BATTLE_EFFECT_", effect),
        "drain": drain,
        "heal": heal,
        "target": name("MOVE_TARGET_", target),
        "stat_changes": changes,
        "flags": [name("MOVE_FLAG_", 1 << bit) for bit in range(32) if flags >> bit & 1],
    }


def dump(files: Path, output: Path):
    members = read_narc((files / "a/0/2/1").read_bytes())
    moves = ordered_moves()
    if len(members) != len(moves):
        sys.exit(f"{len(members)} moves in the archive and {len(moves)} in moves.txt")
    names, descriptions = (message_lines(files / "a/0/0/2", n) for n in (NAMES, DESCRIPTIONS))
    for index, (move, member) in enumerate(zip(moves, members)):
        data = move_json(member)
        text = {"name": to_json(names[index]), "description": to_json(descriptions[index])}
        write(output / directory(move) / "data.json", {"$schema": data.pop("$schema"), **text, **data})
    print(f"wrote {len(members)} moves to {output}")


def move_bytes(data: dict, where: str) -> bytes:
    inflicts = data["inflicts"]
    changes = data["stat_changes"] + [{"stat": 0, "stages": 0, "chance": 0}] * (3 - len(data["stat_changes"]))
    flags = 0
    for flag in data["flags"]:
        flags |= value(flag, where)
    return RECORD.pack(
        value(data["type"], where), value(data["quality"], where), value(data["category"], where), data["power"],
        data["accuracy"], data["pp"], data["priority"], data["hits"]["min"] | data["hits"]["max"] << 4,
        value(inflicts["condition"], where), inflicts["chance"], inflicts["duration"], inflicts["min_turns"],
        inflicts["max_turns"], data["crit_stage"], data["flinch_chance"], value(data["effect"], where), data["drain"],
        data["heal"], value(data["target"], where), *(value(c["stat"], where) for c in changes),
        *(c["stages"] for c in changes), *(c["chance"] for c in changes), MARKER, flags,
    )


def pack(root: Path, output: Path):
    schema = load_schema(root / "move.schema.json")
    members = []
    for move in ordered_moves():
        path = root / directory(move) / "data.json"
        members.append(move_bytes(load(path, schema), label(path)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/moves/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the move data archive from data/moves/")
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
