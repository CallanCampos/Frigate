#!/usr/bin/env python3
"""Runs the melonDS oracle and the native port the same way, for compare.py (port/tools/oracle/README.md).

    oracle.py run --out DIR --frames N [--screens LIST] [--input FILE] [--dump SPEC]... [--time TIME]
                  [--version b2_us] [--rom PATH] [--oracle BIN] [--save FILE] [--save-out FILE] [--no-counter]
    oracle.py port --binary BIN --out DIR --frames LIST [--input FILE] [--dump SPEC]... [--time TIME] [--version b2_us]
                   [--rom PATH] [--save FILE] [--save-out FILE]
    oracle.py save NAME --out FILE [--oracle BIN] [--version b2_us] [--work DIR]
    oracle.py boot --out DIR [--port BIN] [--frames LIST] [--version b2_us]
    oracle.py sequence NAME --port BIN [--out DIR] [--update] [--oracle BIN] [--version b2_us]
    oracle.py sequence-check NAME --port BIN [--work DIR] [--version b2_us]
    oracle.py check --oracle BIN [--version b2_us] [--work DIR] [--update]

run calls pbw-oracle with the repository's defaults: the version's ROM in orig/, its symbols from config/, the
statics of the ROM build's link map (symbols.py), and GFL's frame count (sVBlankCount) as the counter for aligning
frames and for input files on game frames. It passes --time only when given one, so that pbw-oracle takes an input
file's time line, else its default. port runs the port's program once from power-on with a blank save, to the last
frame of LIST, and has it write its run directory (--run-dir): frames.tsv for every frame, a PNG file for each frame of
LIST, memory.txt with the --dump specs (the program's --run-dump: globals by name, with a size, * through a pointer).
Its clock is --time, else the input file's, else the default. Both take --save FILE, the save memory at power-on (the
port's copy of it is the save directory's file, named after the ROM's game code, as the port names its saves), and
--save-out FILE, the save memory when the run ends. save makes one of the saves the sequences start from on the oracle
(SAVES: first_save, power-on through New Game into the field and the field menu's SAVE), writes it to FILE and checks
it against the SHA-1 kept for it. boot does both for the boot sequence and compares
them on the game's count, the movies by their pictures (compare.py), at the DS's 6 bits and at the palettes' 5.
sequence does both on a comparison sequence's input file, port/tests/oracle/<name>_<version>.pbwinput (SEQUENCES:
opening, power-on to the title with nothing pressed; title, the opening skipped to the title and START to the start
menu; continue, the same from first_save's save, the start menu's CONTINUE and the field it goes to), from the save the
sequence names, made first on the oracle, and compares every frame the same way but the known differences, up to the
frame where the port stops when the sequence allows it to stop on code with no C yet; with --update it writes
port/tests/oracle/<name>_<version>.tsv, the oracle's hashes at sampled frames. sequence-check runs the port alone on
that input file and checks its counts and its screens without 3D against that reference. check is the oracle's ctest
test: the oracle runs the boot sequence twice with the same results, and its screens match the hashes in
port/tests/oracle/boot_<version>.tsv. save, sequence and check exit 77 without the oracle or the ROM, sequence-check
without the ROM.

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
from dataclasses import dataclass, field

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
# The comparison sequences (README.md, "The comparison sequences"), each an input file on game frames for each version,
# port/tests/oracle/<name>_<version>.pbwinput, and a reference that holds the port to the oracle's screens without the
# oracle (sequence-check), port/tests/oracle/<name>_<version>.tsv
SEQUENCE_INPUT = os.path.join("port", "tests", "oracle", "{name}_{version}.pbwinput")
SEQUENCE_REFERENCE = os.path.join("port", "tests", "oracle", "{name}_{version}.tsv")


@dataclass(frozen=True)
class Sequence:
    """How a sequence is run and compared. port_frames and oracle_frames: how far each program runs (the DS takes
    hundreds of frames more to get as far). sample: the port frames whose screens the reference keeps. clock: the port
    frames where the game waits on a clock, which pair by their pictures (compare.py --clock); everywhere else frames
    pair on the game's count. lags: by version, the count lags declared (compare.py --lag). known: by version, the port
    frames whose screens differ from the oracle's by construction, left out of the comparison and the reference, and
    known_oracle the oracle frames that no port frame can show; each with why (README.md, "Known differences").
    state_frames: the port frames whose game state (STATE) is compared with the oracle's at the frames they pair with,
    which state_oracle, the oracle frames dumped, covers; known_state, by version, the port frames whose state differs
    by construction, with why"""
    port_frames: int
    oracle_frames: int
    sample: str
    clock: str
    lags: dict[str, str] = field(default_factory=dict)
    known: dict[str, dict[str, str]] = field(default_factory=dict)
    known_oracle: dict[str, dict[str, str]] = field(default_factory=dict)
    state_frames: str = ""
    state_oracle: str = ""
    known_state: dict[str, dict[str, str]] = field(default_factory=dict)
    # The save both programs start from, a SAVES name; none for a blank save
    save: str = ""
    # The port frame from which the port may stop on a trap stub (TRAP_STATUS), and why: code or data the decomp
    # doesn't have yet. The frames it ran are compared, and the run passes if it got that far
    stop_from: int = 0
    stop: str = ""


# The game state the sequences compare by symbol, with its size: NNS G3D's global state (glbstate.c), the projection,
# the camera and the matrices it makes of them, 612 bytes without a pointer, so the same on a host as on the DS
STATE = (("NNS_G3dGlb", 0x264),)


# The known differences, by kind (README.md, "Known differences")
TORN_OPENING = "the opening's first frame, written partway through the DS's frame: black above line 77"
TORN_HIDE = "the title hides its layers partway through the DS's frame; on the port, from the frame's first line"
TORN_BACK = "START brings the layers back partway through the DS's frame; on the port, from the frame's first line"
TORN_START = ("START whitens the screens partway through the second frame of the DS's pass, which took two; on the "
              "port, from the first line of the pass's frame")
CAMERA = "the DS's pass at START took two frames, so its camera, stepped by VBlanks, leads Kyurem's animation a step"
CRY = "the DS starts Kyurem's cry later against its passes, so the title fades out a pass later there"
CURSOR = ("the start menu's cursor blinks a pass ahead on the port since the title's cry (the lag of -1), so the press, "
          "which both read on the same pass, finds it a step apart, a step the DS doesn't show")
SEQUENCES = {
    # Power-on, the boot screens, the Game Freak logo and the whole opening to the title and its first 274 frames of
    # 3D, nothing pressed: the movies run from port frame 313 (count 312) to the title, at port frame 3527 in both
    # versions
    "opening": Sequence(
        port_frames=3800,
        oracle_frames=4300,
        sample="1-3800/12,3520-3540",
        clock="313-3526",
        known_oracle={"w2_us": {"1283": TORN_OPENING}},
        state_frames="3527-3800",
        state_oracle="3700-4300",
    ),
    # START at game frame 1100 skips the opening, which holds its frame until count 1107 on both programs (port frame
    # 1108): the counts line up again after it. The title hides its layers at count 2877, START at 3000 brings them
    # back, START at 3100 goes to the start menu
    "title": Sequence(
        port_frames=3600,
        oracle_frames=5700,
        sample="1-3600/12,1096-1120,2870-2885,2996-3010,3096-3135,3215-3250",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY},
        },
        known_oracle={"b2_us": {"4943": TORN_START}, "w2_us": {"1283": TORN_OPENING, "4875": TORN_START}},
        state_frames="1116-3600",
        state_oracle="2800-5700",
        known_state={"b2_us": {"3110-3124": CAMERA}, "w2_us": {"3110-3124": CAMERA}},
    ),
    # The title sequence's presses from first_save's save, then A at game frame 3400 on the start menu's CONTINUE,
    # which now shows the saved game: the player, the place, the badges and the play time (the Pokédex once the
    # player has one). CONTINUE blinks, the menu wipes out to black from port frame 3425, and the field loads; the port
    # stops there on overlay 36's field code and data, which have no C yet (stop_from). The title's known differences
    # are the title sequence's; on the oracle they come 23 frames sooner, since its start-up, which reads the save,
    # starts the main loop at frame 219 with this save and at 242 with a blank one. In White 2 the press of CONTINUE
    # brings the counts together again
    "continue": Sequence(
        port_frames=3600,
        oracle_frames=6000,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR},
        },
        known_oracle={"b2_us": {"4920": TORN_START}, "w2_us": {"1260": TORN_OPENING, "4852": TORN_START}},
        save="first_save",
        stop_from=3425,
        stop="the field after CONTINUE, whose overlay 36 code and data have no C yet",
    ),
}
REFERENCE_COLUMNS = ["port", "oracle", "counter", "lag", "top", "bottom"]
# The exit status of a port run that reached a function or data with no C yet (port/hal/trap.c)
TRAP_STATUS = 70


@dataclass(frozen=True)
class SaveMaker:
    """A save made on the oracle, from power-on with a blank save, by the input file
    port/tests/oracle/<name>_<version>.pbwinput (SEQUENCE_INPUT's place): frames, how many frames the oracle runs;
    sha1, by version, the SHA-1 of the save memory it writes at the end, which is the same at every run (the oracle is
    deterministic), so that what the sequences start from is the save that was examined (README.md, "Saves")"""
    frames: int
    sha1: dict[str, str] = field(default_factory=dict)


SAVES = {
    # New Game as new_game.pbwinput plays it (the player AAAAAAA, the rival HughAAA), into the field in front of the
    # player's house, through the first scripts to the player's room, then the field menu's SAVE: the adventure's
    # first save
    "first_save": SaveMaker(18800, {"b2_us": "a7a58889e2399d1f6a0b4c0f5600748dd139449e",
                                    "w2_us": "479529f98b29cdd852502360aeb33531ca111d7f"}),
}


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
    if getattr(args, "save_out", None):
        command += ["--save-out", args.save_out]
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


def rom_game_code(path: str) -> str:
    """The ROM header's game code (IREO, IRDO), which the port names its save file after"""
    with open(path, "rb") as f:
        f.seek(0x0c)
        return f.read(4).decode("ascii", "replace")


def run_port(binary: str, out: str, frames: int, screens: str, rom: str, time: str | None,
             input_path: str | None = None, timeout: int = 1200, dumps: list[str] | None = None,
             save: str | None = None, save_out: str | None = None) -> tuple[int | None, str]:
    """Runs the port's program once, from power-on with a blank save or with the save memory in the file save, for
    frames frames, writing its run directory in out (--run-dir): frames.tsv for every frame it ran, a PNG file for each
    frame of screens (a frame list, "none" or "all") and memory.txt with the dumps (--run-dump specs). The save
    directory is a new one, given the save as the port names its saves (the game code and .sav); save_out gets the
    save memory the run left there, if any. Returns its exit status (None when it timed out) and its last message
    about why it stopped"""
    os.makedirs(out, exist_ok=True)
    # A picture left from another run would pass for this one's
    for name in os.listdir(out):
        if name.startswith("frame_") and name.endswith(".png"):
            os.remove(os.path.join(out, name))
    saves = tempfile.mkdtemp(prefix="saves-", dir=out)
    save_file = os.path.join(saves, f"{rom_game_code(rom)}.sav") if save or save_out else None
    if save:
        shutil.copyfile(save, save_file)
    command = [binary, "--headless", "--frames", str(frames), "--rom", rom, "--save-dir", saves, "--run-dir", out,
               "--run-screens", screens]
    if time:
        command += ["--fixed-time", time]
    if input_path:
        command += ["--input", input_path]
    for spec in dumps or []:
        command += ["--run-dump", spec]
    try:
        done = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        status = done.returncode
        log = done.stderr + done.stdout
    except subprocess.TimeoutExpired:
        status = None
        log = "timed out"
    finally:
        if save_file and save_out and os.path.exists(save_file):
            shutil.copyfile(save_file, save_out)
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
                               port_time(args), args.input, dumps=args.dump, save=args.save, save_out=args.save_out)
    print(f"port: {describe_port_run(status, message, last, args.out)}")
    run = compare.load_run(args.out)
    missing = [frame for frame in frames if run.png(frame) is None]
    if missing:
        print(f"port: no picture of frames {compare.describe_ranges(missing)}")
    return 0 if status == 0 and not missing else 1


