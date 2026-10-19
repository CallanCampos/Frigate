#!/usr/bin/env python3
"""The test of port/tools/frame_times.py, the reader of the files the program's --frame-times writes
(port/hal/os/frame_time.h), on files made up here: each frame's least time over several runs, the nearest-rank
quantiles the program's own summary takes, the scenes, and check's budget, report-only mode and skips, with a stand-in
program that writes its --frame-times file as Frigate does. Nothing of the game runs. Standard library only.
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
TOOL = os.path.join(HERE, "..", "..", "tools", "frame_times.py")
sys.path.insert(0, os.path.dirname(TOOL))
import frame_times  # noqa: E402

HEADER = "frame\twall_us\tbusy_us\trender3d_us\tgeometry_us\trender2d_us\tcapture_us\tarm7_us\tpresent_us\tpace_us\t" \
         "other_us\n"


def write(path: str, busy: list[int]) -> None:
    """A file of len(busy) frames, numbered from 1, whose 3D part is half the frame and the rest other"""
    with open(path, "w") as f:
        f.write(HEADER)
        for frame, us in enumerate(busy, 1):
            f.write(f"{frame}\t{us}\t{us}\t{us // 2}\t0\t0\t0\t0\t0\t0\t{us - us // 2}\n")


# A stand-in for the program: it writes the busy times its first argument lists to the file after --frame-times, and
# exits with the status its second argument gives
STAND_IN = """
import sys
args = sys.argv[1:]
busy = [int(us) for us in args[0].split(",")]
status = int(args[1])
path = args[args.index("--frame-times") + 1]
assert "--save-dir" in args
with open(path, "w") as f:
    f.write(%r)
    for frame, us in enumerate(busy, 1):
        f.write(f"{frame}\\t{us}\\t{us}\\t0\\t0\\t0\\t0\\t0\\t0\\t0\\t{us}\\n")
print("port: frame times of", len(busy), "frames")
sys.exit(status)
""" % HEADER


class FrameTimesTest(unittest.TestCase):
    def setUp(self) -> None:
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)

    def path(self, name: str) -> str:
        return os.path.join(self.dir.name, name)

    def test_quantiles_by_nearest_rank(self) -> None:
        values = list(range(1, 101))
        self.assertEqual(frame_times.quantile(values, 0.5), 50)
        self.assertEqual(frame_times.quantile(values, 0.95), 95)
        self.assertEqual(frame_times.quantile(values, 1.0), 100)
        self.assertEqual(frame_times.quantile([7], 0.95), 7)
        self.assertEqual(frame_times.quantile([3, 1, 2], 0.5), 2)

    def test_least_of_the_runs_frame_by_frame(self) -> None:
        write(self.path("a.tsv"), [100, 900, 300])
        write(self.path("b.tsv"), [400, 200, 300, 50])
        rows = frame_times.least([frame_times.read(self.path("a.tsv")), frame_times.read(self.path("b.tsv"))])
        # Frame 4 is in one run only, so it is left out
        self.assertEqual(sorted(rows), [1, 2, 3])
        self.assertEqual([rows[f]["busy"] for f in (1, 2, 3)], [100, 200, 300])
        self.assertEqual(rows[2]["render3d"], 100)

    def test_scenes(self) -> None:
        write(self.path("a.tsv"), [1000] * 10 + [5000] * 10)
        rows = frame_times.least([frame_times.read(self.path("a.tsv"))])
        p95s = frame_times.report(rows, [("light", 1, 10), ("heavy", 11, 20)], 1)
        self.assertEqual(p95s, {"light": 1000, "heavy": 5000})
        self.assertEqual(frame_times.parse_scene("title=1110-3100"), ("title", 1110, 3100))

    def run_check(self, busy: str, status: int, *options: str) -> subprocess.CompletedProcess:
        stand_in = self.path("stand_in.py")
        with open(stand_in, "w") as f:
            f.write(STAND_IN)
        return subprocess.run([sys.executable, TOOL, "check", "--runs", "2", *options, "--", sys.executable, stand_in,
                               busy, str(status)], capture_output=True, text=True)

    def test_check_holds_the_budget(self) -> None:
        done = self.run_check("1000,2000,3000", 0, "--budget-ms", "3")
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
        done = self.run_check("1000,2000,3500", 0, "--budget-ms", "3")
        self.assertEqual(done.returncode, 1, done.stdout + done.stderr)
        self.assertIn("above the budget", done.stdout)
        done = self.run_check("1000,2000,3500", 0, "--budget-ms", "3", "--report-only")
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)
        # Only the scene's frames count
        done = self.run_check("1000,2000,3500", 0, "--budget-ms", "3", "--scene", "first=1-2")
        self.assertEqual(done.returncode, 0, done.stdout + done.stderr)

    def test_check_skips_and_fails(self) -> None:
        self.assertEqual(self.run_check("1000", 66).returncode, 77)
        self.assertEqual(self.run_check("1000", 0, "--save", self.path("none.sav")).returncode, 77)
        self.assertEqual(self.run_check("1000", 70).returncode, 1)


if __name__ == "__main__":
    unittest.main()
