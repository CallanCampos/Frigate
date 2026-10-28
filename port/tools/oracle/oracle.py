#!/usr/bin/env python3
"""Runs the melonDS oracle and Frigate's program alike, for compare.py and sound.py, makes the saves the tests start
from on the oracle, and compares the two programs on the comparison sequences, from power-on through the title and
CONTINUE into the field, its menu and the apps it opens, and through New Game into Aspertia City
(port/tools/oracle/README.md).

    oracle.py run --out DIR --frames N [--screens LIST] [--input FILE] [--dump SPEC]... [--time TIME]
                  [--version b2_us] [--rom PATH] [--oracle BIN] [--save FILE] [--save-out FILE] [--no-counter]
    oracle.py port --binary BIN --out DIR --frames LIST [--input FILE] [--dump SPEC]... [--time TIME] [--version b2_us]
                   [--rom PATH] [--save FILE] [--save-out FILE]
    oracle.py save NAME... [--out FILE] [--save-cache DIR] [--oracle BIN] [--version b2_us] [--work DIR]
    oracle.py port-save --port BIN [--oracle BIN] [--version b2_us] [--work DIR]
    oracle.py round-trip NAME --port BIN [--save-cache DIR] [--oracle BIN] [--version b2_us] [--work DIR]
    oracle.py boot --out DIR [--port BIN] [--frames LIST] [--version b2_us]
    oracle.py sequence NAME --port BIN [--out DIR] [--update] [--save-cache DIR] [--oracle BIN] [--version b2_us]
    oracle.py seeds NAME [--frames N] [--save-cache DIR] [--work DIR] [--oracle BIN] [--version b2_us]
    oracle.py sequence-check NAME --port BIN [--work DIR] [--version b2_us]
    oracle.py sound NAME --port BIN [--out DIR] [--update] [--oracle BIN] [--version b2_us]
    oracle.py sound-check NAME --port BIN [--work DIR] [--version b2_us]
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
menu's SAVE; starter_save, first_save's CONTINUE through the story to the first Pokémon and SAVE; center_save, the
starter save's CONTINUE through Hugh's battle to SAVE in the Pokémon Center; route19_save, from there to Bianca's
catching lesson and SAVE beside Route 19's grass; catch_save, from there to a wild Purrloin caught and SAVE), each from
the save it names as its start or a blank one, and checks each step against the SHA-1 kept for it; it writes one save to
FILE, or with --save-cache every save into DIR as NAME_VERSION.sav, where a later run reuses a file only while its SHA-1
is still the one kept.
port-save takes a save the other way: the port's New Game writes one, and the oracle boots it to the start menu frame
for frame as it boots its own save of the same moment, without writing either, and the console's Wi-Fi ID that New Game
copied into it is the oracle's. round-trip does it for a save of phase 4's route (ROUND_TRIPS: catch, a wild Purrloin
caught; hugh, Hugh's trainer battle to its end): the port plays the input file the oracle made the kept save with, from
the same start, or its own where its course past a cut would outlast those presses (ADR 0023), and saves, its run up to
the cut held to the oracle's as the cut's sequence holds it; the two saves differ only in the bytes the trip declares,
their mystery gift blocks compared decrypted; the oracle continues the port's save as it continues its own, frame for
frame, and the port continues it in a new run, compared with the oracle's continuing the same save as sequence
compares. boot does both for the boot sequence and compares them on the game's count, the movies by their pictures
(compare.py), at the DS's 6 bits and at the palettes' 5.
sequence does both on a comparison sequence's input file, port/tests/oracle/<name>_<version>.pbwinput (SEQUENCES:
opening, power-on to the title with nothing pressed; title, the opening skipped to the title and START to the start
menu; continue, the same from first_save's save, the start menu's CONTINUE and the field it goes to; continue_starter,
continue's presses from starter_save's, whose start menu shows the Pokédex; field_menus, continue's presses and then the
field menu's bag and trainer card from first_save's, the card closed by the pen on its return button; party_menus, the
same menu from starter_save's with the party list, Tepig's summary, the Pokédex and its entry, the card and the bag, the
clock pinned for both; aspertia, phase 3's exit route: New Game from a blank save through the intro into the field and
the player's house, out into Aspertia City, a talk with a townsperson, into a house and out, the clock pinned; and phase
4's route, each piece from the oracle's save of the one before: hugh, Hugh's trainer battle from the starter save to his
second move; route19, from the Pokémon Center to Bianca's capture demo and SAVE on Route 19; catch, a wild Purrloin
caught and SAVE; catch_continue, that save continued; center_continue, the save after Hugh's battle continued to the
field; pc, from the catch save back to the Pokémon Center's PC, the PC Box's deposit, withdrawal and swap, and SAVE),
from the save the sequence names, if any, made first on the oracle (or taken from --save-cache), and compares every
frame the same way but the known differences, up to the frame where the port stops when the sequence allows it to stop
on code with no C yet, after checking that the random seeds the input file gives the port are the ones the DS built in
the oracle's run (seeds.py; a file without seeds leaves the port its own); with --update it writes
port/tests/oracle/<name>_<version>.tsv, the oracle's hashes at sampled frames, of its screens and, where the sequence
compares it, of the game's state (STATE). seeds plays a sequence's or a save's input file on the oracle, from the save
it starts from, and prints the random seeds the DS built as the file's seed lines (README.md, "Random seeds").
sequence-check runs the port alone on that input file and checks its counts, its screens without 3D and its state
against that reference. sound runs both programs on a sequence with their sound traces and WAV dumps (--sound-trace,
--audio-dump) and compares the sound driver's state run by run, what the ARM9 sees of it frame by frame, and the sound
by segment (sound.py, SOUND); with --update it writes port/tests/oracle/sound_<name>_<version>.tsv, the oracle's hashes
and statistics. sound-check runs the port alone and checks it against that reference. check is the oracle's ctest test:
the oracle runs the boot sequence twice with the same results (its screens, sound trace and WAV dump), and its screens
match the hashes in port/tests/oracle/boot_<version>.tsv. save, port-save, round-trip, sequence, seeds, sound and check
exit 77 without the oracle or the ROM, sequence-check and sound-check without the ROM.
Standard library only.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import compare  # noqa: E402
import saveimage  # noqa: E402
import seeds  # noqa: E402
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
class StateDump:
    """A part of the game's state that a sequence compares with the oracle's: its label in both programs' memory.txt,
    what each program dumps (pbw-oracle's --dump target by the DS's symbols, the port's --run-dump target by its own
    globals, each with * or ** for a pointer to follow), and its size, the same on both, since none holds a pointer"""
    label: str
    oracle: str
    port: str
    size: int

    def spec(self, program: str, frames: str) -> str:
        """The dump's --dump (program "oracle") or --run-dump (program "port") argument at the frames of a list"""
        return f"{self.label}={self.oracle if program == 'oracle' else self.port}:{self.size}@{frames}"


# The game state the sequences compare by symbol: NNS G3D's global state (glbstate.c), the projection, the camera and
# the matrices it makes of them, 612 bytes without a pointer, so the same on a host as on the DS
STATE = (StateDump("NNS_G3dGlb", "NNS_G3dGlb", "NNS_G3dGlb", 0x264),)
# GFL's random state, which phase 4's route's sequences compare too: the state of GFL's linear generator (the first 8
# bytes of src/system/gfl_use.c's GFLUseWork, a MATHRandContext32's x), and the Mersenne Twister's first word and its
# index (src/gfl/gf_standard.c's GFLStdTables, mt[624] then mti), each through the static that points at it. Given the
# DS's seeds (README.md, "Random seeds"), these are the DS's at a count when the game has drawn as many numbers from
# each generator by then as the DS: the index counts the twister's draws since it was last filled, its first word how
# many times it was. The port reaches the statics through the handles its host files give them
# (port/hal/gfl/random_state.h)
RANDOM_STATE = (
    StateDump("gfl_lc", "*gfl_use.c/sWork", "**port_gfl_use_work", 8),
    StateDump("gfl_mt", "*gf_standard.c/sGFLStd", "**port_gfl_std_tables", 4),
    StateDump("gfl_mt_index", "*gf_standard.c/sGFLStd+2496", "**port_gfl_std_tables+2496", 4),
)


@dataclass(frozen=True)
class Sequence:
    """How a sequence is run and compared. port_frames and oracle_frames: how far each program runs (the DS takes
    hundreds of frames more to get as far). sample: the port frames whose screens the reference keeps. clock: the port
    frames where the game waits on a clock, which pair by their pictures (compare.py --clock); everywhere else frames
    pair on the game's count. lags: by version, the count lags declared (compare.py --lag). known: by version, the port
    frames whose screens differ from the oracle's by construction, left out of the comparison and the reference;
    known_top, those whose top screen alone does, its bottom screen still compared (compare.py --known-top); and
    known_oracle the oracle frames that no port frame can show; each with why (README.md, "Known differences"), a cause
    traced: a difference whose cause is open is a failure to fix, never declared here. close: by version, the port
    frames whose screens with 3D must also have few pixels that differ at all (compare.py --close), with why: where a 2D
    layer the 3D's tolerance would forgive shows. state_frames: the port frames whose game state (state: STATE, NNS
    G3D's, unless the sequence names more) is compared with the oracle's at the frames they pair with, which
    state_oracle, the oracle frames dumped, covers; known_state, by version, the port frames whose state differs by
    construction, with why; and known_random those whose GFL random state alone does (RANDOM_STATE's parts), the rest
    of the state still compared there"""
    port_frames: int
    oracle_frames: int
    sample: str
    clock: str
    lags: dict[str, str] = field(default_factory=dict)
    known: dict[str, dict[str, str]] = field(default_factory=dict)
    known_top: dict[str, dict[str, str]] = field(default_factory=dict)
    known_oracle: dict[str, dict[str, str]] = field(default_factory=dict)
    close: dict[str, dict[str, str]] = field(default_factory=dict)
    state_frames: str = ""
    state_oracle: str = ""
    known_state: dict[str, dict[str, str]] = field(default_factory=dict)
    state: tuple[StateDump, ...] = STATE
    known_random: dict[str, dict[str, str]] = field(default_factory=dict)
    # The save both programs start from, a SAVES name; none for a blank save
    save: str = ""
    # The sequence or the save (SAVES) whose input file it plays, when it isn't its own
    # (port/tests/oracle/<input>_<version>.pbwinput)
    input: str = ""
    # The port frame from which the port may stop on a trap stub (TRAP_STATUS), the trap it may stop on (a regular
    # expression its message must match: the function or data with no C yet), and why. The frames it ran are compared,
    # and the run passes if it got that far and stopped on that trap; another trap is a failure, so the sequence says
    # which gap it stops on and is updated when the decomp fills it
    stop_from: int = 0
    stop_trap: str = ""
    stop: str = ""



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
SUMMARY_SCROLL = ("the summary scrolls its background a pixel every 4 VBlanks since it started (PStatus_Main, "
                  "src/ov207/p_sta_sys.c:264-273), and the DS spends 38 VBlanks more than passes loading it (counts "
                  "4526-4531, in both versions), so its background is about 9 pixels further on there throughout; its "
                  "text, Tepig and the rest of its screens are the port's")
