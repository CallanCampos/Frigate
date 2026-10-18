#!/usr/bin/env python3
"""Runs the melonDS oracle and Frigate's program alike, for compare.py and sound.py (port/tools/oracle/README.md).

    oracle.py run --out DIR --frames N [--screens LIST] [--input FILE] [--dump SPEC]... [--time TIME]
                  [--version b2_us] [--rom PATH] [--oracle BIN] [--save FILE] [--save-out FILE] [--no-counter]
    oracle.py port --binary BIN --out DIR --frames LIST [--input FILE] [--dump SPEC]... [--time TIME] [--version b2_us]
                   [--rom PATH] [--save FILE] [--save-out FILE] [--launch SPEC]
    oracle.py save NAME... [--out FILE] [--save-cache DIR] [--oracle BIN] [--version b2_us] [--work DIR]
    oracle.py port-save --port BIN [--oracle BIN] [--version b2_us] [--work DIR]
    oracle.py boot --out DIR [--port BIN] [--frames LIST] [--version b2_us]
    oracle.py sequence NAME --port BIN [--out DIR] [--update] [--save-cache DIR] [--oracle BIN] [--version b2_us]
    oracle.py sequence-check NAME --port BIN [--work DIR] [--version b2_us]
    oracle.py sound NAME --port BIN [--out DIR] [--update] [--oracle BIN] [--version b2_us]
    oracle.py sound-check NAME --port BIN [--work DIR] [--version b2_us]
    oracle.py launch NAME --port BIN [--out DIR] [--save-cache DIR] [--oracle BIN] [--version b2_us]
    oracle.py check --oracle BIN [--version b2_us] [--work DIR] [--update]

run calls pbw-oracle with the repository's defaults: the version's ROM in orig/, its symbols from config/, the
statics of the ROM build's link map (symbols.py), and GFL's frame count (sVBlankCount) as the counter for aligning
frames and for input files on game frames. It passes --time only when given one, so that pbw-oracle takes an input
file's time line, else its default. port runs the port's program once from power-on with a blank save, to the last
frame of LIST, and has it write its run directory (--run-dir): frames.tsv for every frame, a PNG file for each frame of
LIST, memory.txt with the --dump specs (the program's --run-dump: globals by name, with a size, * through a pointer).
Its clock is --time, else the input file's, else the default. Both take --save FILE, the save memory at power-on (the
port's copy of it is the save directory's file, named after the ROM's game code, as the port names its saves), and
--save-out FILE, the save memory when the run ends (the blank chip, 0xff, when the game never wrote one). save makes
saves the sequences start from on the oracle (SAVES: first_save, power-on through New Game into the field and the field
menu's SAVE; starter_save, first_save's CONTINUE through the story to the first Pokémon and SAVE; route19_save, the
starter save's CONTINUE through Hugh's battle to the grass of Route 19), each from the save it names as its start or a
blank one, and checks each step against the SHA-1 kept for it; it writes one save to FILE, or with --save-cache every
save into DIR as NAME_VERSION.sav, where a later run reuses a file only while its SHA-1 is still the one kept.
port-save takes a save the other way: the port's New Game writes one, and the oracle boots it to the start menu frame
for frame as it boots its own save of the same moment, without writing either, and the console's Wi-Fi ID that New
Game copied into it is the oracle's. boot does both for the boot sequence and compares them on the game's count, the
movies by their pictures (compare.py), at the DS's 6 bits and at the palettes' 5.
sequence does both on a comparison sequence's input file, port/tests/oracle/<name>_<version>.pbwinput (SEQUENCES:
opening, power-on to the title with nothing pressed; title, the opening skipped to the title and START to the start
menu; continue, the same from first_save's save, the start menu's CONTINUE and the field it goes to; continue_starter,
continue's presses from starter_save's, whose start menu shows the Pokédex), from the save the sequence names, made
first on the oracle (or taken from --save-cache), and compares every frame the same way but the known differences, up
to the frame where the port stops when the sequence allows it to stop on code with no C yet; with --update it writes
port/tests/oracle/<name>_<version>.tsv, the oracle's hashes at sampled frames. sequence-check runs the port alone on
that input file and checks its counts and its screens without 3D against that reference. sound runs both programs
on a sequence with their sound traces and WAV dumps (--sound-trace, --audio-dump) and compares the sound driver's state
run by run, what the ARM9 sees of it frame by frame, and the sound by segment (sound.py, SOUND); with --update it
writes port/tests/oracle/sound_<name>_<version>.tsv, the oracle's hashes and statistics. sound-check runs the port
alone and checks it against that reference. launch checks the field menu's apps on Frigate's test launcher (LAUNCHES,
README.md's "The launch checks"): the oracle reaches each app through the field with an input file,
port/tests/oracle/launch_<name>_<version>.pbwinput, from a save; the port plays the same file from the same save with
--launch, which opens each app at the oracle's count in place of the field (ADR 0012); every app screen of
port/tests/oracle/launch_<name>_<version>.screens.tsv, the oracle's hashes, must show on the port at the oracle's
count, every pass of each app's span a picture the oracle shows at that count, the launcher's reports must give the
event types declared, and the save must stay as it was. check is the oracle's ctest test: the oracle runs the boot
sequence twice with the same results (its screens, sound trace and WAV dump), and its screens match the hashes in
port/tests/oracle/boot_<version>.tsv. save, port-save, sequence, sound, launch and check exit 77 without the oracle or
the ROM, sequence-check and sound-check without the ROM.

Standard library only.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import compare  # noqa: E402
import sound  # noqa: E402
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
    # The sequence whose input file it plays, when it isn't its own (port/tests/oracle/<input>_<version>.pbwinput)
    input: str = ""
    # The port frame from which the port may stop on a trap stub (TRAP_STATUS), the trap it may stop on (a regular
    # expression its message must match: the function or data with no C yet), and why. The frames it ran are compared,
    # and the run passes if it got that far and stopped on that trap; another trap is a failure, so the sequence says
    # which gap it stops on and is updated when the decomp fills it
    stop_from: int = 0
    stop_trap: str = ""
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
CURSOR = ("the start menu's cursor pulses a step ahead on the port since the title's cry (the lag of -1), and "
          "CONTINUE's press, which both read on the same pass, ends the pulse on the same count: on the press's frame "
          "the port shows the step after the DS's last one, a step the DS shows elsewhere in the pulse (at counts "
          "3274, 3296, 3338 and 3360 from the first save) but not on that count")
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
        stop_trap=r"\bFIELDMAP_PROC_FUNCS\b.*\(ov036, ",
        stop="the field after CONTINUE, whose process table, overlay 36's FIELDMAP_PROC_FUNCS, has no C yet",
    ),
    # continue's presses from starter_save's save: the start menu's saved game now has the Pokédex line (Pokédex: 1,
    # time 0:04), and CONTINUE goes to the Aspertia City lookout, where the port stops on the same trap
    "continue_starter": Sequence(
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
        save="starter_save",
        input="continue",
        stop_from=3425,
        stop_trap=r"\bFIELDMAP_PROC_FUNCS\b.*\(ov036, ",
        stop="the field after CONTINUE, whose process table, overlay 36's FIELDMAP_PROC_FUNCS, has no C yet",
    ),
}
REFERENCE_COLUMNS = ["port", "oracle", "counter", "lag", "top", "bottom"]
# The exit status of a port run that reached a function or data with no C yet (port/hal/trap.c)
TRAP_STATUS = 70
# The save memory, BW2's 4 Mbit flash (port/hal/card/port_card.h), as both programs write it
BACKUP_SIZE = 0x80000


@dataclass(frozen=True)
class SaveMaker:
    """A save made on the oracle, from power-on, by the input file port/tests/oracle/<name>_<version>.pbwinput
    (SEQUENCE_INPUT's place): frames, how many frames the oracle runs; sha1, by version, the SHA-1 of the save memory
    it writes at the end, which is the same at every run (the oracle is deterministic), so that what the sequences
    start from is the save that was examined (README.md, "Saves"); start, the SAVES name of the save the oracle starts
    from, made (or taken from the cache) and checked first, else a blank backup. A version without a SHA-1 has no such
    save"""
    frames: int
    sha1: dict[str, str] = field(default_factory=dict)
    start: str = ""


SAVES = {
    # New Game as new_game.pbwinput plays it (the player AAAAAAA, the rival HughAAA), into the field in front of the
    # player's house, through the first scripts to the player's room, then the field menu's SAVE: the adventure's
    # first save
    "first_save": SaveMaker(18800, {"b2_us": "a7a58889e2399d1f6a0b4c0f5600748dd139449e",
                                    "w2_us": "479529f98b29cdd852502360aeb33531ca111d7f"}),
    # first_save's CONTINUE, then the story from the player's room to the Aspertia City lookout: Mom's and Hugh's
    # scenes, Bianca's two talks, the middle ball (Tepig, level 5), NO to the nickname, the Pokédex, and the field
    # menu's SAVE. Event flags 0x961 (a Pokémon) and 0x962 (the Pokédex) are set, so the field menu has every item
    "starter_save": SaveMaker(17000, {"b2_us": "ce71fe0b8c1d93436d62678400d1e0722632b67e"}, start="first_save"),
    # starter_save's CONTINUE, Hugh's trainer battle (Tepig grows to level 6), the Pokémon Center's scene, Mom's, the
    # gate's gift and Bianca's catching lesson, then SAVE beside Route 19's grass: for battles, which no test runs yet
    "route19_save": SaveMaker(28600, {"b2_us": "826bc561a328d1943d6435b9bdae7f0abc6fd3e8"}, start="starter_save"),
}

# How each sequence's sound is compared (sound.py, README.md "Sound"): its segments of port frames, each with the range
# of lags (oracle frame = port frame + lag) where the oracle's sound may be, exact where the oracle's samples must be
# the port's to the bit (a stream, a cry: one channel's samples, which the ARM9 starts, at the channel's own timer);
# the count from which the plays aren't compared (a press reaches the two programs at different points of the
# opening's music, which the opening sequence compares whole); and by version the port frames whose status differs by
# construction
LOADING = ("the DS's passes at counts 1111 and 1113 take 2 and 46 frames while it loads the title, so on the count the "
           "port has locked the stream's channels and started it before the DS (until then their registers are what "
           "the opening's music, stopped at a different point on each, left)")
SOUND_CRY = "the DS starts Kyurem's cry a pass later (README.md, Known differences)"
SOUND_CRY_PLAYS = ("Kyurem's cry, which the DS starts later against its passes, ends later too: as this frame ends, "
                   "the DS's channel still plays it and the port's has stopped (README.md, Known differences)")
SOUND_CRY_END = ("the title sees Kyurem's cry end a pass later on the DS (README.md, Known differences), and unlocks "
                 "its channel a pass later")
SOUND = {
    "opening": sound.SoundSpec(
        segments=(
            sound.Segment("logo", 355, 880, 200, 400),
            sound.Segment("opening", 1017, 3495, 200, 400),
            sound.Segment("title", 3530, 3790, 250, 450, exact=True),
        ),
    ),
    "title": sound.SoundSpec(
        before_count=1100,
        segments=(
            sound.Segment("logo", 355, 880, 200, 400),
            sound.Segment("title", 1116, 3099, 1750, 1950, exact=True),
            sound.Segment("cry", 3104, 3210, 1750, 1950, exact=True),
        ),
        known={"b2_us": {"1112-1114": LOADING, "3101": SOUND_CRY},
               "w2_us": {"1112-1114": LOADING, "3101": SOUND_CRY, "3214": SOUND_CRY_PLAYS, "3216": SOUND_CRY_END}},
    ),
}
SOUND_REFERENCE = os.path.join("port", "tests", "oracle", "sound_{name}_{version}.tsv")


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
               quiet: bool = False, audio: str | None = None, trace: str | None = None) -> int:
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
    if audio:
        command += ["--audio-dump", audio]
    if trace:
        command += ["--sound-trace", trace]
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
             save: str | None = None, save_out: str | None = None, audio: str | None = None,
             trace: str | None = None, launch: str | None = None,
             log: list[str] | None = None) -> tuple[int | None, str]:
    """Runs the port's program once, from power-on with a blank save or with the save memory in the file save, for
    frames frames, writing its run directory in out (--run-dir): frames.tsv for every frame it ran, a PNG file for each
    frame of screens (a frame list, "none" or "all") and memory.txt with the dumps (--run-dump specs), and with audio
    and trace its WAV dump and sound trace (--audio-dump, --sound-trace). The save directory is a new one, given the
    save as the port names its saves (the game code and .sav); save_out gets the save memory the run left there, which
    is the blank chip when the game neither had a save nor wrote one, never a file of an earlier run. launch is
    Frigate's test launcher's --launch, if any. Returns its exit status (None when it timed out) and its last message
    about why it stopped; log, if given, gets every line it wrote"""
    os.makedirs(out, exist_ok=True)
    if save_out and os.path.exists(save_out):
        os.remove(save_out)
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
    if audio:
        command += ["--audio-dump", audio]
    if trace:
        command += ["--sound-trace", trace]
    if launch:
        command += ["--launch", launch]
    try:
        done = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        status = done.returncode
        output = done.stderr + done.stdout
    except subprocess.TimeoutExpired:
        status = None
        output = "timed out"
    finally:
        if save_file and save_out:
            if os.path.exists(save_file):
                shutil.copyfile(save_file, save_out)
            else:
                # The port writes its file only once the game writes the chip: until then the chip is erased
                with open(save_out, "wb") as f:
                    f.write(b"\xff" * BACKUP_SIZE)
                print(f"port: the run left no save file, so {save_out} is the blank chip ({BACKUP_SIZE} bytes of 0xff)")
        shutil.rmtree(saves, ignore_errors=True)
    # Its own lines start with "port: ", after the time and process that SDL_Log puts first on some hosts
    reasons = [line[line.index("port: "):].strip() for line in output.splitlines()
               if "port: " in line and "entering" not in line and "port: rom:" not in line]
    if log is not None:
        log[:] = output.splitlines()
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
                               port_time(args), args.input, dumps=args.dump, save=args.save, save_out=args.save_out,
                               launch=args.launch)
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


def save_chain(name: str) -> list[str]:
    """The saves a save is made from, the first made first, ending with it; a ValueError for a start SAVES doesn't
    have or a chain that comes back on itself"""
    chain = []
    while name:
        if name in chain:
            raise ValueError(f"the saves {' -> '.join(reversed(chain + [name]))} start from each other")
        if name not in SAVES:
            raise ValueError(f"no save {name} in SAVES")
        chain.append(name)
        name = SAVES[name].start
    return list(reversed(chain))


def save_kept(name: str, version: str) -> str | None:
    """None when every save of the chain has a SHA-1 for the version, else which hasn't"""
    for link in save_chain(name):
        if version not in SAVES[link].sha1:
            return f"SAVES keeps no SHA-1 of {link} for {version}"
    return None


def cached_save(cache: str, name: str, version: str) -> str:
    return os.path.join(cache, f"{name}_{version}.sav")


def make_save(args, name: str, out: str, work: str, cache: str | None = None) -> str | None:
    """Makes the save SAVES[name] on the oracle (args.oracle, args.version, args.rom), writing the save memory to out,
    with the oracle's runs in work. A save that starts from another is made from it, the start made first (or taken
    from cache, a directory of NAME_VERSION.sav files, while its SHA-1 is the one kept), each step checked. Returns
    None, or why the save isn't the one kept: the oracle failed, or a SHA-1 isn't the one SAVES has for the version
    (which it reports, for a version that has none yet)"""
    start = None
    if SAVES[name].start:
        start, reason = obtain_save(args, SAVES[name].start, work, cache)
        if reason:
            return f"{SAVES[name].start}, which {name} starts from: {reason}"
    return run_save_maker(args, name, out, os.path.join(work, name), start)


def obtain_save(args, name: str, work: str, cache: str | None) -> tuple[str | None, str | None]:
    """A save SAVES has, as a file: cache's, if it holds it with the SHA-1 kept, else made (into cache, if given, else
    into work). Returns its path, or None and why it couldn't be made"""
    wanted = SAVES[name].sha1.get(args.version)
    if cache:
        path = cached_save(cache, name, args.version)
        if wanted and os.path.exists(path) and file_sha1(path) == wanted:
            print(f"{name}: reused {path}, SHA-1 {wanted}")
            return path, None
        os.makedirs(cache, exist_ok=True)
    else:
        path = os.path.join(work, f"{name}.sav")
    reason = make_save(args, name, path, work, cache)
    if reason:
        if os.path.exists(path):
            os.remove(path)
        return None, reason
    return path, None


def run_save_maker(args, name: str, out: str, work: str, start: str | None) -> str | None:
    """One step of a chain: the oracle plays the save's input file from the start save (a blank backup without one)"""
    maker = SAVES[name]
    input_path = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=name, version=args.version))
    if not os.path.exists(input_path):
        return f"no input file {os.path.relpath(input_path, REPO_ROOT)} for {args.version}"
    made = argparse.Namespace(**vars(args))
    made.input = input_path
    made.save = start
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
    if not args.out and not args.save_cache:
        print("oracle.py save: name where the save goes, --out FILE or --save-cache DIR")
        return 2
    if args.out and len(args.names) != 1:
        print("oracle.py save: --out takes one save; --save-cache takes several")
        return 2
    args.oracle = binary
    work = args.work or tempfile.mkdtemp(prefix="oracle-save-")
    status = 0
    for name in args.names:
        # Into the cache, a save this version doesn't keep is left out with why, so that one list of saves serves both
        # versions; to --out it is made all the same, and its SHA-1 reported, for a version that has none yet
        kept = save_kept(name, args.version)
        if kept and not args.out:
            print(f"oracle.py save: {name}: left out: {kept}")
            continue
        if args.out:
            reason = make_save(args, name, args.out, work, args.save_cache)
            where = args.out
        else:
            where, reason = obtain_save(args, name, work, args.save_cache)
        if reason:
            print(f"oracle.py save: {name}: {reason}")
            status = 1
            continue
        print(f"{name}: {where}")
    if not args.work:
        shutil.rmtree(work, ignore_errors=True)
    return status