def file_sha1(path: str) -> str:
    digest = hashlib.sha1()
    with open(path, "rb") as f:
        digest.update(f.read())
    return digest.hexdigest()


def make_save(args, name: str, out: str, work: str) -> str | None:
    """Makes the save SAVES[name] on the oracle (args.oracle, args.version, args.rom), writing the save memory to out,
    with the oracle's run directory in work. Returns None, or why the save isn't the one kept: the oracle failed, or
    its SHA-1 isn't the one SAVES has for the version (which it reports, for a version that has none yet)"""
    maker = SAVES[name]
    input_path = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=name, version=args.version))
    if not os.path.exists(input_path):
        return f"no input file {os.path.relpath(input_path, REPO_ROOT)} for {args.version}"
    made = argparse.Namespace(**vars(args))
    made.input = input_path
    made.save = None
    made.save_out = out
    shutil.rmtree(work, ignore_errors=True)
    if os.path.exists(out):
        os.remove(out)
    status = run_oracle(made, work, maker.frames, "none", quiet=True)
    if status != 0 or not os.path.exists(out):
        return f"the oracle failed (status {status})"
    size = os.path.getsize(out)
    digest = file_sha1(out)
    wanted = maker.sha1.get(args.version)
    print(f"{name}: {size} bytes of save memory, SHA-1 {digest}")
    if wanted is None:
        return f"SAVES has no SHA-1 of {name} for {args.version}; this run's is {digest}"
    if digest != wanted:
        return f"the save isn't the one kept ({wanted}): the oracle, the input file or the ROM changed"
    return None


