# The melonDS oracle

The reference the port is held against ([PLAN.md §6](../../docs/PLAN.md#6-verification), test level T4): the player's
ROM running on [melonDS](https://melonds.kuribo64.net)'s emulation core, headless and deterministic, with an input
file, writing both screens and named memory at chosen frames. `compare.py` compares those with the port's.

It is a **test tool only**. It is never linked into or shipped with the port
([PLAN.md §1](../../docs/PLAN.md#1-goal-and-what-native-means)): it is a CMake project of its own, built only on request, and nothing under `port/` other than this directory, its
tests and its CMake fragment refers to it, except the input file reader (`pbwinput.c`, plain C with no melonDS), which
the port compiles for its replays (`port/app/replay.c`). melonDS is GPL-3.0, as this repository is; the oracle links melonDS's core
and is GPL-3.0 with it. melonDS is fetched at build time, pinned to a release, and never committed.

| File | What it is |
| --- | --- |
| `oracle.cpp`, `platform.cpp`, `symtab.*`, `pngout.*` | `pbw-oracle`, the program on melonDS's core |
| `display3d.c`, `display3d.h` | which screens show 3D in a frame, from the display registers; plain C11 |
| `pbwinput.c`, `pbwinput.h` | the input file reader and writer and the game-frame clock, plain C11, which the port can compile as it is |
| `oracle.py` | runs the oracle and the port with the repository's defaults; the ctest check |
| `compare.py` | compares two runs: screens, memory, alignment |
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
oracle.py port --binary build/port/pokeblack2 --out build/oracle-runs/port --frames 1,30,60,120,300,600,1000
compare.py screens build/oracle-runs/boot build/oracle-runs/port --align sequence --report build/oracle-runs/report
oracle.py boot --out build/oracle-runs/both --port build/port/pokeblack2
oracle.py title --port build/port/pokeblack2 --out build/oracle-runs/title
oracle.py title-check --port build/port/pokeblack2
```

`oracle.py run` calls `pbw-oracle` with the version's ROM (`orig/baserom_b2_us.nds`), its symbols
(`config/b2_us/arm9`), the statics of the ROM build's link map and GFL's frame count as the counter. It passes `--time`
only when it is given one, so that an input file's `time` line applies, else the default clock. `oracle.py port` runs
the port once, from power-on with a blank save, to the last frame of its list, and has it write its own run directory
(`--run-dir`, [below](#run-directories)) with a picture of each frame of the list (`--run-screens`); its clock is
`--time`, else `--input`'s file's, else the default. A run that ends early (a trap stub, a crash) is reported with the
port's message, and the frames it didn't reach have no picture. `oracle.py boot` does both for the boot sequence's
frames, the oracle 400 frames further with a PNG of every frame, and compares them by sequence at 6 bits and at 5.
`oracle.py title` and `title-check` are the title sequence's comparison and its check without the oracle
([below](#the-title-sequence)). `pbw-oracle --help` and the scripts' `-h` list every option.

`pbw-oracle`'s options, which `oracle.py run` fills in:

| Option | Meaning |
| --- | --- |
| `--rom PATH`, `--out DIR`, `--frames N` | the ROM, the run directory, how many frames to run |
| `--input FILE` | an input file (below); without one nothing is held. One on game frames needs `--counter`, and its `version` must be the ROM's (status 2 otherwise) |
| `--screens LIST` | the frames to write as PNG: `N`, `A-B`, `A-B/STEP`, `all` or `none`, comma-separated |
| `--dump [LABEL=]TARGET[:SIZE][@LIST]` | memory to write to `memory.txt` at the frames of LIST (the last frame by default) |
| `--config DIR`, `--symbols FILE` | where symbols come from (below) |
| `--counter TARGET` | a 32-bit counter recorded for every frame: GFL's frame count, for alignment and game frames |
| `--time "YYYY-MM-DD HH:MM:SS"` | the clock at power-on; else the input file's; else `2012-10-07 12:00:00` |
| `--save FILE`, `--save-out FILE` | the save memory at power-on (blank otherwise), and where to write it at the end |

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
frame (`frames.tsv`'s `counter`). Two frames pair when each is the first of its run where the count reaches the same
value (`compare.py --align counter`). That holds while the game counts passes; where it waits on a clock, the DS makes
fewer, longer passes than the port and the counts part ([below](#game-frames-and-video-frames)). The alignments:

- `--align sequence`, the one to use: each port frame, in order, pairs with an oracle frame from the previous pair's on
  that shows the same pictures, compared at the palettes' depth (5 bits a channel) when both runs have a picture of
  every frame, else by `frames.tsv`'s hashes; of those, the one nearest to where the port frame is expected, as many
  game frames on as it is (by the counter, else by frames). Where the port's picture changes, it is expected from the
  end of the previous picture on each, since the DS shows a still for longer while it loads or waits on a clock. A
  port frame that no oracle frame shows (a screen with 3D, or a difference) pairs with where it is expected, and is
  noted "extrapolated": the title's 3D pairs on the counter from the black that comes before it. It checks that the
  port shows the same pictures in the same order, and with the counter the same frames between them.
- `--align counter`, for a stretch where the game counts passes only (the boot screens, the title after a press).
- `--align first-change`: the offset between the first frames whose screens aren't blank (all but a line's worth of
  pixels one colour). It holds until the first frame the DS drops, and it needs every frame up to those in both runs.
- `--align image`: the offset at which most of the port's frames show exactly the screens of an oracle frame, from
  `frames.tsv`'s hashes, which the oracle writes for every frame (`--screens` only decides which get a PNG).

`compare.py align ORACLE PORT` reports the first two offsets and, for each port frame, the oracle frames that show
exactly its screens.

### Game frames and video frames

What the boot's frame offset is, and how it maps game frames, as Black 2 shows it (no input or the title sequence's,
blank save; `frames.tsv` of both programs):

- **Start-up.** The oracle's first 242 frames are direct boot's start-up on a timed CPU: the ARM9's start-up code
  (unpacking the compressed ARM9 binary) and `GameInit` (the save, the sound data). The count moves from frame 244 on
  the oracle and from frame 2 on the port. Through the boot screens and the copyright screen every pass is a frame on
  both, so oracle frame = port frame + 242, with the same count and the same pictures: game frame N is video frame
  N + 1 on the port and N + 243 on the oracle (its pass runs during that frame).
- **Loading and clocks.** From the Game Freak logo on, the offset grows at each load (a pass that takes the DS several
  frames) and the counts part in the movies: the logo and the opening (`src/ov264/cdemo_control.c`) show a new movie
  frame each 1/15 s of `OS_GetTick`, and on the DS a pass that decompresses one takes 4 or 5 frames, on the port 1. The
  oracle and the port show the same movie frames in the same order, the DS each for 4 or 5 frames (late when a pass
  takes 5), the port for 4 (3 when its 1/15 s add up to one more): the logo's first frame is oracle 621 (count 359) and
  port 368 (count 367), the opening's 1283 (633) and 1018 (1017), the title's 3842 (1349) and 3527 (3526). Video frames
  don't map, and game frames don't either once a movie has run.
- **Presses line them up again.** Input on game frames reaches both on the same pass. A press that ends a stretch on a
  clock ends it on the same pass, and what follows counts passes again: START at game frame 1100, in the opening on
  both, skips to the title at count 1115 on both, and from there the title and the start menu pair on the counter
  (oracle = port + 1841), but where the DS drops a frame.

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
# FRAME BUTTONS [TOUCH]
120 A                         # A held from frame 120
122 -                         # nothing held from frame 122
300 START+A
400 - 128,96                  # the pen down at x 128, y 96 of the bottom screen
402 -
900 LID                       # the lid closed
```

- The first line that isn't blank or a comment is `pbwinput 1`, the format's version.
- `time`, `version` and `frames` come before the first frame line, once each. `time` is `YYYY-MM-DD HH:MM:SS` from
  2000 to 2099; the oracle's `--time` overrides it. `version` is checked against the ROM's game code (`IREO` for
  `b2_us`, `IRDO` for `w2_us`).
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
- The port writes its own with `--run-dir DIR` and `--run-screens LIST` (`port/app/run_dir.c`, `port/app/main.c`): a
  `frames.tsv` row at each frame hook with the video frame, NitroSDK's VBlank count (the port's ARM7 side counts each
  VBlank at the VBlank itself, so it is the frame's number), GFL's frame count, the game frame and each screen's hash
  and blank, `-` for `keyread`, `top_3d` and `bottom_3d` (compare.py takes 3D from the oracle's run); PNG files at the
  frames of LIST, as `--screens` takes it; and `run.txt`, written at the start and again at the end with the frames
  completed and the exit status, so that a run a trap stub ends keeps its frames and says it didn't finish.
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
same place, which forgives a rasterizer drawing an edge one pixel over. `--report DIR` writes, for each frame that
isn't exact, the oracle's screens, the port's and the differences side by side (the oracle's picture dimmed, forgiven
differences yellow, the others red). Without a PNG on one side, `frames.tsv`'s hashes decide a screen at 6 bits
without 3D (they are of the 8-bit output, the 6 bits widened); a screen with 3D or at `--depth 5` whose hashes differ
is reported "not compared", which fails the run as a difference does.

`compare.py memory ORACLE PORT` compares `memory.txt` by frame and label, for values whose bytes are the same on both:
counters, flags, random number states, arrays of plain numbers. Pointers and structures that hold them differ on a
64-bit host by design.

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
save unless one is given, no host time, no audio output, and no file but the ROM and the given ones (melonDS's local
files are refused). The firmware is melonDS's generated one with the owner settings of the port's
(`port/hal/nvram/nvram7.c`): nickname "Player", colour 11, birthday 6/23, English, MAC `02:42:57:32:00:01`; melonDS
keeps its own touch calibration. `oracle.py check`, the ctest test `oracle_boot`, runs the first 500 frames twice,
compares the results, checks that the PNG files and `frames.tsv` agree, and compares the screens of 11 frames with
`port/tests/oracle/boot_<version>.tsv`, hashes the pinned melonDS gives (`oracle.py check --update` writes them). It
exits 77 (skipped) without the oracle or without the ROM the decomp builds.

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

## The title sequence

`port/tests/oracle/title_<version>.pbwinput`, on game frames: power-on, the boot screens, the Game Freak logo and the
opening until START at game frame 1100 skips it, the title (its 3D on the bottom screen, then, once the title hides its
layers after 1,740 of its frames, on the top), START at 3000 to bring the layers back, START at 3100 for the start
menu. The port runs 3,600 frames, the oracle 5,700, which take both as far.

- `oracle.py title --port BIN` runs both, compares every port frame by sequence (3D within the tolerance) but the
  known differences (`oracle.py`'s `TITLE_KNOWN`, [below](#known-differences-from-the-hardware)), and with `--update`
  writes `port/tests/oracle/title_<version>.tsv` from a comparison that passes. It is the ctest test `oracle_title`,
  skipped (77) without the oracle or the ROM.
- `oracle.py title-check --port BIN` runs the port alone and checks its screens without 3D, at the frames the
  reference keeps (every 12th, and each around the presses), against the oracle's hashes there. It is the ctest test
  `oracle_title_hashes`, skipped (77) without the ROM. It proves those screens are the oracle's at 6 bits a channel
  and that the port reaches each moment at the same frame as when the reference was written; it can't judge a screen
  with 3D, nor the frames between those it keeps, which only `oracle_title` covers.

Black 2, the port's 3,600 frames: 3,581 match, 1,476 exactly (both screens) and 2,105 with a screen of 3D within the
tolerance; the 19 left out are the known differences. White 2, the same presses (`title_w2_us.pbwinput`): 3,575 match,
1,480 exactly and 2,095 within the tolerance, and 25 are known differences. Through the boot screens, the Game Freak
logo and the opening every frame is exact; the 3D frames' differences are pixels along polygon edges and the floor's far
edge, a median of 79 a screen beyond the pixel tolerance and at most 172 of the 245 allowed. With no input, the port's
first 4,185 frames (the whole opening and the title's first 659) match too, by the same alignment.

## Integration

What the port needs to compare itself frame by frame:

- **Its own run directory** (done: `--run-dir` and `--run-screens`, [above](#run-directories)), so that one run gives
  every frame and `oracle.py port` runs the port once.
- **Replays** (done: `port/app/replay.c`, GFL's frame count read through `port/hal/gfl/gfl_use_host.c`): `--input FILE`
  read with `pbwinput.c` and one `PbwGameClock` for the run, which also gives the run directory its game frames.
  Before the game starts, the program applies `pbw_input_state(&input, 1)`; at the frame hook of each frame,
  `pbw_input_state(&input, pbw_input_frame_at_vblank(&input, &clock, frame, count))`, with `frame` the hook's count
  since `main()` and `count` GFL's frame count. The state goes in through `port_input_set` and the touch panel's host
  setter. The real-time clock at power-on is `--fixed-time`, else the file's `time` line, else the format's default
  (2012-10-07 12:00:00), never the host's; `oracle.py port --input` passes `--fixed-time` only when it is given
  `--time`, as `oracle.py run` does with the oracle.
- **Memory dumps** by name, written as `memory.txt`, for globals whose bytes don't depend on the host (not yet).

## Known differences from the hardware

melonDS is a reference, not the hardware. Where the two programs can't agree by construction:

- The ARM7's polling of X, Y, the lid and the touch screen runs on its own timing in the oracle; the port sets them at
  once.
- melonDS's touch calibration is its own (pixel × 16); the port's is the one its firmware settings give.
- Timing: the oracle's CPUs take time and the DS drops frames, which is why frames align on the game's count, and
  why the counts part where the game waits on a clock ([above](#game-frames-and-video-frames)).
- **A register written partway through a frame.** The DS's CPU takes time, so a pass that changes the display writes
  the registers while the DS draws the frame's lines: that frame shows the old setting above some line and the new one
  below it. The port's passes take no time and run at the VBlank, so the change shows from the next frame's first line.
  In Black 2's title sequence: oracle frame 4719 (count 2877, the title hiding its layers, white from line 93) has no
  counterpart, and port frame 3001 differs from oracle frame 4842 (START bringing the layers back, the swap of the
  screens from line 60); in White 2's, port frame 3001 and oracle frame 4774 (from line 68).
- **A frame the DS drops where the game steps by VBlanks.** The title moves its camera once for each VBlank since its
  last pass (`Title_Update` in `src/ov162/title.c`) and Kyurem's animation once a pass. The DS drops a frame on the
  pass that starts Kyurem's cry at START (count 3100, oracle frames 4942 and 4943), so its camera is a step ahead of
  the animation from then on, which on the port, dropping none, it never is: NNS G3D's camera position at count 3109 on
  the oracle is the port's at 3110. While the camera moves, port frames 3112–3129 (oracle 4954–4971) differ beyond the
  tolerance; they match again once it stops. White 2 drops the same frame (oracle 4874 and 4875): port frames
  3112–3128.
- **The sound driver's clock.** The ARM7 updates the sound every 2,728 ticks of its OS (5.2 ms) from when its driver
  starts, which falls at another point against the VBlanks on each program (the DS's start-up runs on a timed CPU for
  242 frames). So a pass that waits for a sound to end can see it end a pass apart. The title waits for Kyurem's cry
  before it fades out (`STATE_WAIT_CRY`), and in White 2 the port sees the cry end a pass earlier: the fade's master
  brightness steps at counts 3218, 3221, 3224 … on the port and 3219, 3222, 3225 … on the oracle, so port frames
  3219–3237, every third, are a step darker than the oracle frames they pair with, and the start menu shows from count
  3245 on the port, 3246 on the oracle. In Black 2 the cry ends on the same pass on both.
