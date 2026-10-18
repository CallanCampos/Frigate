#!/usr/bin/env python3
"""Runs the port's program once and checks how it ends: its exit status, and what its messages must and mustn't say.
ctest's PASS_REGULAR_EXPRESSION ignores a test's status, so the program's option checks and Frigate's test launcher's
statuses (port/app/launcher.h, ADR 0012) go through this instead (port/cmake/integrate.cmake).

    check_exit.py --status N [--expect REGEX]... [--reject REGEX]... [--skip-status M]... [--fresh-save-dir]
                  -- PROGRAM ARG...

--fresh-save-dir gives the program a save directory of its own, a new temporary one, removed after. Exits 0 when the
program ended with status N, its messages (stderr, then stdout) match every --expect and no --reject; 77 (skipped)
when it ended with a --skip-status (66: no ROM); 1 otherwise. Standard library only.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import tempfile

SKIP = 77


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--status", type=int, required=True, help="the exit status the program must end with")
    parser.add_argument("--expect", action="append", default=[], help="a pattern its messages must have")
    parser.add_argument("--reject", action="append", default=[], help="a pattern its messages mustn't have")
    parser.add_argument("--skip-status", type=int, action="append", default=[], help="a status that skips the test")
    parser.add_argument("--fresh-save-dir", action="store_true", help="a new, empty save directory for the run")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="-- the program and its arguments")
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    saves = tempfile.mkdtemp(prefix="check-exit-") if args.fresh_save_dir else None
    try:
        done = subprocess.run(command + (["--save-dir", saves] if saves else []), capture_output=True, text=True,
                              timeout=1200)
    finally:
        if saves:
            shutil.rmtree(saves, ignore_errors=True)
    output = done.stderr + done.stdout
    if done.returncode in args.skip_status:
        print(f"check_exit.py: skipped: the program ended with status {done.returncode}")
        return SKIP
    failures = []
    if done.returncode != args.status:
        failures.append(f"the program ended with status {done.returncode}, not {args.status}")
    failures += [f"no message matches {pattern}" for pattern in args.expect if not re.search(pattern, output)]
    failures += [f"a message matches {pattern}" for pattern in args.reject if re.search(pattern, output)]
    for line in output.splitlines()[-12:]:
        print(line)
    for line in failures:
        print(f"FAIL: {line}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
