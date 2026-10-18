#!/usr/bin/env python3
"""Runs the port's program twice on the same input, each run with a save directory of its own, and checks that both
leave the same save files byte for byte: what the game writes may depend only on the input file and the clock it
names, as on the DS, never on the host (where the program is loaded, how long a run takes). Each --expect also holds a
save to bytes the oracle wrote at the same offset (port/tools/oracle/README.md, "Saves").

    check_same_save.py [--expect FILE:OFFSET:HEX]... -- PROGRAM ARGUMENT...

The program gets --save-dir DIR after its arguments. Exits 0 when both runs wrote the same files, at least one, with
the expected bytes; 1 when they differ or wrote none; a run's own status when it fails, so 66, the program's status
without the ROM, skips the test.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile


def run(command: list[str], directory: str) -> tuple[int, str]:
    done = subprocess.run(command + ["--save-dir", directory], capture_output=True, text=True)
    # The program's own lines start with "port: ", after the time and process that SDL_Log puts first on some hosts
    log = (done.stderr + done.stdout).splitlines()
    lines = [line[line.index("port: "):].strip() for line in log if "port: " in line]
    return done.returncode, lines[-1] if lines else ""


def saves(directory: str) -> dict[str, bytes]:
    """The save files a run left, by name, without a temporary file of an unfinished write"""
    files = {}
    for name in sorted(os.listdir(directory)):
        if not name.endswith(".tmp"):
            with open(os.path.join(directory, name), "rb") as f:
                files[name] = f.read()
    return files


def differences(a: bytes, b: bytes, limit: int = 12) -> str:
    """The ranges where two files of the same size differ, the first few of them"""
    ranges: list[list[int]] = []
    for offset, (x, y) in enumerate(zip(a, b)):
        if x != y:
            if ranges and offset == ranges[-1][1] + 1:
                ranges[-1][1] = offset
            else:
                ranges.append([offset, offset])
    shown = ", ".join(f"{start:#x}" + (f"-{end:#x}" if end != start else "") for start, end in ranges[:limit])
    count = sum(end - start + 1 for start, end in ranges)
    return f"{count} bytes in {len(ranges)} ranges: {shown}" + (", ..." if len(ranges) > limit else "")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--expect", action="append", default=[],
                        help="FILE:OFFSET:HEX, bytes the save file FILE holds at OFFSET (the oracle's)")
    parser.add_argument("command", nargs="+", help="the program and its arguments, after --")
    args = parser.parse_args(argv)

    with tempfile.TemporaryDirectory(prefix="same-save-") as work:
        runs = []
        for index in (1, 2):
            directory = os.path.join(work, f"run{index}")
            os.makedirs(directory)
            status, message = run(args.command, directory)
            print(f"run {index}: status {status}: {message}")
            if status != 0:
                return status
            runs.append(saves(directory))
        first, second = runs
        if not first:
            print("FAIL: the runs wrote no save")
            return 1
        failed = False
        if sorted(first) != sorted(second):
            print(f"FAIL: the runs wrote different files: {sorted(first)} and {sorted(second)}")
            return 1
        for name in first:
            if first[name] == second[name]:
                print(f"{name}: the same {len(first[name])} bytes in both runs")
            elif len(first[name]) != len(second[name]):
                print(f"FAIL: {name}: {len(first[name])} bytes in one run, {len(second[name])} in the other")
                failed = True
            else:
                print(f"FAIL: {name} differs between the runs in {differences(first[name], second[name])}")
                failed = True
        for expectation in args.expect:
            name, offset_text, hex_text = expectation.split(":")
            offset = int(offset_text, 0)
            wanted = bytes.fromhex(hex_text)
            found = first.get(name, b"")[offset:offset + len(wanted)]
            if found != wanted:
                print(f"FAIL: {name} holds {found.hex()} at {offset:#x}, where the oracle's save holds {wanted.hex()}")
                failed = True
            else:
                print(f"{name} holds the oracle's {wanted.hex()} at {offset:#x}")
        return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
