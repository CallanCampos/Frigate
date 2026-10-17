#!/usr/bin/env python3
"""Runs the melonDS oracle and the native port the same way, for compare.py (port/tools/oracle/README.md).

    oracle.py run --out DIR --frames N [--screens LIST] [--input FILE] [--dump SPEC]... [--time TIME]
                  [--version b2_us] [--rom PATH] [--oracle BIN] [--save FILE] [--no-counter]
    oracle.py port --binary BIN --out DIR --frames LIST [--input FILE] [--time TIME] [--version b2_us] [--rom PATH]
    oracle.py boot --out DIR [--port BIN] [--frames LIST] [--version b2_us]
    oracle.py title --port BIN [--out DIR] [--update] [--oracle BIN] [--version b2_us]
    oracle.py title-check --port BIN [--work DIR] [--version b2_us]
    oracle.py check --oracle BIN [--version b2_us] [--work DIR] [--update]

run calls pbw-oracle with the repository's defaults: the version's ROM in orig/, its symbols from config/, the
statics of the ROM build's link map (symbols.py), and GFL's frame count (sVBlankCount) as the counter for aligning
frames and for input files on game frames. It passes --time only when given one, so that pbw-oracle takes an input
file's time line, else its default. port runs the port's program once from power-on with a blank save, to the last
frame of LIST, and has it write its run directory (--run-dir): frames.tsv for every frame, a PNG file for each frame of
LIST. Its clock is --time, else the input file's, else the default. boot does both for the boot sequence and compares
them by sequence (compare.py), at the DS's 6 bits and at the palettes' 5. title does both on the title sequence's
input file (port/tests/oracle/title_<version>.pbwinput) and compares every frame by sequence but the known
differences; with --update it writes port/tests/oracle/title_<version>.tsv, the oracle's hashes at sampled frames.
title-check runs the port alone on that input file and checks its screens without 3D against those hashes. check is the
oracle's ctest test: the oracle runs the boot sequence twice with the same results, and its screens match the hashes in
port/tests/oracle/boot_<version>.tsv. title and check exit 77 without the oracle or the ROM, title-check without the
ROM.

Standard library only.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import compare  # noqa: E402
import symbols  # noqa: E402

REPO_ROOT = symbols.REPO_ROOT
DEFAULT_TIME = "2012-10-07 12:00:00"
DEFAULT_ORACLE = os.path.join(REPO_ROOT, "build", "port-oracle", "oracle", "pbw-oracle")
COUNTER = "sVBlankCount"
# The boot sequence's frames: the ones PLAN.md's phase 1 exit names, and checkpoints in the boot screens and the
# Game Freak logo as the oracle shows them (README.md)
BOOT_FRAMES = "1,30,60,120,250,300,420,500,600,700,800,1000,1100,1200"
# How much further than the port's last frame boot runs the oracle
BOOT_MARGIN = 400
# check's run: long enough to reach the first logo and the copyright screen
CHECK_FRAMES = 500
CHECK_SCREENS = "1,60,120,249,250,260,300,360,400,450,500"
SKIP = 77
# The title sequence (README.md, "The title sequence"): an input file on game frames from power-on through the boot
# screens, the Game Freak logo and part of the opening to the title, its layers hidden and brought back, and START to
# the start menu; how many frames each program runs (the DS takes 1,860 more than the port to get as far); and the
# reference that holds the port to the oracle's screens without the oracle (title-check)
TITLE_INPUT = os.path.join("port", "tests", "oracle", "title_{version}.pbwinput")
TITLE_REFERENCE = os.path.join("port", "tests", "oracle", "title_{version}.tsv")
TITLE_PORT_FRAMES = 3600
TITLE_ORACLE_FRAMES = 5700
# The port frames whose screens the reference keeps: every 12th, and each one around the three presses
TITLE_SAMPLE = "1-3600/12,1096-1120,2996-3010,3096-3135"
# Port frames whose screens differ from the oracle's by construction, as README.md's "Known differences" shows; the
# title comparison and the reference leave them out
TITLE_KNOWN = {
    "b2_us": {
        "3001": "the layers come back in the middle of the DS's frame (the CPU's time), at its start on the port",
        "3112-3129": "the DS dropped a frame at START, so its camera, stepped by VBlanks, leads Kyurem's animation",
    },
    "w2_us": {
        "3001": "the layers come back in the middle of the DS's frame (the CPU's time), at its start on the port",
        "3112-3128": "the DS dropped a frame at START, so its camera, stepped by VBlanks, leads Kyurem's animation",
        "3219-3237/3": "the cry ends a pass earlier on the port (the sound driver's clock), so its fade-out leads",
    },
}
REFERENCE_COLUMNS = ["port", "oracle", "counter", "top", "bottom"]


def rom_path(version: str, given: str | None) -> str:
    return given or os.path.join(REPO_ROOT, "orig", f"baserom_{version}.nds")


def rom_matches(version: str, path: str) -> str | None:
    """None when the ROM is the dump the decomp builds, else why not"""
    if not os.path.exists(path):
        return f"no ROM at {path}"
    with open(os.path.join(REPO_ROOT, f"{version}.sha1")) as f:
        expected = f.read().split()[0].lower()
    digest = hashlib.sha1()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    if digest.hexdigest() != expected:
        return f"{path} isn't the {version} dump the decomp builds (SHA-1 {digest.hexdigest()})"
    return None


def statics_file(version: str, directory: str) -> str | None:
    """The link map's symbols in symbols.txt's format, written into directory; None without a ROM build"""
    mapped = symbols.read_map(version)
    if not mapped:
        return None
    path = os.path.join(directory, f"statics_{version}.txt")
    with open(path, "w") as f:
        symbols.write_statics(mapped, f)
    return path


def run_oracle(args, out: str, frames: int, screens: str, dumps: list[str] | None = None,
               quiet: bool = False) -> int:
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py: no oracle at {binary}; configure the port with -DPORT_BUILD_ORACLE=ON and build",
              file=sys.stderr)
        return 2
    os.makedirs(out, exist_ok=True)
    command = [binary, "--rom", rom_path(args.version, args.rom), "--out", out, "--frames", str(frames),
               "--screens", screens, "--config", os.path.join(REPO_ROOT, "config", args.version, "arm9")]
    # pbw-oracle's clock is --time, else the input file's time line, else DEFAULT_TIME: --time only when one was given
    if args.time:
        command += ["--time", args.time]
    statics = statics_file(args.version, out)
    if statics:
        command += ["--symbols", statics]
        if not getattr(args, "no_counter", False):
            command += ["--counter", COUNTER]
    elif not quiet:
        print(f"oracle.py: no build/{args.version}/arm9.o.xMAP (run ninja): no statics and no counter",
              file=sys.stderr)
    if getattr(args, "input", None):
        command += ["--input", args.input]
    if getattr(args, "save", None):
        command += ["--save", args.save]
    for spec in dumps or []:
        command += ["--dump", spec]
    return subprocess.call(command)


def command_run(args) -> int:
    return run_oracle(args, args.out, args.frames, args.screens, args.dump)


def port_time(args) -> str | None:
    """The clock the port starts with: --time, else an input file's time line (the port reads it), else the default"""
    if args.time:
        return args.time
    return None if getattr(args, "input", None) else DEFAULT_TIME


