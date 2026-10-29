#!/usr/bin/env python3
"""The interactive sound system's tables in res/sound/interactive/, a JSON file per archive: dump writes them from the
ROM once, and pack builds the five archives from them, which the build does.

    interactive_sound_data.py dump extract/b2_us/files res/sound/interactive
    interactive_sound_data.py pack res/sound/interactive CITY DUNGEON ZONE SWITCH 3D

Each file is a list of the archive's entries, in its order; zones and BGM are their ZONE_* and SEQ_* constants, and a
set of a sequence's tracks is a list of track numbers, 0 to 15.
- cities.json (ARCID_ISS_CITY, a/1/3/3): the towns' units, which play the town's music louder the nearer the player is
  to its center (iss_city_unit.c): the volume at each of six distances, from the farthest.
- dungeons.json (ARCID_ISS_DUNGEON, a/1/4/6): the pitch, tempo and pan of a dungeon's music in each season
  (iss_dungeon_sys.c).
- zones.json (ARCID_ISS_ZONE, a/1/3/6): the tracks that fade in and out when the player enters a zone (iss_zone_sys.c).
- switches.json (ARCID_ISS_SWITCH, a/1/4/7): a BGM's switch sets, the tracks of each of its nine switches and the zones
  where they are on (iss_switch_set.c).
- 3d_sound.json (ARCID_ISS_3D_SOUND, a/1/8/2): the BGMs that play in 3D and their units' tracks (iss_3ds_sys.c).

Both versions have the same files. interactive_sound.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

KINDS = {"cities": "a/1/3/3", "dungeons": "a/1/4/6", "zones": "a/1/3/6", "switches": "a/1/4/7", "3d_sound": "a/1/8/2"}

CITY_KEYS = 6  # ISS_CITY_UNIT_KEY_COUNT
CITY = struct.Struct(f"<HH3i{CITY_KEYS}B{CITY_KEYS}B{CITY_KEYS}B{CITY_KEYS}B")  # ISSCityUnitData
SEASONS = ["spring", "summer", "autumn", "winter"]
DUNGEON = struct.Struct("<HH4h4H4h")  # ISSDungeonData
ZONE = struct.Struct("<4H")  # ISSZoneConfig
SWITCH_COUNT = 9  # ISS_SWITCH_COUNT
SWITCH = struct.Struct(f"<I{SWITCH_COUNT}HHB")
SOUND_3D = struct.Struct("<IHH")  # ISS3DSoundConfig


def tracks(mask: int) -> list[int]:
    return [track for track in range(16) if mask >> track & 1]


def mask(numbers: list[int]) -> int:
    return sum(1 << track for track in set(numbers))


def zone(number: int):
    return name("ZONE_", number)


def bgm(number: int):
    return name("SEQ_", number)


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def city_json(data: bytes) -> dict:
    zone_id, padding, x, y, z, *keys = CITY.unpack(data)
    check(padding == 0, "a city unit's padding isn't 0")
    volumes, ranges = keys[:CITY_KEYS], [keys[CITY_KEYS * (axis + 1):CITY_KEYS * (axis + 2)] for axis in range(3)]
    return {"zone": zone(zone_id), "center": {"x": x, "y": y, "z": z},
            "keys": [{"volume": volumes[i], "range": dict(zip("xyz", (r[i] for r in ranges)))}
                     for i in range(CITY_KEYS)]}


def city_bytes(entry: dict, where: str) -> bytes:
    keys = entry["keys"]
    return CITY.pack(value(entry["zone"], where), 0, *(entry["center"][axis] for axis in "xyz"),
                     *(key["volume"] for key in keys), *(key["range"][axis] for axis in "xyz" for key in keys))


def dungeon_json(data: bytes) -> dict:
    zone_id, padding, *seasons = DUNGEON.unpack(data)
    check(padding == 0, "a dungeon's padding isn't 0")
    return {"zone": zone(zone_id), **{field: dict(zip(SEASONS, seasons[4 * i:4 * i + 4]))
                                      for i, field in enumerate(["pitch", "tempo", "pan"])}}


def dungeon_bytes(entry: dict, where: str) -> bytes:
    return DUNGEON.pack(value(entry["zone"], where), 0,
                        *(entry[field][season] for field in ["pitch", "tempo", "pan"] for season in SEASONS))


def zone_json(data: bytes) -> dict:
    zone_id, tracks_in, tracks_out, fade = ZONE.unpack(data)
    return {"zone": zone(zone_id), "tracks_in": tracks(tracks_in), "tracks_out": tracks(tracks_out),
            "fade_frames": fade}


def zone_bytes(entry: dict, where: str) -> bytes:
    return ZONE.pack(value(entry["zone"], where), mask(entry["tracks_in"]), mask(entry["tracks_out"]),
                     entry["fade_frames"])


def switch_json(data: bytes) -> dict:
    bgm_id, *masks, fade, count = SWITCH.unpack_from(data)
    check(len(data) == SWITCH.size + 2 * count, "a switch set isn't its count of zones")
    zones = struct.unpack_from(f"<{count}H", data, SWITCH.size)
    return {"bgm": bgm(bgm_id), "switches": [tracks(m) for m in masks], "fade_frames": fade,
            "zones": [zone(z) for z in zones]}


def switch_bytes(entry: dict, where: str) -> bytes:
    zones = [value(z, where) for z in entry["zones"]]
    return (SWITCH.pack(value(entry["bgm"], where), *(mask(s) for s in entry["switches"]), entry["fade_frames"],
                        len(zones)) + struct.pack(f"<{len(zones)}H", *zones))


def sound_3d_json(data: bytes) -> dict:
    bgm_id, track_mask, padding = SOUND_3D.unpack(data)
    check(padding == 0, "a 3D sound config's padding isn't 0")
    return {"bgm": bgm(bgm_id), "tracks": tracks(track_mask)}


def sound_3d_bytes(entry: dict, where: str) -> bytes:
    return SOUND_3D.pack(value(entry["bgm"], where), mask(entry["tracks"]), 0)


CODECS = {"cities": (city_json, city_bytes), "dungeons": (dungeon_json, dungeon_bytes),
          "zones": (zone_json, zone_bytes), "switches": (switch_json, switch_bytes),
          "3d_sound": (sound_3d_json, sound_3d_bytes)}


def dump(files: Path, output: Path):
    for kind, archive in KINDS.items():
        entries = [CODECS[kind][0](data) for data in read_narc((files / archive).read_bytes())]
        write(output / f"{kind}.json", {"$schema": "interactive_sound.schema.json", kind: entries})
        print(f"wrote {len(entries)} {kind} entries to {output / f'{kind}.json'}")


def pack(root: Path, outputs: list[Path]):
    schema = load_schema(root / "interactive_sound.schema.json")
    for (kind, archive), output in zip(KINDS.items(), outputs):
        path = root / f"{kind}.json"
        entries = load(path, schema)[kind]
        output.parent.mkdir(parents=True, exist_ok=True)
        members = [CODECS[kind][1](entry, f"{label(path)}: entry {i}") for i, entry in enumerate(entries)]
        output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/sound/interactive/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the interactive sound archives")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=len(KINDS), metavar="ARCHIVE",
                             help="the cities', dungeons', zones', switches' and 3D sound's archives")
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