BUILDING_ZOOM = ("the zoom into the house, a scale of BG 2 that a VBlank callback steps at every VBlank "
                 "(src/ov012/building_enter_effect.c), while the DS spends some 45 VBlanks more than passes loading "
                 "the house (counts 15160-15190), so its zoom is further on at each count and it shows steps no port "
                 "frame shows; the crossfade over it ends on the same count on both")
DISPLAY_OFF = ("after the intro the display turns off late in the DS's pass of some 30 frames (count 7750 in Black 2, "
               "7810 in White 2), partway through a frame (black above line 115 in Black 2's last, above line 189 in "
               "White 2's second-last, whose last is white); on the port, from the first line of the pass's frame")
MINCCINO_IDLE = ("Minccino's idle animation on the top screen, about 290 frames long, which it plays after a random "
                 "wait of 200 to 280 passes in steps of 20 (GFL_RandomMTRange, IntroMcss_Update in "
                 "src/ov294/intro_mcss.c): GFL's random numbers are seeded differently on the port "
                 "(port/docs/decomp-findings.md, buildSeed), so the waits differ by construction. The port starts the "
                 "idle at port frame 4668 in both versions; White 2's DS draws a wait three steps longer and starts it "
                 "60 passes later, so the port's wait is one of the two shortest, and with any draw on either program "
                 "an idle starts from port frame 4648 to 4748 and ends by Minccino's fade-out (5038-5040), whose pose "
                 "follows it. Black 2's DS draws the port's wait, one chance in five, so its frames match, but they "
                 "are left out alike, since another seed would move them. The bottom screen is compared")
MID_PASS = ("the DS's pass at that count takes more than one frame, so the game's state at its first frame (NNS G3D's, "
            "or GFL's random state while the pass draws) is the pass's work in progress; at the pass's later frames "
            "it is the port's")
AI_TIME = ("Hugh's AI runs its scripts for 500 ticks of OS_GetTick a pass (TRAI_TIME_LIMIT, src/ov170/tr_ai.c) and "
           "then picks among its best moves with the twister's next number (TrAI_ChooseMove), while the battle's view "
           "draws from the twister at every pass; the port's virtual clock lets it think in one pass, the DS's CPU "
           "takes three, so for two passes the port's twister has drawn one number more (its index one ahead), and "
           "from the DS's pick on the index is the port's again: each of his two turns, the first pick by chance the "
           "same move, the second another (README.md, Known differences; ADR 0023)")
CURSOR_NEW_GAME = ("as CURSOR, with NEW GAME's press from a blank save in CONTINUE's place: the port's frame shows the "
                   "step after the DS's last one (its top screen hashes d3c85a7e), which the DS shows at counts 3274, "
                   "3296, 3338 and 3360 of the same run but not on that count")
WINDOW_SCROLL = ("Mom's speech balloon, at the top of the screen (lines 4-44), scrolls a line up 4 pixels a pass "
                 "(PrintStream_Task, src/system/printsys.c), and the DS's CPU takes milliseconds to copy the window's "
                 "bitmap before it flushes it to VRAM, after the beam has drawn the window: the DS shows each step a "
                 "frame later than the port, whose passes take no time (its window then equal to the port's frame "
                 "before, pixel for pixel); the letters printed into it, with no copy, and the townsperson's window "
                 "at the bottom of the screen show on the same count on both")
BALLOON_SCROLL = ("a speech balloon at the top of the screen scrolls a line up 4 pixels a pass, as Mom's does in the "
                  "exit route (WINDOW_SCROLL): the DS shows each step a frame later, its balloon then equal to the "
                  "port's frame before pixel for pixel and the port's equal to the DS's frame after (Hugh's at the "
                  "lookout, Mom's and Bianca's on the way to Route 19, in both versions); where the DS's pass takes "
                  "two frames (White 2, count 4963), the first shows the step late and the second on time, which only "
                  "the port's frame of that count shows")
FIELD_WINDOW_OPEN = ("Route 19's first InfoMsg window has reached VRAM after the DS has drawn its top-screen "
                     "region: the whole main BG VRAM, palette and OAM are identical at the ends of DS frames 6588 "
                     "and 6589, but the window appears on the LCD only in the latter. The native window equals "
                     "that next frame exactly (the same InfoMsg/flush path in White 2). Only this top-screen "
                     "presentation frame is omitted; the bottom screen, NNS and random state remain compared")
MOVE_MENU = ("the DS's pass that takes the move menu down after a move is chosen takes two frames, the first showing "
             "it partway down (the moves' names cleared, their buttons still there) and the second the port's picture "
             "of that count: the count pairs the port's frame with the first, and the check of pictures no port frame "
             "shows finds the second neither pair's, its 3D within the tolerance of the port's")
WIPE_STEP = ("a wipe's first frame (into the gate, out of it onto Route 19): the DS's shows lines 32 to 63 as its "
             "frame before, black where the port's frame has them covered or not yet uncovered, and lines 0 to 31 and "
             "64 on as the port's: the wipe's second step, set in the same pass as its first, written after the beam "
             "has drawn those lines; every frame after is the port's")
WIPE_LINES = ("a wipe's step that the DS's pass sets while the beam draws the frame, into the gate from Route 19 and "
              "out of it into Aspertia City: the window that ends each line (WipeLineWnd_HBlank, "
              "src/system/wipe_sub.c) is the step before on the lines the beam has drawn by then, so those lines are "
              "the port's frame before (4 to 10 lines from line 64 at a step whose edge is there, lines 1 to 31 at the "
              "last step of the wipe into the gate and the first of the wipe out of it) and the rest the port's; every "
              "other frame of the wipes is the port's")
CENTER_ZOOM = ("as BUILDING_ZOOM, the zoom into the Pokemon Center, a scale of BG 2 that a VBlank callback steps at "
               "every VBlank (src/ov012/building_enter_effect.c): the DS spends 79 VBlanks more than passes loading "
               "the Pokemon Center in Black 2 and 80 in White 2 (counts 5183-5232), so its zoom is further on at each "
               "count and it shows steps no port frame shows; the crossfade over it ends on the same count on both "
               "(port frame 5273)")
PC_WINDOW = ("the DS's pass that changes one of the PC's windows takes 2 to 6 frames: its first menu closed for "
             "Someone's PC and shown again after B, the storage system's menu shown and shown again after each app of "
             "the PC Box, its cursor moved down it with the message under it, and the deposit screen's top screen "
             "when the cursor moves to Purrloin. The pass's frames but the last show the windows partway (a list's "
             "frame without its text, the message before) or the screen before, its last the port's picture (within "
             "the 3D's tolerance), and the count pairs the port's frame with the first. The check of missing "
             "pictures holds each DS frame between two counts' first frames to those two frames' pictures, not to "
             "the port's, so it names these passes' frames that show neither: the deposit screen's at count 6400, "
             "whose last frame (oracle 8795 in Black 2, 8730 in White 2) is port frame 6403's picture, both screens "
             "the same by hash, where its first frame shows the top screen before and count 6401's first frame "
             "already the next change; and in White 2 the fifth frame of the passes of six frames at counts 6054, "
             "7195, 8145 and 9715 (oracle 8321, 9583, 10653 and 12350, the top screen with 3D), which shows the "
             "message before with 13 pixels of the next at lines 181-182 (x 82-108), the window a step further "
             "partway, a picture no port frame shows; Black 2's same passes show one picture until their last frame")
PC_LOGOFF = ("the PC's script lets the townspeople move again once its log-off sound has ended (pcLogOffSound, "
             "src/ov033/event_pc.c, waits on GFL_SndPlayerIsActive before ActorsUnpauseAll). Both ARM7s end the sound "
             "in the same frame (count 9940; the sound traces), and the DS's script goes on a field pass sooner, as "
             "with Purrloin's cry (CRY_BATTLE) the other way round: the DS's pass reads the players' status at "
             "another moment of its frame than the port's, which reads it at the VBlank. So the townsperson by the "
             "counter who turns at random, paused through the PC with the same wait on both, waits on from count 9944 "
             "on the DS and 9946 on the port, and each of his next turns, a draw from GFL's linear generator, comes "
             "2 counts sooner on the DS (counts 9971 and 10035 against 9973 and 10037), the second one seen (Black "
             "2). White 2's DS goes on a pass sooner too (his wait taken up at count 9944 against 9946: the DS's "
             "actor dumped from melonDS's memory, the port's from a scratch trace), but his wait, 62, doesn't run out "
             "before the field menu stops him, so nothing differs there; his lead shows after the save (SAVE_RANDOM)")
SAVE_GLOBE = ("as SAVE_CAMERA: the field draws its 3D again from a step of the save's dialog on, and its map's "
              "animated props move with it: the Pokemon Center's Geonet globe (top right), which stands still while "
              "the game writes, turns again from count 10711 on the port and from 10855 on the DS, whose write takes "
              "longer (SAVE_WRITES), and stays 144 counts ahead on the port after both dialogs close; with the "
              "townsperson by the counter who turns at random, whose turns the DS makes 126 counts later after the "
              "save (SAVE_RANDOM), the Pokemon Center's 3D is beyond the tolerance in some frames after the DS's "
              "dialog closes (up to 565 pixels, where its frames before the save have 12 to 16)")
BANNER = ("the place name's banner at the top of the screen goes away in a pass whose write the DS makes after the "
          "beam has drawn lines 0 to about 20: there its frame is the port's frame before, below it the port's (Route "
          "19's banner)")
BATTLE_FADE = ("as Bianca's capture demo opens, the lower part of the top screen darkens a step a pass, and the DS "
               "makes each step's write when the beam is at about line 146: below it its six frames are the port's "
               "frame before, above it the port's")
PLAY_TIME = ("the save dialog shows the play time, which counts ticks from CONTINUE on (GameSystemTimer_Start, "
             "src/system/playtime_ctrl.c, from EventFieldContinue_Create): the DS's CPU's, through its passes of "
             "several frames while it loads, and the port's virtual clock's (port/hal/os/core.c), which moves with "
             "each frame and in the busy loops it models (OS_GetTick's among them). They part by seconds (Black 2's "
             "catch saves hold 0:13:11 and 0:13:05), and in White 2 the minute turns between them, 0:11 on the DS and "
             "0:10 on the port at the Route 19 save, 0:13 and 0:12 at the catch's, for as long as the dialog shows on "
             "both")
SAVE_WRITES = ("the game writes the save in steps, waiting for the backup chip's each (GFL's backup system over "
               "CARD's backup commands, src/gfl/backup_system.c): the DS's flash takes time for each, so its progress "
               "dots move on later, its message that the game is saved comes later and its dialog closes later than "
               "the port's, whose chip is written at once (port/hal/card): 192 counts for Black 2's Route 19 save and "
               "168 for White 2's, 228 and 232 for the catch's, 128 for the PC's in both; between, both show the "
               "message frame for frame")
