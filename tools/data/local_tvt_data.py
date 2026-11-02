#!/usr/bin/env python3
"""The Xtransceiver's calls in res/local_tvt/, a JSON file per archive: dump writes them from the ROM once, and pack
builds the two archives from them, which the build does.

    local_tvt_data.py dump extract/b2_us/files res/local_tvt
    local_tvt_data.py pack res/local_tvt CALL_DATA SCRIPTS

- calls.json (ARCID_LOCAL_TVT_CALL_DATA, a/2/4/0, one file of CALL_DATA_COUNT LocalTvtCallEntry): the calls the player
  can make, each a person's call after a count of calls with them (local_tvt_calldata.c), with where the person is
  and the messages of TEXT_BANK_LOCAL_TVT_TALK it says, by ID.
- scripts.json (ARCID_LOCAL_TVT_SCRIPT, a/2/3/4): the calls the story makes, script n being file n - 1 with its
  messages in TEXT_BANK_LOCAL_TVT_TALK + n (local_tvt_scriptmode.c): the persons of its four faces and their zones,
  and its commands, which the packer ends with SCRIPT_COMMAND_END.

Zones are their ZONE_* constants and messages their IDs in their text files. Both versions have the same files.
local_tvt.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, constants, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ARCHIVES = {"calls": "a/2/4/0", "scripts": "a/2/3/4"}

CALL_COUNT = 142  # CALL_DATA_COUNT
CALL = struct.Struct("<10H")  # LocalTvtCallEntry
# The persons the player can call (local_tvt_common.h) and where a call's face is
PERSONS = ["LOCAL_TVT_CALL_DOCTOR", "LOCAL_TVT_CALL_RIVAL", "LOCAL_TVT_CALL_CHELEN", "LOCAL_TVT_CALL_SUPPORT",
           "LOCAL_TVT_CALL_MOTHER", "LOCAL_TVT_CALL_SECRET"]
BACKGROUNDS = ["zone", "background_0", "background_1"]  # CALL_BACKGROUND_*
CALL_MESSAGES = ["greeting_message", "chat_message", "bye_message"]

FACES = 4  # LOCAL_TVT_FACES
PERSON_NONE = 0xFF  # LOCAL_TVT_PERSON_NONE
HEADER = struct.Struct(f"<{FACES}B{FACES}H")  # LocalTvtScriptHeader
COMMAND = struct.Struct("<BBH")  # LocalTvtScriptCommand
COMMANDS = ["end", "talk", "wait", "se", "wait_se"]  # SCRIPT_COMMAND_*
# What each command's param is: a message, frames to wait, a sound effect, or nothing it reads
PARAMS = {"talk": "message", "wait": "frames", "se": "se", "wait_se": "param"}


def bank_messages(bank: int) -> list[str]:
    """The message IDs of a system text bank, whose file is named after its TEXT_BANK_* constant."""
    constant = name("TEXT_BANK_", bank)
    if not isinstance(constant, str):
        raise DataError(f"text bank {bank} has no constant")
    path = ROOT / "res" / "text" / "system" / f"{constant.removeprefix('TEXT_BANK_').lower()}.txt"
    return [message for message, _ in message_numbers(path)]


def talk_bank() -> int:
    return constants()["TEXT_BANK_LOCAL_TVT_TALK"]


def message_number(messages: list[str], message: str, where: str) -> int:
    if message not in messages:
        raise DataError(f"{where}: {message} is not a message of its text bank")
    return messages.index(message)


def call_json(data: bytes, messages: list[str]) -> dict:
    person, count, background, zone, listed, *texts, extra, unk12 = CALL.unpack(data)
    if person >= len(PERSONS) or background >= len(BACKGROUNDS) or listed > 1:
        raise ValueError("a call's person, background or listing is out of range")
    return {"person": PERSONS[person], "call_count": count, "background": BACKGROUNDS[background],
            "zone": name("ZONE_", zone), "listed": bool(listed),
            **{key: messages[text] for key, text in zip(CALL_MESSAGES, texts)},
            "extra_message": messages[extra] if extra else None, "unk12": unk12}


def call_bytes(entry: dict, messages: list[str], where: str) -> bytes:
    extra = entry["extra_message"]
    return CALL.pack(PERSONS.index(entry["person"]), entry["call_count"], BACKGROUNDS.index(entry["background"]),
                     value(entry["zone"], where), entry["listed"],
                     *(message_number(messages, entry[key], where) for key in CALL_MESSAGES),
                     message_number(messages, extra, where) if extra is not None else 0, entry["unk12"])


def script_json(data: bytes, messages: list[str]) -> dict:
    *header, = HEADER.unpack_from(data)
    persons, zones = header[:FACES], header[FACES:]
    commands = [COMMAND.unpack_from(data, offset) for offset in range(HEADER.size, len(data), COMMAND.size)]
    if (len(data) - HEADER.size) % COMMAND.size or commands[-1] != (0, 0, 0) or any(c[0] == 0 for c in commands[:-1]):
        raise ValueError("a script doesn't end with its one SCRIPT_COMMAND_END")
    entries = []
    for command, face, param in commands[:-1]:
        kind = COMMANDS[command]
        entries.append({"command": kind, "face": face, PARAMS[kind]: messages[param] if kind == "talk" else param})
    return {"persons": [None if p == PERSON_NONE else p for p in persons], "zones": [name("ZONE_", z) for z in zones],
            "commands": entries}


def script_bytes(entry: dict, messages: list[str], where: str) -> bytes:
    data = HEADER.pack(*(PERSON_NONE if p is None else p for p in entry["persons"]),
                       *(value(z, where) for z in entry["zones"]))
    for command in entry["commands"]:
        kind = command["command"]
        param = command[PARAMS[kind]]
        data += COMMAND.pack(COMMANDS.index(kind), command["face"],
                             message_number(messages, param, where) if kind == "talk" else param)
    return data + COMMAND.pack(0, 0, 0)


def dump(files: Path, output: Path):
    (calls_data,) = read_narc((files / ARCHIVES["calls"]).read_bytes())
    if len(calls_data) != CALL.size * CALL_COUNT:
        raise ValueError(f"the call data is {len(calls_data)} bytes")
    talk = bank_messages(talk_bank())
    calls = [call_json(calls_data[offset:offset + CALL.size], talk) for offset in range(0, len(calls_data), CALL.size)]
    write(output / "calls.json", {"$schema": "local_tvt.schema.json", "calls": calls})
    members = read_narc((files / ARCHIVES["scripts"]).read_bytes())
    scripts = [script_json(data, bank_messages(talk_bank() + n)) for n, data in enumerate(members, 1)]
    write(output / "scripts.json", {"$schema": "local_tvt.schema.json", "scripts": scripts})
    print(f"wrote {len(calls)} calls and {len(scripts)} scripts to {output}")


def pack(root: Path, calls_out: Path, scripts_out: Path):
    schema = load_schema(root / "local_tvt.schema.json")
    path = root / "calls.json"
    talk = bank_messages(talk_bank())
    calls = load(path, schema)["calls"]
    data = b"".join(call_bytes(entry, talk, f"{label(path)}: call {i}") for i, entry in enumerate(calls))
    path = root / "scripts.json"
    scripts = load(path, schema)["scripts"]
    members = [script_bytes(entry, bank_messages(talk_bank() + n), f"{label(path)}: script {n}")
               for n, entry in enumerate(scripts, 1)]
    for output in (calls_out, scripts_out):
        output.parent.mkdir(parents=True, exist_ok=True)
    calls_out.write_bytes(write_narc([data]))
    scripts_out.write_bytes(write_narc(members))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/local_tvt/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the Xtransceiver's call data and scripts")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("calls", type=Path)
    pack_parser.add_argument("scripts", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.calls, args.scripts)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
