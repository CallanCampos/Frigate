#!/usr/bin/env python3
"""Plays a route on the port's program, a chain of input files each played from the save the run before left, and
holds the save it ends with to the save the oracle made of the same route, block by block: a replay test of levels T3
and T6 (port/docs/PLAN.md section 6) of the game's saves, as ADR 0023 compares the port's own play with melonDS's
(port/docs/adr/0023-oracle-routes-part-where-the-ds-cpu-time-decides.md): where it saves, byte by byte, each
difference declared with its traced cause.

    check_oracle_save.py REFERENCE --run FRAMES:INPUT [--run FRAMES:INPUT]... -- PROGRAM ARGUMENT...
    check_oracle_save.py REFERENCE --update ORACLE_SAVE

The first form runs PROGRAM with its arguments once for each --run, with --frames FRAMES, --input INPUT and the same
new --save-dir, which starts empty: the first run starts from a blank save, and each later one from the save the one
before left. Every run must end on its last frame: a trap stub, a crash or the watchdog fails the test, and the
program's status without the ROM (66) skips it. Then it checks the save file the last run left against REFERENCE:
each block of the main save's two copies, and the card's memory after them, has the SHA-1 the reference keeps, with
the bytes KNOWN_BLOCKS declares zeroed in both, and each block's footer holds the CRC of its data, which the game
checks when it loads the save. Exits 0 when every run and every block pass, 1 otherwise.

The second form writes REFERENCE, named <name>_<version>.save.tsv after one of oracle.py's SAVES, from ORACLE_SAVE,
the oracle's save of the route (oracle.py save NAME --out FILE, or the oracle_saves test's cache): it checks that the
file is the save SAVES keeps, by its SHA-1, finds the blocks by the CRCs of the save's table, and keeps their hashes
only, never the save (port/CLAUDE.md, what native means).

REFERENCE is tab-separated, with # comments: a header line, then "block offset size sha1" rows, the offset in the
first copy; block "after" is the card's memory after the two copies.

Standard library only.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools", "oracle"))

import oracle  # noqa: E402 - port/tools/oracle/oracle.py's SAVES, the oracle's saves and their SHA-1s

NO_ROM = 66
# Re-export these names for existing replay callers. Old 70-row content references remain readable; new
# references cover all 73 blocks, and every image has both copies checked in full.
from saveimage import (AFTER_COPIES, BACKUP_SIZE, BLOCK_ALIGN, BLOCK_COUNT, BLOCK_FOOTER_SIZE, COPIES, COPY_SIZE,
                       CRC_TABLE, crc16_ccitt, find_blocks, validate_image)  # noqa: E402

# The bytes of the port's save that differ from the oracle's by construction, by block (port/tools/oracle/README.md,
# "Saves"), each with its traced cause: the block's index (save_control.h's SAVE_BLOCK_ order), the ranges as (offset
# in the block, size), and why. The port plays the route from its own first save, not the oracle's, so the blocks New
# Game set up are its first save's
NEW_GAME_KEY = ("New Game encrypts the block with a key it seeds with the VBlank count, (OS_GetVBlankCount() << 8) "
                "| OS_GetVBlankCount(), and the port reaches New Game at another count than the DS (README.md, "
                "\"What the two formatted saves differ in\")")
# The first Pokémon's mail in the party block: the party's first PartyPkm at 8, its mail at 0x9c, and in the mail
# unk18 at 0x18 and each of the three messages' id at 0x22, 0x2a and 0x32
MAIL = 0x08 + 0x9c
# The four townspeople of Aspertia City who walk at random (move code 3, uid 9 to 12) are entries 6 to 9 of the field
# actors' block, 0x50 bytes an entry: their direction (0x0d, 2), their tile (0x26, 6) and their move's work (0x30, 0x10)
WALKERS = tuple((0x50 * entry + offset, size) for entry in range(6, 10)
                for offset, size in ((0x0d, 2), (0x26, 6), (0x30, 0x10)))
KNOWN_BLOCKS = {
    # SAVE_BLOCK_PARTY
    26: (((MAIL + 0x18, 6), (MAIL + 0x22, 2), (MAIL + 0x2a, 2), (MAIL + 0x32, 2)),
         "the starter's mail: PokeParty_CreatePkm takes it from the system heap uncleared (CreateMailData, "
         "src/system/mail.c), and ResetMailData sets neither its unk18 nor each message's id (PMSData_Clear), so "
         "those 12 bytes are what the heap held there, which differs between the DS's heap and the host's"),
    # SAVE_BLOCK_TRAINER_DATA: the PlayTime after the options and the player's info
    27: (((0x24, 8),),
         "the play time and the date and time of the save: the play time counts the CPU's time (playtime_ctrl.c), "
         "the clock runs with the video frames (README.md, \"Input files\"), and the DS takes more frames than the "
         "port to the same count"),
    # SAVE_BLOCK_WIFI_LIST: DWC's user data
    30: (((0x0c, 4), (0x3c, 4)),
         "the login ID's player ID, a random number DWC seeds with OS_GetLowEntropyData (dwc_account.c), and DWC's "
         "CRC of the user data over it, made at New Game (README.md, \"What the two formatted saves differ in\")"),
    # SAVE_BLOCK_MYSTERY_GIFT
    34: (((0, 0xa94),), NEW_GAME_KEY),
    # SAVE_BLOCK_ADVENTURE: AdventureTime's RTCTime, when the game's clock was last checked
    37: (((0x24, 12),), "the time the game's clock was last checked, which runs with each program's video frames"),
    # SAVE_BLOCK_RECORDS: the trainer card's records after their key
    38: (((0x04, 0x1dc),), NEW_GAME_KEY),
    # SAVE_BLOCK_FIELD_ACTORS
    41: (WALKERS,
         "the townspeople who walk at random: after Bianca's first talk the field loads the city's music again in "
         "GFL's sound thread (GFL_SndBGMLoadStep, src/system/snd_sys.c), whose card reads take the DS 56 passes and "
         "the port 6, so the fade-in that the field waits for before the starter's selection "
         "(EventFieldClose_Callback, src/system/field_event.c) ends 50 passes later on the DS, and the rest of the "
         "scene with it; the "
         "townspeople the scene paused walk again from another count, drawing GFL's numbers at other passes"),
    # SAVE_BLOCK_SYMBOL
    60: (((0, 0x850),), NEW_GAME_KEY),
}


def masked_sha1(data: bytes, ranges: tuple[tuple[int, int], ...]) -> str:
    """The SHA-1 of a block's data with the ranges declared different zeroed"""
    data = bytearray(data)
    for offset, size in ranges:
        data[offset:offset + size] = bytes(size)
    return hashlib.sha1(data).hexdigest()


