#!/usr/bin/env python3
"""The gates' news boards in res/field/gate_boards/: dump writes them from the ROM once, and pack builds the gates'
archive (ARCID_FIELD_GIMMICK_GATE_ZONE_DATA, a/1/7/2) and the news entries' (ARCID_FIELD_GIMMICK_GATE_BOARD_ENTRIES,
a/1/6/4) from them, which the build does.

    gate_board_data.py dump extract/b2_us/files res/field/gate_boards
    gate_board_data.py pack res/field/gate_boards GATES NEWS

The gate gimmick (field_gimmick_gate.c) shows a board in the gates between routes and towns that scrolls the day's
news. Each file is a list of its archive's members, in order:
- gates.json: a gate's board, a GimmickGateZoneData each, which GimmickGate_LoadZoneData finds by the player's zone and
  the game's version: where the board stands and which way it faces, its animation and the messages it shows;
- news.json: the news that a board shows while its event flag is set, a GimmickGateBoardEntry each, which
  GimmickGate_FindBoardEntry and GimmickGate_FindListEntry search.
Zones are their ZONE_* constants, directions DIR_*, versions VERSION_* (null for both) and flags their constants of
generated/flags.txt; messages are numbers of the system text bank 0x2b, which the boards' ElboardMessageArg names.

Both versions have the same files. gate_boards.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import LISTS_DIR, parse  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVES = {"gates": "a/1/7/2", "news": "a/1/6/4"}
GATE = struct.Struct("<6I19I4III")  # GimmickGateZoneData
NEWS = struct.Struct("<5I4I")  # GimmickGateBoardEntry
ZONE_MAX = 615  # include/field/zone.h: no zone
ROTATION = 9  # messageIds[3] to [11], which GimmickGate_AddWeekdayMessage3 to 5 take by the day of the week


def flag_names() -> dict[int, str]:
    return {number: flag for flag, number in reversed(parse(LISTS_DIR / "flags.txt").items())}


def version_json(version: int):
    return name("VERSION_", version) if version else None


def version_value(version, where: str) -> int:
    return 0 if version is None else value(version, where)


def zones_json(zones) -> list:
    """A list of up to four zones, ZONE_MAX for none: the zones before the first ZONE_MAX, which must end it."""
    count = zones.index(ZONE_MAX) if ZONE_MAX in zones else len(zones)
    if any(zone != ZONE_MAX for zone in zones[count:]):
        raise ValueError("a zone follows ZONE_MAX in a list of zones")
    return [name("ZONE_", zone) for zone in zones[:count]]


def zones_values(zones: list, where: str) -> list[int]:
    return [value(zone, where) for zone in zones] + [ZONE_MAX] * (4 - len(zones))


def gate_json(data: bytes) -> dict:
    zone, version, x, y, z, direction, *rest = GATE.unpack(data)
    messages, fallback, animation, enabled = rest[:19], rest[19:23], rest[23], rest[24]
    if direction > 3 or enabled > 1:
        raise ValueError("unexpected gate board values")
    return {"zone": name("ZONE_", zone), "version": version_json(version), "position": {"x": x, "y": y, "z": z},
            "direction": name("DIR_", direction), "animation": animation, "enabled": bool(enabled),
            "messages": {"date": messages[0], "message1": messages[1], "swarm": messages[2],
                         "rotation": list(messages[3:3 + ROTATION]), "weekdays": list(messages[3 + ROTATION:])},
            "weather_zones": zones_json(list(fallback))}


def gate_bytes(gate: dict, where: str) -> bytes:
    messages = gate["messages"]
    return GATE.pack(value(gate["zone"], where), version_value(gate["version"], where),
                     *(gate["position"][axis] for axis in "xyz"), value(gate["direction"], where),
                     messages["date"], messages["message1"], messages["swarm"], *messages["rotation"],
                     *messages["weekdays"], *zones_values(gate["weather_zones"], where), gate["animation"],
                     int(gate["enabled"]))


def news_json(data: bytes, flags: dict[int, str]) -> dict:
    version, flag, clear, message, kind, *zones = NEWS.unpack(data)
    if clear > 1:
        raise ValueError("unexpected news entry values")
    return {"version": version_json(version), "flag": flags.get(flag, flag), "clear_flag": bool(clear),
            "message": message, "type": kind, "zones": zones_json(zones)}


def news_bytes(entry: dict, where: str) -> bytes:
    return NEWS.pack(version_value(entry["version"], where), value(entry["flag"], where), int(entry["clear_flag"]),
                     entry["message"], entry["type"], *zones_values(entry["zones"], where))


def dump(files: Path, output: Path):
    gates = read_narc((files / ARCHIVES["gates"]).read_bytes())
    news = read_narc((files / ARCHIVES["news"]).read_bytes())
    if any(len(data) != GATE.size for data in gates) or any(len(data) != NEWS.size for data in news):
        raise ValueError("a gate board or news entry isn't its struct's size")
    flags = flag_names()
    write(output / "gates.json", {"$schema": "gate_boards.schema.json", "gates": [gate_json(data) for data in gates]})
    write(output / "news.json", {"$schema": "gate_boards.schema.json",
                                 "news": [news_json(data, flags) for data in news]})
    print(f"wrote {len(gates)} gate boards and {len(news)} news entries to {output}")


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "gate_boards.schema.json")
    codecs = {"gates": gate_bytes, "news": news_bytes}
    for kind, output in zip(ARCHIVES, outputs):
        path = root / f"{kind}.json"
        entries = load(path, schema)[kind]
        members = [codecs[kind](entry, f"{label(path)}: {kind}[{i}]") for i, entry in enumerate(entries)]
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/gate_boards/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the gate boards' archives from res/field/gate_boards/")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE", help="the gates' and the news' archives")
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
