#!/usr/bin/env python3
"""The crowds of Castelia City's streets, res/field/castelia_rush/castelia_rush.json (ARCID_CASTELIA_RUSH, a/1/8/3):
dump writes it from the ROM once, and pack builds the archive from it, which the build does.

    castelia_rush_data.py dump extract/b2_us/files res/field/castelia_rush
    castelia_rush_data.py pack res/field/castelia_rush ARCHIVE

castelia_rush.c reads a zone's crowd, the first 0x32 bytes of a CasteliaRushConfig, from the file that the zones'
file (CASTELIA_RUSH_FILE_ZONES) gives its zone, and the scripts of its passers-by from CASTELIA_RUSH_FILE_SCRIPTS:
"crowds" is the zones' files in order, each with its zone, and "scripts" the passers-by's scripts by object code.
Zones and object codes are their ZONE_* and OBJ_CODE_* constants.

Both versions have the same file. castelia_rush.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

PERIODS = 5  # the day periods
OBJ_CODES = 4
LANE_COUNT = 2  # LANE_COUNT
SCRIPTS = 6
CONFIG = struct.Struct(f"<{PERIODS}hHBB{OBJ_CODES}HBBBB")  # CasteliaRushConfig, before its lanes
LANE = struct.Struct("<6H")  # CasteliaRushLane
LANE_FIELDS = ["z_min", "z_max", "x_min", "x_max", "dir", "steps"]
ZONE = struct.Struct("<I")
SCRIPT = struct.Struct(f"<I{SCRIPTS}H")  # CasteliaRushScripts


def crowd_json(data: bytes, zone: int) -> dict:
    if len(data) != CONFIG.size + LANE.size * LANE_COUNT:
        raise ValueError(f"a crowd is {len(data)} bytes, not 0x32")
    values = CONFIG.unpack_from(data)
    intervals, (unk_a, lanes, obj_count, o0, o1, o2, o3, msg_base, msg_count, step_frames, unk19) = (
        values[:PERIODS], values[PERIODS:])
    return {"zone": name("ZONE_", zone), "intervals": list(intervals), "lane_count": lanes,
            "obj_code_count": obj_count, "obj_codes": [name("OBJ_CODE_", o) for o in (o0, o1, o2, o3)],
            "msg_base": msg_base, "msg_count": msg_count, "step_frames": step_frames,
            "lanes": [dict(zip(LANE_FIELDS, LANE.unpack_from(data, CONFIG.size + LANE.size * i)))
                      for i in range(LANE_COUNT)], "unkA": unk_a, "unk19": unk19}


def crowd_bytes(entry: dict, where: str) -> bytes:
    return CONFIG.pack(*entry["intervals"], entry["unkA"], entry["lane_count"], entry["obj_code_count"],
                       *(value(o, where) for o in entry["obj_codes"]), entry["msg_base"], entry["msg_count"],
                       entry["step_frames"], entry["unk19"]) + b"".join(
        LANE.pack(*(lane[field] for field in LANE_FIELDS)) for lane in entry["lanes"])


def dump(files: Path, output: Path):
    members = read_narc((files / "a/1/8/3").read_bytes())
    zones = [z for (z,) in ZONE.iter_unpack(members[-2])]
    if len(members) != len(zones) + 2 or len(members[-1]) % SCRIPT.size:
        raise ValueError("the archive isn't the crowds, their zones and the scripts")
    crowds = [crowd_json(data, zone) for data, zone in zip(members, zones)]
    scripts = [{"obj_code": name("OBJ_CODE_", obj), "scripts": list(ids)}
               for obj, *ids in SCRIPT.iter_unpack(members[-1])]
    write(output / "castelia_rush.json", {"$schema": "castelia_rush.schema.json", "crowds": crowds, "scripts": scripts})
    print(f"wrote {len(crowds)} crowds and {len(scripts)} object codes' scripts to {output}")


def pack(root: Path, output: Path):
    path = root / "castelia_rush.json"
    data = load(path, load_schema(root / "castelia_rush.schema.json"))
    where = label(path)
    members = [crowd_bytes(entry, f"{where}: crowd {i}") for i, entry in enumerate(data["crowds"])]
    members.append(b"".join(ZONE.pack(value(entry["zone"], where)) for entry in data["crowds"]))
    members.append(b"".join(SCRIPT.pack(value(entry["obj_code"], where), *entry["scripts"])
                            for entry in data["scripts"]))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/castelia_rush/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the Castelia crowds' archive")
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
