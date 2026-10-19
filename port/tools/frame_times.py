#!/usr/bin/env python3
"""How long Frigate's frames take on this machine: reads the files the program's --frame-times writes
(port/hal/os/frame_time.h: a row a frame, in microseconds, the whole frame, the frame without the pacing wait, which
is the work the program did for it, and its parts), and reports the median, 95th percentile and most of each scene's
frames. Nothing of the game is touched: the program's frames are the same with --frame-times as without.

    frame_times.py summary FILE... [--scene NAME=FIRST-LAST]...
    frame_times.py check [--runs N] [--budget-ms MS] [--report-only] [--max-load LOAD] [--scene NAME=FIRST-LAST]...
                         [--save FILE --save-name NAME] [--work DIR] -- PROGRAM ARG...

summary reads one or more files of the same route: a replay does the same work at every run, so each frame's time is
the least of its runs', which leaves out what other programs took from it in bursts that came and went. Load that
stays, such as other Frigate programs running beside it, at the same priority (port/app/main.c's user-interactive
QoS), takes from every run alike, and the least keeps it. A scene is a range of the program's frame numbers (both
included); without --scene the whole run is one.

check runs PROGRAM ARG... --runs times (3 by default), each with --frame-times and a save directory of its own in
--work (a temporary directory by default), holding --save under the name --save-name when given, then reports as
summary does and fails (status 1) when a scene's 95th percentile without the pacing wait is above --budget-ms, unless
--report-only, saying how a machine too slow for the budget leaves the timing tests out. A scene over the budget
skips the check instead (77) when the host's 1-minute load average, read before each run and after the last, was
above --max-load (the number of cores by default): other programs had the cores, so the times measure the machine. A
pass stands at any load, and a host without a load average (Windows) never skips. It exits 77 too when PROGRAM has no
ROM (its status 66) or --save doesn't exist, as the tests' other checks do; a run that ends with another status than
0 fails. The timing tests of port/cmake/frame_times.cmake run it. Standard library only.
"""

from __future__ import annotations

import argparse
import math
import os
import shutil
import subprocess
import sys
import tempfile

SKIP = 77
NO_ROM = 66
# The DS's frame: 560,190 cycles of its 33.514 MHz clock
DS_FRAME_MS = 560190 / 33514000 * 1000
# The columns summary reports beside busy, the frame without the pacing wait
PARTS = ("render3d", "geometry", "render2d", "capture", "arm7", "present", "other")


def read(path: str) -> dict[int, dict[str, int]]:
    """A file's rows by frame number, each column in microseconds without its _us"""
    rows = {}
    with open(path) as f:
        header = f.readline().rstrip("\n").split("\t")
        names = [name[:-3] if name.endswith("_us") else name for name in header]
        for line in f:
            fields = line.rstrip("\n").split("\t")
            if len(fields) != len(names):
                continue
            row = dict(zip(names, (int(value) for value in fields)))
            rows[row["frame"]] = row
    return rows


def least(runs: list[dict[int, dict[str, int]]]) -> dict[int, dict[str, int]]:
    """Each frame that every run has, with each column's least value over the runs"""
    frames = set(runs[0])
    for run in runs[1:]:
        frames &= set(run)
    return {frame: {name: min(run[frame][name] for run in runs) for name in runs[0][frame]} for frame in frames}


def quantile(values: list[int], q: float) -> int:
    """The nearest-rank quantile, as the program's summary takes it: the least value that a fraction q of them don't
    exceed"""
    ordered = sorted(values)
    rank = math.ceil(round(q * len(ordered), 6))
    return ordered[min(max(rank, 1), len(ordered)) - 1]


def load_average() -> float | None:
    """The host's load average over the last minute, the threads running or waiting for a core; None on a host without
    one (Windows)"""
    try:
        return os.getloadavg()[0]
    except (AttributeError, OSError):
        return None


def parse_scene(text: str) -> tuple[str, int, int]:
    name, _, span = text.partition("=")
    first, _, last = span.partition("-")
    if not name or not first.isdigit() or not last.isdigit() or int(first) > int(last):
        raise argparse.ArgumentTypeError(f"a scene is NAME=FIRST-LAST, not {text}")
    return name, int(first), int(last)


def report(rows: dict[int, dict[str, int]], scenes: list[tuple[str, int, int]], runs: int) -> dict[str, int]:
    """Prints each scene's times and returns each scene's 95th percentile without the pacing wait, in microseconds"""
    if not scenes:
        scenes = [("run", min(rows), max(rows))] if rows else []
    p95s = {}
    print(f"frame times in ms, each frame's least of {runs} run{'s' if runs != 1 else ''}; busy is the frame without "
          f"the pacing wait, the DS's frame {DS_FRAME_MS:.2f} ms")
    print(f"{'scene':<16}{'frames':>8}  {'busy median':>11} {'p95':>6} {'most':>6}   " +
          " ".join(f"{part:>8}" for part in PARTS) + "  (medians)")
    for name, first, last in scenes:
        chosen = [rows[frame] for frame in sorted(rows) if first <= frame <= last]
        if not chosen:
            print(f"{name:<16}{0:>8}  no frames in {first}-{last}")
            continue
        busy = [row["busy"] for row in chosen]
        p95s[name] = quantile(busy, 0.95)
        parts = " ".join(f"{quantile([row.get(part, 0) for row in chosen], 0.5) / 1000:>8.2f}" for part in PARTS)
        print(f"{name:<16}{len(chosen):>8}  {quantile(busy, 0.5) / 1000:>11.2f} {p95s[name] / 1000:>6.2f} "
              f"{max(busy) / 1000:>6.2f}   {parts}")
    return p95s


