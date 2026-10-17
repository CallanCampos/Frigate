"""Runs the program on a replay and checks that it shows the screens the DS shows: a replay test of test level T3
(port/docs/PLAN.md section 6), with the oracle's screens as the goldens.

    check_screens.py SCREENS -- PROGRAM ARGUMENT...

runs PROGRAM with its arguments and --run-dir, whose frames.tsv gives every frame's screens as the SHA-1 of their RGB
bytes, then reads SCREENS, a table of the screens the run must show: each row gives a range of the port's frames and
the two hashes of a screen that one of those frames must show. The hashes are pbw-oracle's, from playing the same input
file on the player's ROM (port/tools/oracle/README.md), so a row passes only when the port draws that screen as the DS
does, bit for bit; the ranges are where the port shows them, with room for its timing to move. A run that stops before
its last frame (a trap stub, a crash, the watchdog) fails, and one without the ROM is skipped as the program says
(status 66). Exits 0 when the run and every row pass, 1 otherwise.

SCREENS is tab-separated, with # comments: a header line, then "from to top bottom what" rows.
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools", "oracle"))

import compare  # noqa: E402 - port/tools/oracle's reader of frames.tsv

NO_ROM = 66


def read_screens(path):
    """The rows of a screens table: (from, to, top, bottom, what)."""
    with open(path) as f:
        lines = [line.rstrip("\n") for line in f if line.strip() and not line.startswith("#")]
    if not lines or lines[0].split("\t")[:4] != ["from", "to", "top", "bottom"]:
        raise ValueError(f"{path}: the first line must be the header \"from to top bottom what\", tab-separated")
    rows = []
    for line in lines[1:]:
        cells = line.split("\t")
        if len(cells) != 5:
            raise ValueError(f"{path}: bad row {line!r}")
        rows.append((int(cells[0]), int(cells[1]), cells[2], cells[3], cells[4]))
    return rows


def main(argv):
    if "--" not in argv or argv.index("--") != 2 or len(argv) < 4:
        print(__doc__)
        return 2
    rows = read_screens(argv[1])
    command = argv[3:]
    with tempfile.TemporaryDirectory(prefix="check_screens-") as run:
        status = subprocess.call(command + ["--run-dir", run])
        if status == NO_ROM:
            return NO_ROM
        if status != 0:
            print(f"FAIL: the program stopped with status {status} before its last frame")
            return 1
        frames = compare.load_run(run).frames
    failed = 0
    for first, last, top, bottom, what in rows:
        shown = [f for f in range(first, last + 1)
                 if f in frames and (frames[f].top, frames[f].bottom) == (top, bottom)]
        if shown:
            print(f"ok   {what}: frames {compare.describe_ranges(shown)}")
        else:
            print(f"FAIL {what}: no frame from {first} to {last} shows it")
            failed += 1
    print(f"{len(rows) - failed} of {len(rows)} screens shown as on the DS")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