# The port's save on the oracle (README.md, "Saves"), the way from the port to melonDS. Both programs play first_save's
# input file, which starts with New Game as new_game.pbwinput plays it: the port past New Game's formatting of the
# blank backup, up to the field's first script, which has no C yet; the oracle to the same moment, since the DS reaches
# each screen later. Then the title sequence's input file boots each program's save on the oracle
PORT_SAVE_INPUT = "first_save"
PORT_SAVE_PORT_FRAMES = 10600
PORT_SAVE_ORACLE_FRAMES = 13000
PORT_SAVE_BOOT = "title"
PORT_SAVE_BOOT_FRAMES = 5400
# The console's pending Wi-Fi ID as New Game copies it into the save, which DWC makes from the clock, the MAC address
# and the DS's address of OS_GetTick (lib/dwc/src/bm/util_wifiidtool.c): the user ID of the login ID in DWC's user data
# (save block 30, at 0x1aa00), 8 bytes from 0x1aa04, in each copy of the main save (0x26000 apart). The rest of the two
# saves differs by when each program formatted the backup (README.md, "Saves")
WIFI_ID = ((0x1aa04, 8), (0x40a04, 8))


def is_blank(path: str) -> bool:
    with open(path, "rb") as f:
        return f.read() == b"\xff" * BACKUP_SIZE


