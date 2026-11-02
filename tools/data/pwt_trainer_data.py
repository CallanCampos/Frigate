#!/usr/bin/env python3
"""The Pokémon World Tournament's Trainers, res/trainers/facilities/pwt_trainers.json (ARCID_WBT_TOOL, a/2/4/7): dump
writes it from the ROM once, and pack builds the archive from it, which the build does.

    pwt_trainer_data.py dump extract/b2_us/files res/trainers/facilities/pwt_trainers.json
    pwt_trainer_data.py pack res/trainers/facilities/pwt_trainers.json ARCHIVE

The archive's one file is a WbtTrainer (field/wbt.h) for each Trainer, which WbtTrainers_Load reads: the tournaments
the Trainer enters, the type the Type Expert Tournament picks them for, their trainer class and the music of their
battle, by constants, and the values that wbt_makematch.c passes to the Trainer's entrant.

Both versions have the same file. pwt_trainers.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

TRAINER = struct.Struct("<BBHBBBBBBHHH")  # WbtTrainer
TOURNAMENT_BITS = [11, 4, 10, 12, 14, 13, 15]  # inTournament11 to inTournament15, from bit 0


def trainer_json(values) -> dict:
    kind, gender, unk2, trainer_class, bits, tournament, expert_type, byte8, unk9, bgm, win_bgm, unk_e = values
    return {"trainer_class": name("TRAINER_CLASS_", trainer_class), "gender": gender, "type": kind, "unk2": unk2,
            "tournaments": [t for bit, t in enumerate(TOURNAMENT_BITS) if bits >> bit & 1],
            "unk5_7": bool(bits >> 7), "tournament": tournament, "expert_type": name("TYPE_", expert_type),
            "rank": byte8 & 7, "unk8_3": byte8 >> 3, "unk9": unk9, "bgm": name("SEQ_", bgm),
            "win_bgm": name("SEQ_", win_bgm), "unkE": unk_e}


def trainer_bytes(entry: dict, where: str) -> bytes:
    bits = sum(1 << TOURNAMENT_BITS.index(t) for t in set(entry["tournaments"])) | entry["unk5_7"] << 7
    return TRAINER.pack(entry["type"], entry["gender"], entry["unk2"], value(entry["trainer_class"], where), bits,
                        entry["tournament"], value(entry["expert_type"], where), entry["rank"] | entry["unk8_3"] << 3,
                        entry["unk9"], value(entry["bgm"], where), value(entry["win_bgm"], where), entry["unkE"])


def dump(files: Path, output: Path):
    (data,) = read_narc((files / "a/2/4/7").read_bytes())
    if len(data) % TRAINER.size:
        raise ValueError("the Trainers aren't whole WbtTrainers")
    trainers = [trainer_json(TRAINER.unpack_from(data, offset)) for offset in range(0, len(data), TRAINER.size)]
    write(output, {"$schema": "pwt_trainers.schema.json", "trainers": trainers})
    print(f"wrote {len(trainers)} Trainers to {output}")


def pack(path: Path, output: Path):
    trainers = load(path, load_schema(path.parent / "pwt_trainers.schema.json"))["trainers"]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([b"".join(trainer_bytes(entry, f"{label(path)}: Trainer {i}")
                                            for i, entry in enumerate(trainers))]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write pwt_trainers.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the tournament Trainers' archive")
    pack_parser.add_argument("path", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.path, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
