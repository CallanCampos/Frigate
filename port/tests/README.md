# port/tests/

The host build's tests, run with `ctest --test-dir build/port` (add `-R PATTERN` for some of them, `-j4` to run them
in parallel, `--output-on-failure` to see why one failed). Each module's CMake fragment in
[../cmake/](../cmake/README.md) defines its tests, and each test's name starts with its module's. The levels they
belong to (T0 to T7) are in [../CLAUDE.md](../CLAUDE.md#testing-planmd-6) and
[PLAN.md §6](../docs/PLAN.md#6-verification).

Most tests need nothing but the build. The ones that need the player's ROM (`orig/baserom_<version>.nds`, or the
`FRIGATE_ROM` variable) skip without it, so CI, which has no ROM, passes; they are the `rom_game_*` tests, most of the
`rom_*` ones, `nnsys_g2d_rom_equivalence`, `snd_rom_*`, the `replay_*` tests and the `oracle_*` tests that run the
game. `asm_oracle` also needs the extracted ROM and its own Python environment, and the `oracle_*` tests that run
melonDS need the oracle, built with `-DPORT_BUILD_ORACLE=ON` ([../tools/oracle/README.md](../tools/oracle/README.md)).
A skipped test says so in ctest's summary.

| Directory | What it tests | Game functionality covered | Tests |
| --- | --- | --- | --- |
| [app/](app) | The program's own modules: gamepads without a device, the run directory, symbol lookup, the window's icon, `--help` against the names the build read; the macOS app's scripts, bundle identifier and log, with stand-in programs and projects | The controls; the oracle's comparisons; the app | `app_*` |
| [asm_oracle/](asm_oracle/README.md) | The C that replaces the SDK's assembly, against the ROM's own machine code run in Unicorn (a test tool only) on seeded random inputs | Copies, fills, decompression, matrices, sorting, SHA-1, RC4 | `asm_oracle` |
| [assets/](assets) | Frigate's artwork as committed: each SVG is what its generator writes, the PNG masters' sizes, and the small images' ships pixel for pixel | None: the program's look | `assets_branding` |
| [fx/](fx) | The decomp's libfx C as the host compiles it | 3D maths | `fx_libfx` |
| [g3d/](g3d) | The geometry engine and the CPU 3D renderer, against values worked out by hand from GBATEK; the program glue against fakes; timing | Every 3D scene | `g3d_*` |
| [gfx/](gfx) | The CPU 2D renderer and display capture, against pixels worked out by hand; timing | Every 2D screen, capture | `gfx_*` |
| [gx/](gx) | VRAM banks in every backend, every VRAM setting of the SDK through the real `gx_vramcnt.c`, the loads, the FIFO | Where every picture's data lives | `gx_*` |
| [hal/](hal) | The C versions of the SDK's assembly on known vectors | as `asm_oracle` | `hal_mi`, `hal_fx` |
| [integrate/](integrate) | The platform layer's pieces on the whole program's objects: DMA, the allocator, the main arena, input, the screens and the screenshot | Uploads, heaps, buttons, the screens | `integrate_*` |
| [nnsys/](nnsys) | The G2D loaders on made files and on every G2D file of the ROM, the relocation registry, the DS layouts | Sprites, cells, animations, fonts | `nnsys_*` |
| [oracle/](oracle) | The oracle's input files, per-screen 3D rule and comparison scripts (`compare.py`; `sound.py` on made-up sound traces and WAV dumps); the input sequences, the oracle's hashes of their screens, and its hashes and statistics of their sound (`sound_<name>_<version>.tsv`: the sound driver's state run by run, levels and envelopes; hashes only, never pictures or samples) | Boot, opening, title, start menu, CONTINUE, a save from Frigate to melonDS; the opening's and the title's music, stream and cry | `oracle_*`; the sound's `oracle_sound_tool`, `oracle_sound_<name>`, `oracle_sound_<name>_hashes` |
| [os/](os) | The OS core: each scenario runs as the game's `main()` and checks the order of thread switches, interrupts, timers and other modules' timed work against the DS's | Threads, interrupts, timing; the sound driver's alarms (`os_timed_work`) | `os_*` |
| [pxi/](pxi) | The PXI bus and the host ARM7's services, driven by the decomp's own ARM9 libraries | Power, touch, firmware settings, the clock, reset, the cartridge slot, the microphone, sound commands | `pxi_*` |
| [replay/](replay) | Replays of recorded input on the whole program: to the start menu, through New Game's intro (checking nine screens by the oracle's hashes), and twice to the same save | Boot to the start menu, New Game's intro, saving | `replay_*` |
| [rom/](rom) | The ROM's check on variants of the player's dump, the backup service word by word, and the file system, card and overlays as the game links them | Every file read, overlays, saves | `rom_*`, `card_backup_*`, `rom_game_*` |
| [snd/](snd) | The ARM7's sound driver and sound unit against values worked out by hand from the driver's arithmetic and GBATEK (the timer through a pitch change, the FIFO, the mixer's widths, the alarms at their cycle), the trace's record layout, every sequence of the ROM, and the audio output | Music, sound effects, cries | `snd_*` |

`smoke_reaches_game_main` ([../CMakeLists.txt](../CMakeLists.txt)) runs the program's first frame, and the
`overlay_sections_*` tests ([../cmake/overlay_sections.cmake](../cmake/overlay_sections.cmake)) check the overlay
sections of every program that links the game.

## Writing a test

- Expected values are worked out by hand from the hardware's rules (GBATEK) or recorded from the ROM's own code or
  the oracle, never from running the code under test; the test's comments say which.
- A test that runs the program gives it a `--save-dir` in the build tree, and a `--watchdog`, never the player's own
  save folder.
- Nothing from the ROM is committed: screens are kept as hashes, saves as SHA-1s ([../CLAUDE.md](../CLAUDE.md)).