def read_reference(path: str) -> list[tuple[str, int, int, str]]:
    rows = []
    header = None
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            columns = line.split("\t")
            if header is None:
                header = columns
                continue
            row = dict(zip(header, columns))
            rows.append((row["block"], int(row["offset"], 16), int(row["size"], 16), row["sha1"]))
    return rows


def write_reference(path: str, oracle_save: str) -> int:
    match = re.fullmatch(r"(.+)_(b2_us|w2_us)\.save\.tsv", os.path.basename(path))
    if not match or match.group(1) not in oracle.SAVES:
        print(f"check_oracle_save.py: {path} isn't named <save>_<version>.save.tsv after one of oracle.py's SAVES")
        return 2
    name, version = match.groups()
    with open(oracle_save, "rb") as f:
        save = f.read()
    digest = hashlib.sha1(save).hexdigest()
    wanted = oracle.SAVES[name].sha1.get(version)
    if digest != wanted:
        print(f"check_oracle_save.py: {oracle_save} has the SHA-1 {digest}, not that of oracle.py's {name} for "
              f"{version} ({wanted})")
        return 1
    problems = validate_image(save)
    if problems:
        print("check_oracle_save.py: invalid save image: " + "; ".join(problems))
        return 1
    blocks = find_blocks(save)
    with open(path, "w") as f:
        f.write(f"# The oracle's {name} save of {version}, block by block (port/tools/oracle/oracle.py's SAVES,\n"
                f"# SHA-1 {digest}), for port/tests/replay/check_oracle_save.py: each\n"
                "# block of the main save's first copy at its offset, with the SHA-1 of its data with the bytes that\n"
                "# KNOWN_BLOCKS declares zeroed, and the card's memory after the two copies. Hashes only, which\n"
                "# check_oracle_save.py --update writes from the oracle's save\n")
        f.write("block\toffset\tsize\tsha1\n")
        for index, (start, size) in enumerate(blocks):
            ranges = KNOWN_BLOCKS.get(index, ((), ""))[0]
            f.write(f"{index}\t{start:#07x}\t{size:#06x}\t{masked_sha1(save[start:start + size], ranges)}\n")
        f.write(f"after\t{AFTER_COPIES:#07x}\t{BACKUP_SIZE - AFTER_COPIES:#06x}\t"
                f"{hashlib.sha1(save[AFTER_COPIES:]).hexdigest()}\n")
    print(f"check_oracle_save.py: wrote {path} from {name}'s save, {len(blocks)} blocks")
    return 0