def command_save(args) -> int:
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py save: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    reason = rom_matches(args.version, rom_path(args.version, args.rom))
    if reason:
        print(f"oracle.py save: skipped: {reason}")
        return SKIP
    args.oracle = binary
    work = args.work or tempfile.mkdtemp(prefix=f"oracle-save-{args.name}-")
    reason = make_save(args, args.name, args.out, work)
    if reason:
        print(f"oracle.py save: {reason}")
        return 1
    if not args.work:
        shutil.rmtree(work, ignore_errors=True)
    print(f"wrote {args.out}")
    return 0


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
    # The boot screens pair on the count, the movies from the Game Freak logo on by their pictures, as the opening
    # sequence does
    align = ["--align", "counter", "--clock", SEQUENCES["opening"].clock]
    print("\nat the DS's 6 bits:")
    six = compare.main(["screens", oracle_dir, port_dir, *align, "--report", report])
    print("\nat the palettes' 5 bits:")
    five = compare.main(["screens", oracle_dir, port_dir, *align, "--depth", "5"])
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


def by_frame(table: dict[str, str]) -> dict[int, str]:
    """{"3112-3129": why} as {3112: why, 3113: why, ...}"""
    out = {}
    for frames, why in table.items():
        for frame in compare.parse_frames(frames):
            out[frame] = why
    return out


