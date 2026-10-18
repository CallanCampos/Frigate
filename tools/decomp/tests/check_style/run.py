#!/usr/bin/env python3
"""Tests check_style.py against its fixtures: every rule found in src/flagged.c and lib/flagged.c as expected.txt lists,
nothing in src/clean.c (near misses: forward declarations, an aligned global, commented volatiles, macros, a shared
cleanup goto, gotos to shared tail blocks, pointer sums, an allowed exception, words in strings, a BUGFIX), exit 1
with --strict and 2 on a path it cannot read.

    python3 tools/decomp/tests/check_style/run.py [CHECKER]    (CHECKER: another copy of check_style.py, to test it)
"""
import subprocess
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
checker = sys.argv[1] if len(sys.argv) > 1 else str(here.parents[1] / "check_style.py")


def run(*args):
    return subprocess.run([sys.executable, checker, *args], cwd=here, capture_output=True, text=True)


failures = []
got = run("src/flagged.c", "lib/flagged.c")
if got.stdout != (here / "expected.txt").read_text():
    failures.append("findings differ from expected.txt:\n" + got.stdout)
if got.returncode != 0:
    failures.append(f"exit {got.returncode} without --strict, want 0")
clean = run("src/clean.c")
if clean.stdout:
    failures.append("src/clean.c has findings:\n" + clean.stdout)
if run("--strict", "src/flagged.c").returncode != 1:
    failures.append("--strict with findings must exit 1")
if run("--strict", "src/clean.c").returncode != 0:
    failures.append("--strict without findings must exit 0")
if run("src/no-such-file.c").returncode != 2:
    failures.append("an unreadable path must exit 2")
for f in failures:
    print("FAIL:", f)
if not failures:
    print("check_style.py: fixtures passed")
sys.exit(1 if failures else 0)