CRY_BATTLE = ("as Kyurem's cry at the title (CRY): the wild Purrloin's cry, which the battle waits for as Purrloin "
              "appears (BtlvEffvm_WaitNative's wait for the voices, src/ov168/btlv_effvm.c), stops in the frame of "
              "count 5135 on both, and that frame's pass reads the channel (PokeVoice_Update, src/system/pm_voice.c) "
              "after the port's ARM7 has stopped it but before the DS's has, so the DS lets the cry go and goes on a "
              "pass later: its battle scene shows the port's frame before from port frame 5152, its command menu "
              "slides in a pass later (5580-5590), and the sprites' idle animations and the battle's idle camera "
              "moves, whose waits and picks draw from the twister at the pass they start, start at other passes and "
              "differ for the rest of the battle, its menus and messages on the same count as the port's (White 2)")
SAVE_CAMERA = ("as SAVE_WRITES: from a step of the save's dialog on, the field's passes alternate between ending with "
               "NNS G3D's camera at the field's and at a 2D one (no rotation, 128 units back), and the port, whose "
               "chip is written at once, reaches that step before the DS (232 counts in Black 2's catch, count 10174 "
               "against 10406; 144 in the PC's save in both versions, 10711 against 10855): until the DS gets there, "
               "every other pass of the port's ends with the 2D camera where the DS's ends with the field's; from "
               "there both alternate on the same counts")
SAVE_RANDOM = ("the field's townspeople who walk or turn at random draw their waits and directions from GFL's linear "
               "generator when their timers run out (FieldActorMovecode_GetRandomTableEntry and "
               "FieldActorMovecode_GetRandomDirFromSet, src/ov036/field_actor_movecode.c), and their timers wait while the "
               "field menu and the save's dialog show (DisableAllActorsMovement as the menu opens, "
               "src/ov012/event_field_menu.c), which on the DS closes later (SAVE_WRITES): from the port's first draw "
               "after its close, the DS makes each draw of the port's, the same number, later by as many counts (192 "
               "for Black 2's Route 19 save and 168 for White 2's, 228 for Black 2's catch and 232 for White 2's), "
               "less a field pass for a timer that ran a pass ahead on the DS before the menu stopped it: the PC's "
               "save closes 128 counts later in both versions, and the townsperson by the counter, whose wait the DS "
               "took up again a field pass sooner after the PC's log-off (PC_LOGOFF), draws 126 counts later, his "
               "wait taken up again at count 11220 on the DS and 11092 on the port with one step less to go on the DS "
               "(25 against 26 in Black 2, 9 against 10 in White 2: the DS's actor dumped from melonDS's memory, the "
               "port's from a scratch trace)")
CRY_RANDOM = ("as CRY_BATTLE: from Purrloin's cry's end the sprites' idle animations and the battle's idle camera "
              "moves draw from GFL's twister at other passes on the two programs, so the twister's index differs, "
              "equal again only by chance, for the rest of the battle and after it (its first word too where it is "
              "filled again at another count) (White 2)")
CRY_SAVE_RANDOM = "as CRY_RANDOM for the twister, and as SAVE_RANDOM for GFL's linear generator"
# The frames held close, by kind
TITLE_FADE = ("the title's bottom screen fading in over its 3D, where the copyright screen's tiles read past their "
              "file show as a band (port/docs/sanitizers.md, H1), a step or two from black: within the 3D's tolerance; "
              "from a blank save the port's heap holds the DS's bytes there")
