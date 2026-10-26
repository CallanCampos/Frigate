# The melonDS oracle

The reference the port is held against ([PLAN.md §6](../../docs/PLAN.md#6-verification), test level T4): the player's
ROM running on [melonDS](https://melonds.kuribo64.net)'s emulation core, headless and deterministic, with an input
file, writing both screens and named memory at chosen frames, and the sound: the mixer's output sample by sample and
the ARM7 sound driver's state after each run of its thread. `compare.py` compares the screens and memory with the
port's, `sound.py` the sound ([Sound](#sound)). It also makes the saves the tests start from ([Saves](#saves)).

It is a **test tool only**. It is never linked into or shipped with Frigate
([PLAN.md §1](../../docs/PLAN.md#1-goal-and-what-native-means)): it is a CMake project of its own, built only on
request, and nothing under `port/` other than this directory, its tests and its CMake fragment refers to it, except two
files of plain C11 with no melonDS in them. The input file reader (`pbwinput.c`) is compiled into Frigate for its
replays (`port/app/replay.c`). The sound trace's format (`sndtrace.h`) is included by Frigate's sound driver, which
writes its work in the DS's layout with it (`port/hal/snd7/snd7.c`, `port_snd7_trace_record`), by Frigate's
`--sound-trace` (`port/app/sound_trace.c`) and by the sound driver's tests (`port/tests/snd/test_snd.c`).
melonDS is GPL-3.0, as this repository is; the oracle links melonDS's core and is GPL-3.0 with it. melonDS is fetched
at build time, pinned to a release, and never committed.

| File | What it is |
| --- | --- |
| `oracle.cpp`, `oracle.h`, `platform.cpp`, `symtab.*`, `pngout.*` | `pbw-oracle`, the program on melonDS's core: `oracle.cpp` its options and run, `oracle.h` what melonDS's Platform layer (`platform.cpp`) reports to it, `symtab.*` the decomp's symbols for its memory dumps, `pngout.*` the screens' PNG files |
| `display3d.c`, `display3d.h` | which screens show 3D in a frame, from the display registers; plain C11 |
| `pbwinput.c`, `pbwinput.h` | the input file reader and writer and the game-frame clock, plain C11, which the port can compile as it is |
| `oracle.py` | runs the oracle and the port with the repository's defaults; makes the saves the tests start from on the oracle, each from the save before it ([Saves](#saves)); the comparison sequences, the field menu's and its apps' among them, and the sound's comparison; the ctest check |
| `compare.py` | compares two runs: screens, memory, alignment |
| `sound.py` | compares two runs' sound: the sound driver's state run by run, what the ARM9 sees of it frame by frame, and the sound by segment ([Sound](#sound)) |
| `sndtrace.h` | the sound trace's format, plain C11, which the port's sound driver (`port/hal/snd7/snd7.c`), its `--sound-trace` (`port/app/sound_trace.c`) and the sound driver's tests include |
| `seeds.py` | the random seeds the DS built in an oracle run, read from the memory it dumps ([Random seeds](#random-seeds)) |
| `symbols.py` | the decomp's symbols, statics included, for memory dumps |
| `melonds.cmake` | the pinned melonDS release (tag and SHA-256) |
| `CMakeLists.txt` | the oracle's own build |

Tests are in [port/tests/oracle](../../tests/oracle) and the build rules in
[port/cmake/oracle.cmake](../../cmake/oracle.cmake).

## Building

```sh
cmake -S port -B build/port-oracle -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT_BUILD_ORACLE=ON
cmake --build build/port-oracle --target port_oracle      # build/port-oracle/oracle/pbw-oracle
ctest --test-dir build/port-oracle -R '^oracle_' --output-on-failure
```

`PORT_BUILD_ORACLE` is off by default, so normal builds and CI download nothing. When it is on, configuring fetches the
release that `melonds.cmake` pins into `<build>/_deps`, and the build compiles melonDS's core (no frontend, no JIT, no
OpenGL, no GDB stub) and the oracle, as an external project in `<build>/oracle`. `-DPORT_ORACLE_SANITIZE=ON` builds the
oracle's own sources with AddressSanitizer and UndefinedBehaviorSanitizer. melonDS's core compiles with `-fwrapv` and
hands it on to what links it; the oracle's build keeps it for the core's sources only, so that the sanitizer checks the
oracle's signed arithmetic. The project also builds on its own, fetching
melonDS itself: `cmake -S port/tools/oracle -B build/oracle-standalone -G Ninja`.

No Nintendo BIOS or firmware file is needed: the oracle uses melonDS's FreeBIOS, its generated firmware and direct boot,
which melonDS 1.1 requires with FreeBIOS. Without the ARM7 BIOS's key table, melonDS encrypts a dump's decrypted secure
area and decrypts it again with the same (empty) table, so the decomp's dump boots; the oracle stops with a message if
melonDS would boot through the firmware instead.

## Running

```sh
oracle.py run --out build/oracle-runs/boot --frames 1400 --screens all
oracle.py port --binary build/port/frigate-black2 --out build/oracle-runs/port --frames 1,30,60,120,300,600,1000
compare.py screens build/oracle-runs/boot build/oracle-runs/port --align counter --clock 313-1000 \
    --report build/oracle-runs/report
oracle.py boot --out build/oracle-runs/both --port build/port/frigate-black2
oracle.py sequence title --port build/port/frigate-black2 --out build/oracle-runs/title
oracle.py sequence-check title --port build/port/frigate-black2
oracle.py save first_save --out build/oracle-runs/first_save.sav
oracle.py sequence continue --port build/port/frigate-black2 --out build/oracle-runs/continue
oracle.py seeds first_save
oracle.py sound opening --port build/port/frigate-black2 --out build/oracle-runs/sound
oracle.py sound-check title --port build/port/frigate-black2
```

`oracle.py run` calls `pbw-oracle` with the version's ROM (`orig/baserom_b2_us.nds`), its symbols (`config/b2_us/arm9`),
the statics of the ROM build's link map and GFL's frame count as the counter. It passes `--time` only when it is given
one, so that an input file's `time` line applies, else the default clock. `oracle.py port` runs the port once, from
power-on with a blank save (unless `--save` gives one), to the last frame of its list, and has it write its own run
directory (`--run-dir`, [below](#run-directories)) with a picture of each frame of the list (`--run-screens`) and the
memory its `--dump`s name (`--run-dump`); its clock is `--time`, else `--input`'s file's, else the default. A run that
ends early (a trap stub, a crash) is reported with the port's message, and the frames it didn't reach have no picture.
`oracle.py boot` does both for the boot sequence's frames, the oracle 400 frames further with a PNG of every frame, and
compares them on the count, the movies by their pictures, at 6 bits and at 5. `oracle.py sequence` and `sequence-check`
are the comparison sequences and their check without the oracle ([below](#the-comparison-sequences)). Both programs take
`--save FILE`, the save memory at power-on, and `--save-out FILE`, where to write it at the end; the port's run gets a
new save directory with FILE in it under the ROM's game code (`IREO.sav`, `IRDO.sav`), as the port names its saves, and
its `--save-out` is the blank chip (524,288 bytes of 0xff) when the game neither had a save nor wrote one, never a file
left by an earlier run. `oracle.py save NAME` makes one of the saves the sequences start from on the oracle, and
`oracle.py port-save` boots the port's own save on it ([below](#saves)). `oracle.py seeds NAME` prints the random seeds
the DS builds playing a sequence's or a save's input file, as the file's `seed` lines ([below](#random-seeds)).
`pbw-oracle --help` and the scripts' `-h` list every option.

`pbw-oracle`'s options, which `oracle.py run` fills in:

| Option | Meaning |
| --- | --- |
| `--rom PATH`, `--out DIR`, `--frames N` | the ROM, the run directory, how many frames to run |
| `--input FILE` | an input file (below); without one nothing is held. One on game frames needs `--counter`, and its `version` must be the ROM's (status 2 otherwise) |
| `--screens LIST` | the frames to write as PNG: `N`, `A-B`, `A-B/STEP`, `all` or `none`, comma-separated |
| `--dump [LABEL=][*]TARGET[:SIZE][@LIST]` | memory to write to `memory.txt` at the frames of LIST (the last frame by default); with `*`, SIZE bytes from where the 32-bit pointer at TARGET points at that frame, plus TARGET's `+OFFSET`, as the port's `--run-dump` takes it (none at a frame where the pointer leads outside the memory a dump reads, a NULL one) |
| `--config DIR`, `--symbols FILE` | where symbols come from (below) |
| `--counter TARGET` | a 32-bit counter recorded for every frame: GFL's frame count, for alignment and game frames |
| `--time "YYYY-MM-DD HH:MM:SS"` | the clock at power-on; else the input file's; else `2012-10-07 12:00:00` |
| `--save FILE`, `--save-out FILE` | the save memory at power-on (blank otherwise), and where to write it at the end |
| `--audio-dump FILE` | the sound unit's mixed output as a WAV file: every sample melonDS's mixer makes, one each 1024 cycles from power-on, 16-bit stereo, before melonDS resamples it for a sound card ([Sound](#sound)) |
| `--sound-trace FILE` | the ARM7 sound driver's state after each run of its thread (`sndtrace.h`, [Sound](#sound)); needs `--config` |

A dump's TARGET is a symbol (`sVBlankCount`), `MODULE/NAME` or `FILE.c/NAME` for a name several modules or files have
(`ov033/sWork`, `gfl_use.c/sWork`), with an optional `+OFFSET`, or an address (`0x0209da64`). Dumps read the ARM9's view
of memory without the bus's side effects: the ITCM and DTCM where CP15 maps them, main RAM, the shared WRAM, palettes,
VRAM and OAM; never I/O. An overlay's symbol reads whatever is loaded at its address at that frame.

## Frames

Both programs count video frames from their start: the oracle from direct boot's jump to the ARM9's entry, the port
from the game's `main()`. **Frame N ends with the N-th VBlank** (the start of line 192):

- **Screens of frame N** are lines 0–191 as drawn before VBlank N: the oracle's framebuffer after that frame, the port's
  `--screenshot` after `--frames N`.
- **State at frame N** (memory dumps, the counter, `frames.tsv`'s `vblank` and the display registers) is taken at
  VBlank N, after the hardware raised the interrupt and before either CPU takes it: the frame's lines are drawn and the
  game waits in `OS_WaitIrq`. The port's equivalent is its frame hook (`port_os_add_frame_hook`), which runs at the same
  point. The oracle gets there with an event in melonDS's scheduler at line 192 of each frame, after the LCD's own
  event; it borrows the slot of a DSi-only device that a DS-mode console never registers (`Event_DSi_DSPHLE`, checked
  against the pinned release). The same slot first serves a probe at line 96, which sees whether the capture unit is
  writing this frame (it latches DISPCAPCNT's enable at line 0 and clears it at line 192).
- **Input** is applied at a VBlank too, so it is what the game's main loop reads after it (`GCTX_HIDUpdate` is the first
  thing the loop does), and an input file's frames are **game frames** unless it says otherwise ([below](#input-files)):
  at each VBlank the oracle applies the state of the game frame whose pass reads the keys next, from GFL's frame count
  (`frames.tsv`'s `game_frame`). The state of frame 1 also holds from power-on.

### Aligning the oracle with the port

The same frame number doesn't show the same thing on both. On the DS, direct boot first runs the ARM9's start-up code,
which unpacks the compressed ARM9 binary, and `GameInit` (the save, the sound data) on a timed CPU: the main loop
starts at frame 242 of a boot without input or save, where the port, whose CPU takes no time, starts it at its first
frames. The DS also drops frames under load: through the Game Freak logo movie, the main loop runs 116 times in the 400
frames from 600 to 1000.

So frames are aligned on the **game's own frame count**: GFL's `sVBlankCount` (`src/system/gfl_use.c`), which
`GFL_VBlankUpdate` increments once per pass of the main loop, after its VBlank wait. Both programs record it for every
frame (`frames.tsv`'s `counter`). While the game counts passes, a moment of the game is a count on both programs:
from power-on, and again after a press that ends a wait ([below](#game-frames-and-video-frames)). Where the game waits
on a clock (the movies), the DS makes fewer, longer passes than the port and the counts part, so there frames pair by
their pictures. The alignments:

- `--align counter`, the one the comparisons use: each port frame pairs with the oracle frame where the count is the
  port frame's less the **count lag**, 0 from power-on (the first of them where a pass takes the DS several frames). A
  port that shows a picture a pass late or early, or for a pass more or fewer, fails there. `--lag FRAME=N,...`
  declares the lag from a port frame on (a press that ends a clock's wait reaches both on the same pass, so the lag is
  0 again after it, or a known difference moves the oracle a pass).
  `--clock LIST` names the port frames where the game waits on a clock; they pair by their pictures, as `sequence`
  below does, and after them the lag is the declared one, else the one where the next frame is expected after the end
  of the last picture on each (the DS shows a still for longer while it loads or waits). A declared lag right after a
  clock lines the counts up again: what the oracle showed between the last pair and the next isn't checked, and
  compare.py says which frames those are.
- `--align sequence`: each port frame, in order, pairs with an oracle frame from the previous pair's on that shows the
  same pictures, compared at the palettes' depth (5 bits a channel) when both runs have a picture of every frame, else
  by `frames.tsv`'s hashes; of those, the one nearest to where the port frame is expected, as many game frames on as
  it is (by the counter, else by frames), from the end of the previous picture on each when the port's picture
  changes. A port frame that no oracle frame shows (a screen with 3D, or a difference) pairs with where it is
  expected, and is noted "extrapolated". It shows that the port shows the oracle's pictures in the same order,
  whatever the timing: a picture held too long, or shown late, passes.
- `--align first-change`: the offset between the first frames whose screens aren't blank (all but a line's worth of
  pixels one colour). It holds until the first frame the DS drops, and it needs every frame up to those in both runs.
- `--align image`: the offset at which most of the port's frames show exactly the screens of an oracle frame, from
  `frames.tsv`'s hashes, which the oracle writes for every frame (`--screens` only decides which get a PNG).

With `counter` or `sequence`, which pair every port frame, `compare.py screens` also checks that **no picture is
missing**: every oracle frame between the frames that two consecutive port frames pair with shows the screens of one of
them. The frames the DS repeats while a pass or a movie frame lasts do; a picture the port skips, or a frame the DS
tore by writing a register partway through it, doesn't, and fails the run unless `--known-oracle` lists it.

`compare.py align ORACLE PORT` reports the first two offsets and, for each port frame, the oracle frames that show
exactly its screens.

### Game frames and video frames

What the boot's frame offset is, and how it maps game frames, as Black 2 shows it (no input or the title sequence's,
blank save; `frames.tsv` of both programs):

- **Start-up.** The oracle's first 241 frames are direct boot's start-up on a timed CPU: the ARM9's start-up code
  (unpacking the compressed ARM9 binary) and `GameInit` (the save, the sound data). The count moves from frame 242 on
  the oracle (count 1) and from frame 2 on the port. The DS's passes at counts 2 and 4 take two frames each (oracle
  frames 243-244 and 246-247), so oracle frame = port frame + 240 at count 1 and + 242 from count 5 (oracle frame 248,
  port frame 6) on; every screen until then is the white of a display that is off. Through the boot screens and the
  copyright screen every pass is a frame on both, with the same count and the same pictures: game frame N is video
  frame N + 1 on the port and N + 243 on the oracle (its pass runs during that frame).
- **Loading and clocks.** From the black after the copyright screen (port frame 313, count 312) the game waits on
  clocks, and the counts part: that black lasts 47 passes on the DS and 55 on the port, and the logo and the opening
  (`src/ov264/cdemo_control.c`) show a new movie frame each 1/15 s of `OS_GetTick`, where on the DS a pass that
  decompresses one takes 3 to 5 frames, on the port 1. The oracle and the port show the same movie frames in the same
  order, the port each for 4 frames (3 when its 1/15 s add up to one more), the DS for 4 or 5, falling behind the clock
  while its passes take 5 and catching up with passes of 3 (oracle frame = port frame + 253 to + 327 through the
  movies, by their pictures): the logo's first frame is oracle 621 (count 359) and port 368 (count 367), the opening's
  1283 (633) and 1018 (1017), the title's 3842 (1349) and 3527 (3526). Video frames don't map, and game frames don't
  either once a movie has run. From the title on the game counts passes again, with the counts 2,177 apart (Black 2;
  2,154 in White 2).
- **Presses line them up again.** Input on game frames reaches both on the same pass. A press that ends a stretch on a
  clock ends it on the same pass, and what follows counts passes again, at the same counts: START at game frame 1100,
  in the opening on both, skips it, which holds its frame until count 1107 on both (port frame 1108), and the title
  starts at count 1115 on both; from count 1108 on the frames pair on the counter, oracle = port + 1795 in the black
  and + 1841 from the title (the DS loads it in one pass of 46 frames), but where the DS drops a frame. The press
  reaches the movie at different points, though: 83 frames into the opening on the port, about 1,610 on the DS
  (oracle frame 2892, count 1099), so the oracle frames between (1365 to 2903 in Black 2) have no counterpart.

**START held through the opening doesn't skip it, on melonDS or on the port.** The movies and the logos skip on a press
of A, B or START (`CDemo_Main`, `GCTX_HIDGetPressedKeys`), and GFL reports a button as pressed only on the pass where it
goes down: held from game frame 400, START skips the Game Freak logo at count 408 on both, then both play the whole
opening with START still held (to oracle frame 3230, port 2873) and show the title at count 1134 and 2928; held from
power-on it skips nothing, its press falling on a pass before the logos read it. START pressed in the opening (down at
game frame 1100) skips it to the title on both, as above.

## Input files

A text file, the same for the oracle and the port's replays. `pbwinput.c` reads and writes it; it has no dependency, so
the port can compile it as it is.

```
pbwinput 1
# A comment runs from # to the end of the line; blank lines are ignored
time 2012-10-07 12:00:00      # optional: the clock at power-on
version b2_us                 # optional: b2_us or w2_us
frames game                   # optional: game (the default) or video
clock pinned                  # optional: running (the default) or pinned
seed 0xea56a175a2fffc8a       # optional, repeated: the random seeds the DS built, in order
# FRAME BUTTONS [TOUCH]
120 A                         # A held from frame 120
122 -                         # nothing held from frame 122
300 START+A
400 - 128,96                  # the pen down at x 128, y 96 of the bottom screen
402 -
900 LID                       # the lid closed
```

- The first line that isn't blank or a comment is `pbwinput 1`, the format's version.
- `time`, `version`, `frames` and `clock` come before the first frame line, once each, and so do the `seed` lines.
  `time` is `YYYY-MM-DD HH:MM:SS` from 2000 to 2099; the oracle's `--time` overrides it. `version` is checked against
  the ROM's game code (`IREO` for `b2_us`, `IRDO` for `w2_us`).
- `clock` says whether the real-time clock runs on from `time`: **`running`**, the default, as a console's does, or
  **`pinned`**, at `time` for the whole run. A running clock moves with each program's video frames, and the DS takes
  more of them than the port to reach the same count (its movies, its loads), so a screen that shows the clock, such as
  the field menu's bottom screen, shows another time on each at the same count: some 34 seconds apart once the field
  has opened, which turns the colon on or off (it shows on odd seconds, `src/system/infowin.c`) and the minute over on
  different frames. Pinned, both show the same time, so the comparison can hold that screen exact. The oracle stops
  melonDS's clock chip timer, which counts the seconds on, and the port's clock stays at its fixed time
  (`port/hal/rtc/rtc7.c`); the game never uses the chip's interrupts, which the timer also raises. Only the date and
  time stop: the play time counts the CPU's ticks (`src/system/playtime_ctrl.c`), and goes on as before.
- `seed` lines give the **random seeds the DS built**, in the order it built them, each a 64-bit number in
  hexadecimal (`0x` and 1 to 16 digits), up to 64 of them. The game builds a seed from the console's entropy at
  start-up, for GFL's random numbers, which the field draws from (a townsperson's walk, a wild Pokémon), and at each
  battle's start, for the battle's (its damage rolls, critical hits, a Poké Ball's shakes): `buildSeed` in
  `src/gfl/gf_standard.c`, which hashes the console's timers, the line being drawn, the VBlank count and the geometry
  engine's status as the call finds them, and 20 bytes of whatever the DS's stack held. A host can't have any of
  those as the DS has them (its CPU takes no time, and its stack holds other things), so the port builds other seeds,
  and every random outcome differs from the DS's (port/docs/decomp-findings.md). The port's game takes the file's
  seeds in place of the ones it builds, the n-th line for the n-th seed, and builds its own after the last
  (port/hal/gfl/random_seed.h); its run directory's `run.txt` lists the seeds it used. melonDS builds the DS's own, so
  the oracle reads the lines only as part of the file. With the DS's seeds the two programs draw the same random
  numbers, and a battle or a wild encounter plays as the DS plays it. A file's seeds belong to its start: a DS that
  starts from another save, or reaches a battle by other presses, builds other seeds. [Random seeds](#random-seeds)
  says how to find a DS's.
- `frames` says what FRAME counts. **`game`**, the default, counts passes of the game's main loop, the only count that
  gives both programs the same input at the same moment of the game ([PLAN.md §6](../../docs/PLAN.md#6-verification)):
  the DS drops frames under load and its start-up takes 241 frames, and the port does neither. Game frame N is the pass
  after GFL's frame count (`sVBlankCount`) has gone up N times since power-on: the pass during which the count is N,
  until a soft reset restarts the count (the steps go on counting). The first pass, game frame 0, has frame 1's state,
  as everything before it does. A video frame the console sleeps through counts as one game frame, so that a later line
  can open the lid. **`video`** counts VBlanks since power-on (on the port, since `main()`), for the boot itself or a
  test of timing. `pbwinput.h`'s `pbw_input_frame_at_vblank` and `pbw_input_frame_asleep` say which frame's state a
  program applies at each VBlank: the oracle uses them, and the port's replays are to ([Integration](#integration)).
  In a run, `frames.tsv`'s `game_frame` is the number to write for a state the game reads right after that frame's
  picture.
- A frame line gives the **whole state from that frame on**, until the next frame line: not a change of state. Frames
  count from 1 and increase strictly. Before the first frame line nothing is held, the lid is open and the pen is up.
- BUTTONS is `-` for none, or names joined by `+`, in any case: `A B SELECT START RIGHT LEFT UP DOWN R L X Y DEBUG`,
  and `LID` for the lid closed. The bits are NitroSDK's `PAD_*` values, as `port_input.h`'s `PORT_INPUT_*`.
- TOUCH is `-` (the pen up, as when the column is left out) or `X,Y`, a bottom-screen pixel: X from 0 to 255, Y from 0
  to 191.
- Columns are separated by spaces or tabs; lines may end with CR LF. Anything else is an error that names the line.

### Recordings

`--record FILE` isn't in the program yet: this is its format, for review before its code. The port writes the run's
input as an input file that `--input FILE` replays to the same frames, given the same ROM, the same starting save (the
save directory's contents) and a compatible build. It writes the input the game reads, not the window's events: the
buttons and lid held and the touch state as the program's frame hook leaves them for the game (`port_input.h`,
`tp_host.h`), at each VBlank.

```
pbwinput 1
# Recorded by Frigate: frigate-black2 <commit>, ROM <the dump's SHA-1>
version b2_us
frames game
time 2026-10-25 14:03:27      # the clock at power-on, captured once
clock running
seed 0x...                    # 64 lines, declared before the game starts
# FRAME BUTTONS [TOUCH]
...
```

- **The clock.** `time` is `--fixed-time`'s, or the host's local time when the program starts, and the run's clock
  starts there and runs with the virtual clock, as `--fixed-time` makes it (`port/hal/rtc/rtc7.c`). Without
  `--record` the clock reads the host's time at each request, which no file can replay; while recording it follows the
  frames the game plays, behind the wall clock while the program is paused or slower than the DS, ahead while it runs
  faster.
- **The seeds.** A file's `seed` lines come before its first frame and number 64 at most, and a battle's seed is built
  when the battle starts, after many frames have been written. So the recorder declares the run's seeds up front: it
  draws 64 from the host's random source, writes them, and gives them to the game as a replay's are given
  (`port/hal/gfl/random_seed.h`), so the recorded run and its replay take the same seeds in the same order, GFL's at
  start-up and one per battle. After 63 battles the game builds its own, which on a host depend only on the clock and
  the input ([Random seeds](#random-seeds)). `run.txt` lists the seeds a run used and whether each was given.
- **The frames** are game frames, by `pbw_input_frame_at_vblank`: the state goes with the frame whose pass reads it
  next, which is the frame a replay applies it at. Several VBlanks share a frame when a pass takes longer than a frame.
  For the buttons and the lid the last of them wins, since the game reads them once, at the start of its pass. The
  touch is sampled at every VBlank while the game samples automatically (`port/hal/tp/tp7.c`), so a pen down at any
  VBlank of a frame is written as down for the frame, at its last position, and its release in the next frame: a tap
  can last a frame longer in the replay, never vanish. A frame's line is written, and the file flushed, when the frame
  number moves on, and only when the state changed; a run that crashes leaves a file that ends at the last complete
  frame.
- **With `--input FILE`** the recording is of what the file plays, with the file's `time`, `clock` and `seed` lines
  instead of new ones, so that it replays the same. Handing the controls to the window when the file ends is for later.
- A file that can't be created is a bad option (status 2) before the game starts; a write that fails during the run
  stops it with status 1, as `--run-dir`'s do.

## Run directories

What `pbw-oracle` and `oracle.py port` write, and `compare.py` reads:

- `frame_NNNNNN.png`: both screens, top above bottom, 256×384. Colours are the DS's 6 bits a channel widened to 8 as
  `(c << 2) | (c >> 4)` (melonDS's output and the port's renderer alike), so a palette's white (31) is `#fbfbfb` and a
  display that is off is `#ffffff`.
- `frames.tsv`: after a `#pbwframes 2` line, a header and one row per frame:
  `frame vblank counter game_frame keyread top_3d bottom_3d top bottom top_blank bottom_blank`. `vblank` is
  NitroSDK's VBlank count (`HW_VBLANK_COUNT_BUF`) and `counter` the `--counter` value, both at the frame's VBlank;
  `game_frame` is the game frame whose input the game reads after that VBlank (needs `--counter`); `keyread` is 1 when
  the game read the keys during the frame (melonDS's lag flag, inverted); `top_3d` and `bottom_3d` are 1 when the
  screen showed the 3D engine's output (below); `top` and `bottom` are the SHA-1 of each screen's RGB bytes; `*_blank`
  are 1 when all but 256 of a screen's pixels have one colour. `-` is a value the program doesn't have (the port's runs
  have only the hashes). `compare.py` still reads version 1, whose one `3d` column it takes for both screens.
- `memory.txt`: after a `#pbwmem 1` line, `FRAME LABEL ADDRESS SIZE HEX` for each dump.
- The port writes its own with `--run-dir DIR`, `--run-screens LIST` and `--run-dump SPEC` (`port/app/run_dir.c`,
  `port/app/main.c`): a `frames.tsv` row at each frame hook with the video frame, NitroSDK's VBlank count (the port's
  ARM7 side counts each VBlank at the VBlank itself, so it is the frame's number), GFL's frame count, the game frame
  and each screen's hash and blank, `-` for `keyread`, `top_3d` and `bottom_3d` (compare.py takes 3D from the
  oracle's run); PNG files at the frames of LIST, as `--screens` takes it; `memory.txt` with each `--run-dump`
  (`[LABEL=][*]SYMBOL[+OFFSET]:SIZE[@LIST]`: a global of the program by name, which the program finds with `dlsym`, so
  not a static; a size, which the host doesn't know; `*` for the bytes where the pointer at SYMBOL points, the
  oracle's `--dump` of the address that pointer holds there; the last frame of `--frames` without a list); and
  `run.txt`, written at the start and again at the end with the frames completed, the exit status and the random seeds
  the game used (`seeds`, then `seed_N: 0x... given` for one the input file gave, `built` for its own), so that a run a
  trap stub ends keeps its frames and says it didn't finish. A dump's ADDRESS is the host's, and its bytes are too: only
  data without pointers, longs or host padding compare with the DS's.
- `run.txt`: `key: value` lines on how the run was made.

A screen shows 3D (`display3d.h`) when engine A is on it (POWCNT1 bit 15) in graphics display mode with BG0 shown as
the 3D layer, or when it shows a VRAM bank that holds a capture of 3D: engine A in VRAM display mode, or a bitmap BG
or OBJs in a bank mapped to either engine. A bank holds 3D from a capture whose source has the 3D layer (source A as
engine A's output with BG0 as 3D, or the 3D picture alone; source B a bank that holds 3D) until a capture without 3D
covers that region or the CPU or a DMA changes it, which a hash of the region taken after the capture shows before the
next frame. The title screen is drawn so (`src/ov162/title.c`: VRAM display of bank D, captured into each frame), as
are dual-screen 3D (`src/gfl/double3Ddisp.c`, bitmaps on engine B) and the 3D demos (`src/ov293`).

## Comparing

`compare.py screens ORACLE PORT` pairs frames (`--offset N`, or `--align`), then compares each pair screen by screen
at the DS's colour depth (each channel >> 2), or with `--depth 5` at its palettes' (>> 3), which leaves out the output's
low bit (green's takes bit 15 of the colour in some renderers). A screen without 3D in the oracle's frame must be
**exact**, whatever the other screen shows. A screen with 3D passes when at most `--tolerance` (0.5%) of its own
pixels differ by more than `--pixel-tolerance` (2) steps from every pixel of the other picture within one pixel of the
same place, which forgives a rasterizer drawing an edge one pixel over. On the port frames `--close` lists, the frames
held close, a screen with 3D must also have at most `--close-tolerance` (1%) of its pixels differ at all: a 2D layer
over the 3D that differs by a step or two everywhere, as in a fade from black, is within the pixel tolerance at every
pixel, and only this catches it. On the port frames `--known-top` lists, the top screen differs by construction and is
left out, while the bottom screen is compared as usual. `--report DIR` writes, for each frame that isn't exact, the
oracle's screens, the port's and the differences side by side (the oracle's picture dimmed, forgiven differences
yellow, the others red). The summary gives, after the count of frames that match, the screens with 3D compared by their
pictures, with the median and the most of their pixels beyond the pixel tolerance and the port frame of the most.
Without a PNG on one side, `frames.tsv`'s hashes decide a screen at 6 bits without 3D (they are of the 8-bit output,
the 6 bits widened); a screen with 3D or at `--depth 5` whose hashes differ is reported "not compared", which fails the
run as a difference does.

`compare.py memory ORACLE PORT` compares `memory.txt` by frame and label, for values whose bytes are the same on both:
counters, flags, random number states, arrays of plain numbers, NNS G3D's state (`NNS_G3dGlb`, 612 bytes). Pointers and
structures that hold them differ on a 64-bit host by design. It pairs frames as `screens` does, and `--frames` takes
port frames.

The screens comparison first says how each stretch of port frames was paired, and after the pairs lists the oracle
frames whose pictures no port frame shows ([above](#aligning-the-oracle-with-the-port)); `--known-oracle` names those
that can't have a counterpart, which it leaves out of that check.

## Symbols

`--config config/<version>/arm9` loads every `symbols.txt` there: main, the ITCM and DTCM, the autoloads and the
overlays, each module named after its directory (main is `main`). Functions have sizes; data and .bss run to the next
symbol of their module, 4 KiB at most, unless the dump gives a size. The statics of complete files, which `symbols.txt`
doesn't name (`sVBlankCount` among them), are in the ROM build's link map, `build/<version>/arm9.o.xMAP`, with every
linked symbol's size: `symbols.py statics` writes them in `symbols.txt`'s format for `--symbols`, which `oracle.py`
does at each run. Without a ROM build there are no statics and no counter.

## Determinism

The same arguments give the same files on every run: no JIT (the interpreter runs every instruction in order), the
software 3D renderer on the emulation's thread, the clock fixed at power-on and moved by emulated time only, a blank
save unless one is given, no host time, no audio device (the sound goes only to `--audio-dump`, sample by sample), and
no file but the ROM and the given ones (melonDS's local files are refused). The firmware is melonDS's generated one with
the owner settings of the port's (`port/hal/nvram/nvram7.c`): nickname "Player", colour 11, birthday 6/23, English, MAC
`02:42:57:32:00:01`; melonDS keeps its own touch calibration. `oracle.py check`, the ctest test `oracle_boot`, runs the
first 500 frames twice, compares the results (`frames.tsv`, the sound trace and the WAV dump byte for byte), checks that
the PNG files and `frames.tsv` agree, that the WAV dump has a sample for each 1024 cycles and the trace the driver's
periodic runs 2728 ticks apart, each record in the frame whose VBlank comes next, and compares the screens of 11 frames
with `port/tests/oracle/boot_<version>.tsv`, hashes the pinned melonDS gives (`oracle.py check --update` writes them).
It exits 77 (skipped) without the oracle or without the ROM the decomp builds.

## Random seeds

The game seeds its random numbers from the console's entropy with `buildSeed` (`src/gfl/gf_standard.c`): GFL's at
start-up (`GFL_RandomInit`, `src/system/gfl_use.c`: a linear congruential generator, and a Mersenne Twister seeded from
its first number), which the field, the trainers' AI and the battle's view draw from, and each battle's at its start
(`src/ov167/btl_main.c`), which its damage rolls, critical hits and a Poké Ball's shakes draw from. `buildSeed` hashes
20 bytes of whatever its stack held and the 32 of `OS_GetLowEntropyData`: the line being drawn and the tick when the
call comes, the tick counter and the VBlank count, the geometry engine's status, the clock, the MAC address, and the
microphone's, touch screen's and buttons' state. Read at its return in an instrumented scratch build of melonDS
(2026-10-24), Black 2's start-up hashes the tick `0x0f91` at line 119, VBlank 5, and return addresses and pointers on
its stack (`0x0209aee8`, `0x02039de9`, `0x02200010`, ...), and builds `0xea56a175a2fffc8a`; the port hashes its virtual
clock's tick and line there and zeros (`-ftrivial-auto-var-init=zero`), and builds `0x91230e3906d9f8f8` from the first
save's input file. No host has what the DS hashes: its CPU takes no time, so the tick and the line at the call are its
clock's, and its stack holds its own frames ([../../docs/decomp-findings.md](../../docs/decomp-findings.md)).

- **The DS's seeds are the same at every run of an input file**, as the rest of the oracle is. Start-up builds GFL's at
  frame 20, before it reads the save, so every file from power-on with the default clock gives the same first seed
  (Black 2 `0xea56a175a2fffc8a`, White 2 `0x81d63ca2c9f3143e`), and every file with the clock pinned (`clock pinned`)
  another, the clock being among what it hashes (Black 2 `0x5a0af77416c50a14`, White 2 `0x270fd5258c56ebe3`); a
  battle's depends on how the DS reached it, the clock, the presses and the save among it.
- **An input file gives them to the port** ([`seed` lines](#input-files)), which takes them in place of the ones it
  builds (`port/hal/gfl/random_seed.h`) and lists the seeds it used in `run.txt`
  ([ADR 0022](../../docs/adr/0022-the-ds-random-seeds-given-by-input-files.md)).
- **`oracle.py seeds NAME`** plays a sequence's or a save's input file on the oracle, from the save it starts from, and
  prints the seeds the DS built as the file's lines; `oracle.py sequence` reads them from its own oracle run the same
  way, and fails when the file gives seeds that aren't the DS's, all of them in order (a file without seeds leaves the
  port its own, and says so). melonDS has no hook where `buildSeed` returns, so they are read from memory the oracle
  dumps at each frame (`seeds.py`): GFL's from its context and the twister's first word in the frames after start-up has
  built it, the context's state walked back to the draw that seeded the twister, whose step before is the seed; a
  battle's from `btl_calc.c`'s work (`data_ov167_021dd844`), which takes a copy of the battle's context at its start,
  before its first roll. The instrumented build's seeds and these are the same: the start-up's and each battle's of the
  Route 19 route in both versions.
- **With the DS's seeds the port draws the DS's numbers** wherever its passes draw as the DS's do: New Game's day care
  egg seed, which it writes into the save, is the DS's (`replay_same_save`), and on the Route 19 route GFL's linear
  generator's state and the twister's index were the DS's count for count, through the title, CONTINUE and the field, to
  Hugh's battle, in a trace with scratch instrumentation that no test holds yet. Where the CPU's time decides when the
  game draws they part: the trainers' AI ([below](#known-differences-from-the-hardware)).

## The boot sequence (Black 2, no input, blank save)

As the oracle shows it, by frame (`oracle.py run --frames 4500`); the frame count (`counter`) starts at frame 242.

| Frames | Top screen | Bottom screen |
| --- | --- | --- |
| 1–248 | white: the display is off (`#ffffff`) | the same |
| 249–363 | "The Pokémon Company" and the Nintendo logo on white (`#fbfbfb`), fading in until about 272 | white (`#fbfbfb`) |
| 364–384 | both screens fade to black, a step every 3 frames | |
| 385–419 | black | black |
| 420–555 | the copyright screen: © 2012 Pokémon, © 1995–2012 Nintendo, Creatures, GAME FREAK | black |
| 556–620 | black | black |
| 621–1207 | the Game Freak logo movie: a shooting star, its burst, the letters of GAME FREAK flying in, then the logo still from about 1040, fading out | black |
| 1208–1282 | black | black |
| 1283–3752 | the opening movie in letterbox: Kyurem's silhouette, its eyes lighting up, the two player characters, Team Plasma's emblem, Kyurem | black |
| 3753–3841 | black | black |
| from 3842 | the title: "Pokémon Black Version 2", "PRESS START" | the 3D Kyurem |

The frame count is 354 at frame 600, 470 at 1000, 633 at 1283 and 1349 at 3842: the DS drops frames through the movies.
Nothing before the title uses the 3D layer. The title's 3D Kyurem comes later, on the bottom screen: from frame 3841
(count 1348) engine A shows BG0 as 3D and captures it into bank D every frame, and from 3842 the bottom screen shows
that bank in VRAM display mode (`bottom_3d` is 1; the top screen, engine B's logo, stays 2D). The count is 1507 at frame
4000.

## The comparison sequences

Six input files on game frames for each version, `port/tests/oracle/<name>_<version>.pbwinput`, played by seven
sequences, compared with `--align counter`, the movies by their pictures (`oracle.py`'s `SEQUENCES` says how):

- **opening**: nothing pressed. Power-on, the boot screens, the Game Freak logo and the whole opening, the title and
  its first 274 frames of 3D on the bottom screen. The port runs 3,800 frames, the oracle 4,300. The game counts passes
  to port frame 312 (lag 0) and waits on clocks from the black after the copyright screen to the title, port frames
  313-3526 in both versions; from the title on it counts passes again, with the lag where the title starts after the
  black on each (2,177 in Black 2, 2,154 in White 2).
- **title**: START at game frame 1100 skips the opening, which holds its frame until count 1107 on both; the title
  (its 3D on the bottom screen, then, once the title hides its layers after 1,740 of its frames, on the top), START
  at 3000 to bring the layers back, START at 3100 for the start menu. The port runs 3,600 frames, the oracle 5,700.
  The clock runs over port frames 313-1108, and the counts are the same again from 1109 (`--lag 1109=0`). The press
  reaches the DS some 1,500 frames further into the opening than the port, so this sequence leaves most of the
  opening to the other.
- **continue**: the title's presses from a save, `first_save`'s ([below](#saves)), which the oracle makes first; the
  start menu then shows CONTINUE with the saved game, and A at game frame 3400 takes CONTINUE into the field. The port
  runs 3,900 frames, the oracle 6,000 (count 3,955): the start menu wipes out to black from port frame 3,425, the
  field loads behind the black, shows the season banner from 3,461 and fades in from 3,605 on the player in his room,
  where the last 294 frames show the field's 3D.
- **continue_starter**: continue's input file from the starter save ([below](#saves)): the start menu's saved game has
  the Pokédex line (Pokédex: 1, time 0:04), and CONTINUE goes to the Aspertia City lookout, in both versions, each
  from its own starter save.
- **field_menus**: continue's presses from the first save, then the field menu in the player's room, the real one,
  on both programs: X at game frame 3900 slides it in on the bottom screen (BAG, the trainer card, SAVE and OPTIONS,
  since the save has no Pokémon and no Pokédex yet), A on BAG opens the bag, its items pocket empty, B takes it back
  to the menu, RIGHT and A open the trainer card, the pen on the card's return button (240,180) takes it back to
  the menu, which shows without its cursor since the touch leaves the UI in its touch mode, and B closes the menu.
  It is the only sequence that touches the screen. The port runs 5,200 frames, the oracle 7,400. Its file pins the
  clock ([Input files](#input-files)): the menu's bottom screen shows it, and running it would show each program's
  own time there.
- **party_menus**: the same presses from the starter save at the lookout, whose menu has every item: the party list,
  Tepig's menu, SUMMARY and its summary, back to the list and the menu; the Pokédex, its Unova list and Tepig's entry;
  the trainer card with its Pokédex line; the bag. The port runs 8,500 frames, the oracle 11,100. The clock is pinned
  as in field_menus. These two were Frigate's test launcher's routes while the field had no C
  ([ADR 0012](../../docs/adr/0012-test-launcher-for-the-field-menus-apps.md)); every app screen they reach now comes
  through the field menu on both programs.
- **aspertia**: phase 3's exit route ([PLAN.md §7](../../docs/PLAN.md#phase-3-new-game-to-the-overworld-m3-m4)), from a
  blank save. The title sequence's presses, then NEW GAME at game frame 3400 and A twice a second through the intro
  (overlay 294): Professor Juniper, Minccino out of its Poké Ball in a burst of particles, the player's and the rival's
  names, the season's name and the 3D flight over Aspertia City (overlay 293), into the field in front of the player's
  house and its first scripts there and in the house, to the player's room. Then the story's first steps as the starter
  save's file takes them: Mom's scene and out of the door into Aspertia City. Then west along z = 762, short of Hugh's
  scene, to a townsperson at (39,764) (npc 4 of `res/field/events/aspertia_city.json`) and his three lines, into the
  house at (38,761) (zone 431), three tiles up, back out of its door and east: the player walks, talks, enters a
  building and changes maps, three times after the intro's own change into the house. The port runs 15,800 frames, the
  oracle 18,700, and the clock is pinned, as in field_menus. White 2's intro runs 60 passes longer from its gender
  question to the room, and the route's presses from the room on line the two versions up again, so both files have
  the same presses. NNS G3D's state is compared at every port frame from the title on, but the first frames of the DS's
  passes that take several ([below](#known-differences-from-the-hardware)). Before it, no sequence had reached a
  particle: run before the geometry command log's fix of the same day (`port/hal/hw/hw.c`, which the Route 19 route
  found), it showed the burst missing on the port, SPL's read of POLYGON_ATTR having sent a POLYGON_ATTR of 0; with the
  fix, the burst matches. It found the lake of the flight over the city a line lower on the port in one 3D frame (port
  frames 8064-8065 in Black 2, 8124-8125 in White 2; 301 pixels beyond the pixel tolerance, where the flight's other
  frames have 17 to 38): the polygon lists of both programs, dumped from instrumented builds of the port's geometry
  engine and of melonDS's renderer, differed only in the screen Y of the water's two far vertices, 135 on the port and
  134 on melonDS. The geometry engine divided the viewport transform exactly; the DS divides with a 32-bit divider,
  halving W and the numerators above a W of 0xffff and dropping their low bits, and keeps W to 24 bits (melonDS's
  measurements). Since `port/gfx/ge/ge.c`'s `ToScreen` divides that way (with `g3d_ge`'s cases) those frames have 18
  pixels beyond the tolerance in both versions, like the frames around them, and the flight 80 at most, where it had
  200.

A sequence can also allow the port to stop on code with no C yet: from a port frame (`stop_from`), on the trap stub
it names (`stop_trap`, a pattern of the trap's message), with why (`stop`). A stop on any other trap fails, so the
sequence says which gap it stops on and changes when the decomp fills it. The continue sequences stopped that way on
overlay 36's `FIELDMAP_PROC_FUNCS` until its C arrived; none stops now.

The opening and the title hold the title's first 34 frames of 3D close (their `close`, compare.py's `--close`): port
frames 3527-3560 of the opening, 1116-1149 of the title. There the title's bottom screen fades in over its 3D, and the
tiles that the copyright screen read past its file's end show as a band over lines 38 to 95, a step or two from black,
which the 3D's tolerance forgives everywhere ([../../docs/sanitizers.md](../../docs/sanitizers.md), H1). The 3D there
differs from the oracle's in at most 235 pixels (Black 2) of the 491 a frame held close may; a build without the band
differed in 14,330 at the first frame, 1,823 at the fourth, and passed the tolerance. continue and continue_starter
start from a save, which leaves other bytes after the copyright screen's file on the port than on the DS
([below](#known-differences-from-the-hardware)), so they don't hold those frames close.

Each is two ctest tests, but the four that start from a save (continue, continue_starter, field_menus and
party_menus), whose saves only the oracle can make, have only the first:

- `oracle_<name>` (`oracle.py sequence NAME --port BIN`), with the oracle built: runs both programs and compares every
  port frame but the known differences ([below](#known-differences-from-the-hardware)): where the game counts passes,
  each with the oracle frame at its count (plus the lag), so that a picture shown a pass late or early, or held a pass
  too long, fails; in the movies, by their pictures in order. Screens without 3D must be exact at 6 bits, screens with
  3D within the tolerance, and every picture the oracle shows between two of the port's frames must be one the port
  shows (a skipped picture fails). It compares NNS G3D's state (`NNS_G3dGlb`) on the count at every frame of the title
  too, and of the exit route from the title on. It can't time the movies: there the DS's passes take 3 to 5 frames by
  its CPU's load, so only their pictures and their order are held, and in the title sequence the oracle frames the
  press skips aren't compared. With `--update` it writes the reference from a comparison that passes. Skipped (77)
  without the oracle or the ROM.
- `oracle_<name>_hashes` (`oracle.py sequence-check NAME --port BIN`), every build: runs the port alone and checks it
  against `port/tests/oracle/<name>_<version>.tsv`, the oracle's hashes at every 12th port frame and at each frame
  around a press, the title's start and the cry. Where the game counts passes, each row is the oracle frame at the
  port's count less the declared lag, so the check holds the port to the oracle's count there (plus the lag) and its
  screens without 3D to the oracle's at 6 bits; in the movies a row is the oracle frame that showed the port frame's
  pictures when the reference was written, which holds the port to that timing. Where the sequence compares NNS G3D's
  state, each row in its state frames has the SHA-1 of the oracle's `NNS_G3dGlb` there (the `state` column; `-`
  elsewhere), and the port, run with the same dumps, must have the same state at that row: in the field that is the
  camera, which follows the player, so that the exit route's walk and changes of map are held without the oracle. A top
  screen that `known_top` leaves out is `known` in the reference. It can't judge a screen with 3D (in the exit route,
  the townsperson's lines, whose window shows over the field's 3D, which only `oracle_aspertia` holds) nor the frames
  between the ones it keeps. Skipped (77) without the ROM.

Results, every port frame but the known differences:

| Sequence | Port frames | Match | Exact | 3D within the tolerance | Known differences | NNS_G3dGlb equal |
| --- | --- | --- | --- | --- | --- | --- |
| opening, Black 2 | 3,800 | 3,800 | 3,526 | 274 | none | 274 of 274 |
| opening, White 2 | 3,800 | 3,800 | 3,526 | 274 | oracle frame 1283 | 274 of 274 |
| title, Black 2 | 3,600 | 3,579 | 1,474 | 2,105 | 21 port frames, oracle frame 4943 | 2,470 of 2,470, 15 left out |
| title, White 2 | 3,600 | 3,572 | 1,477 | 2,095 | 28 port frames, oracle frames 1283, 4875 | 2,470 of 2,470, 15 left out |
| continue, Black 2 | 3,900 | 3,879 | 1,480 | 2,399 | the title's 21 port frames, oracle frame 4920 | not compared |
| continue, White 2 | 3,900 | 3,871 | 1,482 | 2,389 | the title's 28 port frames and 3402, oracle frames 1260, 4852 | not compared |
| continue_starter, Black 2 | 3,900 | 3,879 | 1,479 | 2,400 | the title's 21 port frames, oracle frame 4920 | not compared |
| continue_starter, White 2 | 3,900 | 3,871 | 1,481 | 2,390 | the title's 28 port frames and 3402, oracle frames 1260, 4852 | not compared |
| field_menus, Black 2 | 5,200 | 5,179 | 2,169 | 3,010 | the title's 21 port frames, oracle frame 4920 | not compared |
| field_menus, White 2 | 5,200 | 5,171 | 2,171 | 3,000 | the title's 28 port frames and 3402, oracle frames 1260, 4852 | not compared |
| party_menus, Black 2 | 8,500 | 8,190 | 4,250 | 3,940 | the title's 21 port frames, the summary's 289 (4534-4822), oracle frame 4920 | not compared |
| party_menus, White 2 | 8,500 | 8,182 | 4,252 | 3,930 | the title's 28 port frames and 3402, the summary's 289, oracle frames 1260, 4852 | not compared |
| aspertia, Black 2 | 15,800 | 15,708 | 8,469 | 6,846 | the title's 21 port frames, 7751, Mom's balloon's 16, the zoom's 54, oracle frames 4943, 9841, 17462-17519; the top screen of Minccino's 393 (its bottom screen exact) | 14,656 of 14,656, 29 left out |
| aspertia, White 2 | 15,800 | 15,700 | 8,531 | 6,776 | the title's 28 port frames and 3402, 7811, Mom's balloon's 16, the zoom's 54, oracle frames 1283, 4875, 9837-9838, 17401-17457; the top screen of Minccino's 393 (its bottom screen exact) | 14,656 of 14,656, 29 left out |

Each comparison's summary gives its screens with 3D, compared by their pictures, and their pixels beyond the pixel
tolerance at the median and at most: 5 to 18 at the median by sequence and version, 0 in the exit route, and in
party_menus, whose 3D is mostly the Aspertia City lookout, 35 in Black 2 and 21 in White 2; at most 80, the title's
(port frame 2144 in Black 2; White 2's 54 at 1884, and 80 at 9152 in its exit route), of the 245 allowed, nearly all on
polygon edges (their coverage and fill). Inside polygons the pixels match: they didn't until the rasterizer took the
values along an edge by line, as melonDS does, where it had taken them along an X-major edge's run, which put whole
spans' colours a step apart (before, a median of 79 and at most 172). The field is no known difference: its 464 frames
in each continue sequence match, the black of its load, the season banner and the bottom screen exactly (170 frames, 169
from the starter save) and its 3D within the tolerance. Once it had faded in, the player's room had 21 to 25 pixels
beyond the pixel tolerance in every frame, and the Aspertia City lookout 135 to 138, the edges of its trees and of the
hills on the horizon (141 to 143 until the geometry engine divided the viewport transform as the DS's 32-bit divider
does: the exit route's lake, above), in both versions, until `port/gfx/ref/ds3d.c` interpolated Z-buffer depth as
melonDS does (`DepthLine`, derived from melonDS's code; the railing's seam at the lookout, which phase 4's route found;
[../../docs/STATUS.md](../../docs/STATUS.md)): now the room has 2 in every frame from port frame 3640 to 3900 and the
lookout 40 or 41, in both versions. With the field menu open the game darkens the field on the top screen, every channel
to about 5/8 (a wall's 243, 231, 178 to 154, 146, 113 on the oracle), which brought the two rasterizers' edge pixels
closer together before that change: the room's 25 became 18 and the lookout's 135 to 138 became 103 to 108, in both
versions. These are the "18 pixels on the 3D edges behind the open menu" of the deep dive of 2026-10-23, whose 28 frames
that failed failed on the bottom screen's clock alone, the top passing. Every frame of the apps is exact on the count,
but the summary's ([below](#known-differences-from-the-hardware)). The menu's bottom screen shows the clock; pinned, it
shows 12:00 on both, so that screen is held exact. The house replay, whose clock runs, holds it to the oracle's within
its frame ranges ([port/tests/replay](../../tests/replay/check_screens.py)). In the exit route (aspertia) a screen with
3D has 0 pixels beyond the pixel tolerance at the median in both versions since the depth change, and 8,469 of Black 2's
frames are exact (8,531 of White 2's), where 5,446 (5,508) were before it, when the median was 18 (17): about 16 in the
flight over the city, 58 in front of the player's house, 18 in the house and on Aspertia City's street, 25 in the house
the route enters and 38 back on the street, and at most 80 in the flight and 89 on the street (port frame 14549), in
both versions; the flight had up to 200 before the divider. Outside Minccino's idle, whose top screen is left out, the
intro's 3D is exact in both versions, Minccino's Poké Ball burst, the first particles any sequence reached, among it.
NNS G3D's state matches at every frame compared.

## Saves

**The format.** A save of BW2 is the cartridge's backup chip, a 4 Mbit flash, and melonDS's `.sav` of the game and
the port's save file (`port/hal/card/port_card.h`) are both that chip's 524,288 bytes, with nothing before or after
them, so a save moves between the two by copying the file under the other's name. The evidence, in the pinned melonDS
1.1's sources (`<build>/_deps/melonds-1.1-src/src`):

- `ROMList.cpp` gives the game codes `IREO` (`0x4F455249`) and `IRDO` (`0x4F445249`) save type 6, and
  `NDSCart.cpp`'s `CartRetail` makes type 6 a flash (`SRAMType` 3) of `512*1024` bytes (`sramlengths`). A save of
  that length becomes the chip's memory as it is; one of another length is cut, or padded with 0xff, without a word.
- The frontend reads the whole `.sav` as that memory (`EmuInstance.cpp`, the cart's `SRAM`) and writes the memory to
  the `.sav` whole after a write (`SaveManager.cpp`). melonDS names the file after the ROM's, next to it or in its
  save folder; the port names its own after the game code (`IREO.sav`, `IRDO.sav`) in its save directory.
- `pbw-oracle --save` hands melonDS a file's bytes the same way, and `--save-out` writes `GetNDSSave()`, the same
  memory, so a save the oracle writes is a melonDS `.sav`.

And by running both programs:

- **melonDS to the port:** `first_save`'s file, which melonDS wrote, continues in the port: the start menu shows its
  saved game exactly as melonDS does (the continue sequence, above and below). The port leaves a save the game only
  reads as it was: the file has the same SHA-1 after the run.
- **The port to melonDS** (`oracle.py port-save`, ctest `oracle_port_save`): both programs play
  `first_save_<version>.pbwinput`, whose start is New Game as `new_game.pbwinput` plays it, the port 10,600 frames,
  past New Game's formatting of the blank backup and before the field's first script, the oracle 13,000 frames for
  its own save of that moment. The oracle then boots each save with `title_<version>.pbwinput`, 5,400 frames: the two
  runs are the same frame for frame, count and both screens (the start menu with NEW GAME first, no message about a
  damaged save), and neither save is written. Black 2 and White 2 both pass. A save with an adventure in it waits for
  the port's field (overlay 36).

**What the two formatted saves differ in**, and why. In Black 2 they differ in 10,712 bytes, 5,356 in each copy of the
main save and none after `0x4c000`; in White 2 in 10,720. None of it is the format: it is when each program set the
blocks up, which the game takes from the clock, the VBlank count and the hardware's state. In each copy (Black 2):

- 5,292 bytes in three blocks the game encrypts with a key it seeds with the VBlank count, `(OS_GetVBlankCount() << 8)
  | OS_GetVBlankCount()`: the mystery gift block at `0x1c800` (`mystery_data.c:391`, 2,695 bytes), the trainer card's
  records at `0x1da04` (`trainer_card_savedata.c:18`, 472) and the symbol encounters' data at `0x22a00`
  (`symbol_save.c:39`, 2,125). The port's three seeds are `0x5d4c`, the DS's `0x5959`, `0x5959` and `0x5b5a`: the DS
  gets to New Game at another count and crosses a VBlank between the blocks.
- 8 bytes of DWC's user data at `0x1aa00`: the login ID's player ID at `0x1aa0c`, a random number seeded with
  `OS_GetLowEntropyData` (`dwc_account.c:285`), the timers and the hardware's state at that moment, and DWC's CRC of the
  user data at `0x1aa3c`.
- 8 bytes, the day care's egg seed at `0x20ecc` (`GFL_RandomMT`, `daycare_savedata.c:90`).
- 13 bytes in five other fields that blocks take when New Game sets them up (`0x1fa02`, `0x21200`–`0x21204`,
  `0x213b7`, `0x24fd8`, `0x2585c`; 16 in White 2), not traced one by one.
- 35 bytes of checks: the CRC-16 of each of the eight blocks above and the checksum table at `0x25f00` (36 in White 2).

The console's pending Wi-Fi ID, which DWC makes from the clock and the MAC address and New Game copies into the login
ID (`0x1aa04`, 8 bytes), is the same in both, and `port-save` checks that it is. It wasn't: DWC adds the address of
`OS_GetTick` to the ID's seed (`util_wifiidtool.c`'s `BUG`), a constant on the DS (`0x0207bb0d`, `0x0207bb39` in
White 2) and on a host the program's address, which macOS moves at every launch. Every run of the port then wrote
another save (22 bytes apart: the ID, the CRCs and the checksum tables over it, in both copies), so the 10,718 bytes
counted before were one run's. The host build
now adds the DS's address there (`port/docs/game-changes.md`), the port's saves are the same at every run
(`replay_same_save`, which also holds the ID to the oracle's), and the count above is the same at every run.

The port refuses a file of another size and never overwrites it, with what it seems to be: a size another chip's saves
have (the sizes of melonDS's other save types), DeSmuME's `.dsv`, which ends with DeSmuME's own footer (its text, six
32-bit words and the cookie `|-DESMUME SAVE-|`, as DeSmuME's `src/mc.cpp` writes them), this chip's image padded with
0xff to a larger size (with how to cut it), or a directory. Where the save directory has no file of the game's but a
`.sav` of the chip's size under another name (a melonDS save copied in as it was named), the port names it and says to
rename it, unless it is named as the port names the saves of this game's other versions (`IRE?.sav`, `IRD?.sav`). A
save path that is a symbolic link stays one: the port writes the file it links to.

**The console in the save.** The adventure block (`AdventureSave`) keeps the clock offset, the MAC address and the
owner's birthday of the console the game was last continued on. CONTINUE checks the offset against the console's
(`EventFieldContinue_Create` in `event_mapchange.c`, `hasClockNotBeenTampered`), on the way into the field: another
offset takes the path of a changed clock, a day's wait for the daily events (`setNewDayForCountdown`),
`ShayminForm_RevertPartyAndRegister` on the party (Shaymin back to its Land Forme) and the trainer card's time. Then
`setAdvTimeBlkRtcOffsetOwnerMacBdayMonthDay` writes the console's offset, MAC and birthday over the save's, for the next
save. The port's firmware and the oracle's are the same console (`port/hal/nvram/nvram7.c`, `oracle.cpp`'s
`MakeFirmware`: MAC `02:42:57:32:00:01`, owner Player, birthday 6/23, offset 0), so the saves here move without it, and
melonDS's own generated firmware has offset 0 too. A save from a DS whose owner once set its clock has another offset:
the port takes it as a changed clock, and the next save carries the port's console, so a dump moved from a DS to the
port and back takes the changed clock's path on both. PLAN.md asks the import to take on the save's offset and MAC
instead; with the console's Wi-Fi ID, which the port makes again at every launch since its firmware isn't kept between
runs, that is the console the D7 sidecar would keep. None of it is built yet; a continued game reaches the field on the
port now, so the path can be shown there.

**The first save** (`oracle.py save first_save`): the oracle plays `port/tests/oracle/first_save_<version>.pbwinput`
from power-on with a blank save, 18,800 frames: NEW GAME and the intro as `new_game.pbwinput` plays them (the player
AAAAAAA, the rival HughAAA), the opening's flight into the field in front of the player's house in Aspertia City ("Oh,
hi!", about frame 13,900), A twice a second through the scripts outside and in the house to the player's room, where the
player has the field from about frame 15,100, then X for the field menu (BAG, the trainer card, SAVE and OPTIONS before
the first Pokémon), DOWN twice to SAVE, A, and A on YES: "Would you like to save the game?" shows the saved game
(Aspertia City, 0 badges, 0:01), the game saves from frame 17,787 to 18,291 (17,788 in White 2), says "AAAAAAA saved the
game." and closes the menu at frame 18,653. The file holds the main save twice, the same bytes at `0x00000` and
`0x26000` (the player's name at `0x19404` and `0x3f404`, the rival's at `0x23ba4` and `0x49ba4`), and one page of 256
bytes at `0x7f000`. It is the same at every run, so `oracle.py`'s `SAVES` keeps its SHA-1 (Black 2 `a7a58889e239`, White
2 `479529f98b29`) and a sequence that starts from it fails if the oracle, the input file or the ROM gives another save.
It is made at test time and never committed.

**Saves made from saves.** A save in `SAVES` can name the save it starts from (`start`): `oracle.py` makes or takes
that one first, checks its SHA-1, and gives it to the oracle with `--save` for the next input file. Each step is held
to its own SHA-1, so a chain is the same at every run or fails at the step that changed. Black 2 has two, measured
with the pinned melonDS, and White 2 the first of them; White 2's Route 19 save, which no test uses, is still to be
measured:

- **The starter save** (`starter_save`, from `first_save`, `port/tests/oracle/starter_save_b2_us.pbwinput`, 17,000
  frames, SHA-1 `ce71fe0b8c1d`): CONTINUE as the continue sequence presses it, then the story from the player's room:
  Mom's scene, out of the house, Hugh's scene, a detour west of the Trainers' School (a sign blocks x = 37), up x = 54
  and west along z = 725 to the stairs, up to Bianca at the lookout (36, 710) and through both her talks. A at Bianca
  takes the middle ball, Tepig; B, mashed, answers NO to the nickname and takes the Pokédex; then the field menu's
  SAVE. Decoded by hand in Gen 5's format, with a valid checksum: Tepig, level 5, 135 experience, 21/21 HP; event flags
  0x961 (a Pokémon) and 0x962 (the Pokédex), the city's story variable 0x40a1 at 4, the starter variable 0x4030 at 1.
  The game writes it by frame 16,400 and the menu closes at 16,874; the two copies of the main save are the same, and
  2,086 bytes differ from `first_save`. It costs about 2.5 minutes at a load average near 10.

  White 2's (`starter_save_w2_us.pbwinput`, the same 17,000 frames, SHA-1 `3b9dac231d1f`, the same in three runs) stops
  at the same point by another street. Black 2's input file goes astray in White 2: going up x = 37, the townsperson who
  wanders around (36, 747) (`res/field/events/aspertia_city.json`, npc 11, movement 3) steps in front of the player, who
  stands some 30 frames, so each timed walk after it ends short and the player is left in front of the Trainers' School
  without a Pokémon (0x40a1 at 1). Until then White 2 plays Black 2's story within a few counts (Mom's scene from count
  3978 against 3979, Hugh's to 7704 against 7708), so White 2's file is Black 2's to (41, 751), then one tile west and
  up x = 40, out of that walker's reach, along z = 741 to x = 54 and on as Black 2's, eight tiles shorter; the presses
  from the stairs on are Black 2's, 124 frames sooner. Its screens show the same story: Hugh's comment on the stairs,
  Bianca's talks, Tepig chosen, the Pokédex, and "Would you like to save the game?" with Aspertia City, badges 0,
  Pokédex 1, time 0:04. Decoded the same way: Tepig, level 5, 135 experience, 22/22 HP (another personality value than
  Black 2's), its checksum valid; event flags 0x961 and 0x962; all 431 event variables and all 3,064 saved flags as in
  Black 2's save (the route sets 0x4030 at 1, 0x4043 at 255, 0x40a0 at 1, 0x40a1 at 4 and 0x40a4 at 1; 0x4165 is at 1
  from `first_save` on, in both versions). The game's passes take two frames or more while it saves, from frame 15,802
  to 16,067, and the menu closes at 16,695 (count 14,394); the two copies are the same, and 2,080 bytes differ from
  White 2's `first_save`.
- **The Route 19 save** (`route19_save`, from `starter_save`, 28,600 frames, SHA-1 `826bc561a328`): Hugh's trainer
  battle (FIGHT, Tackle: Tepig grows to level 6, 229 experience), the Pokémon Center's scene, Mom's, the gate's gift
  and Bianca's catching lesson, B through each, then SAVE beside the grass at (56, 696). It is for the battles, which
  no test runs yet.

`oracle.py save NAME... --save-cache DIR` makes each save into DIR as `NAME_VERSION.sav`, with the saves it starts
from, and a later run takes a file from there only while its SHA-1 is still the one kept: anything else is made again.
`--out FILE` writes one save, made fresh. ctest's `oracle_saves` test makes `first_save` and `starter_save` into
`<build>/oracle-saves` before the sequences that start from them (a fixture), so they cost their oracle runs once a
build tree; without `--save-cache`, as before, every save is made fresh. Nothing the game writes is committed: only
the input files and the SHA-1s.

**The port plays the starter route too** (ctest `replay_starter_save`,
[../../tests/replay/check_oracle_save.py](../../tests/replay/check_oracle_save.py)): `first_save`'s input file from a
blank save to its 18,800th frame, the field menu's SAVE, then `starter_save`'s from the save it made, 17,000 frames,
each with the DS's start-up seed (the files' `seed` lines, which the oracle reads only as part of the file). Its save is
held to the oracle's starter save block by block, by the SHA-1s of `port/tests/replay/starter_save_<version>.save.tsv`
(written from the oracle's save by `check_oracle_save.py --update`; hashes only), and each block's CRC to its data. The
story's state is the oracle's: Tepig, its personality, stats and moves, the party, the bag, the Pokédex, every event
flag and variable, the day care's egg seed and the console's Wi-Fi ID. What differs is declared in the script with its
traced cause, in both versions alike: the four blocks New Game set up on the port's own first save (the mystery gift,
the trainer card's records and the symbol encounters, encrypted with keys seeded with the VBlank count, and DWC's random
player ID, as [above](#saves)); the play time, which counts the CPU's time, and the clock's times of the save and of its
last check, which run with the video frames; 12 bytes of Tepig's mail, which `PokeParty_CreatePkm` takes from the system
heap uncleared (`CreateMailData`; `ResetMailData` sets neither its `unk18` nor the messages' `id`s), so they hold what
each program's heap left there; and the state of the four townspeople who walk at random. Those wander again after
Bianca's scene from another count: after her first talk the field loads the city's music again in GFL's sound thread
(`GFL_SndBGMLoadStep`, `src/system/snd_sys.c`), whose card reads take the DS 56 passes (counts 11257 to 11313 in Black
2) and the port 6, and the field waits for the music's fade-in before the starter's selection
(`EventFieldClose_Callback`, `src/system/field_event.c`), so the rest of the scene runs some 50 passes later on the DS
(Tepig is made at count 12182 against the port's 12134) and B, mashed, absorbs it. Traced by GFL's linear generator on
both programs, dumped at every frame: at the end of each pass it is the DS's at every count but 12134 to 12181, where
the port has made Tepig and the DS not yet, and from 13232, when the walkers' draws come at other passes. Before PR 29
the port crashed in Bianca's scene at `ActorCmdExec 2, Movement_0528` after the answer to the nickname, an animation of
hers that the script reaches 314 bytes back ([../../docs/game-changes.md](../../docs/game-changes.md)), and the test
failed there in both versions. The route answers NO to the nickname, so the test holds that actor command's offset
alone; the nickname loop's jump back, which only YES takes, is `integrate_script_offsets`'s
([../../tests/integrate/script_offsets.c](../../tests/integrate/script_offsets.c)).

**Continuing it** (the continue sequence): the start menu shows CONTINUE first, then NEW GAME, MYSTERY GIFT, UNOVA
LINK, NINTENDO WFC SETTINGS and MIC TEST, and the saved game: the player's picture, AAAAAAA, Aspertia City, Badges: 0,
Time: 0:01 (no Pokédex line: the player has none yet). Black 2's port frames 3,244 to 3,436 (the start menu from frame
3,250, CONTINUE's press at 3,401, its blink, the wipe out from 3,419 and the black from 3,425) are exact at the DS's 6
bits on the game's count, and White 2's are with the lag after the cry and one frame at the press (above). Both then
load the field and show the player in his room, on the same count frame for frame (the DS takes 20 frames for its
first pass there, port frame 3,438 against oracle frame 5,452, and the counts pair again). Overlay 36's C gave the
field's process table in the sync of 2026-10-22; the port then stopped itself after frame 3,436 when the field's heap
ran out, until [ADR 0021](../../docs/adr/0021-root-heaps-in-the-games-own-memory-from-host-memory.md) gave the root
heaps in the game's own memory host memory, and it crashed three frames later until upstream typed
`Field.colorPostFX` for the host (`b62c655bfa`, merged into Frigate by PR 16).

## Sound

The port's ARM7 sound driver ([port/hal/snd7](../../hal/snd7), [ADR 0006](../../docs/adr/0006-sound-driver-as-built.md))
held to the DS's, by what each program writes for it:

- **A sound trace** (pbw-oracle's and the port's `--sound-trace FILE`, the format in `sndtrace.h`): a record after each
  run of the driver's thread, with the driver's work in the DS's layout (`SNDi_Work`, 0x1180 bytes: the 16 channels,
  16 players, 32 tracks and 8 alarms), the shared work, each channel's SOUNDxCNT and the mixer sample that took its
  last start, the mixer sample by which the last periodic run had begun, the locks, the random generator, the periodic
  runs the record covers, the cycle, the frame (frame N ends with the N-th VBlank: on the oracle, a record made after
  melonDS's line 192 belongs to the next frame, since melonDS's own frame runs on to line 262) and GFL's frame count.
  The oracle reads the ARM7's WRAM: a run has been made when the driver's random generator moved on (each run draws
  from it last), and it is finished when the ARM7 is halted (its thread waits for its next message), which the oracle
  checks after each mixer sample; a run that comes before the ARM7 halts again shares the record of the one before
  it, which the periodic alarm's fire time tells (`periods` 2 or more). The fire time moving on also tells the sample
  by which a periodic run began. The port writes the same layout from its own structures (`port_snd7_trace_record`),
  with the links inside the work as the ARM7's addresses; addresses of data the ARM9 loaded differ by construction, so
  a track's place in its sequence is `cur - base`.
- **A WAV dump** (`--audio-dump FILE`): 16-bit stereo, every sample of the mixer, one each 1024 cycles from power-on
  (the port: from its driver's start). The oracle takes melonDS's mixer output sample by sample, after each of the
  mixer's events and before melonDS resamples it for a sound card (`SPU::OutputLastSamples`, which an explicit
  instantiation names); its SOUNDBIAS is direct boot's 0x200, which melonDS centres on 0. The WAV header says 32,728
  Hz, as the port's, for 32,728.5.

`sound.py` compares them three ways, which `oracle.py sound NAME` runs on the comparison sequences:

- **The driver's state, run by run.** The driver moves its sequences on at its periodic runs only, and the two
  programs make those runs at different moments of a frame (the DS's driver starts during the boot the port doesn't
  take, and its passes take longer), so a state at a VBlank can't be the same by construction. Instead each player's
  play (from its start to its stop) on the oracle pairs with the same play on the port, and their records with the
  same count of periodic runs since the start: the player, its tracks by their place in the player (the driver's
  track numbers depend on the other players), each track's place in its sequence, call stack, parameters and channel
  list, each channel's envelope, volume, pan, timer, LFO, sweep, wave and SOUNDxCNT, the player's tick count,
  variables and status. Both plays must have the same runs: a run that only one program has fails the comparison, so
  a play that ends earlier or later on the port fails, and so does a play that only one program has (a run of the
  port that a record of several runs on the oracle stands for isn't one). Where a press reaches the two programs at
  different points of a sequence (START in the title sequence, `SoundSpec.before_count`), a play is compared up to
  the run at which either program's game count reaches the press, and the runs from there on are left out, counted
  for each program.
- **Frame by frame**, where the game counts passes (on the frames `compare.py --align counter` pairs; the movies'
  frames pair by their pictures, which the DS shows late against its clock, and the sound isn't timed by them): what
  the ARM9 sees of the driver as each frame's VBlank comes: the shared work's players and channels, the tick counts of
  the active players, the global variables, the locks, the alarms with their setting, and the SOUNDxCNT of the
  channels the ARM9 drives itself (a stream's, a cry's). A difference that the other program shows one run earlier or
  later (the run that made it fell on the other side of the VBlank) counts as equal a run apart. In these sequences no
  frame on the count has a player active (the opening's music plays over the movies), so this holds the locks, the
  alarms, the shared work and the channels the ARM9 drives; the players and tracks are held run by run, above.
  `sound.py` says how many compared frames have a player active and how many statuses they have between them. No
  setting of the port's driver could make a player's state at a VBlank the DS's: a frame of the port pairs with a
  frame of the DS some hundreds of frames later (the lags below), which differ from part to part of a sequence as the
  DS drops frames, and the driver's runs, every 2728 ticks, fall at another moment of each.
- **The sound, by segment of port frames**: each frame's RMS lined up at the lag (oracle frame = port frame + lag)
  where the two correlate best, which must be 0.97 or more; the segment's RMS and peak within 2%, its samples at full
  scale within 10% (or 32); and for a stream or a cry, sound the ARM9 writes and a channel plays at its own timer,
  every sample to the bit at the offset the loudest point gives. For the other segments it says how many samples are
  the oracle's at the sample offset where most are, and, where any are at full scale, in which seconds of the port's
  run and how many at most in a second on each program.

Results (`oracle.py sound`, melonDS 1.1):

| Sequence | Runs paired | Equal | Fields equal | Runs left out after the press (oracle, port) | Frames on the count | Equal | A run apart | With a player | Left out |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| opening, Black 2 | 9,639 | 9,639 | 6,279,801 of 6,279,801 | none | 579 | 579 | 0 | 0 | none |
| opening, White 2 | 9,639 | 9,639 | 6,279,801 of 6,279,801 | none | 579 | 579 | 0 | 0 | none |
| title, Black 2 | 1,957 | 1,957 | 636,043 of 636,043 | 4,940, 26 | 2,792 | 2,791 | 1 (3218) | 0 | 1112-1114, 3101 |
| title, White 2 | 1,957 | 1,957 | 636,043 of 636,043 | 4,706, 26 | 2,790 | 2,790 | 0 | 0 | 1112-1114, 3101, 3214, 3216 |

No run is on one program only. The plays the press cuts are the opening's music (player 1), which the title sequence
skips with START at count 1100: by then the port has played 295 runs of it and the DS, whose movie passes take two
frames, 5,209 (White 2: 4,975).

| Segment (port frames) | Lag | Envelope correlation | RMS, oracle and port | Peak | Samples at full scale | Samples the oracle's |
| --- | --- | --- | --- | --- | --- | --- |
| opening: the Game Freak logo (355-880), Black 2 | 254 | 0.9892 | 3846.7, 3846.7 | 18601, 18601 | 0, 0 | 89.8% at the best offset |
| opening: the opening's music (1017-3495), Black 2 | 263 | 0.9890 | 6980.3, 6983.8 | 32768, 32768 | 2277, 2271 | 15.6% at the best offset |
| opening: the title's stream (3530-3790), Black 2 | 315 | 0.9962 | 6350.2, 6354.2 | 29483, 29483 | 0, 0 | 285,564 of 285,564 |
| opening: the logo, White 2 | 254 | 0.9892 | 3846.1, 3846.7 | 18601, 18601 | 0, 0 | 36.2% at the best offset |
| opening: the opening's music, White 2 | 263 | 0.9890 | 6981.9, 6983.8 | 32768, 32768 | 2301, 2271 | 10.3% at the best offset |
| opening: the title's stream, White 2 | 314 | 0.9987 | 6352.2, 6354.2 | 29483, 29483 | 0, 0 | 285,564 of 285,564 |
| title: the title's stream (1116-3099), Black 2 | 1841 | 0.9891 | 8328.7, 8329.5 | 32668, 32668 | 0, 0 | 2,170,736 of 2,170,736 |
| title: Kyurem's cry (3104-3210), Black 2 | 1843 | 0.9993 | 5390.9, 5390.2 | 15240, 15240 | 0, 0 | 117,070 of 117,070 |
| title: the title's stream, White 2 | 1773 | 0.9872 | 8328.3, 8329.5 | 32668, 32668 | 0, 0 | 2,170,736 of 2,170,736 |
| title: Kyurem's cry, White 2 | 1775 | 0.9977 | 5999.9, 5998.9 | 15240, 15240 | 0, 0 | 117,070 of 117,070 |

What the comparison found in the port, each fixed in `port/hal` with a test:

- **The stream dropped a block every 7 frames or so.** The driver ran lazily, at each VBlank and each word of the
  ARM9, so its alarms' words (2.14 a frame at the title stream's 256 samples a block) reached the ARM9 in twos and
  threes at the VBlank; NitroSystem's stream player queues a block to read at each alarm and drops the oldest when a
  third comes before its thread has read one (`snd_arc_stream.c`'s `StrmCallback`), which zeroed a block and played
  the rest of the stream a block later: the title's music lost 27 frames against the DS's in 420. The next alarm is now
  the OS core's timed work, so each word comes when its alarm goes off (`snd_pxi_alarm_timing`, `os_timed_work`). An
  alarm set for no ticks, due at once, went off only at the next VBlank until review found it; it now goes off at once
  too.
- **The sound unit started its sounds at once.** On melonDS a PCM sound's first sample comes on the channel's third
  timer step, an IMA-ADPCM one's on its eleventh, a square wave's or noise's on its first, so a one-shot sound ends that
  much later: one channel of the Game Freak logo's ended a run earlier on the port. And a channel reads its sound ahead
  through a FIFO of 8 words, so a stream's buffer rewritten just ahead of the channel plays as it was: the ARM9,
  which takes no time on the port, rewrote the title stream's blocks two samples before the channel had played them,
  which played those two samples of every block from the buffer's next round (`snd_fifo`).
- **The mixer's widths.** melonDS mixes by GBATEK's widths and counts a volume, pan or master volume of 127 as 128;
  the port's had kept the whole product and 127, 1.6% quieter at full volume (`snd_mixer`, `snd_psg_duty`).
- **A channel's timer restarted at a write to SOUNDxTMR** (found by review). The port counted each step from its
  start against the current period, so a pitch change (an LFO's, a sweep's, a bend's, which most of the opening's
  notes have) moved the note's phase; the hardware's timer, and melonDS's, counts on and reloads from SOUNDxTMR at the
  next overflow (`snd_timer_write`). It changed 0.8% of the opening's music's samples.
- The oracle's own first reading had the two lock words the wrong way round (the ARM9's alone are at `0x03809f98`,
  `SND_LockChannel`), and so did `snd7_exchannel.c`'s comment.

With those, the sound of a stream and of a cry is melonDS's to the bit, and every state the driver goes through in
the four sequences is the DS's. What stays apart, by construction:

- **Note starts.** The port's runs take no time; the DS's ARM7 reaches its register writes some way into a run.
  Counted from the first mixer sample after each program's periodic alarm, a note that the run starts takes its start
  in that sample on the port and 1 to 5 samples later on the DS (the opening's music in Black 2: 43, 232, 147, 8 and 3
  notes at 1 to 5 samples; the logo's 12 and 3 at 1 and 2), and the run's other register writes (the envelopes'
  volumes) are as late. So the sequences' sound matches in its envelope and levels but not sample by sample: at the
  sample offset where most agree, 15.6% of the opening's music's samples are the oracle's, and 89.8% of the logo's.
  The oracle's sampler can't see a start that comes within one mixer sample of the channel's stop (61 in the
  opening's music, whose start it reports from before the run); those aren't counted.
- **The DS's long passes.** While it loads the title (counts 1111 and 1113, passes of 2 and 46 frames) the DS locks
  the stream's channels and starts the stream at the end of its pass, the port at its start: port frames 1112-1114
  of the title sequence, whose status is left out.
- **Kyurem's cry** starts a pass later against the DS's passes (port frame 3101) and so ends later too, as
  [below](#known-differences-from-the-hardware): as port frame 3214 ends (White 2) the DS's channel still plays it, in
  Black 2 only a run later (port frame 3218, equal a run apart); and in White 2 the title sees it end a pass later and
  unlocks its channel a pass later (port frame 3216).

**The clipping in the opening's music is the game's, on the DS's scale.** Both mixers output 16 bits on the scale
of the DS's 10 bits times 64, clipped at the same level (a channel at full volume and center pan is half the range);
neither cuts the low 6 bits as the DS's 10-bit output does (melonDS only does when told to, and the oracle doesn't
tell it). The opening's music reaches full scale in the same seconds on both: in Black 2, 2,271 samples at full
scale on the port and 2,277 on the oracle, in seconds 24 to 57 of the port's run, at most 421 in a second on the port
and 440 on the oracle (both in second 52); in White 2, 2,271 and 2,301, at most 421 and 434. Before the mixer took
melonDS's 127 as 128 the port had 2,007. So there was no scaling or saturation to fix beyond that.

Each sequence is two ctest tests:

- `oracle_sound_<name>` (`oracle.py sound NAME --port BIN`), with the oracle built: both programs, compared as above.
  With `--update` it writes the reference, `port/tests/oracle/sound_<name>_<version>.tsv`, from a comparison that
  passes: the oracle's values only, as hashes and small statistics: the press's count, each player's plays before it
  and each play's runs (the first one left out at the press, or none, so that the play must end where the oracle's
  does), the SHA-1 of the oracle's states of each 64 runs of each play, the hash of its status at every 8th frame and
  each frame where it changes (of those equal at their ends, not a run apart, which the moment of a run decides), each
  segment's lag, RMS, peak and samples at full scale and its RMS by blocks of 4 frames, and for a segment to the bit
  the SHA-1 of the oracle's samples over it and at its loudest point. Skipped (77) without the oracle or the ROM.
- `oracle_sound_<name>_hashes` (`oracle.py sound-check NAME --port BIN`), every build: the port alone, with its trace
  and WAV dump, against the reference: each play's runs (none missing, none extra, no play the oracle doesn't have),
  every run's state, each record in the frame whose VBlank comes next, the statuses, the segments' envelopes and levels,
  and the samples to the bit (wherever the loudest point's hash is, within two frames). Skipped (77) without the ROM.

## Integration

What the port needs to compare itself frame by frame:

- **Its own run directory** (done: `--run-dir`, `--run-screens` and `--run-dump`, [above](#run-directories)), so
  that one run gives every frame and `oracle.py port` runs the port once.
- **Replays** (done: `port/app/replay.c`, GFL's frame count read through `port/hal/gfl/gfl_use_host.c`): `--input FILE`
  read with `pbwinput.c` and one `PbwGameClock` for the run, which also gives the run directory its game frames.
  Before the game starts, the program applies `pbw_input_state(&input, 1)`; at the frame hook of each frame,
  `pbw_input_state(&input, pbw_input_frame_at_vblank(&input, &clock, frame, count))`, with `frame` the hook's count
  since `main()` and `count` GFL's frame count. The state goes in through `port_input_set` and the touch panel's host
  setter. The real-time clock at power-on is `--fixed-time`, else the file's `time` line, else the format's default
  (2012-10-07 12:00:00), never the host's, and it stays there for the run when the file's `clock` line pins it;
  `oracle.py port --input` passes `--fixed-time` only when it is given `--time`, as `oracle.py run` does with the
  oracle.
- **Memory dumps** by name, written as `memory.txt` (done for globals: `--run-dump`, `oracle.py port --dump`). The
  game's statics have no name the program can look up; a static the comparisons need will need the port to export
  its address.
- **The sound** (done: `--sound-trace`, `port/app/sound_trace.c`, and `--audio-dump`, [Sound](#sound)): the sound
  driver's state after each run, which no dump at a VBlank can give the DS's of (the driver's runs fall at other
  moments of the frame on each program), and its samples.

## Known differences from the hardware

melonDS is a reference, not the hardware. Where the two programs can't agree by construction:

- The ARM7's polling of X, Y, the lid and the touch screen runs on its own timing in the oracle; the port sets them at
  once.
- melonDS's touch calibration is its own (pixel × 16); the port's is the one its firmware settings give.
- Timing: the oracle's CPUs take time and the DS drops frames, which is why frames align on the game's count, and
  why the counts part where the game waits on a clock ([above](#game-frames-and-video-frames)).
The comparison sequences declare the rest frame by frame (`oracle.py`'s `SEQUENCES`: `known`, port frames left out of
the screens; `known_top`, port frames whose top screen alone is left out; `known_oracle`, oracle frames no port frame
can show; `known_state`, port frames whose state differs; `lags`), so that every frame that differs at the game's count
is either one of these or a failure. A difference is declared only with its cause traced: one whose cause is open is a
failure to fix, never a known difference. By kind:

- **A register written partway through a frame.** The DS's CPU takes time, so a pass that changes the display writes
  the registers while the DS draws the frame's lines: that frame shows the old setting above some line and the new one
  below it. The port's passes take no time and run at the VBlank, so the change shows from the next frame's first
  line. At the same count, the port's frame is then the new picture and the oracle's a torn one, or the old one when
  the DS's pass took two frames and tore the second:
  - the title hiding its layers (count 2877): port frame 2878 against oracle 4719 (Black 2, white from line 93) and
    4651 (White 2);
  - START bringing them back (count 3000): port frame 3001 against oracle 4842 (Black 2, the screens swapping from line
    60) and 4774 (White 2, from line 68);
  - START leaving for the start menu (count 3100), a pass of two frames on the DS: port frame 3101, white, against the
    pass's first frame, oracle 4942 (Black 2) and 4874 (White 2), still the title; its second, 4943 and 4875, is torn,
    the title above line 173 or so and white below, and no port frame shows it;
  - the opening's first frame in White 2, oracle 1283, black above line 77, which no port frame shows (in Black 2 it
    comes whole);
  - in the exit route (aspertia), the display turned off after the intro, late in a DS pass of some 30 frames (count
    7750 in Black 2, 7810 in White 2): the port's frame at that count is white from its first line (port frame 7751,
    7811), where the DS's is still the intro's black, and the DS tears a later frame, black above line 115 (oracle
    9841, Black 2) or line 189 (oracle 9837, White 2, whose next frame is white), which no port frame shows.
- **VRAM written after the beam has drawn it.** The same holds for VRAM: a pass that writes a part of the screen that
  the DS has already drawn shows it a frame later there. In the exit route Mom's speech balloon, at the top of the
  screen (lines 4 to 44), scrolls a line up 4 pixels a pass (`PrintStream_Task`, `src/system/printsys.c`), copying the
  window's bitmap with the CPU before it flushes it to VRAM, which takes the DS milliseconds: each of the four steps
  shows a frame later on the DS, its window region then equal to the port's frame before, pixel for pixel, with no torn
  line (port frames 12911-12914, 13727-13730, 13967-13970 and 14223-14226, in both versions). The letters printed into
  the same window, with no copy, and the townsperson's window at the bottom of the screen, drawn after the copy ends,
  show on the same count on both.
- **Memory the game reads past a heap block.** The copyright screen loads 0x8000 bytes of a smaller file's
  characters, so the tiles past them are whatever the heap held after the file's block
  ([../../docs/sanitizers.md](../../docs/sanitizers.md), H1), and the title's bottom screen shows them as a band over
  lines 38 to 95 as it fades in (port frames 3527-3536 of the opening, 1116-1125 of the title). From a blank save the
  port's heap holds the DS's bytes there, 8 pixels apart at the first frame, and the opening and the title hold those
  frames close. From a save (continue, continue_starter) it holds other bytes there than the DS's heap does, most
  likely because what the start-up's earlier allocations left there sits elsewhere in the host's larger heaps and
  blocks, and the band's noise differs: 10,764 of the bottom screen's pixels at port frame 1116 in Black 2, a step or
  two from black, within the 3D's tolerance, and fewer each frame as it fades.
- **A frame the DS drops where the game steps by VBlanks.** The title moves its camera once for each VBlank since its
  last pass (`Title_Update` in `src/ov162/title.c`) and Kyurem's animation once a pass. The DS's pass at START
  (count 3100) takes two frames, so from then on its camera is a step ahead of the animation, which on the port,
  dropping none, it never is. NNS G3D's state shows it: `NNS_G3dGlb` (the camera matrix among it, 612 bytes) is the
  same on both at every count compared but 3109 to 3123, where the oracle's at count c is the port's at c + 1 (port
  frames 3110-3124, in both versions); it agrees again once the camera stops. The screens of port frames 3112-3129
  (Black 2) and 3112-3128 (White 2) differ beyond the tolerance meanwhile. To see it:

  ```sh
  oracle.py run --out o --frames 4990 --input port/tests/oracle/title_b2_us.pbwinput --dump NNS_G3dGlb@4925-4985
  oracle.py port --binary build/port/frigate-black2 --out p --frames 3140 \
      --input port/tests/oracle/title_b2_us.pbwinput --dump NNS_G3dGlb:612@3090-3140
  compare.py memory o p --align counter --clock 313-1108 --lag 1109=0
  ```

  The summary of party_menus is the same kind: it scrolls its background a pixel right and down every 4 VBlanks since
  it started (`PStatus_Main` in `src/ov207/p_sta_sys.c`), and the DS spends 38 VBlanks more than passes loading it
  (counts 4526 to 4531 in both versions, by each program's VBlank count in `frames.tsv`), where the port spends none.
  So its background is about 9 pixels further on on the DS for as long as it shows, port frames 4534-4822 (counts
  4533-4821) in both versions, while its text, Tepig and the rest of its screens are the port's (`SUMMARY_SCROLL`).
  The party list before and after it, Tepig's menu and every other app of the sequence match frame for frame.

  The exit route's zoom into the house is the same kind: entering a building, the field captures the screen to BG 2 and
  a VBlank callback scales it up at every VBlank (`src/ov012/building_enter_effect.c`) while the house loads and the
  screens crossfade, and the DS spends some 45 VBlanks more than passes there (counts 15160 to 15190, from 79 VBlanks
  against the port's 33), so at each count its zoom is further on, and it shows steps no port frame shows (port frames
  15176-15229, oracle frames 17462-17519 in Black 2 and 17401-17457 in White 2). Leaving the house shows none: its
  effect steps by passes.

- **A save the DS writes in time, which the port writes at once.** New Game's intro saves its first data in the
  background, and before it fades to the name entry it waits for that save to pause (`IntroCmd_SavePause`,
  `src/ov294/intro_cmd.c`): the DS's flash writes take time, so it pauses a pass later there. From the fade on, the
  port is a pass ahead (`--lag 5538=-1` in Black 2, `5598=-1` in White 2) until the name entry waits for its first
  press, which both read on the same pass (`5577=0`, `5637=0`). The house replay's SAVE shows the same: the DS shows
  more frames of its saving.
- **The game's state in the middle of a long pass.** Where a DS pass takes several frames (loading the intro, the
  season's name, the field and each map), NNS G3D's state at its first frame, which the comparison pairs with the
  port's frame on the count, is the pass's work in progress; at its later frames it is the port's. In the exit route
  these are 14 frames in each version (`MID_PASS`), each checked against the DS's later frames of the same count.
- **A budget of the CPU's time a pass.** The trainers' AI runs its scripts for at most 500 ticks of `OS_GetTick` a pass,
  about a millisecond, and goes on where it stopped at the next (`TRAI_TIME_LIMIT`, `TrAI_RunScripts` in
  `src/ov170/tr_ai.c`). On the DS the scripts take the CPU's time; on the port each `OS_GetTick` moves the virtual clock
  a tick, so the AI runs 500 of its checks a pass and thinks in one. The battle's view draws two numbers a pass from
  GFL's Mersenne Twister (`BtlvMcss_Main`'s blinks, `src/ov168/btlv_mcss.c`), and the AI picks among its best moves with
  the next number (`TrAI_ChooseMove`), so its pick depends on how many passes it thought. Traced by the twister's
  callers on both programs, with the DS's seeds (Black 2, the Route 19 route from the starter save): GFL's linear
  generator's state and the twister's index are the DS's at every count to Hugh's battle; at each of his turns his AI's
  setup draws in the same pass on both (count 7495 for the first), and the port's AI picks at the next pass, the DS's
  two passes later, the seventh number after the setup where the port's is the third. Oshawott's first pick is the same
  by chance, its second (count 7969 on the port) is another, and the battle differs from that turn's rolls on (count
  8193) ([../../docs/decomp-findings.md](../../docs/decomp-findings.md)). The trace was made with scratch
  instrumentation (a log of `GFL_RandomMT`'s callers in a scratch build of the port and a PC hook in a scratch build of
  melonDS), which no test holds yet. No host has the DS's time, so a trainer battle is held to melonDS only while the AI
  thinks within a pass there: a sequence stops before the AI's first decision that the DS's time moves, and the route
  goes on from melonDS's save made after the battle ([ADR
  0023](../../docs/adr/0023-oracle-routes-part-where-the-ds-cpu-time-decides.md)). A wild Pokémon has no AI scripts (no
  AI flags, `GetClientAIFlags` in `src/ov167/btl_main.c`) and picks at once.
- **Random numbers.** GFL seeds its random numbers from a hash that includes a buffer the DS's stack fills
  ([../../docs/decomp-findings.md](../../docs/decomp-findings.md), `buildSeed`), so the port's differ from the DS's
  unless the input file gives the DS's seeds ([Random seeds](#random-seeds)), which the exit route's doesn't.
  In the exit route, Minccino plays its idle animation, about 290 frames long, after a random wait of 200 to 280
  passes in steps of 20 (`GFL_RandomMTRange(5) * 20 + 200`, `IntroMcss_Update` in `src/ov294/intro_mcss.c`), so its
  pose on the top screen can differ. The port starts the idle at port frame 4668 in both versions. White 2's DS draws a
  wait three steps longer and starts it 60 passes later (count 4727), so that its top screen differs for as long as
  either program plays it, port frames 4668-5017 (beyond the tolerance from 4673 to 5007), and again as Minccino fades
  out, 5038-5040, its pose following the idle. Black 2's DS draws the port's wait, which is chance, one in five: its
  frames match, but another seed, a change to what `buildSeed` hashes on the port among them, would move them. Since
  the port's wait is one of the two shortest, an idle can start from port frame 4648 to 4748 whatever either program
  draws, and it ends by the fade-out. So port frames 4648-5040 leave the top screen out in both versions (`known_top`),
  and their bottom screen, the same picture throughout, is compared exactly.
  The route keeps to streets the city's wandering townspeople (npcs 9 to 12) don't reach on screen, and the lens flare
  that a random choice picks on leaving a building (`src/ov036/field_goout_effect.c`) shows none.

- **Kyurem's cry starts later against the DS's passes (White 2).** The title waits for the cry it starts at START
  before it fades out (`STATE_WAIT_CRY`). The pass that starts it (count 3100) takes the DS two frames, loading the cry,
  and the ARM7's driver runs its commands only after that pass: the shared work's count of finished commands
  (`SNDi_SharedWork->finishedCommandTag`) stays put through both frames and moves on, with the cry's channel (15)
  playing, by the VBlank after them (oracle frame 4876 in White 2, 4944 in Black 2). The port's driver runs a packet
  as soon as the ARM9 sends it (`port/hal/snd7/snd7.c`), where the DS's waits for its next update, up to 5.2 ms
  later; so the port's channel plays from that pass, at the VBlank of port frame 3101. Against the passes that follow,
  the DS's cry starts 1 to 2 frames later. It lasts as long on both: by the dumps at each VBlank (the oracle's taken
  before either CPU runs, the port's before its ARM7 catches up), the port's channel stops in the frame before the
  pass of count 3214 starts (Black 2: 3218), 113 to 114 frames after it began, and the DS's in the frame that pass
  runs in, 112 to 114 frames after. That pass reads the channels' status on the port after the stop; on the DS, in
  White 2, before it, so that the title sees the cry end a pass later there and fades out a pass later, and in Black
  2 after it, on the same pass as the port. White 2's fade steps its master brightness at counts 3218, 3221 … on the
  port and 3219, 3222 … on the oracle: port frames 3219-3240, every third, are a step darker than the oracle's at the
  same count, and from the start menu on (port frame 3241) the port is a pass ahead (`--lag 3241=-1`). In the continue
  sequence the start menu's cursor, which pulses through a cycle of pictures a step at each pass, pulses a step ahead on
  the port with it, and CONTINUE's press, which both read on the same pass, stops the pulse on the same count, so the
  counts pair again from port frame 3403 (`--lag 3403=0`). On the press's own frame, port frame 3402 (count 3401), the
  port shows the step after the DS's last one (its top screen hashes `5e43ed31`), a picture the DS shows elsewhere in
  the pulse, at counts 3274, 3296, 3338 and 3360 of the same run, but not on that count: a known frame. To see the cry
  (the oracle's shared work is where `SNDi_SharedWork` points, 0x0214c740 in White 2's run):

  ```sh
  oracle.py run --version w2_us --out o --frames 5030 --input port/tests/oracle/title_w2_us.pbwinput \
      --dump SNDi_SharedWork:4@4860 --dump snd=0x0214c740:12@4860-5030
  oracle.py port --version w2_us --binary build/port-w2/frigate-white2 --out p --frames 3260 \
      --input port/tests/oracle/title_w2_us.pbwinput --dump 'snd=*SNDi_SharedWork:12@3090-3260'
  ```

  Each dump is the count of finished commands (a word), the players' status (a word) and the channels' (a half-word),
  whose bit 15 is the cry's channel.

  The exit route presses NEW GAME, the start menu's first item without a save, where continue presses CONTINUE: its
  press ends the pulse the same way, and port frame 3402 shows the step after the DS's last one (top screen
  `d3c85a7e`), which the DS shows at counts 3274, 3296, 3338 and 3360 but not on that count.