def compare_boots(port_run: compare.Run, oracle_run: compare.Run) -> tuple[int, str | None]:
    """How many frames two oracle runs share, and the first where their counts or screens differ (None if none)"""
    if sorted(port_run.frames) != sorted(oracle_run.frames):
        return 0, f"the runs have {len(port_run.frames)} and {len(oracle_run.frames)} frames"
    for frame in sorted(port_run.frames):
        a = port_run.frames[frame]
        b = oracle_run.frames[frame]
        if (a.counter, a.top, a.bottom) != (b.counter, b.top, b.bottom):
            return frame - 1, (f"frame {frame}: count {a.counter} against {b.counter}, top {a.top[:8]} against "
                               f"{b.top[:8]}, bottom {a.bottom[:8]} against {b.bottom[:8]}")
    return len(port_run.frames), None


def command_port_save(args) -> int:
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py port-save: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    rom = rom_path(args.version, args.rom)
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py port-save: skipped: {reason}")
        return SKIP
    args.oracle = binary
    return port_save(args)


def port_save(args) -> int:
    """port-save's work, with the oracle and the ROM found: returns 0 when the port's save boots on the oracle as the
    oracle's own does, else 1"""
    rom = rom_path(args.version, args.rom)
    input_path = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=PORT_SAVE_INPUT, version=args.version))
    boot_input = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=PORT_SAVE_BOOT, version=args.version))
    for path in (input_path, boot_input):
        if not os.path.exists(path):
            print(f"oracle.py port-save: no input file {os.path.relpath(path, REPO_ROOT)} for {args.version}")
            return 1
    work = args.work or tempfile.mkdtemp(prefix="oracle-port-save-")
    saves = {"port": os.path.join(work, "port.sav"), "oracle": os.path.join(work, "oracle.sav")}

    # The port's save, which New Game's formatting wrote
    status, message = run_port(args.port, os.path.join(work, "port"), PORT_SAVE_PORT_FRAMES, "none", rom, args.time,
                               input_path, save_out=saves["port"])
    print(f"port: {describe_port_run(status, message, PORT_SAVE_PORT_FRAMES, os.path.join(work, 'port'))}")
    if status not in (0, TRAP_STATUS) or is_blank(saves["port"]):
        print("oracle.py port-save: the port's New Game left no save")
        return 1
    # The oracle's own save of the same moment
    made = argparse.Namespace(**vars(args))
    made.input = input_path
    made.save = None
    made.save_out = saves["oracle"]
    status = run_oracle(made, os.path.join(work, "oracle"), PORT_SAVE_ORACLE_FRAMES, "none", quiet=True)
    if status != 0 or not os.path.exists(saves["oracle"]):
        print(f"oracle.py port-save: the oracle failed (status {status})")
        return 1
    contents = {}
    for name, path in saves.items():
        with open(path, "rb") as f:
            contents[name] = f.read()
        print(f"{name}'s save: {len(contents[name])} bytes, SHA-1 {file_sha1(path)}")
    failed = False
    for offset, size in WIFI_ID:
        port_id = contents["port"][offset:offset + size]
        oracle_id = contents["oracle"][offset:offset + size]
        if port_id != oracle_id:
            print(f"the console's Wi-Fi ID at {offset:#x}: the port's {port_id.hex()}, the oracle's {oracle_id.hex()}")
            failed = True
    differ = sum(1 for a, b in zip(contents["port"], contents["oracle"]) if a != b)
    print(f"the saves differ in {differ} bytes; the console's Wi-Fi ID in them is "
          f"{'not ' if failed else ''}the same")

    # Each save booted on the oracle: the same frames, and neither save written
    runs = {}
    for name, path in saves.items():
        boot = argparse.Namespace(**vars(args))
        boot.input = boot_input
        boot.save = path
        boot.save_out = os.path.join(work, f"{name}-after.sav")
        out = os.path.join(work, f"boot-{name}")
        shutil.rmtree(out, ignore_errors=True)
        status = run_oracle(boot, out, PORT_SAVE_BOOT_FRAMES, "none", quiet=True)
        if status != 0 or not os.path.exists(boot.save_out):
            print(f"oracle.py port-save: the oracle failed on the {name}'s save (status {status})")
            return 1
        if file_sha1(boot.save_out) != file_sha1(path):
            print(f"the oracle wrote the {name}'s save while booting it")
            failed = True
        runs[name] = compare.load_run(out)
    same, difference = compare_boots(runs["port"], runs["oracle"])
    print(f"booted on the oracle: {same} of {len(runs['oracle'].frames)} frames the same (count and both screens)"
          + (f"; first difference at {difference}" if difference else ""))
    if difference or failed:
        print(f"oracle.py port-save: FAIL; the runs are in {work}")
        return 1
    if not args.work:
        shutil.rmtree(work, ignore_errors=True)
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