def load_reference(path: str) -> dict[int, tuple[int, int | None, int | None, str, str]]:
    """<name>_<version>.tsv: for each port frame it keeps, the oracle frame it pairs with, the oracle's frame count
    there, the count lag (None where the frame pairs by its pictures), and the oracle's hash of each screen, or "3d"
    for a screen that shows 3D"""
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
            rows[int(cells[0])] = (int(cells[1]), compare.optional_int(cells[2]), compare.optional_int(cells[3]),
                                   cells[4], cells[5])
    return rows


def write_reference(path: str, name: str, version: str, oracle_run: compare.Run, port_run: compare.Run) -> int:
    """Writes <name>_<version>.tsv from a comparison's runs: for the sampled port frames that aren't known differences,
    the oracle frame the comparison pairs each with. Where the game counts passes that is the oracle frame at the
    port's count less the lag, which comes from the oracle and the declared lags; in a clock stretch it is the oracle
    frame that shows the port frame's pictures. Returns the number of rows"""
    sequence = SEQUENCES[name]
    clock = set(compare.parse_frames(sequence.clock))
    pairing = compare.counter_mapping(oracle_run, port_run, clock, compare.parse_lags(sequence.lags.get(version)))
    known = by_frame(sequence.known.get(version, {}))
    # Where the frames pair on the count, the lag between them
    lags = {}
    for frame, target in pairing.mapping.items():
        mine, theirs = port_run.frames.get(frame), oracle_run.frames.get(target)
        if frame not in clock and mine and theirs and None not in (mine.counter, theirs.counter):
            lags[frame] = mine.counter - theirs.counter
    melonds = oracle_run.info.get("melonds", "?")
    count = 0
    with open(path, "w") as f:
        f.write(f"# pbw-oracle's screens on {SEQUENCE_INPUT.format(name=name, version=version)} (melonDS {melonds}),\n"
                "# at the oracle frames that compare.py --align counter pairs with the port frames listed: where the\n"
                "# game counts passes (lag is a number), the oracle frame whose count is the port's less the lag; in\n"
                f"# a clock stretch (port frames {sequence.clock}, lag -), the one that shows the port frame's\n"
                "# pictures. The SHA-1 of each screen's RGB bytes, or 3d for a screen that shows 3D, which only the\n"
                "# oracle comparison can judge, and GFL's frame count there. Hashes only. Written by oracle.py\n"
                f"# sequence {name} --update; checked by oracle.py sequence-check {name}.\n")
        for frames, why in sequence.known.get(version, {}).items():
            f.write(f"# Left out, port frames {frames}: {why}\n")
        f.write("\t".join(REFERENCE_COLUMNS) + "\n")
        for frame in compare.parse_frames(sequence.sample):
            target = pairing.mapping.get(frame)
            row = oracle_run.frames.get(target) if target is not None else None
            if frame in known or row is None:
                continue
            counter = "-" if row.counter is None else row.counter
            lag_cell = lags.get(frame, "-")
            top = "3d" if row.top_3d else row.top
            bottom = "3d" if row.bottom_3d else row.bottom
            f.write(f"{frame}\t{target}\t{counter}\t{lag_cell}\t{top}\t{bottom}\n")
            count += 1
    return count