# How far the continue sequences run into the field: the port to its frame 3900, the oracle to its 6000, where its
# count is 3955 (it reaches the field 2045 frames after the port, most of them in the movies)
PORT_FRAMES_CONTINUE = 3900
ORACLE_FRAMES_CONTINUE = 6000
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
        close={"b2_us": {"3527-3560": TITLE_FADE}, "w2_us": {"3527-3560": TITLE_FADE}},
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
        close={"b2_us": {"1116-1149": TITLE_FADE}, "w2_us": {"1116-1149": TITLE_FADE}},
        state_frames="1116-3600",
        state_oracle="2800-5700",
        known_state={"b2_us": {"3110-3124": CAMERA}, "w2_us": {"3110-3124": CAMERA}},
    ),
    # The title sequence's presses from first_save's save, then A at game frame 3400 on the start menu's CONTINUE,
    # which now shows the saved game: the player, the place, the badges and the play time (the Pokédex once the
    # player has one). CONTINUE blinks, the menu wipes out to black from port frame 3425, and the field loads and shows
    # the player in his room. The title's known differences are the title sequence's; on the oracle they come 23
    # frames sooner, since its start-up, which reads the save, starts the main loop at frame 219 with this save and at
    # 242 with a blank one. In White 2 the press of CONTINUE brings the counts together again. The title's first frames
    # of 3D aren't held close as the title sequence's are: from a save, the heap after the copyright screen's file holds
    # other bytes on the port than on the DS, so the band they show differs (README.md, "Known differences")
    "continue": Sequence(
        port_frames=PORT_FRAMES_CONTINUE,
        oracle_frames=ORACLE_FRAMES_CONTINUE,
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
    ),
    # continue's presses from starter_save's save: the start menu's saved game now has the Pokédex line (Pokédex: 1,
    # time 0:04), and CONTINUE goes to the Aspertia City lookout
    "continue_starter": Sequence(
        port_frames=PORT_FRAMES_CONTINUE,
        oracle_frames=ORACLE_FRAMES_CONTINUE,
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
    ),
    # continue's presses from first_save's save with the clock pinned, then the field menu in the player's room: X
    # opens it (no Pokémon, no Pokédex: BAG, the trainer card, SAVE and OPTIONS), A on BAG opens the bag, its items
    # pocket empty, and B closes it; RIGHT and A open the trainer card, the pen on its return button closes it (the only
    # touch of the sequences), and B closes the menu
    "field_menus": Sequence(
        port_frames=5200,
        oracle_frames=7400,
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
    ),
    # The same field menu at the Aspertia City lookout from starter_save's save, every item in it, the clock pinned: the
    # party list, Tepig's menu and its summary (port frames 4534-4822, counts 4533-4821, left out: SUMMARY_SCROLL),
    # back to the list and the menu; the Pokédex, its list and Tepig's entry; the trainer card with its Pokédex line;
    # the bag; B closes the menu
    "party_menus": Sequence(
        port_frames=8500,
        oracle_frames=11100,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA,
                      "4534-4822": SUMMARY_SCROLL},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR, "4534-4822": SUMMARY_SCROLL},
        },
        known_oracle={"b2_us": {"4920": TORN_START}, "w2_us": {"1260": TORN_OPENING, "4852": TORN_START}},
        save="starter_save",
    ),
    # Phase 3's exit route (PLAN.md section 7), from a blank save: the title sequence's presses, NEW GAME at game frame
    # 3400 and the intro with A twice a second (Minccino's Poké Ball burst, the names, the season's name, the flight
    # over Aspertia City), the field in front of the player's house from count 9781 (9841 in White 2, whose intro runs
    # 60 passes longer from its gender question to the room), the first scripts, the house; then Mom's scene, out of
    # the door into Aspertia City from count 14538, west along z = 762 to the townsperson at (39,764) and his three
    # lines, into the house at (38,761) from 15199, up and back out from 15444, and east. The intro saves New Game's
    # first data in the background and waits for the save to pause before it fades to the name entry
    # (IntroCmd_SavePause, src/ov294/intro_cmd.c): the DS's flash writes take time, so from that fade until the name
    # entry waits for its first press the DS is a pass later (the lag of -1, port frames 5538-5576, 5598-5636 in White
    # 2). The DS's passes take several frames where it loads (the intro, the season's name, each map), and NNS G3D's
    # state is compared at every port frame from the title on but at the first frame of those passes (MID_PASS); the
    # reference keeps it at its sampled frames, the camera that follows the player through the field among it
    "aspertia": Sequence(
        port_frames=15800,
        oracle_frames=18700,
        sample="1-15800/12,1096-1120,2870-2885,2996-3010,3096-3135,3215-3250,3396-3410",
        clock="313-1108",
        lags={"b2_us": "1109=0,5538=-1,5577=0", "w2_us": "1109=0,3241=-1,3403=0,5598=-1,5637=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA,
                      "7751": DISPLAY_OFF, "12911-12914,13727-13730,13967-13970,14223-14226": WINDOW_SCROLL,
                      "15176-15229": BUILDING_ZOOM},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR_NEW_GAME, "7811": DISPLAY_OFF,
                      "12911-12914,13727-13730,13967-13970,14223-14226": WINDOW_SCROLL, "15176-15229": BUILDING_ZOOM},
        },
        known_top={"b2_us": {"4648-5040": MINCCINO_IDLE}, "w2_us": {"4648-5040": MINCCINO_IDLE}},
        known_oracle={"b2_us": {"4943": TORN_START, "9841": DISPLAY_OFF, "17462-17519": BUILDING_ZOOM},
                      "w2_us": {"1283": TORN_OPENING, "4875": TORN_START, "9837-9838": DISPLAY_OFF,
                                "17401-17457": BUILDING_ZOOM}},
        close={"b2_us": {"1116-1149": TITLE_FADE}, "w2_us": {"1116-1149": TITLE_FADE}},
        state_frames="1116-15800",
        state_oracle="2800-18700",
        known_state={
            "b2_us": {"3110-3124": CAMERA,
                      "3431,5818,6388,7751-7752,9770,9781,11561,11588,14508,14535,15187,15416,15443": MID_PASS},
            "w2_us": {"3110-3124": CAMERA,
                      "3431,5878,6448,7811-7812,9830,9841,11621,11648,14508,14535,15187,15416,15443": MID_PASS},
        },
    ),
    # Phase 4's route, from the starter save, in pieces that each start from melonDS's save of the piece before
    # (SAVES), so that each starts as the DS does: Hugh's battle (hugh), the Pokémon Center to Bianca's catching lesson
    # on Route 19 (route19), a wild Pokémon caught in Route 19's grass and the game saved (catch), and that save
    # continued (catch_continue). Each input file pins the clock and gives the random seeds the DS builds there
    # (README.md, "Random seeds"). hugh: continue_starter's presses, then down the lookout to Hugh's arrival and his
    # challenge, and his trainer battle to the second turn's rolls, where it parts by construction (README.md, "Known
    # differences": the AI's time a pass)
    "hugh": Sequence(
        port_frames=8200,
        oracle_frames=10450,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA,
                      "5023-5026,5359-5362,5887-5890": BALLOON_SCROLL, "7591,8167": MOVE_MENU},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR, "5023-5026,5359-5362,5887-5890": BALLOON_SCROLL,
                      "7591,8167": MOVE_MENU},
        },
        known_oracle={"b2_us": {"4920": TORN_START, "9742,10331": MOVE_MENU},
                      "w2_us": {"1260": TORN_OPENING, "4852": TORN_START, "9679,10268": MOVE_MENU}},
        state_frames="1116-8200",
        state_oracle="2800-10450",
        known_state={
            "b2_us": {"3110-3124": CAMERA,
                      "3432,3447,3454,3600,6346,6408,6410,6412,6414,6416,6660,6668,6674,7067,7222,7271,7372,7513,7554,"
                      "7591,7618,7695,7872,8103,8130,8167,8194": MID_PASS},
            "w2_us": {"3110-3124": CAMERA,
                      "3432,3447,3454,3600,6346,6408,6410,6412,6414,6416,6660,6668,6674,7067,7218,7267,7368,7509,7554,"
                      "7591,7618,7695,7872,8103,8130,8167,8194": MID_PASS},
        },
        known_random={"b2_us": {"7501-7502,7970-7971": AI_TIME}, "w2_us": {"7497-7498,7970-7971": AI_TIME}},
        state=STATE + RANDOM_STATE,
        save="starter_save",
    ),
    # route19: from the Pokémon Center save, out into the city, Mom's scene, the gate's gift and Bianca's catching
    # lesson on Route 19 with her capture demo, to the field menu's SAVE beside the grass (the Route 19 save's input
    # file)
    "route19": Sequence(
        port_frames=15400,
        oracle_frames=18200,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA,
                      "4969-4972,9305-9308": BALLOON_SCROLL, "7623,8791": WIPE_STEP, "8864": BANNER,
                      "9795-9800": BATTLE_FADE, "10274": MOVE_MENU, "14129-14447,14563-14758": SAVE_WRITES},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR, "4969-4972,9305-9308": BALLOON_SCROLL,
                      "7623,8791": WIPE_STEP, "8864": BANNER, "9795-9800": BATTLE_FADE, "10274": MOVE_MENU,
                      "13723-14128,14420-14558": PLAY_TIME, "14129-14419,14559-14730": SAVE_WRITES},
        },
        known_oracle={"b2_us": {"4920": TORN_START, "12714": MOVE_MENU},
                      "w2_us": {"1260": TORN_OPENING, "4852": TORN_START, "7060": BALLOON_SCROLL, "12659": MOVE_MENU}},
        state_frames="1116-15400",
        state_oracle="2800-18200",
        known_state={
            "b2_us": {"3110-3124": CAMERA,
                      "3432,3454,3461,3607,3928,3931,3988,5236,5394,7652,7667,8747,8788,9572,9626,9694,9700,9710,9845,"
                      "9882,9983,10098,10100,10187,10274,10352,10501,10878,10981,11068,11156,11254,11452,11484,11866,"
                      "12097,12152,12166,12205,13533,13717,13723,13979,14756": MID_PASS,
                      "14205-14395/2": SAVE_CAMERA},
            "w2_us": {"3110-3124": CAMERA,
                      "3432,3454,3461,3607,3928,3931,3988,4969,4971,5236,5394,5897,5899,6009,6011,6665,6667,7652,7667,"
                      "8747,8788,9305,9307,9572,9626,9694,9700,9710,9845,9882,9983,10098,10100,10187,10274,10355,10517,"
                      "10878,10981,11068,11156,11254,11454,11486,11868,12099,12152,12166,12205,12460,12462,12700,12702,"
                      "12764,12766,13132,13134,13533,13717,13723,13979,14728": MID_PASS,
                      "14197-14367/2": SAVE_CAMERA},
        },
        known_random={"b2_us": {"14606-15400": SAVE_RANDOM}, "w2_us": {"14634-15400": SAVE_RANDOM}},
        state=STATE + RANDOM_STATE,
        save="center_save",
        input="route19_save",
    ),
    # catch: from the Route 19 save, into the grass and up and down until a wild Pokémon appears, its battle, a Poké
    # Ball, the catch, and the field menu's SAVE (the catch save's input file)
    "catch": Sequence(
        port_frames=11600,
        oracle_frames=14200,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA,
                      "10102-10460,10540-10771": SAVE_WRITES},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR, "5580-5588,5590": CRY_BATTLE,
                      "9696-10101,10465-10539": PLAY_TIME, "10102-10464,10540-10775": SAVE_WRITES},
        },
        known_top={"w2_us": {"5152-5579,5589,5591-8136": CRY_BATTLE}},
        known_oracle={"b2_us": {"4920": TORN_START}, "w2_us": {"1260": TORN_OPENING, "4852": TORN_START}},
        state_frames="1116-11600",
        state_oracle="2800-14200",
        known_state={
            "b2_us": {"3110-3124": CAMERA,
                      "3432,3447,3454,3600,4826,4880,4948,4954,4964,5197,5234,5335,5476,6733,6872,6972,7076,7085,7114,"
                      "7406,7438,7786,8013,8547,8561,8586,9506,9690,9696,9952,10769": MID_PASS,
                      "10178-10408/2": SAVE_CAMERA},
            "w2_us": {"3110-3124": CAMERA,
                      "3432,3447,3454,3600,4930,4984,5052,5058,5068,5439,8561,8586,9506,9690,9696,9952,10773": MID_PASS,
                      "5300-5332,5519-5520,5522-5523,5537-5549,5579-5586,6483-7101,7254-7310,8017-8056": CRY_BATTLE,
                      "10178-10412/2": SAVE_CAMERA},
        },
        known_random={"b2_us": {"10569-11600": SAVE_RANDOM},
                      "w2_us": {"5337-10556": CRY_RANDOM, "10557-11600": CRY_SAVE_RANDOM}},
        state=STATE + RANDOM_STATE,
        save="route19_save",
        input="catch_save",
    ),
    # catch_continue: the catch save continued in a new run, its start menu, CONTINUE into the grass, and the field
    # menu's party list with the Pokémon caught (oracle.py round-trip catch also plays it from the port's own save)
    "catch_continue": Sequence(
        port_frames=4700,
        oracle_frames=6900,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR},
        },
        known_oracle={"b2_us": {"4920": TORN_START}, "w2_us": {"1260": TORN_OPENING, "4852": TORN_START}},
        state_frames="1116-4700",
        state_oracle="2800-6900",
        known_state={"b2_us": {"3110-3124": CAMERA, "3432,3447,3454,3600,3804,4440,4443,4497": MID_PASS},
                     "w2_us": {"3110-3124": CAMERA, "3432,3447,3454,3600,3804,4440,4443,4497": MID_PASS}},
        state=STATE + RANDOM_STATE,
        save="catch_save",
    ),
    # Phase 5's first story leg: Alder on Route 19 and in Floccesy Town, then the road to Route 20. Compare the
    # whole route before SAVE; round-trip floccesy checks the actual write and fresh-process continuations.
    "floccesy": Sequence(
        port_frames=31900,
        oracle_frames=40000,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR},
        },
        known_oracle={"b2_us": {"4920": TORN_START}, "w2_us": {"1260": TORN_OPENING, "4852": TORN_START}},
        known_top={"b2_us": {"4530": FIELD_WINDOW_OPEN, "4983-4986,5063-5066": BALLOON_SCROLL},
                   "w2_us": {"17230": FIELD_WINDOW_OPEN, "17987-17990,18067-18070": BALLOON_SCROLL}},
        state_frames="1116-31900",
        state_oracle="2800-40000",
        known_state={"b2_us": {"3110-3124": CAMERA, "3432,3447,3454,3600,18524": MID_PASS},
                     "w2_us": {"3110-3124": CAMERA,
                               "3432,3447,3454,3600,4248,4302,4370,4376,4613,4892,5739,5764,10204,10258,"
                               "10326,10332,10579,10864,12495,12758,12889,12914,31528": MID_PASS}},
        known_random={"w2_us": {"4386,4613,4650,4751,4892,5725,10342,10579,10616,10717,10864,12059,"
                                  "12196,12495,12875": MID_PASS}},
        state=STATE + RANDOM_STATE,
        save="catch_save",
        input="floccesy_save",
    ),
    # A new process continues the Route 20 save, opens the party and returns to the field. These are the same
    # controls as catch_continue, checked again on the new map by the sequence and the complete round trip.
    "floccesy_continue": Sequence(
        port_frames=4700,
        oracle_frames=6900,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR},
        },
        known_oracle={"b2_us": {"4920": TORN_START}, "w2_us": {"1260": TORN_OPENING, "4852": TORN_START}},
        state_frames="1116-4700",
        state_oracle="2800-6900",
        known_state={"b2_us": {"3110-3124": CAMERA, "3432,3447,3454,3600,3804,4440,4443,4497": MID_PASS},
                     "w2_us": {"3110-3124": CAMERA, "3432,3447,3454,3600,3804,4440,4443,4497": MID_PASS}},
        state=STATE + RANDOM_STATE,
        save="floccesy_save",
    ),
    # center_continue: the Pokemon Center save, made after Hugh's battle, continued in a new run to the last frame
    # before the field fades in (oracle.py round-trip hugh also plays it from the port's own save): the start menu
    # with the saved game, CONTINUE, the black and the season's banner. The field would show the townsperson whose
    # turn the two saves hold differently (README.md, "Saves")
    "center_continue": Sequence(
        port_frames=3610,
        oracle_frames=5700,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR},
        },
        known_oracle={"b2_us": {"4920": TORN_START}, "w2_us": {"1260": TORN_OPENING, "4852": TORN_START}},
        state_frames="1116-3610",
        state_oracle="2800-5700",
        known_state={"b2_us": {"3110-3124": CAMERA, "3432,3447,3454,3461,3600,3607": MID_PASS},
                     "w2_us": {"3110-3124": CAMERA, "3432,3447,3454,3461,3600,3607": MID_PASS}},
        state=STATE + RANDOM_STATE,
        save="center_save",
    ),
    # pc: from the catch save, back through the gate to the Pokemon Center's PC and the PC Box's first three apps:
    # Purrloin deposited into Box 1, the box looked at and Purrloin withdrawn, Tepig and Purrloin swapped in the party;
    # the PC switched off and the field menu's SAVE
    "pc": Sequence(
        port_frames=11600,
        oracle_frames=15000,
        sample="",
        clock="313-1108",
        lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
        known={
            "b2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3129": CAMERA,
                      "4123,4125,4539": WIPE_LINES, "5187-5272": CENTER_ZOOM,
                      "5871,6057,6403,7198,7304,8148,8304,8334,9718,9868": PC_WINDOW, "10040-10041": PC_LOGOFF,
                      "10654-10663,10666-10908,11088-11219": SAVE_WRITES,
                      "11403-11406,11469-11472,11485-11498,11533-11600": SAVE_GLOBE},
            "w2_us": {"2878": TORN_HIDE, "3001": TORN_BACK, "3101": TORN_START, "3112-3128": CAMERA,
                      "3219-3240/3": CRY, "3402": CURSOR, "4107,4109,4537,4539": WIPE_LINES, "5187-5272": CENTER_ZOOM,
                      "5871,6057,6403,7198,7304,8148,8304,8334,9718,9868": PC_WINDOW,
                      "10654-10663,10666-10908,11088-11219": SAVE_WRITES,
                      "11243-11272,11469-11594,11597-11600": SAVE_GLOBE},
        },
        known_oracle={
            "b2_us": {"4920": TORN_START,
                      "7431-7435,7438-7456,7459,7461-7464,7466-7471,7473-7474,7476-7485,7487,7491-7512,7550-7558":
                          CENTER_ZOOM,
                      "8795": PC_WINDOW},
            "w2_us": {"1260": TORN_OPENING, "4852": TORN_START,
                      "7365-7369,7372-7390,7393,7395-7398,7400-7405,7407-7408,7410-7419,7421,7425-7447,7485-7493":
                          CENTER_ZOOM,
                      "8321,8730,9583,10653,12350": PC_WINDOW},
        },
        state_frames="1116-11600",
        state_oracle="2800-15000",
        known_state={
            "b2_us": {"3110-3124": CAMERA,
                      "3432,3447,3454,3600,4150,4165,4491,4532,5198,5871,6057,7171,7180,7198,7304,8121,8130,8148,8304,"
                      "8334,9691,9700,9718,9868,10056,10240,10246,10502,11217": MID_PASS,
                      "10714-10856/2": SAVE_CAMERA},
            "w2_us": {"3110-3124": CAMERA,
                      "3432,3447,3454,3600,4134,4149,4491,4532,5198,5871,6057,7171,7180,7198,7304,8121,8130,8148,8304,"
                      "8334,9691,9700,9718,9868,10056,10240,10246,10502,11217": MID_PASS,
                      "10714-10856/2": SAVE_CAMERA},
        },
        known_random={"b2_us": {"9974-9975,10038-10039": PC_LOGOFF, "11147-11600": SAVE_RANDOM},
                      "w2_us": {"11115-11600": SAVE_RANDOM}},
        state=STATE + RANDOM_STATE,
        save="catch_save",
    ),
}
REFERENCE_COLUMNS = ["port", "oracle", "counter", "lag", "top", "bottom", "state"]
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
    # menu's SAVE. Event flags 0x961 (a Pokémon) and 0x962 (the Pokédex) are set, so the field menu has every item.
    # White 2's walk to the stairs takes another street: a townsperson wanders into Black 2's there (README.md, Saves)
    "starter_save": SaveMaker(17000, {"b2_us": "ce71fe0b8c1d93436d62678400d1e0722632b67e",
                                      "w2_us": "3b9dac231d1f73a3599a60131311c9b375eb7342"}, start="first_save"),
    # starter_save's CONTINUE and Hugh's trainer battle (Tepig grows to level 6), the Pokémon Center's scene, then SAVE
    # there once the player is free, the clock pinned: phase 4's route continues from it after Hugh's battle, which
    # parts from melonDS's by construction (README.md, "Known differences")
    "center_save": SaveMaker(19000, {"b2_us": "a74d51fa94d9141668b4464dd91b4ba87f8e946a",
                                     "w2_us": "ee13618f8defaa00d5850e245ab111263bd763d4"}, start="starter_save"),
    # center_save's CONTINUE, Mom's scene, the gate's gift and Bianca's catching lesson with her capture demo, then SAVE
    # beside Route 19's grass: the route's second piece (route19), whose input file this is
    "route19_save": SaveMaker(17800, {"b2_us": "26d75e2638a63959a6813bdb9f2bb7443223b575",
                                      "w2_us": "12b20ac0dd82434844b6ceb0635606573f89e91e"}, start="center_save"),
    # route19_save's CONTINUE, a wild Pokémon in Route 19's grass caught with a Poké Ball, then SAVE: the route's third
    # piece (catch), whose input file this is
    "catch_save": SaveMaker(13700, {"b2_us": "baf426358777119d6a3f22faaed08ca1270ada98",
                                    "w2_us": "64960699601c9f9a3409f5a0325af080f11dc325"}, start="route19_save"),
    # Catch save through Alder's Route 19 introduction and both Floccesy scenes to Route 20, then SAVE at 32000.
    # Both copies have 73 valid block CRCs, zone 446 at grid (130,661), and event work 40a3=2 / 40a5=2.
    "floccesy_save": SaveMaker(40000, {"b2_us": "07d26e593786080b3a50f4d2afecd276bfed50b8",
                                      "w2_us": "26ef0ed2e6cae38298b5543580e64172556e52dc"}, start="catch_save"),
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
             trace: str | None = None) -> tuple[int | None, str]:
    """Runs the port's program once, from power-on with a blank save or with the save memory in the file save, for
    frames frames, writing its run directory in out (--run-dir): frames.tsv for every frame it ran, a PNG file for each
    frame of screens (a frame list, "none" or "all") and memory.txt with the dumps (--run-dump specs), and with audio
    and trace its WAV dump and sound trace (--audio-dump, --sound-trace). The save directory is a new one, given the
    save as the port names its saves (the game code and .sav); save_out gets the save memory the run left there, which
    is the blank chip when the game neither had a save nor wrote one, never a file of an earlier run. Returns its exit
    status (None when it timed out) and its last message about why it stopped"""
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
# blank backup, to frame 10,600, before the field's first load (frame 10,759); the oracle to the same moment, since the
# DS reaches each screen later. Then the title sequence's input file boots each program's save on the oracle
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