def check_sound_outputs(run: str) -> int:
    """The oracle's sound trace and WAV dump of the boot: a sample each 1024 cycles from power-on, and every periodic
    run of the sound driver, 2728 ticks of 64 cycles apart, in the records (a record covers the runs since the last,
    seen once the driver's thread waits again), each in the frame whose VBlank comes next. Returns the failures"""
    trace, audio = sound.run_files(run)
    failed = 0
    _, samples = sound.load_wav(audio)
    expected = CHECK_FRAMES * sound.CYCLES_PER_FRAME // sound.CYCLES_PER_SAMPLE
    if abs(len(samples) // 2 - expected) > 1:
        print(f"FAIL: the WAV dump has {len(samples) // 2} samples, not {expected}")
        failed += 1
    records = sound.load_trace(trace)
    periodic = [r for r in records if r.periods]
    runs = sum(r.periods for r in periodic[1:])
    elapsed = (periodic[-1].cycle - periodic[0].cycle) / (2728 * 64) if len(periodic) > 1 else 0
    if len(periodic) < CHECK_FRAMES * 2 or abs(runs - elapsed) > 2:
        print(f"FAIL: the sound trace has {runs} periodic runs in {elapsed:.1f} periods of the driver")
        failed += 1
    wrong = sound.misstamped(records)
    if wrong:
        print(f"FAIL: records of the sound trace not in the frame whose VBlank comes next: {len(wrong)} (the first: "
              f"record {wrong[0]}, cycle {records[wrong[0]].cycle}, frame {records[wrong[0]].frame})")
        failed += 1
    return failed


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
        os.makedirs(out)
        trace, audio = sound.run_files(out)
        status = run_oracle(args, out, CHECK_FRAMES, CHECK_SCREENS, quiet=True, audio=audio, trace=trace)
        if status != 0:
            print(f"oracle.py check: the oracle failed (status {status})")
            return 1
        runs.append(compare.load_run(out))
    failed = 0
    for file in ("frames.tsv", *(os.path.basename(path) for path in sound.run_files(work))):
        with open(os.path.join(work, "a", file), "rb") as a, open(os.path.join(work, "b", file), "rb") as b:
            if a.read() != b.read():
                print(f"FAIL: two runs of the same boot differ ({file})")
                failed += 1
    failed += check_sound_outputs(os.path.join(work, "a"))
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
    """The sequence's input file (its own, or the one of the sequence it names) and reference for the version, and the
    ROM"""
    played = SEQUENCES[args.name].input or args.name
    return (os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=played, version=args.version)),
            os.path.join(REPO_ROOT, SEQUENCE_REFERENCE.format(name=args.name, version=args.version)),
            rom_path(args.version, args.rom))


