# port/tests/

The host build's tests, run with `ctest --test-dir build/port` (add `-R PATTERN` for some of them, `-j4` to run them
in parallel, `--output-on-failure` to see why one failed). Each module's CMake fragment in
[../cmake/](../cmake/README.md) defines its tests, and each test's name starts with its module's. The levels they
belong to (T0 to T7) are in [../CLAUDE.md](../CLAUDE.md#testing-planmd-6) and
[PLAN.md §6](../docs/PLAN.md#6-verification).

Most tests need nothing but the build. The ones that need the player's ROM (`orig/baserom_<version>.nds`, or the
`FRIGATE_ROM` variable) skip without it, so CI, which has no ROM, passes; they are the `rom_game_*` tests, most of the
`rom_*` ones, `nnsys_g2d_rom_equivalence`, `snd_rom_*`, the `replay_*` tests, `integrate_launch_never_asked`, the
`frame_times_*` routes and the `oracle_*` and `launch_*` tests that run the game. `asm_oracle` also needs the extracted
ROM, its own Python environment and a host build directory configured with `-G Ninja`, whose `build.ninja` it reads the
game's flags from (it skips in a build made with another generator, [asm_oracle/README.md](asm_oracle/README.md#setup),
and on macOS in one with no slice for its Python's architecture, such as an x86_64 build on Apple Silicon), and the
`oracle_*` tests that run melonDS need the oracle, built with `-DPORT_BUILD_ORACLE=ON`
([../tools/oracle/README.md](../tools/oracle/README.md)), as do the test launcher's `launch_*` tests, which start from
saves the oracle makes (`oracle_saves`). The oracle counts game frames with the ROM build's link map
(`build/<version>/arm9.o.xMAP`), so those tests need `python3 configure.py && ninja` to have run: without it they fail
at once ("counts game frames, which need --counter") rather than skip. A skipped test says so in ctest's summary.

Four labels pick tests by what they need: `route`, the tests whose program runs the game from power-on on the player's
ROM (the replays, the oracle's sequences and the port's checks of their hashes, the test launcher's routes,
`integrate_launch_never_asked`, the `frame_times_*` routes, and the smoke test, which passes without a ROM too, stopping
where the program looks for one); `oracle`, the tests that need the melonDS oracle or the saves it makes; and `timing`,
the tests that hold the host's time to a limit, which a slow or loaded machine can fail: the `frame_times_*` routes,
whose scenes' frames must stay within a budget in an optimized build
([../cmake/frame_times.cmake](../cmake/frame_times.cmake)), and the renderers' timing gates, which exist only with
`PORT_G3D_TIMING_GATE` or `PORT_GFX_TIMING_GATE`; and `static`, the tests that read the game's C rather than run it:
the pointer cast guard, `game_ptrcasts`, which compiles the game's C again with the cast warnings on and holds what it
finds to a baseline ([../docs/SYNC.md](../docs/SYNC.md#when-the-pointer-cast-guard-fails)), and its own test,
`game_ptrcasts_tool` ([game/](game/test_check_ptrcasts.py)). So `ctest -L route -LE oracle` runs the routes that need
no oracle, `ctest -LE route` the rest, `ctest -LE timing` leaves out the timing, and `ctest -L static` runs the guard
alone; ctest adds the fixtures they need (`oracle_saves`, the replays' `*_clean`). A test gets its
labels where it is defined, with `port_test_labels(TEST LABEL...)` ([../CMakeLists.txt](../CMakeLists.txt)): a new test
that runs the game from power-on gets `route` in the fragment that adds it, and `oracle` too when it needs the oracle or
its saves.
In the sanitized build (`-DPORT_SANITIZE=ON`, [../docs/sanitizers.md](../docs/sanitizers.md)) every test runs under
AddressSanitizer and UndefinedBehaviorSanitizer, and a report fails it on every host: the program aborts, where the
runtime's own default off macOS is status 1, which a test that passes on its output or expects a failure would
accept, and a sanitizer's report in a test's output fails the test too, which holds because no test inverts its result
with `WILL_FAIL` (configuring stops on one, in every build). A test of how a program ends, a failure's status, a
crash or an abort, runs it through [app/check_exit.py](app/check_exit.py), which also fails a run with a sanitizer's
report whatever ending it wants. The routes are that build's T5 runs.

| Directory | What it tests | Game functionality covered | Tests |
| --- | --- | --- | --- |
| [app/](app) | The program's own modules: gamepads without a device, the run directory, symbol lookup, the window's icon, `--help` against the names the build read and the options it describes, the test launcher's option and the kinds of field menu (`test_launcher_spec.c`), the reader of `--frame-times`' files ([../tools/frame_times.py](../tools/frame_times.py), `test_frame_times.py`); the macOS app's scripts, bundle identifier and log, with stand-in programs and projects, and a universal program's two slices running the opening the same (`test_universal_slices.sh`); and `check_exit.py`, which runs the program and checks its exit status, or that it crashed or aborted, since ctest's `PASS_REGULAR_EXPRESSION` ignores the status and fails a crash, and fails a run with a sanitizer's report whatever ending it wants (`test_check_exit.py` tests it) | The controls; the oracle's comparisons; the field menu's items; the app | `app_*` (`app_check_exit`), `frame_times_tool`, and the `integrate_unknown_option`, `integrate_launch_*`, `host_*`, `nnsys_ds_read_outside` and launcher tests that run through `check_exit.py` |
| [asm_oracle/](asm_oracle/README.md) | The C that replaces the SDK's assembly and MSL's runtime division, against the ROM's own machine code run in Unicorn (a test tool only) on seeded random inputs | Copies, fills, decompression, matrices, sorting, SHA-1, RC4, the field's lens flare data's division | `asm_oracle` |
| [assets/](assets) | Frigate's artwork as committed: each SVG is what its generator writes, the PNG masters' sizes, and the small images' ships pixel for pixel | None: the program's look | `assets_branding` |
| [fx/](fx) | The decomp's libfx C as the host compiles it | 3D maths | `fx_libfx` |
| [g3d/](g3d) | The geometry engine and the CPU 3D renderer, against values worked out by hand from GBATEK, in one band of lines and in three; the renderer in 2 to 8 bands against one, on random scenes (`g3d_scene.h`, the timing's); the program glue against fakes, the last picture kept among them; timing | Every 3D scene | `g3d_*` |
| [gfx/](gfx) | The CPU 2D renderer and display capture, against pixels worked out by hand; timing | Every 2D screen, capture | `gfx_*` |
| [game/](game) | The pointer cast guard ([../tools/check_ptrcasts.py](../tools/check_ptrcasts.py)) on a small tree the test makes with the build's compiler: new findings fail with their sites and the ways out, `--update` and the classes, counts above the baseline, renames and what isn't one, the narrowing and callback kinds, the headers reached through `-isystem` and their host variants, a wrapper's decomp file, flags that hide the warnings, a file that doesn't compile, `--require` (`test_check_ptrcasts.py`). The guard itself runs as `game_ptrcasts` on the game's C | Every part of the game: the casts that would cut an address on a host | `game_ptrcasts_tool`, `game_ptrcasts` |
| [gx/](gx) | VRAM banks in every backend, every VRAM setting of the SDK through the real `gx_vramcnt.c`, the loads, the FIFO, a read of a geometry command port and SPL's billboard through the command log and the geometry engine (`parameter_read`), the field's building transition's clear of LCDC VRAM (`lcdc_clear`) | Where every picture's data lives; every particle | `gx_*` |
| [hal/](hal) | The C versions of the SDK's assembly on known vectors; MSL's runtime division against values worked out by hand from the ARM routine (`test_msl.c`); the heaps' host sizes and the host memory of the root heaps the game creates in its own, kept, zeroed when its overlay loads again and, under ASan, poisoned between two heaps (`test_heap_policy.c`); the card slot's SPI bus and its infrared chip against melonDS's and GBATEK's behaviour (`test_card_spi.c`) | as `asm_oracle`; the field's lens flare data at every map change; the field's, the trade screen's and GFL's extra heap; the genuine-card check and the infrared library | `hal_mi`, `hal_fx`, `hal_msl`, `hal_heap_policy`, `hal_card_spi_*`, `hal_wstring` |
| [host/](host) | The host's system (`port/hal/host`) the same on every host: a trap placeholder touched stops the program with its name and status 70, a crash is reported with a backtrace, both on a fiber too with the C library's code making the access, and so is a fiber's guard touched; a division by zero, an illegal instruction and a breakpoint, on the host's stack and on a fiber, and `__builtin_trap` on the host's stack, are reported the same way (on macOS with the function that crashed named), and each ends the program by the signal or exception its report names; the watchdog goes off and can be kicked, the static data holds the program's statics, and directories, a folder and an argument whose names aren't ASCII, the environment and private file mappings | Traps where the game reaches data with no C yet; crash reports; the watchdog; the PXI bus's pointers | `host_*` |
| [integrate/](integrate) | The platform layer's pieces on the whole program's objects: DMA, the allocator, the main arena, input, the screens and the screenshot, Frigate's test launcher when it is off, and the overlays' sections, with a probe file compiled as an overlay's (`ovsec_probe.c`); the program's option checks, the launcher's among them | Uploads, heaps, buttons, the screens; CONTINUE without the launcher; every overlay's statics set back at its load | `integrate_*` |
| [layout/](layout) | Struct layouts that the game's files must agree on in the host build, from the objects' debug information (`check_struct_layout.py`): a file's own copy of a struct whose header lays it out differently on a host | The trainer card's param, which the field's app call and the card share | `layout_*` |
| [nnsys/](nnsys) | The G2D loaders on made files and on every G2D file of the ROM, the relocation registry, the DS layouts; a read the DS makes past a heap block, let through in the game's heaps and the main arena and stopped outside them | Sprites, cells, animations, fonts; the copyright screen's read past its file | `nnsys_*` (`nnsys_ds_reads`, `nnsys_ds_read_outside`) |
| [oracle/](oracle) | The oracle's input files, per-screen 3D rule and comparison scripts (`compare.py`; `sound.py` on made-up sound traces and WAV dumps; `oracle.py`'s saves and launch checks with stand-in programs); the input sequences, the oracle's hashes of their screens, and its hashes and statistics of their sound (`sound_<name>_<version>.tsv`: the sound driver's state run by run, levels and envelopes; hashes only, never pictures or samples); the routes that make saves on the oracle (`first_save` from a blank save and `starter_save` from it, in both versions, and `route19_save` from that, Black 2's only, which no test uses yet); the launch routes of the field menu's apps (`launch_<route>_<version>.pbwinput` and the oracle's hashes of their screens, `.screens.tsv`) and `check_launch_smoke.py`, which runs Frigate's test launcher alone from a save the oracle made | Boot, opening, title, start menu, CONTINUE, a save from Frigate to melonDS, saves with a Pokémon made on melonDS; the field menu's apps (the bag, the trainer card, the party list, the summary, the Pokédex) through the test launcher, in both versions; the opening's and the title's music, stream and cry | `oracle_*`, the launcher's `launch_*`; the sound's `oracle_sound_tool`, `oracle_sound_<name>`, `oracle_sound_<name>_hashes` |
| [os/](os) | The OS core: each scenario runs as the game's `main()` and checks scheduling, interrupts, timers and SDK workspace contracts | Threads, interrupts, timing; sound alarms (`os_timed_work`); screen-wipe HBlank flags (`os_hblank_flag_in_handler`); native WCM workspace and AP-list bounds (`os_wcm_workspace`) | `os_*` |
| [pxi/](pxi) | The PXI bus and the host ARM7's services, driven by the decomp's own ARM9 libraries | Power, touch, firmware settings, the clock, reset, the cartridge slot, the microphone, sound commands | `pxi_*` |
| [replay/](replay) | Replays of recorded input on the whole program: to the start menu, through New Game's intro (checking nine screens by the oracle's hashes), on into the field and the house to the field menu's SAVE (checking the intro's screens and the field menu's bottom screen by the oracle's hashes, in both versions), and twice to the same save; the screens' checker on its own (`test_check_screens.py`) | Boot to the start menu, New Game's intro, the field, the field menu, saving | `replay_*` |
| [rom/](rom) | The ROM's check on variants of the player's dump, the backup service word by word, and the file system, card and overlays as the game links them, with the game's genuine-card check against the card's infrared chip | Every file read, overlays, saves, the genuine-card check | `rom_*`, `card_backup_*`, `rom_game_*` |
| [snd/](snd) | The ARM7's sound driver and sound unit against values worked out by hand from the driver's arithmetic and GBATEK (the timer through a pitch change, the FIFO, the mixer's widths, the alarms at their cycle), the trace's record layout, every sequence of the ROM, and the audio output | Music, sound effects, cries | `snd_*` |

`smoke_reaches_game_main` ([../CMakeLists.txt](../CMakeLists.txt)) runs the program's first frame, and the
`overlay_sections_*` tests ([../cmake/overlay_sections.cmake](../cmake/overlay_sections.cmake)) check the overlay
sections of every program that links the game. The `frame_times_*` routes
([../cmake/frame_times.cmake](../cmake/frame_times.cmake)) play the title screen, New Game (Black 2) and the field
menu's apps 3 times each with `--frame-times`, alone, and fail when a scene's 95th percentile, each frame's least time
over the runs without the pacing wait, is above `PORT_FRAME_TIME_BUDGET_MS` (half the DS's frame, 8.36 ms, by default).
They skip such a scene instead when the machine's load average, before a run or after the last, was above its cores:
other programs had them, so the times measure the machine. Only an optimized build has them (`CMAKE_BUILD_TYPE`
`Release`, `RelWithDebInfo` or `MinSizeRel`): a Debug build, or one with no build type, runs the frames unoptimized.
A failure says what a machine too slow for the budget does: `ctest -LE timing` leaves them out, and
`-DPORT_FRAME_TIME_REPORT_ONLY=ON` makes them report only.

## Writing a test

- Expected values are worked out by hand from the hardware's rules (GBATEK) or recorded from the ROM's own code or
  the oracle, never from running the code under test; the test's comments say which.
- A test that runs the program gives it a `--save-dir` in the build tree, and a `--watchdog`, never the player's own
  save folder.
- Nothing from the ROM is committed: screens are kept as hashes, saves as SHA-1s ([../CLAUDE.md](../CLAUDE.md)).
