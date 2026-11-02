#!/usr/bin/env python3
"""The musical's four built-in programs in res/musicals/programs/<n>/: dump writes them from the ROM once, and pack
builds their archives from them, which the build does.

    musical_program_data.py dump extract/b2_us/files res/musicals/programs
    musical_program_data.py pack res/musicals/programs ARCHIVE0 ... ARCHIVE3 --scripts SCRIPT.bin ...

Program n is the archive ARCID_MUSICAL_PROGRAM + n (a/1/1/2 to a/1/1/5), which MusicalSystem_LoadProgramData
(musical_system.c) loads; a downloaded program comes in an archive of the same members. Its directory holds them:
- data.json, member 0: the program's MusicalProgramData (musical_program.c), its audience's points for each kind of
  prop and its six performers, the Pokémon the game picks the other three entrants from. program.schema.json describes
  the fields.
- messages.txt, member 1: the program's messages, as res/text/'s files (tools/text/text_data.py); the first is its
  title.
- script.s, member 2: the stage's scripts, assembled with include/asm/sta_script.inc (tools/script/sta_script.py) by
  the build, which passes the assembled binaries to pack with --scripts.
- bank.json, sequence.s and waves/, members 3 to 5: a sound bank, sequence and wave archive, which the stage plays
  with PlayStrm and PlayWave in place of the sound archive's, in res/sound/'s formats (sbnk.py, sseq.py, swar.py). The
  bank's instruments name a wave archive by its slot, 0 to 3, among the four of the bank of the sequence they play as.

Both versions have the same programs.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data import sbnk, sseq  # noqa: E402
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.data.sound_data import dump_waves, pack_waves  # noqa: E402
from tools.text.text_data import message_ids, pack_file, unpack_file  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

PROGRAMS = 4
FIRST_ARCHIVE = 112  # ARCID_MUSICAL_PROGRAM, a/1/1/2
# The performers' names are these messages, and their Pokémon's the same lines of musical_event.txt (musical_event.c)
NAMES = Path(__file__).resolve().parents[2] / "res/text/system/btl_main_9.txt"
# The waves are a wave archive of the sound archive's format
WAVES_SCHEMA = Path(__file__).resolve().parents[2] / "res/sound/wave_archive.schema.json"

HEADER = struct.Struct("<B4BBB5B")
PERFORMERS = 6
PERFORMER = struct.Struct("<HBBBBHHH")
PROPS = 8
PROP = struct.Struct("<HBB")
# What the original has in the bytes the struct pads, and the item of a prop slot that is empty
FILL = 0xAA
NO_PROP = 0x1FF
NO_PROP_SLOT = 3

DATA, MESSAGES, SCRIPT, BANK, SEQUENCE, WAVES = range(6)


def archive_path(program: int) -> str:
    number = f"{FIRST_ARCHIVE + program:03d}"
    return f"a/{number[0]}/{number[1]}/{number[2]}"


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def names() -> dict[int, str]:
    return {number: message_id for message_id, number in message_numbers(NAMES)}


def program_json(data: bytes) -> dict:
    check(len(data) == HEADER.size + PERFORMERS * (PERFORMER.size + PROPS * PROP.size), "the program's size")
    base, *rest = HEADER.unpack_from(data, 0)
    points, bonus, prop_list, unk07 = rest[:4], rest[4], rest[5], rest[6:]
    by_number = names()
    performers = []
    offset = HEADER.size
    for _ in range(PERFORMERS):
        species, obj_code, msg, personality, form, unk06, min_shots, fill = PERFORMER.unpack_from(data, offset)
        check(fill == FILL, "a performer's padding")
        offset += PERFORMER.size
        props = []
        for slot in range(PROPS):
            item, position, fill = PROP.unpack_from(data, offset)
            offset += PROP.size
            check(fill == FILL, "a prop's padding")
            if item == NO_PROP:
                check(position == NO_PROP_SLOT, "an empty prop's slot")
            else:
                check(len(props) == slot, "a prop after an empty one")
                props.append([item, position])
        performers.append({"species": name("SPECIES_", species), "form": form, "obj_code": name("OBJ_CODE_", obj_code),
                           "name": by_number.get(msg, msg), "personality_byte_2": personality, "unk06": unk06,
                           "min_shots": min_shots, "props": props})
    return {"$schema": "../program.schema.json", "base_program": base, "points": list(points), "bonus": bonus,
            "prop_list": prop_list, "unk07": list(unk07), "performers": performers}


def program_bytes(data: dict, where: str) -> bytes:
    by_id = {message_id: number for message_id, number in message_numbers(NAMES)}
    out = HEADER.pack(data["base_program"], *data["points"], data["bonus"], data["prop_list"], *data["unk07"])
    for i, performer in enumerate(data["performers"]):
        at = f"{where}: performers[{i}]"
        msg = performer["name"]
        if isinstance(msg, str):
            if msg not in by_id:
                raise DataError(f"{at}: {msg} isn't a message of {label(NAMES)}")
            msg = by_id[msg]
        out += PERFORMER.pack(value(performer["species"], at), value(performer["obj_code"], at), msg,
                              performer["personality_byte_2"], performer["form"], performer["unk06"],
                              performer["min_shots"], FILL)
        props = [tuple(prop) for prop in performer["props"]]
        props += [(NO_PROP, NO_PROP_SLOT)] * (PROPS - len(props))
        out += b"".join(PROP.pack(item, slot, FILL) for item, slot in props)
    return out


def messages_text(program: int, data: bytes) -> str:
    lines = unpack_file(data).split("\n")[:-1]
    ids = iter(message_ids(f"MusicalProgram{program}_Text_", [line for line in lines if not line.startswith("\\pad{")]))
    return "".join((line if line.startswith("\\pad{") else f"{next(ids)} = {line}".rstrip()) + "\n" for line in lines)


def bank_bytes(path: Path) -> bytes:
    instruments = load(path, load_schema(path.parent.parent / "bank.schema.json"))["instruments"]
    try:
        return sbnk.write(instruments, list(range(4)))
    except sbnk.BankError as error:
        raise DataError(f"{label(path)}: {error}") from None
    except KeyError as error:
        raise DataError(f"{label(path)}: an instrument needs {error}") from None


def sequence_bytes(path: Path) -> bytes:
    try:
        return sseq.assemble(path.read_text(), label(path))
    except sseq.SequenceError as error:
        raise DataError(str(error)) from None


def waves_bytes(path: Path) -> bytes:
    data = load(path / "data.json", load_schema(WAVES_SCHEMA))
    return pack_waves(path, data["waves"])


def members(directory: Path, script: bytes, schema: dict) -> list[bytes]:
    data_path = directory / "data.json"
    messages = directory / "messages.txt"
    try:
        text = pack_file(messages.read_text(encoding="utf-8"))
    except ValueError as error:
        raise DataError(f"{label(messages)}: {error}") from None
    return [program_bytes(load(data_path, schema), label(data_path)), text, script, bank_bytes(directory / "bank.json"),
            sequence_bytes(directory / "sequence.s"), waves_bytes(directory / "waves")]


def dump(files: Path, root: Path):
    from tools.script.sta_script import disassemble

    schema = load_schema(root / "program.schema.json")
    for program in range(PROGRAMS):
        archive = read_narc((files / archive_path(program)).read_bytes())
        check(len(archive) == 6, f"program {program}'s archive has {len(archive)} members")
        directory = root / str(program)
        directory.mkdir(parents=True, exist_ok=True)
        write(directory / "data.json", program_json(archive[DATA]))
        (directory / "messages.txt").write_text(messages_text(program, archive[MESSAGES]), encoding="utf-8")
        header = (f"// Musical program {program}'s stage scripts: member {SCRIPT} of ARCID_MUSICAL_PROGRAM + {program} "
                  f"({archive_path(program)})\n\n")
        (directory / "script.s").write_text(f'{header}#include "asm/sta_script.inc"\n\n'
                                            f"{disassemble(archive[SCRIPT], program=True)}")
        write(directory / "bank.json", {"$schema": "../bank.schema.json",
                                        "instruments": sbnk.read(archive[BANK], list(range(4)))})
        (directory / "sequence.s").write_text(sseq.disassemble(archive[SEQUENCE]))
        waves = directory / "waves"
        waves.mkdir(exist_ok=True)
        write(waves / "data.json", {"$schema": "../../../../sound/wave_archive.schema.json",
                                    "waves": dump_waves(waves, archive[WAVES], f"program {program}'s waves")})
        if members(directory, archive[SCRIPT], schema) != archive:
            raise SystemExit(f"program {program} doesn't pack again from {directory}")
        print(f"wrote program {program} to {directory}")


def pack(root: Path, outputs: list[Path], scripts: list[Path]):
    schema = load_schema(root / "program.schema.json")
    by_program = {path.parent.name: path for path in scripts}
    for program, output in enumerate(outputs):
        if str(program) not in by_program:
            raise DataError(f"no assembled script of program {program} was given")
        archive = write_narc(members(root / str(program), by_program[str(program)].read_bytes(), schema))
        output.parent.mkdir(parents=True, exist_ok=True)
        output.unlink(missing_ok=True)
        output.write_bytes(archive)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/musicals/programs/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the programs' archives")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=PROGRAMS, metavar="ARCHIVE")
    pack_parser.add_argument("--scripts", type=Path, nargs=PROGRAMS, required=True, metavar="SCRIPT",
                             help="each program's assembled script.s, build/<version>/res/musicals/programs/<n>/...")
    args = parser.parse_args()
    try:
        if args.command == "dump":
            dump(args.files, args.output)
        else:
            pack(args.root, args.outputs, args.scripts)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