def stop_allowed(sequence: Sequence, status: int | None, last: int, message: str) -> bool:
    """Whether a port run that didn't end with status 0 ended as the sequence allows: on the trap stub it names (code
    or data with no C yet), its message matching stop_trap, after the frame from which the sequence allows it"""
    return (status == TRAP_STATUS and sequence.stop_from > 0 and last >= sequence.stop_from
            and bool(sequence.stop_trap) and re.search(sequence.stop_trap, message) is not None)


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
    reason = save_kept(sequence.save, args.version) if sequence.save else None
    if reason:
        print(f"oracle.py sequence: skipped: {reason}, so the oracle can't make the save the sequence starts from")
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
        args.save, reason = obtain_save(args, sequence.save, work, args.save_cache)
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
        if not stop_allowed(sequence, status, last, message):
            if status == TRAP_STATUS and sequence.stop_trap:
                print(f"oracle.py sequence: the port didn't stop where the sequence allows, on {sequence.stop_trap} "
                      f"from frame {sequence.stop_from}: update the sequence if the decomp moved the gap")
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


def sound_paths(args) -> tuple[str, str, str]:
    """The sequence's input file and sound reference for the version, and the ROM"""
    return (os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=args.name, version=args.version)),
            os.path.join(REPO_ROOT, SOUND_REFERENCE.format(name=args.name, version=args.version)),
            rom_path(args.version, args.rom))


def command_sound(args) -> int:
    sequence = SEQUENCES[args.name]
    spec = SOUND[args.name]
    input_path, reference, rom = sound_paths(args)
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py sound: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py sound: skipped: {reason}")
        return SKIP
    args.oracle = binary
    args.input = input_path
    work = args.out or tempfile.mkdtemp(prefix=f"oracle-sound-{args.name}-")
    oracle_dir = os.path.join(work, "oracle")
    port_dir = os.path.join(work, "port")
    shutil.rmtree(oracle_dir, ignore_errors=True)
    os.makedirs(oracle_dir)
    oracle_trace, oracle_wav = sound.run_files(oracle_dir)
    status = run_oracle(args, oracle_dir, sequence.oracle_frames, "none", quiet=True, audio=oracle_wav,
                        trace=oracle_trace)
    if status != 0:
        print(f"oracle.py sound: the oracle failed (status {status})")
        return 1
    os.makedirs(port_dir, exist_ok=True)
    port_trace, port_wav = sound.run_files(port_dir)
    status, message = run_port(args.port, port_dir, sequence.port_frames, "none", rom, args.time, input_path,
                               audio=port_wav, trace=port_trace)
    print(f"port: {describe_port_run(status, message, sequence.port_frames, port_dir)}")
    if status != 0:
        return 1
    comparison = sound.compare_runs(oracle_dir, port_dir, spec, args.version, sequence.clock,
                                    sequence.lags.get(args.version))
    for failure in comparison.failures:
        print(f"FAIL: {failure}")
    result = 1 if comparison.failures else 0
    if args.update:
        if result != 0:
            print("oracle.py sound: the comparison failed; the reference stays as it was")
        else:
            melonds = compare.load_run(oracle_dir).info.get("melonds", "?")
            description = (
                f"pbw-oracle's sound on {SEQUENCE_INPUT.format(name=args.name, version=args.version)} (melonDS "
                f"{melonds}),\nat the port's frames and runs that oracle.py sound {args.name} pairs them with: the "
                "oracle's values only,\nhashes and statistics, never sound. limit: the game's count from which the "
                "plays aren't compared;\nplays: how many plays each player starts before it; window: a play's runs "
                "that the port must have, and\nthe first run left out at the limit (- for the whole play, so that it "
                "must end where the oracle's\ndoes); play: a player's play, by the periodic runs since its start, the "
                "SHA-1 of the oracle's\nstates of those runs (sound.py's player_state); status: what the ARM9 sees of "
                "the driver as\nthe port frame ends, at the oracle frame it pairs with "
                "on the count; segment: a segment's lag (oracle\nframe = port frame + lag) and the oracle's RMS, "
                "peak and samples at full scale; envelope: the oracle's\nRMS by blocks of 4 frames; probe and exact: "
                "the SHA-1 of the oracle's samples at the loudest point and\nover the segment, at the port's samples "
                f"plus the offset. Written by oracle.py sound {args.name} --update;\nchecked by oracle.py sound-check "
                f"{args.name}.")
            rows = sound.write_reference(reference, comparison, oracle_dir, port_dir, spec, args.version, description)
            print(f"wrote {rows} rows to {os.path.relpath(reference, REPO_ROOT)}")
    if not args.out:
        if result == 0:
            shutil.rmtree(work, ignore_errors=True)
        else:
            print(f"the runs are in {work}")
    return result


def command_sound_check(args) -> int:
    sequence = SEQUENCES[args.name]
    input_path, reference, rom = sound_paths(args)
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py sound-check: skipped: {reason}")
        return SKIP
    if not os.path.exists(reference):
        print(f"oracle.py sound-check: no reference {os.path.relpath(reference, REPO_ROOT)} for {args.version}")
        return 1
    work = args.work or tempfile.mkdtemp(prefix=f"oracle-sound-{args.name}-check-")
    os.makedirs(work, exist_ok=True)
    trace, audio = sound.run_files(work)
    status, message = run_port(args.port, work, sequence.port_frames, "none", rom, args.time, input_path,
                               audio=audio, trace=trace)
    print(f"port: {describe_port_run(status, message, sequence.port_frames, work)}")
    if status != 0:
        return 1
    failures, counts = sound.check_reference(reference, work)
    for line in failures:
        print(f"FAIL: {line}")
    print(f"{counts['runs']} runs of the driver's state, {counts['frames']} frames of its status, {counts['segments']} "
          f"segments of sound ({counts['blocks']} blocks of envelope, {counts['exact']} to the bit) checked: "
          f"{len(failures)} failures")
    if not args.work:
        if failures:
            print(f"the port's run is in {work}")
        else:
            shutil.rmtree(work, ignore_errors=True)
    return 1 if failures else 0