def run_port(binary: str, out: str, frames: int, screens: str, rom: str, time: str | None,
             input_path: str | None = None, timeout: int = 1200) -> tuple[int | None, str]:
    """Runs the port's program once, from power-on with a blank save, for frames frames, writing its run directory
    in out (--run-dir): frames.tsv for every frame it ran and a PNG file for each frame of screens (a frame list,
    "none" or "all"). Returns its exit status (None when it timed out) and its last message about why it stopped"""
    os.makedirs(out, exist_ok=True)
    # A picture left from another run would pass for this one's
    for name in os.listdir(out):
        if name.startswith("frame_") and name.endswith(".png"):
            os.remove(os.path.join(out, name))
    saves = tempfile.mkdtemp(prefix="saves-", dir=out)
    command = [binary, "--headless", "--frames", str(frames), "--rom", rom, "--save-dir", saves, "--run-dir", out,
               "--run-screens", screens]
    if time:
        command += ["--fixed-time", time]
    if input_path:
        command += ["--input", input_path]
    try:
        done = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        status = done.returncode
        log = done.stderr + done.stdout
    except subprocess.TimeoutExpired:
        status = None
        log = "timed out"
    finally:
        shutil.rmtree(saves, ignore_errors=True)
    # Its own lines start with "port: ", after the time and process that SDL_Log puts first on some hosts
    reasons = [line[line.index("port: "):].strip() for line in log.splitlines()
               if "port: " in line and "entering" not in line and "port: rom:" not in line]
    return status, reasons[-1] if reasons else ""


