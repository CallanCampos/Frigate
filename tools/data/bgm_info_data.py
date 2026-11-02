#!/usr/bin/env python3
"""The BGM info in res/sound/interactive/bgm_info.json, which interactive sound subsystem each BGM uses: dump writes it
from the ROM once, and pack builds ARCID_BGM_INFO (a/0/8/6) from it, which the build does.

    bgm_info_data.py dump extract/b2_us/files res/sound/interactive
    bgm_info_data.py pack res/sound/interactive ARCHIVE

The archive's one file is a count, a byte, and that many entries of a u16 BGM and a byte of its subsystem, unaligned,
which BGMInfo_LoadArcData reads (bgm_info.c) and BGMInfo_GetISSSubsystem searches for the BGM that plays: the
interactive sound system (iss_sys.c) runs that subsystem, and none for a BGM that isn't listed. BGMs are their SEQ_*
constants and subsystems the names of the ISS_SUBSYSTEM_* enum of include/system/iss_sys.h.

Both versions have the same file. bgm_info.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/0/8/6"
ENTRY = struct.Struct("<HB")
# The ISS_SUBSYSTEM_* enum, in its order
SUBSYSTEMS = ["ISS_SUBSYSTEM_NONE", "ISS_SUBSYSTEM_UNUSED_1", "ISS_SUBSYSTEM_ROAD", "ISS_SUBSYSTEM_CITY",
              "ISS_SUBSYSTEM_3D_SOUND", "ISS_SUBSYSTEM_DUNGEON", "ISS_SUBSYSTEM_UNUSED_6", "ISS_SUBSYSTEM_SWITCH",
              "ISS_SUBSYSTEM_ZONE"]


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != 1 or len(members[0]) != 1 + ENTRY.size * members[0][0]:
        raise ValueError("the BGM info isn't one file of its count of entries")
    entries = list(ENTRY.iter_unpack(members[0][1:]))
    if any(subsystem >= len(SUBSYSTEMS) for _, subsystem in entries):
        raise ValueError("a BGM's subsystem isn't one of ISS_SUBSYSTEM_*")
    write(output / "bgm_info.json", {"$schema": "bgm_info.schema.json", "bgms": [
        {"bgm": name("SEQ_", bgm), "subsystem": SUBSYSTEMS[subsystem]} for bgm, subsystem in entries]})
    print(f"wrote {len(entries)} BGMs to {output / 'bgm_info.json'}")


def pack(root: Path, output: Path):
    path = root / "bgm_info.json"
    bgms = load(path, load_schema(root / "bgm_info.schema.json"))["bgms"]
    data = bytes([len(bgms)]) + b"".join(
        ENTRY.pack(value(entry["bgm"], f"{label(path)}: bgms[{i}]"), SUBSYSTEMS.index(entry["subsystem"]))
        for i, entry in enumerate(bgms))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([data]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/sound/interactive/bgm_info.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the BGM info archive from res/sound/interactive/")
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
