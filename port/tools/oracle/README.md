# The melonDS oracle

The reference the port is held against ([PLAN.md §6](../../docs/PLAN.md#6-verification), test level T4): the player's
ROM running on [melonDS](https://melonds.kuribo64.net)'s emulation core, headless and deterministic, with an input
file, writing both screens and named memory at chosen frames. `compare.py` compares those with the port's.

It is a **test tool only**. It is never linked into or shipped with the port
([PLAN.md §1](../../docs/PLAN.md#1-goal-and-what-native-means)): it is a CMake project of its own, built only on request, and nothing under `port/` other than this directory, its
tests and its CMake fragment refers to it. melonDS is GPL-3.0, as this repository is; the oracle links melonDS's core
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
```

`oracle.py run` calls `pbw-oracle` with the version's ROM (`orig/baserom_b2_us.nds`), its symbols
(`config/b2_us/arm9`), the statics of the ROM build's link map and GFL's frame count as the counter. It passes `--time`
only when it is given one, so that an input file's `time` line applies, else the default clock. `oracle.py port` runs
the port once for each frame of its list, since the port writes one screenshot a run (at the end of `--frames N`),
each from power-on with a blank save and a fixed clock (`--time`, else the default: the port takes no input file yet),
and writes a run directory in the oracle's format; a run that ends early (a trap stub, a crash) is reported with the
port's message and its frame is left out.
`oracle.py boot` does both for the boot sequence's frames, the oracle 400 frames further with a PNG of every frame, and
compares them by sequence at 6 bits and at 5. `pbw-oracle --help` and the scripts' `-h` list every option.

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
`GFL_VBlankUpdate` increments once per pass of the main loop, after its VBlank wait. The oracle records it for every
frame (`frames.tsv`'s `counter`). Two frames pair when each is the first of its run where the count reaches the same
value (`compare.py --align counter`). The port needs to write the same column (see [Integration](#integration)); until
it does, three fallbacks:

- `--align sequence`, the one to use: each port frame, in order, pairs with the first oracle frame from the previous
  pair's on that shows the same pictures, compared at the palettes' depth (5 bits a channel) so that a difference in
  the output's low bit doesn't break the pairing. It needs the oracle's PNG files for the frames in reach
  (`--screens all`); a port frame that no oracle frame shows pairs as far after the previous pair as it is, and is
  noted "extrapolated". It checks that the port shows the same pictures in the same order, whatever the timing.
- `--align first-change`: the offset between the first frames whose screens aren't blank (all but a line's worth of
  pixels one colour). It holds until the first frame the DS drops, and it needs every frame up to those in both runs:
  a run of chosen frames (`oracle.py port`'s) is refused, since its first frame that isn't blank needn't be the first
  the program showed.
- `--align image`: the offset at which most of the port's frames show exactly the screens of an oracle frame, from
  `frames.tsv`'s hashes, which the oracle writes for every frame (`--screens` only decides which get a PNG).

`compare.py align ORACLE PORT` reports the first two offsets and, for each port frame, the oracle frames that show
exactly its screens.

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

As the oracle shows it, by frame (`oracle.py run --frames 2000`); the frame count (`counter`) starts at frame 242.

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
| 1283–about 1650 | the title screen's opening in letterbox: Kyurem's silhouette, its eyes lighting up | black |
| from about 1680 | the title: "Pokémon Black Version 2" | black |

The frame count is 354 at frame 600, 470 at 1000 and 633 at 1283: the DS drops frames through the movie. Nothing in the
first 2,000 frames uses the 3D layer. The title's 3D Kyurem comes later, on the bottom screen: from frame 3841 (count
1348) engine A shows BG0 as 3D and captures it into bank D every frame, and from 3842 the bottom screen shows that bank
in VRAM display mode (`bottom_3d` is 1; the top screen, engine B's logo, stays 2D). The count is 1507 at frame 4000.

## Integration

What the port needs to compare itself frame by frame:

- **Its own run directory**: a `frames.tsv` row at each frame hook with the logic counter (the value
  `GFL_VBlankUpdate` keeps in `sVBlankCount`) and the game frame, and PNGs at the frames of a list, so that one run
  gives every frame instead of `oracle.py port`'s run per frame.
- **Replays**: `--input FILE` read with `pbwinput.c` and one `PbwGameClock` for the run (`pbw_game_clock_init`).
  Before the game starts, apply `pbw_input_state(&input, 1)`. At the frame hook of each frame, read GFL's frame count
  (`sVBlankCount`, a static of `src/system/gfl_use.c`: the port needs a host-side way to read it, its address in the
  host build or a count of `GFL_VBlankUpdate`'s calls kept outside the decomp) and apply
  `pbw_input_state(&input, pbw_input_frame_at_vblank(&input, &clock, frame, count))`, with `frame` the hook's count
  since `main()`; a frame the console sleeps through takes `pbw_input_frame_asleep` instead. The state goes in through
  `port_input_set` and the touch panel's host setter. The real-time clock at power-on is `--fixed-time`, else the file's
  `time` line, else the format's default (2012-10-07 12:00:00), never the host's; `oracle.py port --input` then passes
  `--fixed-time` only when it is given `--time`, as `oracle.py run` does with the oracle.
- **Memory dumps** by name, written as `memory.txt`, for globals whose bytes don't depend on the host.

## Known differences from the hardware

melonDS is a reference, not the hardware. Where the two programs can't agree by construction:

- The ARM7's polling of X, Y, the lid and the touch screen runs on its own timing in the oracle; the port sets them at
  once.
- melonDS's touch calibration is its own (pixel × 16); the port's is the one its firmware settings give.
- Timing: the oracle's CPUs take time and the DS drops frames, which is why frames align on the game's count.
