#!/usr/bin/env python3
"""Pokéstar Studios' cut-in scripts in res/pokewood/cutins.json: dump writes it from the ROM once, and pack builds the
archive from it, which the build does.

    pokewood_cutin_data.py dump extract/b2_us/files res/pokewood/cutins.json
    pokewood_cutin_data.py pack res/pokewood/cutins.json CUTIN_SCRIPT

ARCID_POKEWOOD_CUTIN_SCRIPT (a/2/7/5) holds the cut-ins that play in the movies' battles after some scenes
(pokewood_cutin.c): script n is the cut-in of sCutins[n]. A script is CUTIN_SCRIPT_HEADER_COUNT halfwords, its movie
and scene, as sCutins has them, and a halfword, none of which the code reads, then the resources it loads first, a set
of files of ARCID_POKEWOOD_CUTIN_GRA (res/graphics/pokewood_cutin_gra/) for each of the player's sexes. Steps of
CUTIN_STEP_SIZE halfwords follow, a command of sCommands and its CUTIN_STEP_ARG_COUNT arguments, which run in order to
the last; there are no jumps. Each command's arguments are named after what its handler reads them as; an argument it
doesn't read is written as argN when it isn't 0. Both versions have the same file. cutin.schema.json describes the
fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ARCHIVE = "a/2/7/5"
ARGS = 8  # CUTIN_STEP_ARG_COUNT
HEADER = struct.Struct("<11H")  # CUTIN_SCRIPT_HEADER_COUNT
STEP = struct.Struct(f"<{1 + ARGS}H")  # CUTIN_STEP_SIZE
RESOURCES = ["palette", "characters", "cells", "animations"]  # Cutin_LoadResources's files
SEXES = ["male", "female"]  # by the player's gender, 0 for male

MOTIONS = ["linear", "accelerating"]  # sMoveFuncs
# Cutin_GetCenterMove's and Cutin_GetDistMove's directions, and sNudgeFuncs's
DIRECTIONS = ["down", "up", "left", "right", "up_left", "up_right", "down_left", "down_right"]
NUDGES = ["up", "down", "left", "right"]

# sCommands in order: each command's arguments by index, as a field and what it is: a number, a name of a list, a
# boolean, a sound effect or a set of resources (four arguments)
COMMANDS = {
    "move": [("actor", int), ("motion", MOTIONS), ("direction", DIRECTIONS), ("frames", int)],
    "move_dist": [("actor", int), ("motion", MOTIONS), ("direction", DIRECTIONS), ("frames", int), ("distance", int)],
    "set_visible": [("actor", int), ("visible", bool)],
    "nudge": [("actor", int), ("direction", NUDGES)],
    "load_resources": [("male", "resources"), None, None, None, ("female", "resources")],
    "wait": [("frames", int)],
    "print_message": [("step", int), ("kind", int)],
    "view_effect": [],
    "play_se": [("se", "se")],
    "wait_sound": [],
    "stop_sound": [],
}


def read_args(kind: str) -> set[int]:
    """The arguments a command's handler reads."""
    read = set()
    for index, field in enumerate(COMMANDS[kind]):
        if field:
            read.update(range(index, index + (len(RESOURCES) if field[1] == "resources" else 1)))
    return read


def resources_json(values) -> dict:
    return dict(zip(RESOURCES, values))


def step_json(command: int, args: tuple) -> dict:
    if command >= len(COMMANDS):
        raise ValueError(f"command {command} is not of sCommands")
    kind = list(COMMANDS)[command]
    entry = {"command": kind}
    for index, field in enumerate(COMMANDS[kind]):
        if field is None:
            continue
        key, what = field
        if what == "resources":
            entry[key] = resources_json(args[index:index + len(RESOURCES)])
            continue
        arg = args[index]
        if isinstance(what, list):
            if arg >= len(what):
                raise ValueError(f"{kind}'s {key} {arg} is out of range")
            entry[key] = what[arg]
        elif what is bool:
            if arg > 1:
                raise ValueError(f"{kind}'s {key} {arg} is not a boolean")
            entry[key] = bool(arg)
        elif what == "se":
            entry[key] = name("SEQ_", arg)
        else:
            entry[key] = arg
    entry.update({f"arg{i}": arg for i, arg in enumerate(args) if i not in read_args(kind) and arg})
    return entry


def step_bytes(entry: dict, where: str) -> bytes:
    kind = entry["command"]
    if kind not in COMMANDS:
        raise DataError(f"{where}: {kind} is not a command")
    allowed = {"command"} | {field[0] for field in COMMANDS[kind] if field} | \
        {f"arg{i}" for i in range(ARGS) if i not in read_args(kind)}
    for key in entry.keys() - allowed:
        raise DataError(f"{where}: {kind} reads {key} as a named field" if key.startswith("arg") else
                        f"{where}: {kind} has no {key}")
    args = [entry.get(f"arg{i}", 0) for i in range(ARGS)]
    for index, field in enumerate(COMMANDS[kind]):
        if field is None:
            continue
        key, what = field
        if key not in entry:
            raise DataError(f"{where}: {kind} needs {key}")
        item = entry[key]
        if what == "resources":
            args[index:index + len(RESOURCES)] = [item[r] for r in RESOURCES]
        elif isinstance(what, list):
            args[index] = what.index(item)
        elif what == "se":
            args[index] = value(item, where)
        else:
            args[index] = int(item)
    return STEP.pack(list(COMMANDS).index(kind), *args)


def script_json(data: bytes) -> dict:
    if len(data) < HEADER.size or (len(data) - HEADER.size) % STEP.size:
        raise ValueError(f"a script of {len(data)} bytes isn't a header and whole steps")
    movie, scene, unk04, *resources = HEADER.unpack_from(data)
    steps = [STEP.unpack_from(data, offset) for offset in range(HEADER.size, len(data), STEP.size)]
    return {"movie": movie, "scene": scene, "unk04": unk04,
            **{sex: resources_json(resources[i * len(RESOURCES):(i + 1) * len(RESOURCES)])
               for i, sex in enumerate(SEXES)},
            "steps": [step_json(command, args) for command, *args in steps]}


def script_bytes(entry: dict, where: str) -> bytes:
    data = HEADER.pack(entry["movie"], entry["scene"], entry["unk04"],
                       *(entry[sex][r] for sex in SEXES for r in RESOURCES))
    return data + b"".join(step_bytes(step, f"{where}: step {i}") for i, step in enumerate(entry["steps"]))


def dump(files: Path, output: Path):
    scripts = [script_json(data) for data in read_narc((files / ARCHIVE).read_bytes())]
    write(output, {"$schema": "cutin.schema.json", "scripts": scripts})
    print(f"wrote {len(scripts)} cut-in scripts to {output}")


def pack(path: Path, output: Path):
    scripts = load(path, load_schema(path.parent / "cutin.schema.json"))["scripts"]
    members = [script_bytes(entry, f"{label(path)}: script {n}") for n, entry in enumerate(scripts)]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/pokewood/cutins.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the cut-in scripts' archive")
    pack_parser.add_argument("path", type=Path)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.path, args.output)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
