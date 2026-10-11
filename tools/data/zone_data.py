#!/usr/bin/env python3
"""The zone headers in data/zones/, one JSON file per zone: dump writes them from the ROM once, and pack builds the
archive (a/0/1/2) from them, which the build does.

    zone_data.py dump extract/b2_us/files data/zones
    zone_data.py pack data/zones ARCHIVE

Each zone is data/zones/<zone>.json, named after its constant (black_city.json for ZONE_BLACK_CITY). The archive's
single entry holds the 615 headers in the order of data/constants/zones.txt. zone.schema.json describes each field.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

HEADER = struct.Struct("<BBHHHHH4HHHHHHHHHiii")
assert HEADER.size == 0x30
SEASONS = ["spring", "summer", "autumn", "winter"]
NO_ENCOUNTERS = 0x1FFF
FLAGS = [("cycling", 10), ("flag11", 11), ("escape_rope", 12), ("fly_from", 13), ("cycle_surf_bgm", 14),
         ("entralink_warp", 15)]


def file_name(zone: str) -> str:
    return zone.removeprefix("ZONE_").lower() + ".json"


def ordered_zones() -> list[str]:
    zones = sorted(load_list("zones").items(), key=lambda item: item[1])
    if [v for _, v in zones] != list(range(len(zones))):
        sys.exit("zones.txt doesn't number the zones from 0 in order")
    return [zone for zone, _ in zones]


def zone_json(header: bytes) -> dict:
    # The entities file is the zone's events, data/events/<zone>.json, at its place in data/events/order.json
    (map_type, npc_cache, area, matrix, scripts, init_scripts, text, *rest) = HEADER.unpack(header)
    bgm, (encounters, entities, parent, place, env, flags, cam_bound, icon, fly_x, fly_y, fly_z) = rest[:4], rest[4:]
    data = {"$schema": "zone.schema.json", "map_type": map_type, "npc_cache": npc_cache, "area": area,
            "matrix": name("MAP_MATRIX_", matrix), "scripts": name("SCRIPTS_", scripts)}
    if init_scripts != scripts + 1:
        data["init_scripts"] = name("SCRIPTS_", init_scripts)
    table = encounters & 0x1FFF
    data.update({
        "text": name("SCRIPT_TEXT_", text),
        "bgm": name("SEQ_", bgm[0]) if len(set(bgm)) == 1 else dict(zip(SEASONS, (name("SEQ_", s) for s in bgm))),
        "encounters": None if table == NO_ENCOUNTERS else name("ENCOUNTERS_", table),
        "enc_slot": encounters >> 13,
        "parent": name("ZONE_", parent),
        "place": name("PLACE_", place & 0x3FF),
        "place_display": place >> 10,
        "weather": env & 0x3F,
        "projection": env >> 6 & 7,
        "camera": env >> 9,
        "transition": flags & 0x1F,
        "battle_bg": flags >> 5 & 0x1F,
    })
    for flag, bit in FLAGS:
        if flag != "flag11" or not flags >> bit & 1:
            data[flag] = bool(flags >> bit & 1)
    data.update({"cam_bound": cam_bound, "name_icon": icon & 0x1FFF, "easy_level": icon >> 13,
                 "fly": {"x": fly_x, "y": fly_y, "z": fly_z}})
    return data


def dump(files: Path, output: Path):
    (data,) = read_narc((files / "a/0/1/2").read_bytes())
    zones = ordered_zones()
    if len(data) != HEADER.size * len(zones):
        sys.exit(f"{len(data) // HEADER.size} zone headers and {len(zones)} zones in zones.txt")
    for i, zone in enumerate(zones):
        write(output / file_name(zone), zone_json(data[HEADER.size * i:HEADER.size * (i + 1)]))
    print(f"wrote {len(zones)} zones to {output}")


def zone_bytes(data: dict, entities: int, where: str) -> bytes:
    bgm = data["bgm"]
    seasons = [value(bgm[s], where) for s in SEASONS] if isinstance(bgm, dict) else [value(bgm, where)] * 4
    encounters = NO_ENCOUNTERS if data["encounters"] is None else value(data["encounters"], where)
    flags = data["transition"] | data["battle_bg"] << 5
    for flag, bit in FLAGS:
        flags |= data.get(flag, flag == "flag11") << bit
    return HEADER.pack(
        data["map_type"], data["npc_cache"], data["area"], value(data["matrix"], where), value(data["scripts"], where),
        value(data.get("init_scripts", value(data["scripts"], where) + 1), where), value(data["text"], where),
        *seasons, encounters | data["enc_slot"] << 13, entities, value(data["parent"], where), value(data["place"], where) | data["place_display"] << 10,
        data["weather"] | data["projection"] << 6 | data["camera"] << 9, flags, data["cam_bound"],
        data["name_icon"] | data["easy_level"] << 13, data["fly"]["x"], data["fly"]["y"], data["fly"]["z"],
    )


def pack(root: Path, output: Path):
    schema = load_schema(root / "zone.schema.json")
    events = json.loads((root.parent / "events" / "order.json").read_text())
    headers = []
    for zone in ordered_zones():
        path = root / file_name(zone)
        if zone not in events:
            raise DataError(f"data/events/order.json: {zone} isn't in it")
        headers.append(zone_bytes(load(path, schema), events.index(zone), label(path)))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([b"".join(headers)]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/zones/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the zone header archive from data/zones/")
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