def compare_boots(port_run: compare.Run, oracle_run: compare.Run, last_count: int = 0) -> tuple[int, str | None]:
    """How many frames two oracle runs share, and the first where their counts or screens differ (None if none); with
    last_count, only the frames up to the last of that count, the frames after it left out of both (before the game
    sets the count up at start-up, its word holds whatever the memory did, so the frames end only once it has been
    reached)"""
    if sorted(port_run.frames) != sorted(oracle_run.frames):
        return 0, f"the runs have {len(port_run.frames)} and {len(oracle_run.frames)} frames"
    same = 0
    reached = False
    for frame in sorted(port_run.frames):
        a = port_run.frames[frame]
        b = oracle_run.frames[frame]
        reached = reached or (bool(last_count) and a.counter == last_count)
        if reached and a.counter is not None and a.counter > last_count:
            break
        if (a.counter, a.top, a.bottom) != (b.counter, b.top, b.bottom):
            return same, (f"frame {frame}: count {a.counter} against {b.counter}, top {a.top[:8]} against "
                          f"{b.top[:8]}, bottom {a.bottom[:8]} against {b.bottom[:8]}")
        same += 1
    return same, None


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


@dataclass(frozen=True)
class RoundTrip:
    """A save that both programs make and that each continues in a new run (README.md, "Saves"): maker, the SAVES name
    whose input file both play from its start save, the port to port_frames, by when the game has saved; continued,
    the sequence whose input file continues the save, from power-on. The oracle continues the port's save as it
    continues its own, frame for frame, and the port continues its own save in a new run, held to the oracle's run on
    that same save as the sequence holds the port to the oracle on the oracle's save, its game state too. known: by
    version, the bytes where the two saves differ by construction (save offsets, "0x19427" or "0x18fa2-0x18fa3",
    comma-separated), each with why (README.md, "Saves"), a cause traced; the saves are compared with their mystery
    gift blocks decrypted (mystery_gift_decrypted), so that only the block's key, not its encrypted contents, can be
    declared, and a byte that differs anywhere else fails the trip, as an undeclared frame fails a sequence. A piece
    whose course the DS's CPU time decides (ADR 0023) names its cut, the sequence that holds the piece frame by frame
    up to that point, from the same start save: the port's run of the piece is compared with the oracle's run of the
    cut's input file as that sequence compares it, up to its last port frame, the cut's input file being the start of
    the port's, line for line; past it the port plays its own course. Where the oracle's presses would miss what that
    course shows at them (a trainer battle whose turns, and so its scenes, outlast the presses), port_input names, by
    version, the input file the port plays instead (port/tests/oracle/<name>_<version>.pbwinput), the maker's with
    presses of its own after the cut. continued_last: the last count at which the oracle's runs on the two saves are
    compared frame for frame, where what follows shows a declared byte (0 for all their frames)"""
    maker: str
    port_frames: int
    continued: str
    known: dict[str, dict[str, str]] = field(default_factory=dict)
    cut: str = ""
    port_input: dict[str, str] = field(default_factory=dict)
    continued_last: int = 0


def input_lines(path: str) -> list[str]:
    """An input file's lines without comments and blank lines, each with its words joined by one space: what a program
    reads of it"""
    lines = []
    with open(path) as f:
        for line in f:
            words = line.split("#", 1)[0].split()
            if words:
                lines.append(" ".join(words))
    return lines


def input_starts_with(path: str, start: str) -> bool:
    """Whether the input file at path holds the one at start line for line before any line of its own: its header, its
    seeds and its presses, so that a program playing it plays start's presses to start's end"""
    head = input_lines(start)
    return input_lines(path)[:len(head)] == head


# The main save's second copy, at this offset of the chip, holds the same bytes as its first (README.md, "Saves")
SECOND_COPY = 0x26000


def parse_offsets(text: str) -> list[int]:
    """"0x19427,0x18fa2-0x18fa3" as the offsets it names, in order"""
    out = []
    for item in text.split(","):
        first, _, last = item.partition("-")
        out.extend(range(int(first, 16), int(last or first, 16) + 1))
    return out


def both_copies(text: str) -> str:
    """Byte spans of the main save's first copy, followed by the same spans of its second"""
    spans = [item.partition("-") for item in text.split(",")]
    second = [f"{int(a, 16) + SECOND_COPY:#x}" + (f"-{int(b, 16) + SECOND_COPY:#x}" if b else "") for a, _, b in spans]
    return ",".join([text] + second)


# The mystery gift block (MysteryGiftSaveData, include/save/mystery_gift.h) at this offset of the main save's first
# copy, and its size: the received gifts' flags and the cards, then a checksum of them and the key mgEncryptData
# (src/system/mystery_data.c) encrypted them with, from the VBlank count
MYSTERY_GIFT_BLOCK = 0x1c800
MYSTERY_GIFT_SIZE = 0xa94


def mystery_gift_decrypted(contents: bytes) -> tuple[bytes, list[str]]:
    """A save with the mystery gift block of each copy of its main save decrypted, as mgDecryptData decrypts it
    (src/system/mystery_data.c, with decryptData's generator in src/gfl/gf_standard.c), its checksum and key left as
    the save holds them: two saves whose blocks hold the same contents under other keys then differ only in the key.
    With why a copy's block doesn't decrypt (its contents don't add up to its checksum), for each that doesn't"""
    out = bytearray(contents)
    problems = []
    for copy in (0, SECOND_COPY):
        start = MYSTERY_GIFT_BLOCK + copy
        end = start + MYSTERY_GIFT_SIZE - 4
        if start + MYSTERY_GIFT_SIZE > len(out):
            problems.append(f"the save ends before the mystery gift block at {start:#x}")
            continue
        checksum, key = struct.unpack_from("<HH", out, end)
        seed = checksum + (key << 16)
        for offset in range(start, end, 2):
            seed = (seed * 1103515245 + 24691) & 0xFFFFFFFF
            struct.pack_into("<H", out, offset, struct.unpack_from("<H", out, offset)[0] ^ (seed >> 16))
        if sum(out[start:end]) & 0xFFFF != checksum:
            problems.append(f"the mystery gift block at {start:#x} doesn't add up to its checksum ({checksum:#06x}) "
                            f"decrypted with its key ({key:#06x})")
    return bytes(out), problems