# Frigate's test launcher's checks (README.md, "The launch checks"; ADR 0012). The oracle opens the field menu's apps
# from the field; the port, whose field (overlay 36) has no C, plays the same input file with --launch, which opens each
# app at the count it opens on the oracle. A route's table keeps the oracle's hashes of its labelled screens; its app
# rows are the ones the port must show
LAUNCH_INPUT = os.path.join("port", "tests", "oracle", "launch_{name}_{version}.pbwinput")
LAUNCH_SCREENS = os.path.join("port", "tests", "oracle", "launch_{name}_{version}.screens.tsv")
LAUNCH_COLUMNS = ["label", "oracle_frame", "count", "game_frame", "top_3d", "bottom_3d", "top", "bottom", "where"]
# A launcher's report: "port: --launch: bag: opened at count 4044, closed at count 4326: result 0, event type 0"
LAUNCH_REPORT = re.compile(r"port: --launch: (\w+)(?:, then [\w ,]+)?: opened at count (\d+), closed at count (\d+): "
                           r"result (\d+), event type (\d+)")
LAUNCH_STEPS_ASIDE = "--launch steps aside"


@dataclass(frozen=True)
class Launch:
    """How a launch route is run and checked. save: the SAVES name both programs start from. oracle_frames and
    port_frames: how far each runs. spec: by version, the port's --launch, each app at the count where the oracle's
    first frame of it shows less the passes the port takes from opening an app to its first frame (README.md says how
    they were measured). events: the event type the launcher must report for each app, in order, the one the oracle's
    field menu acts on (0 when it shows the menu again). spans: by version, the counts from each app's first frame to
    the black after it closes, on the oracle: at each of them every picture the port shows must be one the oracle shows
    at that count (screens without 3D by their hashes). slack: how many counts away from the oracle's a row may show on
    the port. known: by version, app rows whose screens differ by construction, with why; known_spans, the counts of
    spans that do"""
    save: str
    oracle_frames: int
    port_frames: int
    spec: dict[str, str]
    events: tuple[int, ...]
    spans: dict[str, str] = field(default_factory=dict)
    slack: int = 0
    known: dict[str, dict[str, str]] = field(default_factory=dict)
    known_spans: dict[str, dict[str, str]] = field(default_factory=dict)


# The known differences of the launch routes
SUMMARY_SCROLL = ("the summary scrolls its background a pixel every 4 VBlanks since it started (PStatus_Main, "
                  "src/ov207/p_sta_sys.c:263-271), and the DS spends some 37 VBlanks more than passes loading it "
                  "(counts 4526-4531, 4530 alone 30 frames): its background is about 9 pixels further on throughout")
POKEDEX_TRAP = ("the Pokédex stops the port on NNSi_objSizeWTbl (ARM9 main, 0x020942b0), NitroSystem G2D's table of "
                "object widths, which has no C yet: the launcher doesn't open it, so its rows have no counterpart")
LAUNCHES = {
    # From the first save (no Pokémon, no Pokédex): X opens the field menu, A on BAG opens the bag, its items pocket
    # empty, B closes it; RIGHT and A open the trainer card, B closes it; B closes the menu. The bag's first frame shows
    # at count 4046 on the oracle and two passes after its opening on the port, the card's at 4588 and four passes
    # after, in both versions
    "field_menus": Launch("first_save", 7400, 5000, {"b2_us": "bag@4044,card@4584", "w2_us": "bag@4044,card@4584"},
                          (0, 0), spans={"b2_us": "4046-4323,4588-4930", "w2_us": "4046-4323,4588-4930"}),
    # From the starter save (Tepig, the Pokédex): the party list, Tepig's menu, its summary and back, the Pokédex and
    # Tepig's entry, the trainer card with its Pokédex line, and the bag. The party list's first frame shows at count
    # 4046 on the oracle (two passes after its opening on the port), the card's at 6978 (four), the bag's at 7675 (two);
    # the Pokédex isn't launched (POKEDEX_TRAP), so the port shows black meanwhile
    "party_menus": Launch("starter_save", 11100, 8400, {"b2_us": "party@4044,card@6974,bag@7673"}, (0, 0, 0),
                          spans={"b2_us": "4046-5123,6978-7330,7675-8023"},
                          known={"b2_us": {"summary": SUMMARY_SCROLL, "pokedex": POKEDEX_TRAP,
                                           "pokedex_entry": POKEDEX_TRAP, "pokedex_after_entry": POKEDEX_TRAP}},
                          known_spans={"b2_us": {"4533-4821": SUMMARY_SCROLL}}),
}


@dataclass(frozen=True)
class LaunchRow:
    label: str
    oracle_frame: int
    count: int
    top_3d: bool
    bottom_3d: bool
    top: str
    bottom: str
    app: bool


def load_launch_rows(path: str) -> list[LaunchRow]:
    """A route's table: after comment lines, a header (LAUNCH_COLUMNS) and a row per labelled screen"""
    rows = []
    header = None
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            fields = line.split("\t")
            if header is None:
                if fields != LAUNCH_COLUMNS:
                    raise ValueError(f"{path}: the header isn't {' '.join(LAUNCH_COLUMNS)}")
                header = fields
                continue
            if len(fields) != len(LAUNCH_COLUMNS) or fields[8] not in ("app", "field"):
                raise ValueError(f"{path}: a row isn't {len(LAUNCH_COLUMNS)} columns ending with app or field: {line}")
            rows.append(LaunchRow(fields[0], int(fields[1]), int(fields[2]), fields[4] == "1", fields[5] == "1",
                                  fields[6], fields[7], fields[8] == "app"))
    return rows


def launch_paths(args) -> tuple[str, str]:
    return (os.path.join(REPO_ROOT, LAUNCH_INPUT.format(name=args.name, version=args.version)),
            os.path.join(REPO_ROOT, LAUNCH_SCREENS.format(name=args.name, version=args.version)))


