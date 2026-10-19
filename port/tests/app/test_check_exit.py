#!/usr/bin/env python3
"""Tests of port/tests/app/check_exit.py, the checker through which the tests of the program's exit statuses, the host's
crash reports and the heap hooks' aborts run (port/cmake/integrate.cmake, host.cmake, nnsys.cmake): each ending it can
want, met and missed, a skip, a crash that its report names and one that it doesn't, and a sanitizer's report, which
fails a run whatever ending it wants, so that a sanitized program that aborted on a report (port/docs/sanitizers.md)
never passes as the crash or abort a test wanted. The program it checks is a stand-in, this Python run with -c. What the
checker prints is kept, not printed: the stand-ins' reports would otherwise reach ctest's output, which the sanitized
build fails a test on (a failing case prints it). No ROM, no game; standard library only.

    test_check_exit.py
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

CHECKER = Path(__file__).resolve().parent / "check_exit.py"
SKIP = 77

# The stand-ins' report lines, put together at run time
ASAN = "ERROR: " + "AddressSanitizer: heap-buffer-overflow on address 0x1"
SUMMARY = "SUMMARY: " + "AddressSanitizer: heap-buffer-overflow"
UBSAN = "test.c:1:1: " + "runtime error: signed integer overflow"
# The crash a "killed" stand-in ends by, as the host's crash report names it (port/hal/host): SIGFPE on macOS and Linux,
# a division by zero's exception on Windows; and another, which a report that misnames it gives
if os.name == "nt":
    CRASH, OTHER = "exception 0xc0000094", "exception 0xc000001d"
else:
    CRASH, OTHER = "signal 8", "signal 4"


def stand_in(lines: list[str], ending: str) -> list[str]:
    """A program that prints lines to stderr, then ends: "exit N", "abort", "crash" (a fault, as a crash is) or "killed"
    (by CRASH)."""
    code = ["import os, sys"]
    code += [f"print({line!r}, file=sys.stderr, flush=True)" for line in lines]
    if ending == "abort":
        # Without the C runtime's report of a fault on Windows, as the port's programs abort there (port/hal/host):
        # status 3, where Python's own abort() ends with a fail-fast's 0xc0000409
        code.append("import ctypes; ctypes.CDLL('ucrtbase')._set_abort_behavior(0, 3) if os.name == 'nt' else None")
        code.append("os.abort()")
    elif ending == "crash":
        code.append("import ctypes; ctypes.string_at(1)")
    elif ending == "killed":
        # SIGFPE's default action, or on Windows TerminateProcess with the exception's code, which os.kill calls there
        code.append("import signal; os.kill(os.getpid(), 0xc0000094 if os.name == 'nt' else signal.SIGFPE)")
    else:
        code.append(f"sys.exit({int(ending.split()[1])})")
    return [sys.executable, "-c", "; ".join(code)]


def check(arguments: list[str], program: list[str]) -> tuple[int, str]:
    done = subprocess.run([sys.executable, str(CHECKER), *arguments, "--", *program], capture_output=True, text=True,
                          timeout=120)
    return done.returncode, done.stdout + done.stderr


def main() -> int:
    cases = [
        # (what, the checker's arguments, the stand-in's lines and ending, the checker's status, its output's line)
        ("a status met", ["--status", "2", "--expect", "unknown"], (["port: unknown option"], "exit 2"), 0, None),
        ("a status missed", ["--status", "2"], ([], "exit 1"), 1, "not 2"),
        ("a message missing", ["--status", "0", "--expect", "here"], ([], "exit 0"), 1, "no message matches"),
        ("a message rejected", ["--status", "0", "--reject", "game"], (["the game"], "exit 0"), 1, "a message matches"),
        ("a skip", ["--status", "0", "--skip-status", "66"], ([], "exit 66"), SKIP, "skipped"),
        ("a crash met", ["--crash"], ([], "crash"), 0, None),
        ("a crash missed", ["--crash"], ([], "exit 0"), 1, "not by a crash"),
        ("a crash its report names", ["--crash"], ([f"port: crash: {CRASH} at address 0x1"], "killed"), 0, None),
        ("a crash its report misnames", ["--crash"], ([f"port: crash: {OTHER} at address 0x1"], "killed"), 1,
         f"the report names {OTHER}"),
        ("an abort met", ["--abort"], (["port: a check failed"], "abort"), 0, None),
        ("an abort missed", ["--abort"], ([], "exit 1"), 1, "not by abort()"),
        ("ASan's report on a crash", ["--crash"], ([ASAN, SUMMARY], "abort"), 1, "a sanitizer's report"),
        ("ASan's report on an abort", ["--abort"], ([ASAN, SUMMARY], "abort"), 1, "a sanitizer's report"),
        ("UBSan's report on a status met", ["--status", "0"], ([UBSAN], "exit 0"), 1, "a sanitizer's report"),
    ]
    failures = 0
    for what, arguments, (lines, ending), status, needle in cases:
        got, output = check(arguments, stand_in(lines, ending))
        ok = got == status and (needle is None or needle in output)
        print(f"{what}: {'ok' if ok else f'FAIL (status {got})'}")
        if not ok:
            # A failing case fails the test anyway, so its output may reach ctest's, reports and all
            print(output)
        failures += not ok
    print("check_exit: " + ("FAIL" if failures else "ok"))
    return 1 if failures else 0


if __name__ == "__main__":
    os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
    sys.exit(main())