def sequence_paths(args) -> tuple[str, str, str]:
    """The sequence's input file and reference for the version, and the ROM"""
    return (os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=args.name, version=args.version)),
            os.path.join(REPO_ROOT, SEQUENCE_REFERENCE.format(name=args.name, version=args.version)),
            rom_path(args.version, args.rom))


def stop_allowed(sequence: Sequence, status: int | None, last: int) -> bool:
    """Whether a port run that didn't end with status 0 ended as the sequence allows: on a trap stub (code or data with
    no C yet) after the frame from which the sequence allows it"""
    return status == TRAP_STATUS and sequence.stop_from > 0 and last >= sequence.stop_from


def command_sequence(args) -> int:
    sequence = SEQUENCES[args.name]
    input_path, reference, rom = sequence_paths(args)
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py sequence: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py sequence: skipped: {reason}")
        return SKIP
    if not os.path.exists(input_path):
        print(f"oracle.py sequence: no input file {os.path.relpath(input_path, REPO_ROOT)} for {args.version}")
        return 1
    args.oracle = binary
    work = args.out or tempfile.mkdtemp(prefix=f"oracle-{args.name}-")
    oracle_dir = os.path.join(work, "oracle")
    port_dir = os.path.join(work, "port")
    shutil.rmtree(oracle_dir, ignore_errors=True)
    # The save both start from, made on the oracle first
    args.save = None
    if sequence.save:
        args.save = os.path.join(work, f"{sequence.save}.sav")
        reason = make_save(args, sequence.save, args.save, os.path.join(work, sequence.save))
        if reason:
            print(f"oracle.py sequence: {sequence.save}: {reason}")
            return 1
    args.input = input_path
    oracle_dumps = [f"{name}:{size}@{sequence.state_oracle}" for name, size in STATE] if sequence.state_frames else []
    port_dumps = [f"{name}:{size}@{sequence.state_frames}" for name, size in STATE] if sequence.state_frames else []
    status = run_oracle(args, oracle_dir, sequence.oracle_frames, "all", oracle_dumps, quiet=True)
    if status != 0:
        print(f"oracle.py sequence: the oracle failed (status {status})")
        return 1
    status, message = run_port(args.port, port_dir, sequence.port_frames, "all", rom, args.time, input_path,
                               dumps=port_dumps, save=args.save)
    print(f"port: {describe_port_run(status, message, sequence.port_frames, port_dir)}")
    port_run = compare.load_run(port_dir)
    last = max(port_run.frames) if port_run.frames else 0
    if status != 0:
        if not stop_allowed(sequence, status, last):
            return 1
        print(f"the port stopped after frame {last}, where the sequence allows it (from frame {sequence.stop_from}): "
              f"{sequence.stop}")
    known = by_frame(sequence.known.get(args.version, {}))
    compared = [frame for frame in range(1, last + 1) if frame not in known]
    align = ["--align", "counter", "--clock", sequence.clock]
    if sequence.lags.get(args.version):
        align += ["--lag", sequence.lags[args.version]]
    command = ["screens", oracle_dir, port_dir, *align, "--brief", "--frames", ",".join(str(f) for f in compared),
               "--report", os.path.join(work, "report")]
    known_oracle = sequence.known_oracle.get(args.version, {})
    if known_oracle:
        command += ["--known-oracle", ",".join(known_oracle)]
    result = compare.main(command)
    for frames, why in sequence.known.get(args.version, {}).items():
        print(f"left out, port frames {frames}: {why}")
    for frames, why in known_oracle.items():
        print(f"no counterpart, oracle frames {frames}: {why}")
    if sequence.state_frames:
        known_state = by_frame(sequence.known_state.get(args.version, {}))
        state = [f for f in compare.parse_frames(sequence.state_frames) if f not in known_state]
        print(f"\nthe game's state ({', '.join(name for name, _ in STATE)}) at port frames {sequence.state_frames}, "
              "on the count:")
        result = max(result, compare.main(["memory", oracle_dir, port_dir, *align,
                                           "--frames", ",".join(str(f) for f in state)]))
        for frames, why in sequence.known_state.get(args.version, {}).items():
            print(f"state left out, port frames {frames}: {why}")
    if args.update:
        if sequence.save:
            print("oracle.py sequence: no reference for a sequence that starts from a save: only the oracle makes the "
                  "save, so only this comparison can check the port on it")
        elif result != 0:
            print("oracle.py sequence: the comparison failed; the reference stays as it was")
        else:
            rows = write_reference(reference, args.name, args.version, compare.load_run(oracle_dir),
                                   compare.load_run(port_dir))
            print(f"wrote {rows} frames to {os.path.relpath(reference, REPO_ROOT)}")
    if not args.out:
        if result == 0:
            shutil.rmtree(work, ignore_errors=True)
        else:
            print(f"the runs and the difference images are in {work}")
    return result


