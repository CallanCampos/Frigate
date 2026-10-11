#!/usr/bin/env python3
"""The town map's table of places in data/town_map/places.json: dump writes it from the ROM once, and pack builds its
archive (a/0/8/5) for a version from it, which the build does.

    town_map_data.py dump extract/b2_us/files extract/w2_us/files data/town_map
    town_map_data.py pack data/town_map black2 ARCHIVE

Each place is 27 u16s, which TownMapData_GetParam reads by TOWNMAP_PARAM_*. Its texts are messages of
data/text/system/town_map.txt by ID; a place whose description differs between the versions gives one per version.
places.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, id_name, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

PLACE = struct.Struct("<27H")
NONE = 0xFFFF
VERSIONS = ["black2", "white2"]
TEXT = Path(__file__).parents[2] / "data/text/system/town_map.txt"


def point(x: int, y: int) -> dict:
    return {"x": x, "y": y}


def signed(v: int) -> int:
    return v - 0x10000 if v >= 0x8000 else v


def flag_name(flag: int):
    if flag == NONE:
        return None
    return name("TOWNMAP_FLAG_", flag) if flag >= 0xF000 else id_name("EVENT_FLAG_", flag)


def place_json(versions: list[tuple], messages: dict) -> dict:
    p = versions[0]
    if any(q[:17] != p[:17] or q[18:] != p[18:] for q in versions):
        raise ValueError("the versions' places differ in more than their descriptions")
    descriptions = [messages[q[17]] for q in versions]
    return {
        "zone": name("ZONE_", p[0]), "type": p[11], "radius": p[1], "position": point(p[2], p[3]),
        "cursor": point(p[4], p[5]), "hit": {"start": point(p[6], p[7]), "end": point(p[8], p[9]), "radius": p[10]},
        "fly": bool(p[12]), "unk13": p[13], "unk14": p[14], "arrival_flag": flag_name(p[15]),
        "shown_by": flag_name(p[16]),
        "description": descriptions[0] if len(set(descriptions)) == 1 else dict(zip(VERSIONS, descriptions)),
        "landmarks": [messages[m] for m in p[18:24] if m != NONE],
        "area": {"animation": p[24], "x": signed(p[25]), "y": signed(p[26])},
    }


def read_places(files: Path) -> list[tuple]:
    (data,) = read_narc((files / "a/0/8/5").read_bytes())
    return list(PLACE.iter_unpack(data))


def dump(black2: Path, white2: Path, output: Path):
    messages = {number: message_id for message_id, number in message_numbers(TEXT)}
    tables = [read_places(black2), read_places(white2)]
    places = [place_json(versions, messages) for versions in zip(*tables)]
    write(output / "places.json", {"$schema": "places.schema.json", "places": places})
    print(f"wrote {len(places)} places to {output / 'places.json'}")


def flag_value(flag, where: str) -> int:
    return NONE if flag is None else value(flag, where)


def place_bytes(place: dict, game: str, messages: dict, where: str) -> bytes:
    def message(message_id: str) -> int:
        if message_id not in messages:
            raise DataError(f"{where}: {message_id} isn't a message of {label(TEXT)}")
        return messages[message_id]

    description = place["description"]
    if isinstance(description, dict):
        description = description[game]
    landmarks = [message(m) for m in place["landmarks"]]
    hit = place["hit"]
    return PLACE.pack(
        value(place["zone"], where), place["radius"], place["position"]["x"], place["position"]["y"],
        place["cursor"]["x"], place["cursor"]["y"], hit["start"]["x"], hit["start"]["y"], hit["end"]["x"],
        hit["end"]["y"], hit["radius"], place["type"], int(place["fly"]), place["unk13"], place["unk14"],
        flag_value(place["arrival_flag"], where), flag_value(place["shown_by"], where), message(description),
        *landmarks, *[NONE] * (6 - len(landmarks)), place["area"]["animation"], place["area"]["x"] & 0xFFFF,
        place["area"]["y"] & 0xFFFF)


def pack(root: Path, game: str, output: Path):
    path = root / "places.json"
    data = load(path, load_schema(root / "places.schema.json"))
    messages = dict(message_numbers(TEXT))
    places = b"".join(place_bytes(place, game, messages, f"{label(path)}: places[{i}]")
                      for i, place in enumerate(data["places"]))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([places]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write data/town_map/ from both versions' extracted files")
    dump_parser.add_argument("black2", type=Path, help="Black 2's extracted files/ directory")
    dump_parser.add_argument("white2", type=Path, help="White 2's extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the town map's archive for a version")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("game", choices=VERSIONS)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.black2, args.white2, args.output)
        return
    try:
        pack(args.root, args.game, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