def check_oracle_rows(rows: list[LaunchRow], run: compare.Run) -> list[str]:
    """Every row is the oracle's own: its frame is at its count and shows its hashes"""
    failures = []
    for row in rows:
        frame = run.frames.get(row.oracle_frame)
        if frame is None:
            failures.append(f"{row.label}: the oracle has no frame {row.oracle_frame}")
        elif (frame.counter, frame.top, frame.bottom) != (row.count, row.top, row.bottom):
            failures.append(f"{row.label}: oracle frame {row.oracle_frame} isn't the row's (count {frame.counter}, "
                            f"top {frame.top[:8]}, bottom {frame.bottom[:8]})")
    return failures


def port_frames_at(run: compare.Run, count: int, slack: int) -> list[int]:
    return [number for number, frame in sorted(run.frames.items())
            if frame.counter is not None and abs(frame.counter - count) <= slack]


def match_app_row(row: LaunchRow, run: compare.Run, pictures: compare.Run | None, slack: int, after: int,
                  oracle_png: str | None) -> tuple[int | None, str]:
    """The first port frame after after, within the row's counts, that shows the row's screens: by hash for a screen
    without 3D, within compare.py's tolerance for one with 3D (its picture, from pictures, the run that wrote them,
    against the oracle's). Returns the frame, or None and why"""
    candidates = [number for number in port_frames_at(run, row.count, slack) if number > after]
    if not candidates:
        return None, f"no port frame at counts {row.count - slack}-{row.count + slack} after frame {after}"
    seen = []
    for number in candidates:
        frame = run.frames[number]
        ok = True
        for name, three_d, wanted, have in (("top", row.top_3d, row.top, frame.top),
                                             ("bottom", row.bottom_3d, row.bottom, frame.bottom)):
            if wanted == have:
                continue
            if not three_d:
                ok = False
                break
            port_png = pictures.png(number) if pictures is not None else None
            if port_png is None or oracle_png is None:
                ok = False
                break
            screen = 0 if name == "top" else 1
            oracle_rgb = compare.read_png(oracle_png)[2]
            port_rgb = compare.read_png(port_png)[2]
            size = compare.SCREEN_PIXELS * 3
            _, beyond = compare.compare_screen(oracle_rgb[screen * size:(screen + 1) * size],
                                               port_rgb[screen * size:(screen + 1) * size],
                                               compare.DEFAULT_PIXEL_TOLERANCE)
            if len(beyond) > compare.DEFAULT_TOLERANCE * compare.SCREEN_PIXELS:
                ok = False
                break
        if ok:
            return number, ""
        seen.append(f"{number} (count {frame.counter}, top {frame.top[:8]}, bottom {frame.bottom[:8]})")
    return None, f"the port's frames at its counts show other screens: {', '.join(seen[:4])}"


def check_spans(oracle_run: compare.Run, port_run: compare.Run, spans: str, known: set[int]) -> tuple[list[str], int]:
    """At each count of the spans but the known ones, every picture the port shows is one the oracle shows at that
    count, a screen with 3D in the oracle's frame left out, and the port shows at least one: the failures, and how many
    counts were checked"""
    def by_count(run: compare.Run) -> dict[int, list[compare.Frame]]:
        frames: dict[int, list[compare.Frame]] = {}
        for number in sorted(run.frames):
            frame = run.frames[number]
            if frame.counter is not None:
                frames.setdefault(frame.counter, []).append(frame)
        return frames

    oracle, port = by_count(oracle_run), by_count(port_run)
    failures = []
    checked = 0
    for count in compare.parse_frames(spans) or []:
        if count in known:
            continue
        checked += 1
        if not port.get(count):
            failures.append(f"count {count}: the port shows no frame at it")
            continue
        for frame in port[count]:
            if not any((theirs.top_3d or theirs.top == frame.top)
                       and (theirs.bottom_3d or theirs.bottom == frame.bottom) for theirs in oracle.get(count, [])):
                failures.append(f"count {count}: port frame {frame.frame} (top {frame.top[:8]}, bottom "
                                f"{frame.bottom[:8]}) is no picture the oracle shows at that count")
    return failures, checked


def check_launch_reports(lines: list[str], launch: Launch) -> list[str]:
    """The launcher's lines: one report per app with the event type declared, and no step aside"""
    failures = []
    if any(LAUNCH_STEPS_ASIDE in line for line in lines):
        failures.append("the launcher stepped aside: overlay 36's FIELDMAP_PROC_FUNCS has C, so retire the launcher "
                        "and turn this route into a sequence (ADR 0012)")
    reports = [match for match in (LAUNCH_REPORT.search(line) for line in lines) if match]
    events = tuple(int(match.group(5)) for match in reports)
    for match in reports:
        print(f"port: {match.group(0)[len('port: '):]}")
    if events != launch.events:
        failures.append(f"the launcher reported event types {events}, not {launch.events}")
    return failures