def describe_port_run(status: int | None, message: str, frames: int, out: str) -> str:
    run = compare.load_run(out)
    ran = max(run.frames) if run.frames else 0
    what = "timed out" if status is None else f"exit {status}"
    return f"{what} after {ran} of {frames} frames" + (f": {message}" if message and status != 0 else "")


def command_port(args) -> int:
    frames = compare.parse_frames(args.frames)
    if frames is None:
        print("oracle.py: port needs a list of frames, not all", file=sys.stderr)
        return 2
    last = max(frames)
    status, message = run_port(args.binary, args.out, last, args.frames, rom_path(args.version, args.rom),
                               port_time(args), args.input)
    print(f"port: {describe_port_run(status, message, last, args.out)}")
    run = compare.load_run(args.out)
    missing = [frame for frame in frames if run.png(frame) is None]
    if missing:
        print(f"port: no picture of frames {compare.describe_ranges(missing)}")
    return 0 if status == 0 and not missing else 1


def command_boot(args) -> int:
    frames = compare.parse_frames(args.frames)
    # The DS falls behind the port (start-up, loading, dropped frames): the oracle runs further, with a picture of
    # every frame, so that sequence alignment finds each of the port's frames
    last = max(frames) + BOOT_MARGIN
    oracle_dir = os.path.join(args.out, "oracle")
    status = run_oracle(args, oracle_dir, last, "all")
    if status != 0:
        return status
    oracle_run = compare.load_run(oracle_dir)
    print(f"oracle: {len(oracle_run.frames)} frames; first frame that isn't blank: {compare.first_change(oracle_run)}")
    if not args.port:
        return 0
    port_dir = os.path.join(args.out, "port")
    status, message = run_port(args.port, port_dir, max(frames), args.frames, rom_path(args.version, args.rom),
                               port_time(args))
    print(f"port: {describe_port_run(status, message, max(frames), port_dir)}")
    if not compare.load_run(port_dir).frames:
        print("the port reached none of the frames")
        return 1
    report = os.path.join(args.out, "report")
    print("\nby sequence, at the DS's 6 bits:")
    six = compare.main(["screens", oracle_dir, port_dir, "--align", "sequence", "--report", report])
    print("\nby sequence, at the palettes' 5 bits:")
    five = compare.main(["screens", oracle_dir, port_dir, "--align", "sequence", "--depth", "5"])
    return max(six, five)