def check_reference(rows: dict[int, tuple[int, int | None, int | None, str, str]],
                    port_run: compare.Run) -> tuple[list[str], int, int, int]:
    """A port run against a reference's rows: what fails, and how many screens without 3D were compared, how many
    with 3D were left to the oracle comparison, and how many rows pair on the count"""
    failures = []
    compared = left = timed = 0
    for frame, (oracle_frame, counter, lag, top, bottom) in sorted(rows.items()):
        mine = port_run.frames.get(frame)
        if mine is None:
            failures.append(f"port frame {frame} has no row")
            continue
        # Where the game counts passes, the port is at the oracle frame's count plus the lag
        if lag is not None and counter is not None:
            timed += 1
            if mine.counter != counter + lag:
                failures.append(f"port frame {frame} is at count {mine.counter}, not {counter + lag} (oracle frame "
                                f"{oracle_frame}'s count {counter} and the lag {lag})")
        for name, wanted, have in (("top", top, mine.top), ("bottom", bottom, mine.bottom)):
            if wanted == "3d":
                left += 1
                continue
            compared += 1
            if wanted != have:
                failures.append(f"port frame {frame}'s {name} screen isn't oracle frame {oracle_frame}'s")
    return failures, compared, left, timed


def command_sequence_check(args) -> int:
    sequence = SEQUENCES[args.name]
    input_path, reference, rom = sequence_paths(args)
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py sequence-check: skipped: {reason}")
        return SKIP
    if not os.path.exists(reference):
        print(f"oracle.py sequence-check: no reference {os.path.relpath(reference, REPO_ROOT)} for {args.version}")
        return 1
    rows = load_reference(reference)
    work = args.work or tempfile.mkdtemp(prefix=f"oracle-{args.name}-check-")
    status, message = run_port(args.port, work, sequence.port_frames, "none", rom, args.time, input_path)
    print(f"port: {describe_port_run(status, message, sequence.port_frames, work)}")
    if status != 0:
        return 1
    failures, compared, left, timed = check_reference(rows, compare.load_run(work))
    for line in failures:
        print(f"FAIL: {line}")
    print(f"{len(rows)} frames, {timed} of them on the count: {len(failures)} failures; {compared} screens without 3D "
          f"compared, {left} with 3D left to the oracle comparison (oracle.py sequence {args.name})")
    if not args.work:
        shutil.rmtree(work, ignore_errors=True)
    return 1 if failures else 0


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
    run.add_argument("--save", help="the save memory at power-on (blank otherwise)")
    run.add_argument("--save-out", help="where to write the save memory at the end")
    run.add_argument("--dump", action="append")
    run.add_argument("--no-counter", action="store_true")

    port = commands.add_parser("port", help="run the port and write its run directory")
    common(port, oracle=False)
    port.add_argument("--binary", required=True, help="the port's program (pokeblack2 or pokewhite2)")
    port.add_argument("--out", required=True)
    port.add_argument("--frames", required=True, help="the frames to write as PNG; the run goes to the last")
    port.add_argument("--input")
    port.add_argument("--dump", action="append", help="memory to dump, as the program's --run-dump takes it: "
                      "[LABEL=][*]SYMBOL[+OFFSET]:SIZE[@LIST], a global by name")
    port.add_argument("--save", help="the save memory at power-on (blank otherwise), a .sav as melonDS writes it")
    port.add_argument("--save-out", help="where to copy the save memory the run leaves")

    save = commands.add_parser("save", help="make one of the saves the sequences start from, on the oracle")
    save.add_argument("name", choices=sorted(SAVES))
    common(save)
    save.add_argument("--out", required=True, help="where to write the save memory")
    save.add_argument("--work", help="keep the oracle's run here")

    boot = commands.add_parser("boot", help="the boot sequence on the oracle and, with --port, the port")
    common(boot)
    boot.add_argument("--out", required=True)
    boot.add_argument("--port", help="the port's program")
    boot.add_argument("--frames", default=BOOT_FRAMES)

    check = commands.add_parser("check", help="the ctest test: determinism and the boot hashes")
    common(check)
    check.add_argument("--work", help="keep the runs here")
    check.add_argument("--update", action="store_true", help="write the boot hashes instead of checking them")

    sequence = commands.add_parser("sequence", help="a comparison sequence on the oracle and the port, compared")
    sequence.add_argument("name", choices=sorted(SEQUENCES))
    common(sequence)
    sequence.add_argument("--port", required=True, help="the port's program")
    sequence.add_argument("--out", help="keep the runs and the difference images here")
    sequence.add_argument("--update", action="store_true", help="write the reference from a comparison that passes")

    sequence_check = commands.add_parser("sequence-check", help="the port on a sequence against its reference")
    sequence_check.add_argument("name", choices=sorted(SEQUENCES))
    common(sequence_check, oracle=False)
    sequence_check.add_argument("--port", required=True, help="the port's program")
    sequence_check.add_argument("--work", help="keep the port's run here")

    args = parser.parse_args(argv)
    try:
        if args.command == "run":
            return command_run(args)
        if args.command == "port":
            return command_port(args)
        if args.command == "save":
            return command_save(args)
        if args.command == "boot":
            return command_boot(args)
        if args.command == "sequence":
            return command_sequence(args)
        if args.command == "sequence-check":
            return command_sequence_check(args)
        return command_check(args)
    except (compare.RunError, OSError, ValueError) as error:
        print(f"oracle.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