def run_chain(command: list[str], runs: list[tuple[int, str]], directory: str) -> int:
    """Each run in turn on the same save directory; the first status that isn't 0"""
    for index, (frames, input_path) in enumerate(runs, 1):
        done = subprocess.run(command + ["--frames", str(frames), "--input", input_path, "--save-dir", directory],
                              capture_output=True, text=True)
        # The program's own lines start with "port: ", after the time and process that SDL_Log puts first on some hosts
        output = (done.stderr + done.stdout).splitlines()
        lines = [line[line.index("port: "):].strip() for line in output if "port: " in line]
        print(f"run {index} ({os.path.basename(input_path)}, {frames} frames): status {done.returncode}: "
              f"{lines[-1] if lines else ''}")
        if done.returncode != 0:
            if done.returncode != NO_ROM:
                # The end of its output: a crash's or a trap stub's report, with its backtrace
                print("\n".join(output[-24:]))
            return done.returncode
    return 0


def check_save(save: bytes, reference: list[tuple[str, int, int, str]]) -> list[str]:
    if len(save) != BACKUP_SIZE:
        return [f"the save is {len(save)} bytes, not the card's {BACKUP_SIZE}"]
    failures = validate_image(save)
    for block, start, size, wanted in reference:
        if block == "after":
            if hashlib.sha1(save[start:start + size]).hexdigest() != wanted:
                failures.append("the card's memory after the main save's copies isn't the oracle's")
            continue
        index = int(block)
        ranges, why = KNOWN_BLOCKS.get(index, ((), ""))
        for copy in COPIES:
            data = save[copy + start:copy + start + size]
            footer = struct.unpack_from("<HH", save, copy + start + size)
            label = f"block {index} at {copy + start:#x}"
            if masked_sha1(data, ranges) != wanted:
                failures.append(f"{label} isn't the oracle's" + (f" outside the bytes it declares ({why})" if ranges
                                                                 else ""))
            if footer[1] != crc16_ccitt(data):
                failures.append(f"{label}: its footer's CRC {footer[1]:#06x} isn't its data's")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("reference", help="the oracle's save's blocks, <save>_<version>.save.tsv")
    parser.add_argument("--run", action="append", default=[], metavar="FRAMES:INPUT",
                        help="a run of the chain: its frames and its input file")
    parser.add_argument("--update", metavar="ORACLE_SAVE", help="write the reference from the oracle's save")
    parser.add_argument("command", nargs="*", help="the program and its arguments, after --")
    args = parser.parse_args(argv)
    if args.update:
        return write_reference(args.reference, args.update)
    if not args.run or not args.command:
        parser.error("give the runs and, after --, the program")
    runs = []
    for spec in args.run:
        frames, _, input_path = spec.partition(":")
        runs.append((int(frames), input_path))
    reference = read_reference(args.reference)

    with tempfile.TemporaryDirectory(prefix="oracle-save-") as directory:
        status = run_chain(args.command, runs, directory)
        if status != 0:
            return status
        names = [name for name in sorted(os.listdir(directory)) if name.endswith(".sav")]
        if len(names) != 1:
            print(f"FAIL: the runs left {len(names)} save files: {names}")
            return 1
        with open(os.path.join(directory, names[0]), "rb") as f:
            save = f.read()
    failures = check_save(save, reference)
    blocks = sum(1 for row in reference if row[0] != "after")
    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    print(f"{names[0]}: both copies' {blocks} blocks and the memory after them are the oracle's, but the "
          f"{sum(size for ranges, _ in KNOWN_BLOCKS.values() for _, size in ranges)} bytes declared in "
          f"{len(KNOWN_BLOCKS)} blocks of each copy, and each block's CRC is its data's")
    return 0


if __name__ == "__main__":
    sys.exit(main())