def command_check(args) -> int:
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py check: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    rom = rom_path(args.version, args.rom)
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py check: skipped: {reason}")
        return SKIP
    args.oracle = binary
    work = args.work or tempfile.mkdtemp(prefix="oracle-check-")
    runs = []
    for name in ("a", "b"):
        out = os.path.join(work, name)
        shutil.rmtree(out, ignore_errors=True)
        status = run_oracle(args, out, CHECK_FRAMES, CHECK_SCREENS, quiet=True)
        if status != 0:
            print(f"oracle.py check: the oracle failed (status {status})")
            return 1
        runs.append(compare.load_run(out))
    failed = 0
    with open(os.path.join(work, "a", "frames.tsv")) as a, open(os.path.join(work, "b", "frames.tsv")) as b:
        if a.read() != b.read():
            print("FAIL: two runs of the same boot differ (frames.tsv)")
            failed += 1
    first = runs[0]
    if len(first.frames) != CHECK_FRAMES:
        print(f"FAIL: {len(first.frames)} frames ran, not {CHECK_FRAMES}")
        failed += 1
    counters = [f.counter for f in first.frames.values() if f.counter is not None]
    if counters and max(counters) == 0:
        print("FAIL: the game's frame count never moved: the main loop never ran")
        failed += 1
    if compare.first_change(first) is None:
        print("FAIL: every frame is blank")
        failed += 1
    # The PNG files say what frames.tsv says: pbw-oracle's writer and hashes agree with compare.py's reader
    for frame in first.captured():
        top, bottom = compare.screen_hashes(compare.read_png(first.png(frame))[2])
        row = first.frames.get(frame)
        if row is None or (top, bottom) != (row.top, row.bottom):
            print(f"FAIL: frame {frame}'s PNG file and frames.tsv disagree")
            failed += 1

    golden = os.path.join(REPO_ROOT, "port", "tests", "oracle", f"boot_{args.version}.tsv")
    wanted = compare.parse_frames(CHECK_SCREENS)
    if args.update:
        with open(golden, "w") as f:
            melonds = first.info.get("melonds", "?")
            f.write(f"# pbw-oracle's screens in the first {CHECK_FRAMES} frames of {args.version}: no input, a blank\n"
                    f"# save, the clock at {DEFAULT_TIME}, melonDS {melonds}. The SHA-1 of each screen's RGB bytes\n"
                    "# and GFL's frame count; hashes only. Written by oracle.py check --update.\n")
            f.write("frame\tcounter\ttop\tbottom\n")
            for frame in wanted:
                row = first.frames[frame]
                counter = "-" if row.counter is None else row.counter
                f.write(f"{frame}\t{counter}\t{row.top}\t{row.bottom}\n")
        print(f"wrote {golden}")
    elif os.path.exists(golden):
        with open(golden) as f:
            lines = [line.rstrip("\n").split("\t") for line in f if not line.startswith("#")]
        for cells in lines[1:]:
            frame = int(cells[0])
            row = first.frames.get(frame)
            if row is None or (row.top, row.bottom) != (cells[2], cells[3]):
                print(f"FAIL: frame {frame}'s screens differ from {os.path.relpath(golden, REPO_ROOT)}")
                failed += 1
            elif cells[1] != "-" and row.counter is not None and int(cells[1]) != row.counter:
                print(f"FAIL: frame {frame}'s frame count is {row.counter}, not {cells[1]}")
                failed += 1
    else:
        print(f"note: no {os.path.relpath(golden, REPO_ROOT)}; determinism checked only")
    if failed:
        print(f"oracle.py check: {failed} failures (runs in {work})")
        return 1
    print(f"oracle.py check: the boot's {CHECK_FRAMES} frames are deterministic and match the hashes")
    if not args.work:
        shutil.rmtree(work, ignore_errors=True)
    return 0


def title_known(version: str) -> dict[int, str]:
    """The known differences of the version's title sequence, by port frame"""
    known = {}
    for frames, reason in TITLE_KNOWN.get(version, {}).items():
        for frame in compare.parse_frames(frames):
            known[frame] = reason
    return known


def load_reference(path: str) -> dict[int, tuple[int, int | None, str, str]]:
    """title_<version>.tsv: for each port frame it keeps, the oracle frame it pairs with, the oracle's frame count
    there, and the oracle's hash of each screen, or "3d" for a screen that shows 3D"""
    rows = {}
    header = None
    with open(path) as f:
        for number, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            cells = line.split("\t")
            if header is None:
                header = cells
                if header != REFERENCE_COLUMNS:
                    raise compare.RunError(f"{path}:{number}: expected the columns {' '.join(REFERENCE_COLUMNS)}")
                continue
            if len(cells) != len(REFERENCE_COLUMNS):
                raise compare.RunError(f"{path}:{number}: expected {len(REFERENCE_COLUMNS)} columns")
            rows[int(cells[0])] = (int(cells[1]), compare.optional_int(cells[2]), cells[3], cells[4])
    return rows


