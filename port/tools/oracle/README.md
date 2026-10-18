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
compare.py screens build/oracle-runs/boot build/oracle-runs/port --align counter --clock 313-1000 \
    --report build/oracle-runs/report
oracle.py boot --out build/oracle-runs/both --port build/port/pokeblack2
oracle.py sequence title --port build/port/pokeblack2 --out build/oracle-runs/title
oracle.py sequence-check title --port build/port/pokeblack2
oracle.py save first_save --out build/oracle-runs/first_save.sav
oracle.py sequence continue --port build/port/pokeblack2 --out build/oracle-runs/continue
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
`oracle.py port-save` boots the port's own save on it ([below](#saves)). `pbw-oracle --help` and the scripts' `-h` list
every option.

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

## The comparison sequences

Three input files on game frames for each version, `port/tests/oracle/<name>_<version>.pbwinput`, played by four
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
  runs to 3,600 frames, the oracle 6,000; the port may stop from frame 3425, once the start menu has wiped out to
  black, on the trap stub the sequence names (`stop_trap`, a pattern of the trap's message): overlay 36's
  `FIELDMAP_PROC_FUNCS`, in the field's first frames (below). A stop on any other trap fails, so the sequence says
  which gap it stops on and changes when the decomp fills it.
- **continue_starter**: continue's input file from the starter save ([below](#saves)): the start menu's saved game has
  the Pokédex line (Pokédex: 1, time 0:04), and CONTINUE goes to the Aspertia City lookout. The port stops on the same
  trap. White 2 has no starter save measured yet, so its test skips with the reason.

Each is two ctest tests, but continue and continue_starter, whose saves only the oracle can make, have only the
first:

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
| continue, Black 2 | 3,436 (trap) | 3,415 | 1,310 | 2,105 | the title's 21 port frames, oracle frame 4920 | not compared |
| continue, White 2 | 3,436 (trap) | 3,407 | 1,312 | 2,095 | the title's 28 port frames and 3402, oracle frames 1260, 4852 | not compared |
| continue_starter, Black 2 | 3,436 (trap) | 3,415 | 1,310 | 2,105 | the title's 21 port frames, oracle frame 4920 | not compared |

A screen with 3D has 6 to 17 pixels beyond the pixel tolerance at the median, by sequence and version, and 80 at most
of the 245 allowed, nearly all on polygon edges (their coverage and fill). Inside polygons the pixels match: they
didn't until the rasterizer took the values along an edge by line, as melonDS does, where it had taken them along an
X-major edge's run, which put whole spans' colours a step apart (before, a median of 79 and at most 172).

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
(`EventFieldContinue_Create` in `event_mapchange.c`, `hasClockNotBeenTampered`), which the port reaches before its trap
(a breakpoint there stops it on the first save's CONTINUE): another offset takes the path of a changed clock, a day's
wait for the daily events (`setNewDayForCountdown`), `func_ov012_02164428` on the party and the trainer card's time.
Then `setAdvTimeBlkRtcOffsetOwnerMacBdayMonthDay` writes the console's offset, MAC and birthday over the save's, for the
next save. The port's firmware and the oracle's are the same console (`port/hal/nvram/nvram7.c`, `oracle.cpp`'s
`MakeFirmware`: MAC `02:42:57:32:00:01`, owner Player, birthday 6/23, offset 0), so the saves here move without it, and
melonDS's own generated firmware has offset 0 too. A save from a DS whose owner once set its clock has another offset:
the port takes it as a changed clock, and the next save carries the port's console, so a dump moved from a DS to the
port and back takes the changed clock's path on both. PLAN.md asks the import to take on the save's offset and MAC
instead; with the console's Wi-Fi ID, which the port makes again at every launch since its firmware isn't kept between
runs, that is the console the D7 sidecar would keep. None of it is built yet, and this path can't be shown in the port
before overlay 36 lets a continued game into the field.

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
with the pinned melonDS; White 2's are still to be measured, and its tests that need them skip with the reason:

- **The starter save** (`starter_save`, from `first_save`, `port/tests/oracle/starter_save_b2_us.pbwinput`, 17,000
  frames, SHA-1 `ce71fe0b8c1d`): CONTINUE as the continue sequence presses it, then the story from the player's room:
  Mom's scene, out of the house, Hugh's scene, a detour west of the Trainers' School (a sign blocks x = 37), up x = 54
  and west along z = 725 to the stairs, up to Bianca at the lookout (36, 710) and through both her talks. A at Bianca
  takes the middle ball, Tepig; B, mashed, answers NO to the nickname and takes the Pokédex; then the field menu's
  SAVE. Decoded by hand in Gen 5's format, with a valid checksum: Tepig, level 5, 135 experience, 21/21 HP; event flags
  0x961 (a Pokémon) and 0x962 (the Pokédex), the city's story variable 0x40a1 at 4, the starter variable 0x4030 at 1.
  The game writes it by frame 16,400 and the menu closes at 16,874; the two copies of the main save are the same, and
  2,086 bytes differ from `first_save`. It costs about 2.5 minutes at a load average near 10.
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

**Continuing it** (the continue sequence): the start menu shows CONTINUE first, then NEW GAME, MYSTERY GIFT, UNOVA
LINK, NINTENDO WFC SETTINGS and MIC TEST, and the saved game: the player's picture, AAAAAAA, Aspertia City, Badges: 0,
Time: 0:01 (no Pokédex line: the player has none yet). Black 2's port frames 3,244 to 3,436 (the start menu from frame
3,250, CONTINUE's press at 3,401, its blink, the wipe out from 3,419 and the black from 3,425) are exact at the DS's 6
bits on the game's count, and White 2's are with the lag after the cry and one frame at the press (above). The DS
then loads the field and shows the player in his room; the port stops after frame 3,436, in the black, on
`FIELDMAP_PROC_FUNCS` (overlay 36's data at `0x021c9eb8` in Black 2, `0x021c9eec` in White 2), the field's process
table, which has no C yet: overlay 36 (issue #83) is what stops a continued game, before it reaches overlay 12's script
tables. The sequence holds the stop to that trap by its message.

## The launch checks

The field menu's apps against melonDS while the port has no field ([ADR 0012](../../docs/adr/0012-test-launcher-for-the-field-menus-apps.md)):
the oracle opens each app from the field menu, where the port's continued game stops on overlay 36's trap; the port
opens the same apps with Frigate's test launcher (`--launch`, [port/app/launcher.c](../../app/launcher.c)) at the
counts the oracle opens them. `oracle.py launch NAME --port BIN [--save-cache DIR]`, ctest `oracle_launch_<NAME>`,
checks a route (`oracle.py`'s `LAUNCHES`):

1. its save, from the cache or made on the oracle ([Saves](#saves));
2. the oracle on `port/tests/oracle/launch_<name>_<version>.pbwinput`: every row of
   `launch_<name>_<version>.screens.tsv`, the oracle's labelled screens (hashes only), is at its frame with its count
   and its hashes;
3. the port with the same save, the same input file and the route's `--launch`: every app row (`where` is `app`;
   `field` rows are the field's and its menu's, the oracle's only) shows on the port at the row's count, screens
   without 3D by their hashes at the DS's 6 bits, a screen with 3D within compare.py's tolerance from pictures of
   those frames only; and at every count of each app's span, from its first frame to the black after it closes, every
   picture the port shows is one the oracle shows at that count;
4. the launcher's report of each app gives the event type declared (0: the oracle's menu shows again), and no notice
   says the launcher stepped aside;
5. the port leaves the save as it was.

Both programs play the same presses; inside an app they reach both on the same pass, since the app opens at the same
count. The presses for the field and its menu reach nothing on the port. **The counts** in `--launch` are measured, not
taken from the port: each app's first frame on the oracle (from its `frames.tsv`, the first picture after the field's
fade to black) less the passes the port takes from opening the app to its first frame (`--launch APP` at once, 2 for
the bag and the party list, 4 for the trainer card). The first frame is then aligned by construction, and every later
pass is a check; the slack is 0.

**What the field leaves** before an app, measured on the oracle at the bag's first frame by two-pass dumps (the oracle
is deterministic: `g_GameSystem` at `0x021410f4`, its `GameData` at `0x0223b330`, `gfl/ui.c`'s `sSystemUI` at
`0x021ff920` in Black 2): the UI's touch flag (`SystemUI.usingTouch`, +0x3c) is 0, set by the menu's choice by a key
after the menu set it to 1; the last subscreen (`GameData`, +0x1ca) is 1, the menu's, which the field's close records.
The launcher sets both through the game's setters. Nothing else was needed: the apps' pictures are the oracle's.

| Route | Save | `--launch` (Black 2) | App screens | Pass by pass |
| --- | --- | --- | --- | --- |
| `field_menus` | `first_save` | `bag@4044,card@4584` | 2 of 2 | 621 of 621 counts: the bag (4046-4323) and the card (4588-4930) |
| `party_menus` | `starter_save` | `party@4044,card@6974,bag@7673` | 5 of 5, 4 left out | 1,491 of 1,491 counts: the party list, Tepig's menu and back (4046-5123 but the summary), the card with its Pokédex line (6978-7330), the bag (7675-8023) |
| `field_menus`, White 2 | `first_save` | `bag@4044,card@4584`, as in Black 2 | 2 of 2 | 621 of 621 counts, the same spans |

Left out of `party_menus`, with their reasons in `LAUNCHES`:

- **The summary** (counts 4533-4821): it scrolls its background a pixel every 4 VBlanks since it started
  (`PStatus_Main`, `src/ov207/p_sta_sys.c:263-271`), and the DS spends some 37 VBlanks more than passes loading it
  (count 4530 alone takes 30 frames), so its background is about 9 pixels further on throughout; the rest of its
  screens are the port's. A frame the DS drops where the game steps by VBlanks, as the title's camera
  ([Known differences](#known-differences-from-the-hardware)).
- **The Pokédex** (its 3 rows): the port stops on `NNSi_objSizeWTbl` (ARM9 main, `0x020942b0`), NitroSystem G2D's
  table of object widths, which has no C yet, so the route doesn't launch it.

White 2 has `field_menus`, measured again on its own oracle: the same counts and spans, the bag's screens the same as
Black 2's, the menu's and the card's bottom screens White 2's own. It has no `party_menus` yet: its starter save isn't
measured (the Black 2 route's walk goes astray in White 2: the player ends in front of the Trainers' School with no
Pokémon), so that test and `oracle_continue_starter` skip with the reason.

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
  oracle.py port --version w2_us --binary build/port-w2/pokewhite2 --out p --frames 3260 \
      --input port/tests/oracle/title_w2_us.pbwinput --dump 'snd=*SNDi_SharedWork:12@3090-3260'
  ```

  Each dump is the count of finished commands (a word), the players' status (a word) and the channels' (a half-word),
  whose bit 15 is the cry's channel.
