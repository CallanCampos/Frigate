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
| `sound.py` | compares two runs' sound: the sound driver's state run by run, what the ARM9 sees of it frame by frame, and the sound by segment ([Sound](#sound)) |
| `sndtrace.h` | the sound trace's format, plain C11, which the port includes for its own trace |
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
compare.py screens build/oracle-runs/boot build/oracle-runs/port --align counter --clock 313-1000 \
    --report build/oracle-runs/report
oracle.py boot --out build/oracle-runs/both --port build/port/pokeblack2
oracle.py sequence title --port build/port/pokeblack2 --out build/oracle-runs/title
oracle.py sequence-check title --port build/port/pokeblack2
oracle.py sound opening --port build/port/pokeblack2 --out build/oracle-runs/sound
oracle.py sound-check title --port build/port/pokeblack2
```

`oracle.py run` calls `pbw-oracle` with the version's ROM (`orig/baserom_b2_us.nds`), its symbols
(`config/b2_us/arm9`), the statics of the ROM build's link map and GFL's frame count as the counter. It passes `--time`
only when it is given one, so that an input file's `time` line applies, else the default clock. `oracle.py port` runs
the port once, from power-on with a blank save, to the last frame of its list, and has it write its own run directory
(`--run-dir`, [below](#run-directories)) with a picture of each frame of the list (`--run-screens`) and the memory its
`--dump`s name (`--run-dump`); its clock is `--time`, else `--input`'s file's, else the default. A run that ends early
(a trap stub, a crash) is reported with the port's message, and the frames it didn't reach have no picture.
`oracle.py boot` does both for the boot sequence's frames, the oracle 400 frames further with a PNG of every frame,
and compares them on the count, the movies by their pictures, at 6 bits and at 5. `oracle.py sequence` and
`sequence-check` are the comparison sequences and their check without the oracle
([below](#the-comparison-sequences)). `pbw-oracle --help` and the scripts' `-h` list every option.

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
- The port writes its own with `--run-dir DIR`, `--run-screens LIST` and `--run-dump SPEC` (`port/app/run_dir.c`,
  `port/app/main.c`): a `frames.tsv` row at each frame hook with the video frame, NitroSDK's VBlank count (the port's
  ARM7 side counts each VBlank at the VBlank itself, so it is the frame's number), GFL's frame count, the game frame
  and each screen's hash and blank, `-` for `keyread`, `top_3d` and `bottom_3d` (compare.py takes 3D from the
  oracle's run); PNG files at the frames of LIST, as `--screens` takes it; `memory.txt` with each `--run-dump`
  (`[LABEL=][*]SYMBOL[+OFFSET]:SIZE[@LIST]`: a global of the program by name, which the program finds with `dlsym`, so
  not a static; a size, which the host doesn't know; `*` for the bytes where the pointer at SYMBOL points, the
  oracle's `--dump` of the address that pointer holds there; the last frame of `--frames` without a list); and
  `run.txt`, written at the start and again at the end with the frames completed and the exit status, so that a run a
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
same place, which forgives a rasterizer drawing an edge one pixel over. `--report DIR` writes, for each frame that
isn't exact, the oracle's screens, the port's and the differences side by side (the oracle's picture dimmed, forgiven
differences yellow, the others red). Without a PNG on one side, `frames.tsv`'s hashes decide a screen at 6 bits
without 3D (they are of the 8-bit output, the 6 bits widened); a screen with 3D or at `--depth 5` whose hashes differ
is reported "not compared", which fails the run as a difference does.

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
no file but the ROM and the given ones (melonDS's local files are refused). The firmware is melonDS's generated one with the owner settings of the port's
(`port/hal/nvram/nvram7.c`): nickname "Player", colour 11, birthday 6/23, English, MAC `02:42:57:32:00:01`; melonDS
keeps its own touch calibration. `oracle.py check`, the ctest test `oracle_boot`, runs the first 500 frames twice,
compares the results (`frames.tsv`, the sound trace and the WAV dump byte for byte), checks that the PNG files and
`frames.tsv` agree, that the WAV dump has a sample for each 1024 cycles and the trace the driver's periodic runs 2728
ticks apart, and compares the screens of 11 frames with `port/tests/oracle/boot_<version>.tsv`, hashes the pinned
melonDS gives (`oracle.py check --update` writes them). It exits 77 (skipped) without the oracle or without the ROM the
decomp builds.

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

Two input files on game frames for each version, `port/tests/oracle/<name>_<version>.pbwinput`, compared with
`--align counter`, the movies by their pictures (`oracle.py`'s `SEQUENCES` says how):

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

Each is two ctest tests:

- `oracle_<name>` (`oracle.py sequence NAME --port BIN`), with the oracle built: runs both programs and compares every
  port frame but the known differences ([below](#known-differences-from-the-hardware)): where the game counts passes,
  each with the oracle frame at its count (plus the lag), so that a picture shown a pass late or early, or held a pass
  too long, fails; in the movies, by their pictures in order. Screens without 3D must be exact at 6 bits, screens with
  3D within the tolerance, and every picture the oracle shows between two of the port's frames must be one the port
  shows (a skipped picture fails). It compares NNS G3D's state (`NNS_G3dGlb`) on the count at every frame of the title
  too. It can't time the movies: there the DS's passes take 3 to 5 frames by its CPU's load, so only their pictures and
  their order are held, and in the title sequence the oracle frames the press skips aren't compared. With `--update`
  it writes the reference from a comparison that passes. Skipped (77) without the oracle or the ROM.
- `oracle_<name>_hashes` (`oracle.py sequence-check NAME --port BIN`), every build: runs the port alone and checks it
  against `port/tests/oracle/<name>_<version>.tsv`, the oracle's hashes at every 12th port frame and at each frame
  around a press, the title's start and the cry. Where the game counts passes, each row is the oracle frame at the
  port's count less the declared lag, so the check holds the port to the oracle's count there (plus the lag) and its
  screens without 3D to the oracle's at 6 bits; in the movies a row is the oracle frame that showed the port frame's
  pictures when the reference was written, which holds the port to that timing. It can't judge a screen with 3D, the
  game's state, nor the frames between the ones it keeps. Skipped (77) without the ROM.

Results, every port frame but the known differences:

| Sequence | Port frames | Match | Exact | 3D within the tolerance | Known differences | NNS_G3dGlb equal |
| --- | --- | --- | --- | --- | --- | --- |
| opening, Black 2 | 3,800 | 3,800 | 3,526 | 274 | none | 274 of 274 |
| opening, White 2 | 3,800 | 3,800 | 3,526 | 274 | oracle frame 1283 | 274 of 274 |
| title, Black 2 | 3,600 | 3,579 | 1,474 | 2,105 | 21 port frames, oracle frame 4943 | 2,470 of 2,470, 15 left out |
| title, White 2 | 3,600 | 3,572 | 1,477 | 2,095 | 28 port frames, oracle frames 1283, 4875 | 2,470 of 2,470, 15 left out |

A screen with 3D has 6 to 17 pixels beyond the pixel tolerance at the median, by sequence and version, and 80 at most
of the 245 allowed, nearly all on polygon edges (their coverage and fill). Inside polygons the pixels match: they
didn't until the rasterizer took the values along an edge by line, as melonDS does, where it had taken them along an
X-major edge's run, which put whole spans' colours a step apart (before, a median of 79 and at most 172).

## Sound

The port's ARM7 sound driver ([port/hal/snd7](../../hal/snd7), [ADR 0006](../../docs/adr/0006-sound-driver-as-built.md))
held to the DS's, by what each program writes for it:

- **A sound trace** (pbw-oracle's and the port's `--sound-trace FILE`, the format in `sndtrace.h`): a record after each
  run of the driver's thread, with the driver's work in the DS's layout (`SNDi_Work`, 0x1180 bytes: the 16 channels,
  16 players, 32 tracks and 8 alarms), the shared work, each channel's SOUNDxCNT and the mixer sample that took its
  last start, the locks, the random generator, the periodic runs the record covers, the cycle, the frame and GFL's
  frame count. The oracle reads the ARM7's WRAM: a run has been made when the driver's random generator moved on
  (each run draws from it last), and it is finished when the ARM7 is halted (its thread waits for its next message),
  which the oracle checks after each mixer sample; a run that comes before the ARM7 halts again shares the record of
  the one before it, which the periodic alarm's fire time tells (`periods` 2 or more). The port writes the same
  layout from its own structures (`port_snd7_trace_record`), with the links inside the work as the ARM7's addresses;
  addresses of data the ARM9 loaded differ by construction, so a track's place in its sequence is `cur - base`.
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
  variables and status. Where a press reaches the two programs at different points of a sequence (START in the
  opening, `SoundSpec.before_count`), the runs from that count on aren't compared.
- **Frame by frame**, where the game counts passes (on the frames `compare.py --align counter` pairs; the movies'
  frames pair by their pictures, which the DS shows late against its clock, and the sound isn't timed by them): what
  the ARM9 sees of the driver as each frame ends: the shared work's players and channels, the tick counts of the
  active players, the global variables, the locks, the alarms with their setting, and the SOUNDxCNT of the channels
  the ARM9 drives itself (a stream's, a cry's). A difference that the other program shows one run earlier or later
  (the run that made it fell on the other side of the VBlank) counts as equal a run apart.
- **The sound, by segment of port frames**: each frame's RMS lined up at the lag (oracle frame = port frame + lag)
  where the two correlate best, which must be 0.97 or more; the segment's RMS and peak within 2%, its samples at full
  scale within 10% (or 32); and for a stream or a cry, sound the ARM9 writes and a channel plays at its own timer,
  every sample to the bit at the offset the loudest point gives.

Results (`oracle.py sound`, melonDS 1.1):

| Sequence | Runs paired | Equal | Fields equal | Frames on the count | Equal | A run apart | Left out |
| --- | --- | --- | --- | --- | --- | --- | --- |
| opening, Black 2 | 9,639 | 9,639 | 6,279,801 of 6,279,801 | 579 | 579 | 0 | none |
| opening, White 2 | 9,639 | 9,639 | 6,279,801 of 6,279,801 | 579 | 579 | 0 | none |
| title, Black 2 | 1,957 (26 after the press left out) | 1,957 | 636,043 of 636,043 | 2,792 | 2,791 | 1 | 1112-1114, 3101 |
| title, White 2 | 1,957 (26 after the press left out) | 1,957 | 636,043 of 636,043 | 2,791 | 2,789 | 2 | 1112-1114, 3101, 3216 |

| Segment (port frames) | Lag | Envelope correlation | RMS, oracle and port | Peak | Samples at full scale | To the bit |
| --- | --- | --- | --- | --- | --- | --- |
| opening: the Game Freak logo (355-880), Black 2 | 254 | 0.9892 | 3846.7, 3846.7 | 18601, 18601 | 0, 0 | |
| opening: the opening's music (1017-3495), Black 2 | 263 | 0.9890 | 6980.3, 6983.7 | 32768, 32768 | 2277, 2271 | |
| opening: the title's stream (3530-3790), Black 2 | 315 | 0.9962 | 6350.2, 6354.2 | 29483, 29483 | 0, 0 | 285,564 of 285,564 |
| opening: the opening's music, White 2 | 263 | 0.9890 | 6981.9, 6983.7 | 32768, 32768 | 2301, 2271 | |
| opening: the title's stream, White 2 | 314 | 0.9987 | 6352.2, 6354.2 | 29483, 29483 | 0, 0 | 285,564 of 285,564 |
| title: the title's stream (1116-3099), Black 2 | 1841 | 0.9891 | 8328.7, 8329.5 | 32668, 32668 | 0, 0 | 2,170,736 of 2,170,736 |
| title: Kyurem's cry (3104-3210), Black 2 | 1843 | 0.9993 | 5390.9, 5390.2 | 15240, 15240 | 0, 0 | 117,070 of 117,070 |
| title: the title's stream, White 2 | 1773 | 0.9872 | 8328.3, 8329.5 | 32668, 32668 | 0, 0 | 2,170,736 of 2,170,736 |
| title: Kyurem's cry, White 2 | 1775 | 0.9977 | 5999.9, 5998.9 | 15240, 15240 | 0, 0 | 117,070 of 117,070 |

What the comparison found in the port, each fixed in `port/hal` with a test:

- **The stream dropped a block every 7 frames or so.** The driver ran lazily, at each VBlank and each word of the
  ARM9, so its alarms' words (2.14 a frame at the title stream's 256 samples a block) reached the ARM9 in twos and
  threes at the VBlank; NitroSystem's stream player queues a block to read at each alarm and drops the oldest when a
  third comes before its thread has read one (`ArcStrmCallback`), which zeroed a block and played the rest of the
  stream a block later: the title's music lost 27 frames against the DS's in 420. The next alarm is now the OS core's
  timed work, so each word comes when its alarm goes off (`snd_pxi_alarm_timing`, `os_timed_work`).
- **The sound unit started its sounds at once.** On melonDS a PCM sound's first sample comes on the channel's third
  timer step, an IMA-ADPCM one's on its eleventh, a square wave's or noise's on its first, so a one-shot sound ends that
  much later: one channel of the Game Freak logo's ended a run earlier on the port. And a channel reads its sound ahead
  through a FIFO of 8 words, so a stream's buffer rewritten just ahead of the channel plays as it was: the ARM9,
  which takes no time on the port, rewrote the title stream's blocks two samples before the channel had played them,
  which played those two samples of every block from the buffer's next round (`snd_fifo`).
- **The mixer's widths.** melonDS mixes by GBATEK's widths and counts a volume, pan or master volume of 127 as 128;
  the port's had kept the whole product and 127, 1.6% quieter at full volume (`snd_mixer`, `snd_psg_duty`).
- The oracle's own first reading had the two lock words the wrong way round (the ARM9's alone are at `0x03809f98`,
  `SND_LockChannel`), and so did `snd7_exchannel.c`'s comment.

With those, the sound of a stream and of a cry is melonDS's to the bit, and every state the driver goes through in
the four sequences is the DS's. What stays apart, by construction:

- **Note starts.** The port's runs take no time; the DS's ARM7 reaches its register writes some way into a run. Of
  the note starts the comparison pairs (the mixer sample that took each start, on both), the oracle's come in the
  same mixer sample as the port's or up to 4 later (the opening's music in Black 2: 1, 135, 241, 49 and 7 at 0 to 4
  samples after the earliest; the logo's 12 and 3 at 0 and 1), so the sequences' sound matches in its envelope and
  levels but not sample by sample (the opening's music: 13.7% of its samples equal). The oracle's sampler can't see a
  start that comes within one mixer sample of the channel's stop (some 70 in the opening's music); those aren't
  paired.
- **The DS's long passes.** While it loads the title (counts 1111 and 1113, passes of 2 and 46 frames) the DS locks
  the stream's channels and starts the stream at the end of its pass, the port at its start: port frames 1112-1114
  of the title sequence, whose status is left out.
- **Kyurem's cry** starts a pass later against the DS's passes (port frame 3101) and, in White 2, ends a pass later
  for the title, which unlocks its channel a pass later (port frame 3216), as [above](#known-differences-from-the-hardware).

**The clipping in the opening's music is the game's, on the DS's scale.** Both mixers output the DS's 10 bits times
64, clipped at the same level (a channel at full volume and center pan is half the range), and the opening's music
reaches it in the same seconds on both: in Black 2, 2,271 samples at full scale on the port and 2,277 on the oracle,
in seconds 24 to 57 of the port's run, at most 421 in a second on the port and 440 on the oracle (seconds 51 to 53);
in White 2, 2,271 and 2,301, at most 421 and 453. Before the mixer took melonDS's 127 as 128 the port had 2,007. So
there was no scaling or saturation to fix beyond that.

Each sequence is two ctest tests:

- `oracle_sound_<name>` (`oracle.py sound NAME --port BIN`), with the oracle built: both programs, compared as above.
  With `--update` it writes the reference, `port/tests/oracle/sound_<name>_<version>.tsv`, from a comparison that
  passes: the oracle's values only, as hashes and small statistics: the SHA-1 of the oracle's states of each 64 runs
  of each play, the hash of its status at every 8th frame and each frame where it changes (of those equal at their
  ends, not a run apart, which the moment of a run decides), each segment's lag, RMS, peak and samples at full scale
  and its RMS by blocks of 4 frames, and for a segment to the bit the SHA-1 of the oracle's samples over it and at its
  loudest point. Skipped (77) without the oracle or the ROM.
- `oracle_sound_<name>_hashes` (`oracle.py sound-check NAME --port BIN`), every build: the port alone, with its trace
  and WAV dump, against the reference: every run of the plays, the statuses, the segments' envelopes and levels, and
  the samples to the bit (wherever the loudest point's hash is, within two frames). Skipped (77) without the ROM.

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
  (2012-10-07 12:00:00), never the host's; `oracle.py port --input` passes `--fixed-time` only when it is given
  `--time`, as `oracle.py run` does with the oracle.
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
the screens; `known_oracle`, oracle frames no port frame can show; `known_state`, port frames whose state differs;
`lags`), so that every frame that differs at the game's count is either one of these or a failure. By kind:

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
    comes whole).
- **A frame the DS drops where the game steps by VBlanks.** The title moves its camera once for each VBlank since its
  last pass (`Title_Update` in `src/ov162/title.c`) and Kyurem's animation once a pass. The DS's pass at START
  (count 3100) takes two frames, so from then on its camera is a step ahead of the animation, which on the port,
  dropping none, it never is. NNS G3D's state shows it: `NNS_G3dGlb` (the camera matrix among it, 612 bytes) is the
  same on both at every count compared but 3109 to 3123, where the oracle's at count c is the port's at c + 1 (port
  frames 3110-3124, in both versions); it agrees again once the camera stops. The screens of port frames 3112-3129
  (Black 2) and 3112-3128 (White 2) differ beyond the tolerance meanwhile. To see it:

  ```sh
  oracle.py run --out o --frames 4990 --input port/tests/oracle/title_b2_us.pbwinput --dump NNS_G3dGlb@4925-4985
  oracle.py port --binary build/port/pokeblack2 --out p --frames 3140 --input port/tests/oracle/title_b2_us.pbwinput \
      --dump NNS_G3dGlb:612@3090-3140
  compare.py memory o p --align counter --clock 313-1108 --lag 1109=0
  ```

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
  same count, and from the start menu on (port frame 3241) the port is a pass ahead (`--lag 3241=-1`). To see it (the
  oracle's shared work is where `SNDi_SharedWork` points, 0x0214c740 in White 2's run):

  ```sh
  oracle.py run --version w2_us --out o --frames 5030 --input port/tests/oracle/title_w2_us.pbwinput \
      --dump SNDi_SharedWork:4@4860 --dump snd=0x0214c740:12@4860-5030
  oracle.py port --version w2_us --binary build/port-w2/pokewhite2 --out p --frames 3260 \
      --input port/tests/oracle/title_w2_us.pbwinput --dump 'snd=*SNDi_SharedWork:12@3090-3260'
  ```

  Each dump is the count of finished commands (a word), the players' status (a word) and the channels' (a half-word),
  whose bit 15 is the cry's channel.