def write_reference(path: str, version: str, oracle_run: compare.Run, port_run: compare.Run) -> int:
    """Writes title_<version>.tsv from a comparison's runs: the sampled port frames that aren't known differences,
    with the oracle frames the sequence alignment pairs them with. Returns the number of rows"""
    mapping, _ = compare.sequence_mapping(oracle_run, port_run)
    known = title_known(version)
    melonds = oracle_run.info.get("melonds", "?")
    count = 0
    with open(path, "w") as f:
        f.write(f"# pbw-oracle's screens on {TITLE_INPUT.format(version=version)} (melonDS {melonds}), at the oracle\n"
                "# frames that compare.py's sequence alignment pairs with the port's frames listed: the SHA-1 of each\n"
                "# screen's RGB bytes, or 3d for a screen that shows 3D, which only the oracle comparison can judge,\n"
                "# and GFL's frame count there. Hashes only. Written by oracle.py title --update; checked by\n"
                "# oracle.py title-check.\n")
        f.write("\t".join(REFERENCE_COLUMNS) + "\n")
        for frame in compare.parse_frames(TITLE_SAMPLE):
            target = mapping.get(frame)
            row = oracle_run.frames.get(target) if target is not None else None
            if frame in known or row is None:
                continue
            counter = "-" if row.counter is None else row.counter
            top = "3d" if row.top_3d else row.top
            bottom = "3d" if row.bottom_3d else row.bottom
            f.write(f"{frame}\t{target}\t{counter}\t{top}\t{bottom}\n")
            count += 1
    return count


def title_paths(args) -> tuple[str, str, str]:
    """The version's input file, reference and ROM"""
    return (os.path.join(REPO_ROOT, TITLE_INPUT.format(version=args.version)),
            os.path.join(REPO_ROOT, TITLE_REFERENCE.format(version=args.version)),
            rom_path(args.version, args.rom))


def command_title(args) -> int:
    input_path, reference, rom = title_paths(args)
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py title: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py title: skipped: {reason}")
        return SKIP
    if not os.path.exists(input_path):
        print(f"oracle.py title: no input file {os.path.relpath(input_path, REPO_ROOT)} for {args.version}")
        return 1
    args.oracle = binary
    args.input = input_path
    work = args.out or tempfile.mkdtemp(prefix="oracle-title-")
    oracle_dir = os.path.join(work, "oracle")
    port_dir = os.path.join(work, "port")
    shutil.rmtree(oracle_dir, ignore_errors=True)
    status = run_oracle(args, oracle_dir, TITLE_ORACLE_FRAMES, "all", quiet=True)
    if status != 0:
        print(f"oracle.py title: the oracle failed (status {status})")
        return 1
    status, message = run_port(args.port, port_dir, TITLE_PORT_FRAMES, "all", rom, args.time, input_path)
    print(f"port: {describe_port_run(status, message, TITLE_PORT_FRAMES, port_dir)}")
    if status != 0:
        return 1
    known = title_known(args.version)
    compared = [frame for frame in range(1, TITLE_PORT_FRAMES + 1) if frame not in known]
    result = compare.main(["screens", oracle_dir, port_dir, "--align", "sequence", "--brief",
                           "--frames", ",".join(str(frame) for frame in compared),
                           "--report", os.path.join(work, "report")])
    for frames, why in TITLE_KNOWN.get(args.version, {}).items():
        print(f"left out, port frames {frames}: {why}")
    if args.update:
        if result != 0:
            print("oracle.py title: the comparison failed; the reference stays as it was")
        else:
            rows = write_reference(reference, args.version, compare.load_run(oracle_dir), compare.load_run(port_dir))
            print(f"wrote {rows} frames to {os.path.relpath(reference, REPO_ROOT)}")
    if not args.out:
        if result == 0:
            shutil.rmtree(work, ignore_errors=True)
        else:
            print(f"the runs and the difference images are in {work}")
    return result