def command_launch(args) -> int:
    launch = LAUNCHES[args.name]
    input_path, screens_path = launch_paths(args)
    rom = rom_path(args.version, args.rom)
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py launch: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    reason = rom_matches(args.version, rom)
    if reason:
        print(f"oracle.py launch: skipped: {reason}")
        return SKIP
    reason = save_kept(launch.save, args.version)
    if reason is None and args.version not in launch.spec:
        reason = f"the route has no --launch for {args.version}"
    if reason is None and not (os.path.exists(input_path) and os.path.exists(screens_path)):
        reason = f"no input file or table for {args.version}"
    if reason:
        print(f"oracle.py launch: skipped: {reason}")
        return SKIP
    args.oracle = binary
    rows = load_launch_rows(screens_path)
    known = launch.known.get(args.version, {})
    work = args.out or tempfile.mkdtemp(prefix=f"oracle-launch-{args.name}-")
    oracle_dir = os.path.join(work, "oracle")
    port_dir = os.path.join(work, "port")
    shutil.rmtree(oracle_dir, ignore_errors=True)
    save, reason = obtain_save(args, launch.save, work, args.save_cache)
    if reason:
        print(f"oracle.py launch: {launch.save}: {reason}")
        return 1
    failures = []

    # The oracle on the route, with pictures of the rows with 3D only
    args.input = input_path
    args.save = save
    pictures = ",".join(str(row.oracle_frame) for row in rows if row.app and (row.top_3d or row.bottom_3d)) or "none"
    status = run_oracle(args, oracle_dir, launch.oracle_frames, pictures, quiet=True)
    if status != 0:
        print(f"oracle.py launch: the oracle failed (status {status})")
        return 1
    oracle_run = compare.load_run(oracle_dir)
    failures += check_oracle_rows(rows, oracle_run)

    # The port: once for every frame's hashes, again for pictures of the frames where a row with 3D may show
    lines: list[str] = []
    save_after = os.path.join(work, "port_save_after.sav")
    status, message = run_port(args.port, port_dir, launch.port_frames, "none", rom, args.time, input_path, save=save,
                               save_out=save_after, launch=launch.spec[args.version], log=lines)
    port_run = compare.load_run(port_dir)
    print(f"port: {describe_port_run(status, message, launch.port_frames, port_dir)}")
    if status != 0:
        failures.append(f"the port's run ended with status {status}: {message}")
    three_d = [row for row in rows if row.app and (row.top_3d or row.bottom_3d) and row.label not in known]
    pictures = None
    if three_d and status == 0:
        # The port is deterministic, so the second run's frames are the first's
        frames = sorted({number for row in three_d for number in port_frames_at(port_run, row.count, launch.slack)})
        if frames:
            pictures_dir = os.path.join(work, "port_pictures")
            status, message = run_port(args.port, pictures_dir, max(frames), ",".join(str(f) for f in frames), rom,
                                       args.time, input_path, save=save, launch=launch.spec[args.version])
            pictures = compare.load_run(pictures_dir)
    failures += check_launch_reports(lines, launch)
    if file_sha1(save_after) != file_sha1(save):
        failures.append("the port's run changed the save")

    # Each app row, in order, at its count
    after = 0
    matched = 0
    for row in rows:
        if not row.app:
            continue
        if row.label in known:
            print(f"left out, {row.label}: {known[row.label]}")
            continue
        number, why = match_app_row(row, port_run, pictures, launch.slack, after, oracle_run.png(row.oracle_frame))
        if number is None:
            failures.append(f"{row.label} (oracle frame {row.oracle_frame}, count {row.count}): {why}")
            continue
        after = number
        matched += 1
        print(f"{row.label}: oracle frame {row.oracle_frame}, port frame {number}, count {row.count}")
    apps = sum(1 for row in rows if row.app and row.label not in known)

    # Every pass of each app's span
    known_spans = launch.known_spans.get(args.version, {})
    span_failures, checked = check_spans(oracle_run, port_run, launch.spans.get(args.version, ""),
                                         set(by_frame(known_spans)))
    for counts, why in known_spans.items():
        print(f"left out, counts {counts}: {why}")
    failures += span_failures[:20]
    if len(span_failures) > 20:
        failures.append(f"and {len(span_failures) - 20} more counts")
    for line in failures:
        print(f"FAIL: {line}")
    failed_counts = len({line.split(":")[0] for line in span_failures})
    print(f"{matched} of {apps} app screens shown on the port at the oracle's counts; {checked - failed_counts} of "
          f"{checked} counts of the apps' spans the oracle's; {len(rows)} rows of the oracle's held to its run")
    if not args.out:
        if not failures:
            shutil.rmtree(work, ignore_errors=True)
        else:
            print(f"the runs are in {work}")
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
    port.add_argument("--binary", required=True, help="the port's program (frigate-black2 or frigate-white2)")
    port.add_argument("--out", required=True)
    port.add_argument("--frames", required=True, help="the frames to write as PNG; the run goes to the last")
    port.add_argument("--input")
    port.add_argument("--dump", action="append", help="memory to dump, as the program's --run-dump takes it: "
                      "[LABEL=][*]SYMBOL[+OFFSET]:SIZE[@LIST], a global by name")
    port.add_argument("--save", help="the save memory at power-on (blank otherwise), a .sav as melonDS writes it")
    port.add_argument("--save-out", help="where to copy the save memory the run leaves")
    port.add_argument("--launch", help="Frigate's test launcher: the field menu's apps to open after CONTINUE, "
                      "APP[@COUNT],... (ADR 0012)")

    save = commands.add_parser("save", help="make saves the sequences start from, on the oracle")
    save.add_argument("names", nargs="+", choices=sorted(SAVES), metavar="NAME")
    common(save)
    save.add_argument("--out", help="where to write the save memory (one save)")
    save.add_argument("--save-cache", help="make or reuse each save, and the saves it starts from, in this directory")
    save.add_argument("--work", help="keep the oracle's runs here")

    port_save = commands.add_parser("port-save", help="the port's New Game save booted on the oracle, against the "
                                    "oracle's own save of that moment")
    common(port_save)
    port_save.add_argument("--port", required=True, help="the port's program")
    port_save.add_argument("--work", help="keep the runs and the saves here")

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
    sequence.add_argument("--save-cache", help="take the save the sequence starts from from this directory, or make "
                          "it there (oracle.py save --save-cache)")

    launch = commands.add_parser("launch", help="the field menu's apps on the oracle and on the port's test launcher")
    launch.add_argument("name", choices=sorted(LAUNCHES))
    common(launch)
    launch.add_argument("--port", required=True, help="the port's program")
    launch.add_argument("--out", help="keep the runs here")
    launch.add_argument("--save-cache", help="take the save the route starts from from this directory, or make it "
                        "there (oracle.py save --save-cache)")

    sequence_check = commands.add_parser("sequence-check", help="the port on a sequence against its reference")
    sequence_check.add_argument("name", choices=sorted(SEQUENCES))
    common(sequence_check, oracle=False)
    sequence_check.add_argument("--port", required=True, help="the port's program")
    sequence_check.add_argument("--work", help="keep the port's run here")

    sound_parser = commands.add_parser("sound", help="a sequence's sound on the oracle and the port, compared")
    sound_parser.add_argument("name", choices=sorted(SOUND))
    common(sound_parser)
    sound_parser.add_argument("--port", required=True, help="the port's program")
    sound_parser.add_argument("--out", help="keep the runs here")
    sound_parser.add_argument("--update", action="store_true",
                              help="write the sound reference from a comparison that passes")

    sound_check = commands.add_parser("sound-check", help="the port's sound on a sequence against its reference")
    sound_check.add_argument("name", choices=sorted(SOUND))
    common(sound_check, oracle=False)
    sound_check.add_argument("--port", required=True, help="the port's program")
    sound_check.add_argument("--work", help="keep the port's run here")

    args = parser.parse_args(argv)
    try:
        if args.command == "run":
            return command_run(args)
        if args.command == "port":
            return command_port(args)
        if args.command == "save":
            return command_save(args)
        if args.command == "port-save":
            return command_port_save(args)
        if args.command == "boot":
            return command_boot(args)
        if args.command == "sequence":
            return command_sequence(args)
        if args.command == "sequence-check":
            return command_sequence_check(args)
        if args.command == "sound":
            return command_sound(args)
        if args.command == "sound-check":
            return command_sound_check(args)
        if args.command == "launch":
            return command_launch(args)
        return command_check(args)
    except (compare.RunError, OSError, ValueError) as error:
        print(f"oracle.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
