# port/tools/

The scripts and programs that build, check, sync and measure Frigate. None of them is part of the program the player
runs; the generators run inside the host build, and everything else is run by hand or by a test. None writes into the
repository unless its header says so: give each an output directory outside it, or under an ignored `build/`.

## Generators the host build runs

The host build ([../CMakeLists.txt](../CMakeLists.txt) and [../cmake/](../cmake/README.md)) runs these at configure or
build time; their output goes into the build directory, never into the repository.

| File | What it makes | Game functionality it serves |
| --- | --- | --- |
| [gen_host_tree.py](gen_host_tree.py) | The source list (every C file of `src/` and `lib/` minus the `replaced.txt` files of [../hal](../hal/README.md)), the `include/` mirror without MSL's C library headers (links, or copies where Windows refuses links), the host variants of the SDK headers that name DS addresses (`hwinclude/`, [ADR 0004](../docs/adr/0004-ds-addresses-by-header-translation.md)), and the generated constants headers | All of the game's C on a host |
| [gen_stubs.py](gen_stubs.py) | Trap stubs and data placeholders for every DS symbol the game references that has no C yet and the host's C library doesn't provide (found through `ctypes` on macOS and Linux, and in the libraries the compiler links, with `nm`, on Linux and Windows) | Shows which missing function the game reaches next |
| [gen_overlay_sections.py](gen_overlay_sections.py) | Per-overlay sections for overlay code's writable statics, from `delinks.txt`; the check of a Mach-O link's section count, slice by slice for a universal program, with the link format of each slice's architecture and macOS | Overlays reloading as on the DS |
| [gen_window_icon.c](gen_window_icon.c) | The window's icon on Linux and Windows: the version's 32 px window icon, drawn pixel by pixel, and its icon master scaled to 64 px ([../assets/branding](../assets/branding/README.md)), as bitmaps in C, built and run for the machine that builds | None: the program's look |
| [research/assets/nitrofs_manifest.json](research/assets/) | Not a generator but build input: the size and SHA-1 of every NitroFS file of both ROMs, which [../cmake/rom.cmake](../cmake/rom.cmake) turns into the ROM manifest that checks a trimmed dump. `research/assets/manifest.py` wrote it | The ROM's check |

## Checking, syncing, packaging

| File | What it does | Run |
| --- | --- | --- |
| [sync_upstream.sh](sync_upstream.sh) | Merges the decomp's `main` into the port's, builds both ROMs and checks both SHA1s, builds and tests the host side, and pushes only when all of that passes. Exit 3 is a merge conflict, 4 an upstream history rewrite, 5 commits that name issues ([../docs/SYNC.md](../docs/SYNC.md)) | first in every port session |
| [check_mwcc_neutral.py](check_mwcc_neutral.py) | Proves that a guard-only change to decomp files (`#ifdef PLATFORM_PC` branches) leaves what MWCC compiles unchanged: MWCC's preprocessed text of every C file, both versions, against the last merged upstream commit ([../docs/game-changes.md](../docs/game-changes.md)) | after a change outside `port/` |
| [bootstrap_checkout.sh](bootstrap_checkout.sh) | Prepares a new clone or worktree: links `orig/`, `.venv` and the decomp's tools from a decomp checkout, sets up Claude Code's rules and write guard, and disables pushes to upstream ([../README.md](../README.md#preparing-a-checkout)) | once per clone or worktree |
| [package_macos.sh](package_macos.sh) | The macOS app, signed ad hoc, in a checked disk image named after it; with `--distribution`, the universal app for macOS 11 and later, configured, built and packaged in one command ([ADR 0015](../docs/adr/0015-universal-macos-app-for-macos-11.md)) | on a Mac, after a host build (or with `--distribution`, from nothing) |
| [ctest_one_slice.sh](ctest_one_slice.sh) | A universal macOS build's whole test suite as one slice, the x86_64 one under Rosetta on Apple Silicon (the game's code, the replays and the oracle's comparisons included): it thins the build's executables for the run and puts them back with their times, leaving out the tests that check the slices ([ADR 0015](../docs/adr/0015-universal-macos-app-for-macos-11.md)) | on Apple Silicon, after the distribution build |
| [hooks/port_guard.py](hooks/port_guard.py) | Claude Code's PreToolUse guard: refuses commands that would write to the decomp's repositories, link an upstream issue or mention upstream's people ([ADR 0002](../docs/adr/0002-solo-project-no-upstream-posts.md)); `--self-test` runs its cases | by Claude Code, before each command |

## The oracle

[oracle/](oracle/README.md) is the melonDS test oracle, `pbw-oracle`: the player's ROM on melonDS's emulation core,
headless and deterministic, which plays an input file and writes screens and memory for `compare.py` to hold Frigate
against (test level T4), and its sound, the mixer's output sample by sample and the ARM7 sound driver's state after each
run, for `sound.py`. It also makes the saves the tests start from (a save with a Pokémon, made from the first save), and
`oracle.py launch` holds the field menu's apps on Frigate's test launcher to the oracle's route through the field menu
([ADR 0012](../docs/adr/0012-test-launcher-for-the-field-menus-apps.md)). It is a test tool only, never linked into or
shipped with Frigate, and built only with `-DPORT_BUILD_ORACLE=ON`. Its README has the formats (input files, run
directories, sound traces), how frames are aligned, the comparison sequences, saves, the sound and the known
differences. `pbwinput.c` and `display3d.c` are plain C11 that the program's replays and the tests compile too, and
`sndtrace.h`, the sound trace's format, is plain C11 that Frigate itself includes: the platform layer's sound driver
writes its trace with it (`port/hal/snd7/snd7.c`), as do the program's `--sound-trace` (`port/app/sound_trace.c`) and
the sound driver's tests (`port/tests/snd/test_snd.c`).

## Measurement

Scripts from the 2026-10-16 research, kept for re-measuring. They read the decomp's tree and write reports. Each
directory's guide has a row per script.

| Directory | Purpose | Run |
| --- | --- | --- |
| [progress/](progress/README.md) | Port readiness of the decomp: per-function tiers (complete C, C not linked, no C), data and pointer coverage, both versions, daily history, GitHub claims, nonmatching rows by kind. Re-run at each sync and record the headline in [../docs/STATUS.md](../docs/STATUS.md) | `sh port/tools/progress/run_all.sh <repo> <outdir>` (about 5 minutes; needs `extract/{b2_us,w2_us}` and `.venv`) |
| [compile-probe/](compile-probe/README.md) | Host-compiler probe: clang over every C file for the DS target and 64-bit hosts, warning counts by kind and module, struct layouts compared between MWCC and each host ABI. The 64-bit warning count is a port metric that must only go down | see its README |
| [research/engine/](research/README.md#engine-the-games-engine-its-boot-path-and-the-ports-milestones) | Boot-path milestones, overlay base groups, data coverage per file (one-off) | scripts take the repository path |
| [research/sdk-surface/](research/README.md#sdk-surface-where-the-platform-layers-boundary-sits) | Library layering, game-side SDK surface, hardware access scan (one-off; `db.py` caches into a scratch directory) | scripts take the repository path |
| [research/assets/](research/README.md#assets-the-roms-files-and-its-sound-archive) | NitroFS survey, SDAT survey, version diff, and the NitroFS manifest above | scripts take the extracted ROM's path |

Requirements: a configured checkout with `extract/` (from `ninja extract/b2_us/config.yaml extract/w2_us/config.yaml`)
and `build/include/generated` (from `ninja constants_headers`), Homebrew LLVM for the compile probe, and read-only
`gh` for the claims report. The reports they produced are in [../docs/research/](../docs/research/README.md).
