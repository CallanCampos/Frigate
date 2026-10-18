#!/usr/bin/env python3
"""A smoke test of Frigate's test launcher on the port alone (port/app/launcher.c, ADR 0012): runs the port's program
from a save the oracle made, with --launch, and checks that it ends with the status --status says (0 by default; the
statuses are port/app/launcher.h's), without a trap, after the launcher reported what --expect says, and without
writing the save. With --table and --show, it also checks that the port shows an oracle screen of a launch route's
table (launch_<route>_<version>.screens.tsv, oracle.py's LAUNCHES) at that row's count.

    check_launch_smoke.py --save FILE --save-name IREO.sav --expect REGEX [--status N]
                          [--table TSV --show LABEL...] -- PROGRAM ARG...

The program gets a save directory of its own, in a temporary directory, holding FILE under the name the port gives
the version's saves, and with --show a run directory there too (--run-dir; its frames.tsv has each frame's count and
screen hashes, the oracle's kind). Exits 77 (skipped) when FILE doesn't exist (the oracle that makes it isn't built:
ctest's oracle_saves) or when the program has no ROM (status 66). Standard library only.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile

SKIP = 77
NO_ROM = 66


def table_rows(path: str) -> dict[str, tuple[int, bool, str, str]]:
    """A launch table's rows by label: the count, whether a screen has 3D, and the two hashes"""
    rows = {}
    with open(path) as f:
        for line in f:
            fields = line.rstrip("\n").split("\t")
            if line.startswith("#") or fields[0] == "label" or len(fields) < 8:
                continue
            rows[fields[0]] = (int(fields[2]), fields[4] == "1" or fields[5] == "1", fields[6], fields[7])
    return rows


def shown(run_dir: str) -> dict[int, set[tuple[str, str]]]:
    """The screens the port showed at each count, from its frames.tsv"""
    counts: dict[int, set[tuple[str, str]]] = {}
    with open(os.path.join(run_dir, "frames.tsv")) as f:
        header = None
        for line in f:
            fields = line.rstrip("\n").split("\t")
            if line.startswith("#"):
                continue
            if header is None:
                header = fields
                continue
            row = dict(zip(header, fields))
            counts.setdefault(int(row["counter"]), set()).add((row["top"], row["bottom"]))
    return counts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--save", required=True, help="the save the program starts from")
    parser.add_argument("--save-name", required=True, help="its name in the save directory (IREO.sav, IRDO.sav)")
    parser.add_argument("--expect", required=True, help="a pattern of the launcher's report the run must print")
    parser.add_argument("--status", type=int, default=0, help="the exit status the run must end with")
    parser.add_argument("--table", help="a launch route's table of the oracle's screens")
    parser.add_argument("--show", action="append", default=[], help="a row of --table the port must show at its count")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="-- the program and its arguments")
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not os.path.exists(args.save):
        print(f"check_launch_smoke.py: skipped: no save {args.save} (oracle_saves makes it with the oracle)")
        return SKIP
    rows = table_rows(args.table) if args.table else {}
    for label in args.show:
        if label not in rows or rows[label][1]:
            print(f"check_launch_smoke.py: --show {label}: no such row without 3D in {args.table}")
            return 1
    with open(args.save, "rb") as f:
        before = hashlib.sha1(f.read()).hexdigest()
    saves = tempfile.mkdtemp(prefix="launch-smoke-")
    counts = {}
    try:
        shutil.copyfile(args.save, os.path.join(saves, args.save_name))
        run_dir = os.path.join(saves, "run")
        extra = ["--run-dir", run_dir] if args.show else []
        done = subprocess.run(command + ["--save-dir", saves] + extra, capture_output=True, text=True, timeout=1200)
        output = done.stderr + done.stdout
        with open(os.path.join(saves, args.save_name), "rb") as f:
            after = hashlib.sha1(f.read()).hexdigest()
        if args.show and os.path.exists(os.path.join(run_dir, "frames.tsv")):
            counts = shown(run_dir)
    finally:
        shutil.rmtree(saves, ignore_errors=True)
    if done.returncode == NO_ROM:
        print("check_launch_smoke.py: skipped: the program has no ROM")
        return SKIP
    failures = []
    if done.returncode != args.status:
        failures.append(f"the program ended with status {done.returncode}, not {args.status}")
    if "has no C yet" in output:
        failures.append("the program reached code or data with no C")
    if not re.search(args.expect, output):
        failures.append(f"no report matches {args.expect}")
    if after != before:
        failures.append("the program wrote the save")
    for label in args.show:
        count, _, top, bottom = rows[label]
        if (top, bottom) in counts.get(count, set()):
            print(f"{label}: the oracle's screen at count {count}")
        else:
            seen = ", ".join(f"top {t[:8]} bottom {b[:8]}" for t, b in sorted(counts.get(count, set()))) or "nothing"
            failures.append(f"{label}: at count {count} the port shows {seen}, not the oracle's top {top[:8]} "
                            f"bottom {bottom[:8]}")
    for line in output.splitlines():
        if "port: --launch" in line or "has no C yet" in line:
            print(line)
    for line in failures:
        print(f"FAIL: {line}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