def summary(args: argparse.Namespace) -> int:
    runs = [read(path) for path in args.files]
    report(least(runs), args.scene, len(runs))
    return 0


def check(args: argparse.Namespace) -> int:
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        print("frame_times.py check: no program to run")
        return 2
    if args.save and not os.path.exists(args.save):
        print(f"frame_times.py: skipped: no save {args.save} (oracle_saves makes it with the oracle)")
        return SKIP
    work = args.work or tempfile.mkdtemp(prefix="frigate-frame-times-")
    os.makedirs(work, exist_ok=True)
    runs = []
    loads = []
    try:
        for n in range(args.runs):
            loads.append(load_average())
            saves = os.path.join(work, f"saves_{n}")
            times = os.path.join(work, f"frame_times_{n}.tsv")
            shutil.rmtree(saves, ignore_errors=True)
            os.makedirs(saves)
            if args.save:
                shutil.copyfile(args.save, os.path.join(saves, args.save_name))
            done = subprocess.run(command + ["--save-dir", saves, "--frame-times", times], capture_output=True,
                                  text=True)
            if done.returncode == NO_ROM:
                print("frame_times.py: skipped: the program has no ROM")
                return SKIP
            lines = (done.stdout + done.stderr).strip().splitlines()
            if done.returncode != 0:
                print("\n".join(lines[-20:]))
                print(f"frame_times.py: run {n + 1} ended with status {done.returncode}")
                return 1
            print(f"run {n + 1}: " + next((line[line.find("port: frame times"):] for line in reversed(lines)
                                           if "port: frame times" in line), "no summary"))
            runs.append(read(times))
        loads.append(load_average())
    finally:
        if not args.work:
            shutil.rmtree(work, ignore_errors=True)
    p95s = report(least(runs), args.scene, len(runs))
    over = {name: p95 for name, p95 in p95s.items() if p95 / 1000 > args.budget_ms}
    for name, p95 in over.items():
        print(f"frame_times.py: {name}: the 95th percentile, {p95 / 1000:.2f} ms, is above the budget of "
              f"{args.budget_ms:g} ms" + (" (reported only)" if args.report_only else ""))
    if not over:
        print(f"frame_times.py: every scene's 95th percentile is within the budget of {args.budget_ms:g} ms")
    known = [load for load in loads if load is not None]
    if known:
        print("frame_times.py: the load average before each run and after the last: " +
              ", ".join(f"{load:.2f}" for load in known) + f", on {os.cpu_count()} cores")
    if over and not args.report_only and known and max(known) > args.max_load:
        print(f"frame_times.py: skipped: the load average reached {max(known):.2f}, above {args.max_load:g}, so other "
              f"programs had the cores and the times measure the machine")
        return SKIP
    if over and not args.report_only:
        print("frame_times.py: on a machine too slow for this budget, ctest -LE timing leaves the timing tests out, "
              "and -DPORT_FRAME_TIME_REPORT_ONLY=ON makes them report only (port/cmake/frame_times.cmake)")
        return 1
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="action", required=True)
    s = commands.add_parser("summary", help="report the frame times of one or more runs of a route")
    s.add_argument("files", nargs="+", help="files --frame-times wrote")
    s.add_argument("--scene", action="append", default=[], type=parse_scene, help="NAME=FIRST-LAST, frame numbers")
    s.set_defaults(run=summary)
    c = commands.add_parser("check", help="run a route and hold its scenes' 95th percentiles to a budget")
    c.add_argument("--runs", type=int, default=3, help="how many times to run the route (3)")
    c.add_argument("--budget-ms", type=float, default=DS_FRAME_MS / 2,
                   help=f"the most a scene's 95th percentile may take, in ms (half the DS's frame, "
                        f"{DS_FRAME_MS / 2:.2f})")
    c.add_argument("--report-only", action="store_true", help="report a scene over the budget without failing")
    c.add_argument("--max-load", type=float, default=float(os.cpu_count() or 1),
                   help=f"skip (77) rather than fail a scene over the budget when the 1-minute load average before a "
                        f"run or after the last was above this (the cores, {os.cpu_count() or 1}; inf never skips)")
    c.add_argument("--scene", action="append", default=[], type=parse_scene, help="NAME=FIRST-LAST, frame numbers")
    c.add_argument("--save", help="the save each run starts from")
    c.add_argument("--save-name", default="IREO.sav", help="its name in the save directory (IREO.sav, IRDO.sav)")
    c.add_argument("--work", help="where the runs' save directories and files go (kept); a temporary one otherwise")
    c.add_argument("command", nargs=argparse.REMAINDER, help="-- the program and its arguments")
    c.set_defaults(run=check)
    args = parser.parse_args(argv)
    if getattr(args, "runs", 1) < 1:
        parser.error("--runs needs 1 or more")
    return args.run(args)


if __name__ == "__main__":
    sys.exit(main())
