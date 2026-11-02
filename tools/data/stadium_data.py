#!/usr/bin/env python3
"""The stadiums' trainers in res/field/stadium/trainers.json: dump writes it from the ROM once, and pack builds
ARCID_SCRCMD_STADIUM (a/2/0/6) from it, which the build does.

    stadium_data.py dump extract/b2_us/files res/field/stadium
    stadium_data.py pack res/field/stadium ARCHIVE

The archive's one file is a table of 0x84 StadiumTrainerEntry (include/field/stadium_script.h), which the stadium's
script commands load (scrcmd_stadium.c): Scrcmd_Stadium_StadiumSetupActorSingle and ...Double find a trainer by its
two keys (FindStadiumTrainerIndex) and give an NPC the trainer's battle script, ...Triple takes three trainers by
their place and gives three NPCs their models too, and Scrcmd_Stadium_StadiumResetTrainerFlags clears every trainer's
battle flag. Models are their OBJ_CODE_* constants and trainers their TRAINER_* constants of generated/trainers.txt.

Both versions have the same table. trainers.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.gen_constants import LISTS_DIR, parse  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/0/6"
ENTRY = struct.Struct("<4H")  # StadiumTrainerEntry
COUNT = 0x84  # FindStadiumTrainerIndex and Scrcmd_Stadium_StadiumResetTrainerFlags read this many


def dump(files: Path, output: Path):
    members = read_narc((files / ARCHIVE).read_bytes())
    if len(members) != 1 or len(members[0]) != COUNT * ENTRY.size:
        raise ValueError("the stadium's trainers aren't one file of 0x84 StadiumTrainerEntry")
    # By generated/trainers.txt alone, as the trainer classes' and sprites' constants share the prefix
    trainers = {number: trainer for trainer, number in reversed(parse(LISTS_DIR / "trainers.txt").items())}
    write(output / "trainers.json", {"$schema": "trainers.schema.json", "trainers": [
        {"unk00": unk00, "unk02": unk02, "model": name("OBJ_CODE_", model), "trainer": trainers.get(trainer, trainer)}
        for unk00, unk02, model, trainer in ENTRY.iter_unpack(members[0])]})
    print(f"wrote {COUNT} stadium trainers to {output / 'trainers.json'}")


def pack(root: Path, output: Path):
    path = root / "trainers.json"
    trainers = load(path, load_schema(root / "trainers.schema.json"))["trainers"]
    if len(trainers) != COUNT:
        raise DataError(f"{label(path)}: {len(trainers)} trainers, but the code reads {COUNT}")
    data = b"".join(ENTRY.pack(entry["unk00"], entry["unk02"], value(entry["model"], f"{label(path)}: trainers[{i}]"),
                               value(entry["trainer"], f"{label(path)}: trainers[{i}]"))
                    for i, entry in enumerate(trainers))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([data]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/field/stadium/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the stadium's trainers from res/field/stadium/")
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