# The catch saves' bytes that differ by construction, the same in both versions (README.md, "Saves")
MAIL_IDS = ("the caught Purrloin's mail's three sentence ids, encrypted with its personality value: "
            "PokeParty_CreatePkm (src/system/poke_tool.c) copies in a mail that CreateMailData allocates from the "
            "system heap without clearing it, and PMSData_Clear (src/system/pms_data.c) sets a sentence's type and "
            "words but not its id, so each id is what the heap held there, the DS's leftovers and the port's")
PLAY_SECONDS = ("the play time's seconds, which each program counts with its own clock's ticks (PLAY_TIME): Black "
                "2's catch saves hold 0:13:11 and 0:13:05, White 2's 0:13:08 and 0:13:02")
SAVE_CRCS = ("the CRCs over those: the party block's and the trainer's, their entries in the table of the blocks' "
             "CRCs, and the footer's CRC of that table (src/gfl/backup_system.c)")
CATCH_SAVE_BYTES = {
    both_copies("0x18fa2-0x18fa3,0x18faa-0x18fab,0x18fb2-0x18fb3"): MAIL_IDS,
    both_copies("0x19427"): PLAY_SECONDS,
    both_copies("0x19336-0x19337,0x194b2-0x194b3,0x25f34-0x25f37,0x25fa2-0x25fa3"): SAVE_CRCS,
}
# The Pokemon Center saves' bytes that differ by construction after Hugh's battle, the same places in both versions
# (README.md, "Saves")
CENTER_PLAY_SECONDS = ("the play time's seconds, which each program counts with its own clock's ticks (PLAY_TIME), "
                       "the DS's some 8 seconds ahead of the port's at the same count by this save in both versions: "
                       "Black 2's saves, made on the same count, hold 0:08:01 on the port and 0:08:09 on the DS; "
                       "White 2's port saves 740 counts (12 seconds of passes) after the DS, past the cut on presses "
                       "of its own, and holds 0:08:10 against the DS's 0:08:06")
MYSTERY_KEY = ("the mystery gift block's key (MysteryGiftSaveData.key, include/save/mystery_gift.h): the Pokemon "
               "Center's deliveryman's scripts (res/field/scripts/global_10395.s, Cmd_01DB, PdwPostman's "
               "func_ov033_02177d78 in src/ov033/pdw_postman.c) read the block as the player enters, and every read "
               "decrypts it and encrypts it again with a key that mgEncryptData (src/system/mystery_data.c) takes "
               "from the VBlank count, (OS_GetVBlankCount() << 8) | OS_GetVBlankCount(): the DS's count runs ahead of "
               "the game's passes (its dropped frames and long loads) and the port's doesn't, so the same contents "
               "come out as other bytes. The trip compares the blocks decrypted, each with its own key "
               "(mystery_gift_decrypted): their contents and checksums are the same, and only the key differs (the "
               "port's 0x7f7f; the DS's 0x7560 in Black 2 and 0xb793 in White 2); the catch saves, made without "
               "entering a Pokemon Center, hold the block as the start save has it")
TURN_TIMER = ("the Pokemon Center's townsperson at (3,15) who turns at random (npc 9 of "
              "res/field/events/aspertia_city_pokemon_center.json, movement code 13, FieldActorMovecode_MoveTurnRandom in "
              "src/ov036/field_actor_movecode.c), the seventh actor of the field actors' block: his direction (+0xd, Black "
              "2's alone) and his turn's timer (+0x32, RailWanderWork.timer in the movement code's work). The timer "
              "counts down at each step of the field's actors, every other pass, and, run out, draws a new wait and "
              "direction from GFL's linear generator; past ADR 0023's cut the port's battle is its own, so it reaches "
              "the Pokemon Center at another count than the DS's (560 counts later in Black 2, 704 in White 2) and "
              "the save finds him at another point of his wait")
CENTER_CRCS = ("the CRCs over those: the trainer block's, the mystery gift block's and the field actors' block's, "
               "their entries in the table of the blocks' CRCs, and the footer's CRC of that table "
               "(src/gfl/backup_system.c)")
CENTER_SAVE_BYTES = {
    both_copies("0x19427"): CENTER_PLAY_SECONDS,
    both_copies("0x1d292-0x1d293"): MYSTERY_KEY,
    both_copies("0x1e3ed,0x1e412"): TURN_TIMER,
    both_copies("0x194b2-0x194b3,0x1d296-0x1d297,0x1f602-0x1f603,0x25f36-0x25f37,0x25f44-0x25f45,0x25f52-0x25f53,"
                "0x25fa2-0x25fa3"): CENTER_CRCS,
}
# The Floccesy route starts from the same examined DS catch save on both programs. All party data, flags and
# decrypted gifts agree; only play-time seconds and the CRCs that cover them differ. No mail or gift bytes omitted.
FLOCCESY_SAVE_BYTES = {
    both_copies("0x19427"): ("the play time's seconds, counted with each program's own clock ticks "
                             "(src/system/playtime_ctrl.c, PLAY_TIME)"),
    both_copies("0x194b2-0x194b3,0x25f36-0x25f37,0x25fa2-0x25fa3"):
        "the trainer block's CRC, its CRC-table entry and the table footer's CRC over those play-time seconds",
}
ROUND_TRIPS = {
    # Phase 4's exit: the wild Purrloin caught on Route 19 and the game saved, then quit, and continued from the save
    "catch": RoundTrip("catch_save", 11600, "catch_continue",
                       known={"b2_us": CATCH_SAVE_BYTES, "w2_us": CATCH_SAVE_BYTES}),
    "floccesy": RoundTrip("floccesy_save", 34600, "floccesy_continue",
                          known={"b2_us": FLOCCESY_SAVE_BYTES, "w2_us": FLOCCESY_SAVE_BYTES}),
    # A trainer battle to its end: Hugh's, from the starter save, held to melonDS frame by frame up to his AI's second
    # move (the hugh sequence, ADR 0023's cut), the port's own past it, to the Pokemon Center's scene and SAVE there.
    # Its save is held to melonDS's byte by byte (Tepig's party data, the money, the bag, the flags and the rest), but
    # the declared bytes; both saves continue on the oracle to the field, and the port's in a new run of its own. In
    # White 2 the port's battle takes more turns, so the port plays presses of its own past the cut
    "hugh": RoundTrip("center_save", 19000, "center_continue",
                      known={"b2_us": CENTER_SAVE_BYTES, "w2_us": CENTER_SAVE_BYTES}, cut="hugh",
                      port_input={"w2_us": "hugh_port"}, continued_last=3602),
}


def join_offsets(offsets: list[int]) -> list[tuple[int, int]]:
    """Byte offsets, in order, as spans (first byte, last byte), spans 8 bytes or less apart joined"""
    spans: list[list[int]] = []
    for i in offsets:
        if spans and i - spans[-1][1] <= 8:
            spans[-1][1] = i
        else:
            spans.append([i, i])
    return [(first, last) for first, last in spans]


def differing_offsets(a: bytes, b: bytes) -> list[int]:
    """The offsets where two saves differ, in order: the bytes both have that differ, then those only the longer has"""
    return [i for i, (x, y) in enumerate(zip(a, b)) if x != y] + list(range(min(len(a), len(b)), max(len(a), len(b))))


def save_differences(a: bytes, b: bytes) -> list[tuple[int, int]]:
    """The spans where two saves differ, as (first byte, last byte), spans 8 bytes or less apart joined"""
    return join_offsets(differing_offsets(a, b))


def undeclared_differences(a: bytes, b: bytes, known: dict[str, str]) -> list[tuple[int, int]]:
    """The spans where two saves differ outside the bytes a round trip declares (RoundTrip.known's for a version)"""
    declared = {offset for text in known for offset in parse_offsets(text)}
    return join_offsets([i for i in differing_offsets(a, b) if i not in declared])


