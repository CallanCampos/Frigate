#!/usr/bin/env python3
"""Tests of port/tests/replay/check_screens.py, the checker through which the replays hold the port's screens to the
oracle's hashes (replay_new_game, replay_new_game_house: port/cmake/app.cmake): a table's rows read and refused, a row
whose top or bottom says 3d (a screen with the field's 3D, which only the oracle's comparison holds) matched on the
other screen alone, and whole runs of a stand-in program, this Python run with -c, that writes a frames.tsv into the
run directory it is given as the port's --run-dir does: every row shown, a row missed, the program's status without the
ROM (66) passed on, and a program that stops early. No ROM, no game; standard library only.

    test_check_screens.py
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
CHECKER = HERE / "check_screens.py"
sys.path.insert(0, str(HERE))

import check_screens  # noqa: E402

TOP, BOTTOM, OTHER = "a" * 40, "b" * 40, "c" * 40
HEADER = "from\tto\ttop\tbottom\twhat\n"
# frames.tsv's header and one frame's row, as port/tools/oracle/compare.py writes them
FRAMES = "#pbwframes 2\n" + "\t".join(check_screens.compare.FRAMES_COLUMNS) + "\n"


def frame_row(frame: int, top: str, bottom: str) -> str:
    """A frame with the field's 3D on the top screen, which the checker leaves to the rows"""
    return f"{frame}\t-\t-\t-\t-\t1\t0\t{top}\t{bottom}\t0\t0\n"


def stand_in(frames: list[tuple[int, str, str]], status: int = 0) -> list[str]:
    """A program that writes frames.tsv with these frames into the directory after its --run-dir, then exits."""
    text = FRAMES + "".join(frame_row(*f) for f in frames)
    code = ["import os, sys",
            "run = sys.argv[sys.argv.index('--run-dir') + 1]",
            f"open(os.path.join(run, 'frames.tsv'), 'w').write({text!r})",
            f"sys.exit({status})"]
    return [sys.executable, "-c", "; ".join(code)]


class Table(unittest.TestCase):
    def read(self, rows: str):
        with tempfile.NamedTemporaryFile("w", suffix=".tsv", delete=False) as f:
            f.write(rows)
        try:
            return check_screens.read_screens(f.name)
        finally:
            os.unlink(f.name)

    def test_rows(self):
        rows = self.read(f"# a comment\n{HEADER}10\t20\t{TOP}\t{BOTTOM}\tboth\n30\t40\t3d\t{BOTTOM}\tthe field\n")
        self.assertEqual(rows, [(10, 20, TOP, BOTTOM, "both"), (30, 40, "3d", BOTTOM, "the field")])

    def test_both_3d_is_refused(self):
        with self.assertRaises(ValueError):
            self.read(f"{HEADER}10\t20\t3d\t3d\tnothing to hold\n")

    def test_header_is_needed(self):
        with self.assertRaises(ValueError):
            self.read(f"10\t20\t{TOP}\t{BOTTOM}\tno header\n")


class Shows(unittest.TestCase):
    frame = check_screens.compare.Frame(1, None, None, None, None, True, False, TOP, BOTTOM, False, False)

    def test_both_hashes(self):
        self.assertTrue(check_screens.shows(self.frame, TOP, BOTTOM))
        self.assertFalse(check_screens.shows(self.frame, TOP, OTHER))
        self.assertFalse(check_screens.shows(self.frame, OTHER, BOTTOM))

    def test_3d_holds_the_other_screen_alone(self):
        self.assertTrue(check_screens.shows(self.frame, "3d", BOTTOM))
        self.assertTrue(check_screens.shows(self.frame, TOP, "3d"))
        self.assertFalse(check_screens.shows(self.frame, "3d", OTHER))
        self.assertFalse(check_screens.shows(self.frame, OTHER, "3d"))


class Runs(unittest.TestCase):
    def check(self, rows: str, program: list[str]) -> tuple[int, str]:
        with tempfile.TemporaryDirectory(prefix="test_check_screens-") as directory:
            table = os.path.join(directory, "screens.tsv")
            with open(table, "w") as f:
                f.write(HEADER + rows)
            done = subprocess.run([sys.executable, str(CHECKER), table, "--", *program], capture_output=True,
                                  text=True, timeout=120)
        return done.returncode, done.stdout + done.stderr

    def test_every_row_shown(self):
        status, output = self.check(f"1\t2\t{TOP}\t{BOTTOM}\tfirst\n3\t5\t3d\t{OTHER}\tthe field\n",
                                    stand_in([(1, OTHER, OTHER), (2, TOP, BOTTOM), (4, TOP, OTHER)]))
        self.assertEqual(status, 0, output)
        self.assertIn("2 of 2 screens shown as on the DS", output)

    def test_a_row_missed(self):
        status, output = self.check(f"1\t2\t{TOP}\t{BOTTOM}\tfirst\n3\t5\t3d\t{OTHER}\tthe field\n",
                                    stand_in([(2, TOP, BOTTOM), (4, TOP, BOTTOM), (6, TOP, OTHER)]))
        self.assertEqual(status, 1, output)
        self.assertIn("FAIL the field: no frame from 3 to 5 shows it", output)

    def test_without_the_rom(self):
        status, output = self.check(f"1\t2\t{TOP}\t{BOTTOM}\tfirst\n", stand_in([], status=66))
        self.assertEqual(status, 66, output)

    def test_a_run_that_stops(self):
        status, output = self.check(f"1\t2\t{TOP}\t{BOTTOM}\tfirst\n", stand_in([(1, TOP, BOTTOM)], status=70))
        self.assertEqual(status, 1, output)
        self.assertIn("stopped with status 70", output)


if __name__ == "__main__":
    unittest.main()
