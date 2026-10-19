#!/usr/bin/env python3
"""Runs the port's program once and checks how it ends: its exit status, and what its messages must and mustn't say.
ctest's PASS_REGULAR_EXPRESSION ignores a test's status, so the program's option checks and Frigate's test launcher's
statuses (port/app/launcher.h, ADR 0012, in port/cmake/integrate.cmake), the host's traps and crash reports
(port/cmake/host.cmake) and the heap hooks' check of a read past a block (port/cmake/nnsys.cmake) go through this
instead.

    check_exit.py (--status N | --crash | --abort) [--expect REGEX]... [--reject REGEX]... [--skip-status M]...
                  [--fresh-save-dir] -- PROGRAM ARG...

--fresh-save-dir gives the program a save directory of its own, a new temporary one, removed after. --crash wants the
program to end by a crash, as each host ends a crashed program: killed by a signal on macOS and Linux, with an
exception's status on Windows (port/hal/host): an error's, 0xc0000000 and up, or a warning's, 0x80000000 and up, such
as a guard page's 0x80000001. --abort wants it to end by abort(), a check of the platform layer's that failed: killed
by SIGABRT on macOS and Linux, status 3 on Windows. Exits 0 when the program ended as wanted, its messages (stderr, then
stdout) match every --expect and no --reject, and hold no sanitizer's report; 77 (skipped) when it ended with a
--skip-status (66: no ROM); 1 otherwise. A sanitizer's report fails the run whatever ending it wants, since none of the
runs this checks expects one: in the sanitized build a report aborts the program (port/docs/sanitizers.md), which
--crash or --abort would otherwise take for the ending it wants. Standard library only.
"""

from __future__ import annotations

import argparse
import os
import re
import signal
import shutil
import subprocess
import sys
import tempfile

SKIP = 77
# The lowest exception status Windows ends a crashed program with: the warnings' severity (port/hal/host)
CRASH_STATUS = 0x80000000
# How abort() ends a program: SIGABRT on macOS and Linux; the C runtime's status 3 on Windows, whose dialog the host
# seam turns off (port/hal/host)
ABORT_STATUS = 3 if os.name == "nt" else -signal.SIGABRT
# A sanitizer's report, by the lines the sanitized build's tests fail on (port_check_tests in
# port/CMakeLists.txt): ctest sees only the lines this prints, so the check is made here too
SANITIZER_REPORT = re.compile(r"ERROR: (Address|Leak)Sanitizer|SUMMARY: [A-Za-z]+Sanitizer|runtime error: "
                              r"|LeakSanitizer has encountered a fatal error")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ending = parser.add_mutually_exclusive_group(required=True)
    ending.add_argument("--status", type=int, help="the exit status the program must end with")
    ending.add_argument("--crash", action="store_true", help="the program must end by a crash")
    ending.add_argument("--abort", action="store_true", help="the program must end by abort()")
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
    reports = [line for line in output.splitlines() if SANITIZER_REPORT.search(line)]
    if reports:
        for line in output.splitlines()[-12:]:
            print(line)
        print(f"FAIL: a sanitizer's report: {reports[0]}")
        return 1
    if done.returncode in args.skip_status:
        print(f"check_exit.py: skipped: the program ended with status {done.returncode}")
        return SKIP
    failures = []
    if args.crash and not (done.returncode < 0 or done.returncode >= CRASH_STATUS):
        failures.append(f"the program ended with status {done.returncode}, not by a crash")
    if args.abort and done.returncode != ABORT_STATUS:
        failures.append(f"the program ended with status {done.returncode}, not by abort() ({ABORT_STATUS})")
    if args.status is not None and done.returncode != args.status:
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