def command_round_trip(args) -> int:
    trip = ROUND_TRIPS[args.name]
    maker = SAVES[trip.maker]
    sequence = SEQUENCES[trip.continued]
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py round-trip: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    rom = rom_path(args.version, args.rom)
    reason = rom_matches(args.version, rom) or save_kept(trip.maker, args.version)
    if reason:
        print(f"oracle.py round-trip: skipped: {reason}")
        return SKIP
    args.oracle = binary
    make_input = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=trip.maker, version=args.version))
    # The port plays the oracle's input file, or its own where its course past the cut takes other passes
    port_input = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=trip.port_input.get(args.version, trip.maker),
                                                               version=args.version))
    continue_input = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=trip.continued, version=args.version))
    cut = SEQUENCES[trip.cut] if trip.cut else None
    cut_input = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=trip.cut, version=args.version)) if cut else None
    for path in (make_input, port_input, continue_input, cut_input):
        if path and not os.path.exists(path):
            print(f"oracle.py round-trip: no input file {os.path.relpath(path, REPO_ROOT)} for {args.version}")
            return 1
    # Up to the cut, the port plays the cut sequence's presses, as the oracle did making its save
    for path in (make_input, port_input) if cut_input else ():
        if not input_starts_with(path, cut_input):
            print(f"oracle.py round-trip: {os.path.relpath(path, REPO_ROOT)} doesn't start with "
                  f"{os.path.relpath(cut_input, REPO_ROOT)}'s lines, so the cut doesn't hold the port's run")
            return 1
    work = args.work or tempfile.mkdtemp(prefix=f"oracle-round-trip-{args.name}-")
    start, reason = obtain_save(args, maker.start, work, args.save_cache)
    if reason is None:
        oracle_save, reason = obtain_save(args, trip.maker, work, args.save_cache)
    if reason:
        print(f"oracle.py round-trip: {reason}")
        return 1
    saves = {"port": os.path.join(work, "port.sav"), "oracle": oracle_save}

    # The oracle's run of the piece up to the cut, from the same start save, with the dumps the cut sequence compares
    # and the random seeds the DS builds, which must be the ones the port's input file gives
    port_dumps = []
    if cut:
        oracle_cut_dir = os.path.join(work, "oracle-cut")
        shutil.rmtree(oracle_cut_dir, ignore_errors=True)
        cut_args = argparse.Namespace(**vars(args))
        cut_args.input = cut_input
        cut_args.save = start
        cut_args.save_out = None
        cut_oracle_dumps, port_dumps = state_dumps(cut)
        status = run_oracle(cut_args, oracle_cut_dir, cut.oracle_frames, "all", cut_oracle_dumps + seeds.DUMPS,
                            quiet=True)
        if status != 0:
            print(f"oracle.py round-trip: the oracle failed on {trip.cut}'s input file (status {status})")
            return 1
        reason = report_seeds(port_input, oracle_cut_dir)
        if reason:
            print(f"oracle.py round-trip: {reason}")
            return 1

    # The port makes its own save, playing the input file the oracle made the kept one with (or its own past the
    # cut), with a picture of each frame up to the cut
    make_dir = os.path.join(work, "port-make")
    status, message = run_port(args.port, make_dir, trip.port_frames, f"1-{cut.port_frames}" if cut else "none", rom,
                               args.time, port_input, timeout=3600, dumps=port_dumps, save=start,
                               save_out=saves["port"])
    described = describe_port_run(status, message, trip.port_frames, make_dir)
    print(f"port, {os.path.basename(port_input)}: {described}")
    if status != 0 or file_sha1(saves["port"]) == file_sha1(start):
        print(f"oracle.py round-trip: the port's run left no save of its own; the runs are in {work}")
        return 1
    failed = False
    if cut:
        print(f"\nthe port's run up to the cut, held as {trip.cut} holds it ({cut.port_frames} port frames):")
        if compare_sequence(cut, args.version, oracle_cut_dir, make_dir, os.path.join(work, "report-cut"),
                            cut.port_frames) != 0:
            failed = True
        print()
    contents = {}
    plain = {}
    for name, path in saves.items():
        with open(path, "rb") as f:
            contents[name] = f.read()
        print(f"{name}'s save: SHA-1 {file_sha1(path)}")
    # A CRC byte allowed to differ is still required to be correct. Validate both copies before masking or
    # booting: the DS can otherwise recover from one good bank and hide a corrupt other bank. Native extents come
    # from the examined DS image, never from potentially corrupt native footers.
    for name in ("oracle", "port"):
        reference = contents["oracle"] if name == "port" else None
        problems = saveimage.validate_image(contents[name], reference=reference)
        if problems:
            for problem in problems:
                print(f"the {name}'s save: {problem}")
            print(f"oracle.py round-trip: FAIL; invalid save image; the runs are in {work}")
            return 1
        print(f"the {name}'s save: both copies' 73 block CRCs, checksum tables and footers valid")
    for name in saves:
        # The mystery gift block is compared by what it holds, each save's decrypted with its own key
        plain[name], problems = mystery_gift_decrypted(contents[name])
        for problem in problems:
            print(f"the {name}'s save: {problem}")
            failed = True
    for label, a, b in (("the saves", contents["port"], contents["oracle"]),
                        ("with their mystery gift blocks decrypted, the saves", plain["port"], plain["oracle"])):
        spans = save_differences(a, b)
        differing = len(differing_offsets(a, b))
        print(f"{label} differ in {differing} bytes, in {len(spans)} spans: "
              + ", ".join(f"{first:#x}-{last:#x}" for first, last in spans))
    # Every byte that differs, the mystery gift blocks decrypted, is one the trip declares, with its cause
    known = trip.known.get(args.version, {})
    for text, why in known.items():
        print(f"may differ, bytes {text}: {why}")
    undeclared = undeclared_differences(plain["port"], plain["oracle"], known)
    if undeclared:
        print("the saves differ where the trip declares nothing: "
              + ", ".join(f"{first:#x}-{last:#x}" for first, last in undeclared))
        failed = True

    # Each save continued on the oracle: the same frames, count and both screens, and neither save written; with the
    # dumps of the game's state the sequence compares
    oracle_dumps, port_dumps = state_dumps(sequence)
    runs = {}
    for name, path in saves.items():
        made = argparse.Namespace(**vars(args))
        made.input = continue_input
        made.save = path
        made.save_out = os.path.join(work, f"{name}-after.sav")
        out = os.path.join(work, f"oracle-{name}")
        shutil.rmtree(out, ignore_errors=True)
        status = run_oracle(made, out, sequence.oracle_frames, "all", oracle_dumps, quiet=True)
        if status != 0 or not os.path.exists(made.save_out):
            print(f"oracle.py round-trip: the oracle failed on the {name}'s save (status {status})")
            return 1
        if file_sha1(made.save_out) != file_sha1(path):
            print(f"the oracle wrote the {name}'s save while continuing it")
            failed = True
        runs[name] = compare.load_run(out)
    same, difference = compare_boots(runs["port"], runs["oracle"], trip.continued_last)
    upto = f" up to count {trip.continued_last}" if trip.continued_last else ""
    print(f"continued on the oracle: {same} of {len(runs['oracle'].frames)} frames the same{upto} (count and both "
          "screens)" + (f"; first difference at {difference}" if difference else ""))
    failed = failed or difference is not None

    # The port continues its own save in a new run, held to the oracle's continuing the same save
    port_dir = os.path.join(work, "port-continue")
    status, message = run_port(args.port, port_dir, sequence.port_frames, "all", rom, args.time, continue_input,
                               dumps=port_dumps, save=saves["port"], save_out=os.path.join(work, "port-after.sav"))
    print(f"port, {trip.continued} from its own save: "
          f"{describe_port_run(status, message, sequence.port_frames, port_dir)}")
    if status != 0:
        return 1
    if file_sha1(os.path.join(work, "port-after.sav")) != file_sha1(saves["port"]):
        print("the port wrote its save while continuing it")
        failed = True
    port_run = compare.load_run(port_dir)
    # Held to the oracle's run on the same save, the port's, which the oracle's run on its own matches as far as
    # compare_boots compared them
    result = compare_sequence(sequence, args.version, os.path.join(work, "oracle-port"), port_dir,
                              os.path.join(work, "report"), max(port_run.frames) if port_run.frames else 0)
    if failed or result != 0:
        print(f"oracle.py round-trip: FAIL; the runs are in {work}")
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


def load_reference(path: str) -> dict[int, tuple[int, int | None, int | None, str, str, str | None]]:
    """<name>_<version>.tsv: for each port frame it keeps, the oracle frame it pairs with, the oracle's frame count
    there, the count lag (None where the frame pairs by its pictures), the oracle's hash of each screen, or "3d" for a
    screen that shows 3D ("known" for a top screen the sequence leaves out), and the hash of the game's state there
    (state_digest), None where the sequence doesn't compare it"""
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
                                   cells[4], cells[5], None if cells[6] == "-" else cells[6])
    return rows


def state_digest(memory: dict[tuple[int, str], tuple[int, bytes]], frame: int,
                 state: tuple[StateDump, ...] = STATE) -> str | None:
    """The SHA-1 of the game's state (a sequence's state, in its order) that a run dumped at a frame, None when it
    dumped none"""
    digest = hashlib.sha1()
    for dump in state:
        if (frame, dump.label) not in memory:
            return None
        digest.update(memory[(frame, dump.label)][1])
    return digest.hexdigest()


def compared_state_frames(sequence: Sequence, version: str, random: bool = False) -> list[int]:
    """The port frames whose game state the sequence compares: its state_frames but its known_state; with random,
    those whose GFL random state it compares, but its known_random too"""
    if not sequence.state_frames:
        return []
    known_state = by_frame(sequence.known_state.get(version, {}))
    if random:
        known_state.update(by_frame(sequence.known_random.get(version, {})))
    return [frame for frame in compare.parse_frames(sequence.state_frames) if frame not in known_state]


def write_reference(path: str, name: str, version: str, oracle_run: compare.Run, port_run: compare.Run) -> int:
    """Writes <name>_<version>.tsv from a comparison's runs: for the sampled port frames that aren't known differences,
    the oracle frame the comparison pairs each with. Where the game counts passes that is the oracle frame at the
    port's count less the lag, which comes from the oracle and the declared lags; in a clock stretch it is the oracle
    frame that shows the port frame's pictures. A row whose frame the sequence compares the game's state at has the
    oracle's state there, which the comparison found the port's. Returns the number of rows"""
    sequence = SEQUENCES[name]
    clock = set(compare.parse_frames(sequence.clock))
    pairing = compare.counter_mapping(oracle_run, port_run, clock, compare.parse_lags(sequence.lags.get(version)))
    known = by_frame(sequence.known.get(version, {}))
    known_top = by_frame(sequence.known_top.get(version, {}))
    with_state = set(compared_state_frames(sequence, version, random=True))
    oracle_memory = compare.load_memory(oracle_run) if with_state else {}
    # Where the frames pair on the count, the lag between them
    lags = {}
    for frame, target in pairing.mapping.items():
        mine, theirs = port_run.frames.get(frame), oracle_run.frames.get(target)
        if frame not in clock and mine and theirs and None not in (mine.counter, theirs.counter):
            lags[frame] = mine.counter - theirs.counter
    melonds = oracle_run.info.get("melonds", "?")
    labels = ", ".join(dump.label for dump in sequence.state)
    count = 0
    with open(path, "w") as f:
        f.write(f"# pbw-oracle's screens on {SEQUENCE_INPUT.format(name=name, version=version)} (melonDS {melonds}),\n"
                "# at the oracle frames that compare.py --align counter pairs with the port frames listed: where the\n"
                "# game counts passes (lag is a number), the oracle frame whose count is the port's less the lag; in\n"
                f"# a clock stretch (port frames {sequence.clock}, lag -), the one that shows the port frame's\n"
                "# pictures. The SHA-1 of each screen's RGB bytes, or 3d for a screen that shows 3D, which only the\n"
                "# oracle comparison can judge (known for a top screen left out), GFL's frame count there, and the\n"
                f"# SHA-1 of the game's state ({labels}) where the sequence compares\n"
                "# it (- elsewhere). Hashes only. Written by oracle.py\n"
                f"# sequence {name} --update; checked by oracle.py sequence-check {name}.\n")
        for frames, why in sequence.known.get(version, {}).items():
            f.write(f"# Left out, port frames {frames}: {why}\n")
        for frames, why in sequence.known_top.get(version, {}).items():
            f.write(f"# Top screen left out, port frames {frames}: {why}\n")
        f.write("\t".join(REFERENCE_COLUMNS) + "\n")
        for frame in compare.parse_frames(sequence.sample):
            target = pairing.mapping.get(frame)
            row = oracle_run.frames.get(target) if target is not None else None
            if frame in known or row is None:
                continue
            counter = "-" if row.counter is None else row.counter
            lag_cell = lags.get(frame, "-")
            top = "3d" if row.top_3d else "known" if frame in known_top else row.top
            bottom = "3d" if row.bottom_3d else row.bottom
            state = (state_digest(oracle_memory, target, sequence.state) if frame in with_state else None) or "-"
            f.write(f"{frame}\t{target}\t{counter}\t{lag_cell}\t{top}\t{bottom}\t{state}\n")
            count += 1
    return count


def sequence_paths(args) -> tuple[str, str, str]:
    """The sequence's input file (its own, or the one of the sequence it names) and reference for the version, and the
    ROM"""
    played = SEQUENCES[args.name].input or args.name
    return (os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=played, version=args.version)),
            os.path.join(REPO_ROOT, SEQUENCE_REFERENCE.format(name=args.name, version=args.version)),
            rom_path(args.version, args.rom))


def input_seeds(path: str) -> list[int]:
    """An input file's "seed" lines, in order (pbwinput.h): the seeds a DS built, which the port's game takes in place
    of its own"""
    found = []
    with open(path) as f:
        for line in f:
            words = line.split("#", 1)[0].split()
            if len(words) == 2 and words[0] == "seed":
                found.append(int(words[1], 16))
    return found


