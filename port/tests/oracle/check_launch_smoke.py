#!/usr/bin/env python3
"""A smoke test of Frigate's test launcher on the port alone (port/app/launcher.c, ADR 0012): runs the port's program
from a save the oracle made, with --launch, and checks that it ends with status 0, without a trap, after the launcher
reported what --expect says, and without writing the save.

    check_launch_smoke.py --save FILE --save-name IREO.sav --expect REGEX -- PROGRAM ARG...

The program gets a save directory of its own, in a temporary directory, holding FILE under the name the port gives
the version's saves. Exits 77 (skipped) when FILE doesn't exist (the oracle that makes it isn't built: ctest's
oracle_saves) or when the program has no ROM (status 66). Standard library only.
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--save", required=True, help="the save the program starts from")
    parser.add_argument("--save-name", required=True, help="its name in the save directory (IREO.sav, IRDO.sav)")
    parser.add_argument("--expect", required=True, help="a pattern of the launcher's report the run must print")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="-- the program and its arguments")
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not os.path.exists(args.save):
        print(f"check_launch_smoke.py: skipped: no save {args.save} (oracle_saves makes it with the oracle)")
        return SKIP
    with open(args.save, "rb") as f:
        before = hashlib.sha1(f.read()).hexdigest()
    saves = tempfile.mkdtemp(prefix="launch-smoke-")
    try:
        shutil.copyfile(args.save, os.path.join(saves, args.save_name))
        done = subprocess.run(command + ["--save-dir", saves], capture_output=True, text=True, timeout=1200)
        output = done.stderr + done.stdout
        with open(os.path.join(saves, args.save_name), "rb") as f:
            after = hashlib.sha1(f.read()).hexdigest()
    finally:
        shutil.rmtree(saves, ignore_errors=True)
    if done.returncode == NO_ROM:
        print("check_launch_smoke.py: skipped: the program has no ROM")
        return SKIP
    failures = []
    if done.returncode != 0:
        failures.append(f"the program ended with status {done.returncode}")
    if "has no C yet" in output:
        failures.append("the program reached code or data with no C")
    if not re.search(args.expect, output):
        failures.append(f"no report matches {args.expect}")
    if after != before:
        failures.append("the program wrote the save")
    for line in output.splitlines():
        if "port: --launch" in line or "has no C yet" in line:
            print(line)
    for line in failures:
        print(f"FAIL: {line}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