def command_title_check(args) -> int:
    input_path, reference, rom = title_paths(args)
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py title-check: skipped: {reason}")
        return SKIP
    if not os.path.exists(reference):
        print(f"oracle.py title-check: no reference {os.path.relpath(reference, REPO_ROOT)} for {args.version}")
        return 1
    rows = load_reference(reference)
    work = args.work or tempfile.mkdtemp(prefix="oracle-title-check-")
    status, message = run_port(args.port, work, TITLE_PORT_FRAMES, "none", rom, args.time, input_path)
    print(f"port: {describe_port_run(status, message, TITLE_PORT_FRAMES, work)}")
    if status != 0:
        return 1
    port_run = compare.load_run(work)
    failed = compared = left = 0
    for frame, (oracle_frame, _, top, bottom) in sorted(rows.items()):
        mine = port_run.frames.get(frame)
        if mine is None:
            print(f"FAIL: port frame {frame} has no row")
            failed += 1
            continue
        for name, wanted, have in (("top", top, mine.top), ("bottom", bottom, mine.bottom)):
            if wanted == "3d":
                left += 1
                continue
            compared += 1
            if wanted != have:
                print(f"FAIL: port frame {frame}'s {name} screen isn't oracle frame {oracle_frame}'s")
                failed += 1
    print(f"{len(rows)} frames: {compared - failed} of {compared} screens without 3D as the oracle's; {left} with 3D "
          "left to the oracle comparison (oracle.py title)")
    if not args.work:
        shutil.rmtree(work, ignore_errors=True)
    return 1 if failed else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)

    def common(p, oracle=True):
        p.add_argument("--version", default="b2_us", choices=["b2_us", "w2_us"])
        p.add_argument("--rom", help="the ROM (orig/baserom_<version>.nds by default)")
        default = f"an input file's, else {DEFAULT_TIME}" if oracle else DEFAULT_TIME
        p.add_argument("--time", help=f"the clock at power-on (default: {default})")
        if oracle:
            p.add_argument("--oracle", help=f"pbw-oracle (default {os.path.relpath(DEFAULT_ORACLE, REPO_ROOT)})")

    run = commands.add_parser("run", help="run the oracle with the repository's defaults")
    common(run)
    run.add_argument("--out", required=True)
    run.add_argument("--frames", type=int, required=True)
    run.add_argument("--screens", default="none")
    run.add_argument("--input")
    run.add_argument("--save")
    run.add_argument("--dump", action="append")
    run.add_argument("--no-counter", action="store_true")

    port = commands.add_parser("port", help="run the port and write its run directory")
    common(port, oracle=False)
    port.add_argument("--binary", required=True, help="the port's program (pokeblack2 or pokewhite2)")
    port.add_argument("--out", required=True)
    port.add_argument("--frames", required=True, help="the frames to write as PNG; the run goes to the last")
    port.add_argument("--input")

    boot = commands.add_parser("boot", help="the boot sequence on the oracle and, with --port, the port")
    common(boot)
    boot.add_argument("--out", required=True)
    boot.add_argument("--port", help="the port's program")
    boot.add_argument("--frames", default=BOOT_FRAMES)

    check = commands.add_parser("check", help="the ctest test: determinism and the boot hashes")
    common(check)
    check.add_argument("--work", help="keep the runs here")
    check.add_argument("--update", action="store_true", help="write the boot hashes instead of checking them")

    title = commands.add_parser("title", help="the title sequence on the oracle and the port, compared")
    common(title)
    title.add_argument("--port", required=True, help="the port's program")
    title.add_argument("--out", help="keep the runs and the difference images here")
    title.add_argument("--update", action="store_true", help="write the reference from a comparison that passes")

    title_check = commands.add_parser("title-check", help="the port's title sequence against the reference")
    common(title_check, oracle=False)
    title_check.add_argument("--port", required=True, help="the port's program")
    title_check.add_argument("--work", help="keep the port's run here")

    args = parser.parse_args(argv)
    try:
        if args.command == "run":
            return command_run(args)
        if args.command == "port":
            return command_port(args)
        if args.command == "boot":
            return command_boot(args)
        if args.command == "title":
            return command_title(args)
        if args.command == "title-check":
            return command_title_check(args)
        return command_check(args)
    except (compare.RunError, OSError, ValueError) as error:
        print(f"oracle.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