def check_seeds(given: list[int], built: list[tuple[int, int, str]]) -> str | None:
    """None when an input file's seeds are every seed the oracle's DS built in its run, in order, or when the file
    gives none (the port then builds its own, and random outcomes differ by construction); else why they aren't"""
    if not given or given == [seed for seed, _, _ in built]:
        return None
    lines = "; ".join(f"{seed:#018x} ({kind}, oracle frame {frame})" for seed, frame, kind in built) or "none"
    return (f"the input file gives the seeds {', '.join(f'{seed:#018x}' for seed in given)}, and the DS built "
            f"{lines}: write the DS's (oracle.py seeds)")


def report_seeds(input_path: str, oracle_dir: str) -> str | None:
    """Prints the seeds the oracle's DS built in a run that dumped seeds.DUMPS and those the input file gives; returns
    check_seeds' answer"""
    built = seeds.ds_seeds(compare.load_memory(compare.load_run(oracle_dir)))
    given = input_seeds(input_path)
    for seed, frame, kind in built:
        print(f"the DS built the seed {seed:#018x} ({kind}, read at oracle frame {frame})")
    if not given:
        print("the input file gives no seeds: the port builds its own, so random outcomes differ from the DS's")
    return check_seeds(given, built)


def stop_allowed(sequence: Sequence, status: int | None, last: int, message: str) -> bool:
    """Whether a port run that didn't end with status 0 ended as the sequence allows: on the trap stub it names (code
    or data with no C yet), its message matching stop_trap, after the frame from which the sequence allows it"""
    return (status == TRAP_STATUS and sequence.stop_from > 0 and last >= sequence.stop_from
            and bool(sequence.stop_trap) and re.search(sequence.stop_trap, message) is not None)


def state_dumps(sequence: Sequence) -> tuple[list[str], list[str]]:
    """The dumps of the game's state a sequence compares: pbw-oracle's --dump arguments at its state_oracle frames and
    the port's --run-dump arguments at its state_frames; none when it compares none"""
    if not sequence.state_frames:
        return [], []
    return ([dump.spec("oracle", sequence.state_oracle) for dump in sequence.state],
            [dump.spec("port", sequence.state_frames) for dump in sequence.state])


def compare_sequence(sequence: Sequence, version: str, oracle_dir: str, port_dir: str, report: str, last: int) -> int:
    """Compares a sequence's oracle and port runs up to the port's frame last, as oracle.py sequence does: every port
    frame but the known differences, on the game's count, and the game's state where the sequence compares it; prints
    what it compared and left out, and returns compare.py's status"""
    known = by_frame(sequence.known.get(version, {}))
    compared = [frame for frame in range(1, last + 1) if frame not in known]
    align = ["--align", "counter", "--clock", sequence.clock]
    if sequence.lags.get(version):
        align += ["--lag", sequence.lags[version]]
    command = ["screens", oracle_dir, port_dir, *align, "--brief", "--frames", ",".join(str(f) for f in compared),
               "--report", report]
    known_oracle = sequence.known_oracle.get(version, {})
    if known_oracle:
        command += ["--known-oracle", ",".join(known_oracle)]
    close = sequence.close.get(version, {})
    if close:
        command += ["--close", ",".join(close)]
    known_top = sequence.known_top.get(version, {})
    if known_top:
        command += ["--known-top", ",".join(known_top)]
    result = compare.main(command)
    for frames, why in sequence.known.get(version, {}).items():
        print(f"left out, port frames {frames}: {why}")
    for frames, why in known_top.items():
        print(f"top screen left out, port frames {frames}: {why}")
    for frames, why in known_oracle.items():
        print(f"no counterpart, oracle frames {frames}: {why}")
    for frames, why in close.items():
        print(f"held close, port frames {frames}: {why}")
    if sequence.state_frames:
        labels = ", ".join(dump.label for dump in sequence.state)
        print(f"\nthe game's state ({labels}) at port frames {sequence.state_frames}, on the count:")
        random = {dump.label for dump in RANDOM_STATE}
        # GFL's random state where the sequence compares it, the rest of the state where it compares that
        for parts, frames in ((random, compared_state_frames(sequence, version, random=True)),
                              ({dump.label for dump in sequence.state} - random,
                               compared_state_frames(sequence, version))):
            parts = [dump.label for dump in sequence.state if dump.label in parts]
            if parts and frames:
                result = max(result, compare.main(["memory", oracle_dir, port_dir, *align, "--labels", ",".join(parts),
                                                   "--frames", ",".join(str(f) for f in frames)]))
        for frames, why in sequence.known_state.get(version, {}).items():
            print(f"state left out, port frames {frames}: {why}")
        for frames, why in sequence.known_random.get(version, {}).items():
            print(f"GFL's random state left out, port frames {frames}: {why}")
    return result


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
    oracle_dumps, port_dumps = state_dumps(sequence)
    status = run_oracle(args, oracle_dir, sequence.oracle_frames, "all", oracle_dumps + seeds.DUMPS, quiet=True)
    if status != 0:
        print(f"oracle.py sequence: the oracle failed (status {status})")
        return 1
    # The random seeds the input file gives the port must be the ones the DS built here
    reason = report_seeds(input_path, oracle_dir)
    if reason:
        print(f"oracle.py sequence: {reason}")
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
    result = compare_sequence(sequence, args.version, oracle_dir, port_dir, os.path.join(work, "report"), last)
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


def command_seeds(args) -> int:
    """The seeds the DS builds playing an input file, a sequence's or a save's, from the save it starts from: printed as
    the file's "seed" lines, and checked against the lines the file has"""
    binary = args.oracle or DEFAULT_ORACLE
    if not os.path.exists(binary):
        print(f"oracle.py seeds: skipped: no oracle at {binary} (configure with -DPORT_BUILD_ORACLE=ON)")
        return SKIP
    reason = rom_matches(args.version, rom_path(args.version, args.rom))
    if reason:
        print(f"oracle.py seeds: skipped: {reason}")
        return SKIP
    if args.name in SEQUENCES:
        sequence = SEQUENCES[args.name]
        played, start, frames = sequence.input or args.name, sequence.save, sequence.oracle_frames
    else:
        played, start, frames = args.name, SAVES[args.name].start, SAVES[args.name].frames
    input_path = os.path.join(REPO_ROOT, SEQUENCE_INPUT.format(name=played, version=args.version))
    if not os.path.exists(input_path):
        print(f"oracle.py seeds: no input file {os.path.relpath(input_path, REPO_ROOT)} for {args.version}")
        return 1
    args.oracle = binary
    work = args.work or tempfile.mkdtemp(prefix=f"oracle-seeds-{args.name}-")
    args.save = None
    if start:
        args.save, reason = obtain_save(args, start, work, args.save_cache)
        if reason:
            print(f"oracle.py seeds: {start}: {reason}")
            return 1
    args.input = input_path
    oracle_dir = os.path.join(work, "oracle")
    shutil.rmtree(oracle_dir, ignore_errors=True)
    status = run_oracle(args, oracle_dir, args.frames or frames, "none", seeds.DUMPS, quiet=True)
    if status != 0:
        print(f"oracle.py seeds: the oracle failed (status {status})")
        return 1
    reason = report_seeds(input_path, oracle_dir)
    print(f"\n{os.path.relpath(input_path, REPO_ROOT)}'s lines for them:")
    for seed, frame, kind in seeds.ds_seeds(compare.load_memory(compare.load_run(oracle_dir))):
        print(f"seed {seed:#018x}  # {'GFL' if kind == 'start-up' else 'a battle'}'s, oracle frame {frame}")
    if not args.work:
        shutil.rmtree(work, ignore_errors=True)
    if reason:
        print(f"oracle.py seeds: {reason}")
        return 1
    return 0


def check_reference(rows: dict[int, tuple[int, int | None, int | None, str, str, str | None]], port_run: compare.Run,
                    parts: tuple[StateDump, ...] = STATE) -> tuple[list[str], int, int, int, int]:
    """A port run against a reference's rows: what fails, and how many screens without 3D were compared, how many
    with 3D (or left out) were left to the oracle comparison, how many rows pair on the count, and how many states were
    compared (the run's memory.txt has the port's state at the rows that have one)"""
    failures = []
    compared = left = timed = states = 0
    memory = compare.load_memory(port_run) if any(row[5] for row in rows.values()) else {}
    for frame, (oracle_frame, counter, lag, top, bottom, state) in sorted(rows.items()):
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
            if wanted in ("3d", "known"):
                left += 1
                continue
            compared += 1
            if wanted != have:
                failures.append(f"port frame {frame}'s {name} screen isn't oracle frame {oracle_frame}'s")
        if state is not None:
            states += 1
            have = state_digest(memory, frame, parts)
            if have is None:
                failures.append(f"port frame {frame} has no dump of the game's state")
            elif have != state:
                failures.append(f"port frame {frame}'s game state isn't oracle frame {oracle_frame}'s")
    return failures, compared, left, timed, states


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
    with_state = ",".join(str(frame) for frame, row in sorted(rows.items()) if row[5])
    dumps = [dump.spec("port", with_state) for dump in sequence.state] if with_state else []
    status, message = run_port(args.port, work, sequence.port_frames, "none", rom, args.time, input_path, dumps=dumps)
    print(f"port: {describe_port_run(status, message, sequence.port_frames, work)}")
    if status != 0:
        return 1
    failures, compared, left, timed, states = check_reference(rows, compare.load_run(work), sequence.state)
    for line in failures:
        print(f"FAIL: {line}")
    print(f"{len(rows)} frames, {timed} of them on the count: {len(failures)} failures; {compared} screens without 3D "
          f"compared, {left} left to the oracle comparison (with 3D, or left out), the game's state at {states} "
          f"frames (oracle.py sequence {args.name})")
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

    round_trip = commands.add_parser("round-trip", help="a save both programs make, each continued in a new run "
                                     "of the other and of itself")
    round_trip.add_argument("name", choices=sorted(ROUND_TRIPS))
    common(round_trip)
    round_trip.add_argument("--port", required=True, help="the port's program")
    round_trip.add_argument("--save-cache", help="take the saves from this directory, or make them there")
    round_trip.add_argument("--work", help="keep the runs and the saves here")

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

    seeds_parser = commands.add_parser("seeds", help="the random seeds the DS builds playing a sequence's or a save's "
                                       "input file, as the file's seed lines")
    seeds_parser.add_argument("name", choices=sorted(set(SEQUENCES) | set(SAVES)))
    common(seeds_parser)
    seeds_parser.add_argument("--frames", type=int, help="how far the oracle runs (the sequence's or the save's frames "
                              "by default)")
    seeds_parser.add_argument("--save-cache", help="take the save the input file starts from from this directory, or "
                              "make it there (oracle.py save --save-cache)")
    seeds_parser.add_argument("--work", help="keep the oracle's run here")

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
        if args.command == "round-trip":
            return command_round_trip(args)
        if args.command == "boot":
            return command_boot(args)
        if args.command == "sequence":
            return command_sequence(args)
        if args.command == "seeds":
            return command_seeds(args)
        if args.command == "sequence-check":
            return command_sequence_check(args)
        if args.command == "sound":
            return command_sound(args)
        if args.command == "sound-check":
            return command_sound_check(args)
        return command_check(args)
    except (compare.RunError, OSError, ValueError) as error:
        print(f"oracle.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
