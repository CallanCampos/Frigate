# port/tools/

The scripts and programs that build, check, sync and measure Frigate. None of them is part of the program the player
runs; the generators run inside the host build, and everything else is run by hand or by a test. None writes into the
repository unless its header says so: give each an output directory outside it, or under an ignored `build/`.

## Generators the host build runs

The host build ([../CMakeLists.txt](../CMakeLists.txt) and [../cmake/](../cmake/README.md)) runs these at configure or
build time; their output goes into the build directory, never into the repository.

| File | What it makes | Game functionality it serves |
| --- | --- | --- |
| [gen_host_tree.py](gen_host_tree.py) | The source list (every C file of `src/` and `lib/` minus the `replaced.txt` files of [../hal](../hal/README.md)), the `include/` mirror without MSL's C library headers, the host variants of the SDK headers that name DS addresses (`hwinclude/`, [ADR 0004](../docs/adr/0004-ds-addresses-by-header-translation.md)), and the generated constants headers | All of the game's C on a host |
| [gen_stubs.py](gen_stubs.py) | Trap stubs and data placeholders for every DS symbol the game references that has no C yet | Shows which missing function the game reaches next |
| [gen_overlay_sections.py](gen_overlay_sections.py) | Per-overlay sections for overlay code's writable statics, from `delinks.txt`; the check of a Mach-O link's section count | Overlays reloading as on the DS |
| [gen_window_icon.c](gen_window_icon.c) | The window's icon on Linux and Windows: the version's 32 px window icon, drawn pixel by pixel, and its icon master scaled to 64 px ([../assets/branding](../assets/branding/README.md)), as bitmaps in C, built and run for the machine that builds | None: the program's look |
| [research/assets/nitrofs_manifest.json](research/assets/) | Not a generator but build input: the size and SHA-1 of every NitroFS file of both ROMs, which [../cmake/rom.cmake](../cmake/rom.cmake) turns into the ROM manifest that checks a trimmed dump. `research/assets/manifest.py` wrote it | The ROM's check |

## Checking, syncing, packaging

| File | What it does | Run |
| --- | --- | --- |
| [sync_upstream.sh](sync_upstream.sh) | Merges the decomp's `main` into the port's, builds both ROMs and checks both SHA1s, builds and tests the host side, and pushes only when all of that passes. Exit 3 is a merge conflict, 4 an upstream history rewrite, 5 commits that name issues ([../docs/SYNC.md](../docs/SYNC.md)) | first in every port session |
| [check_mwcc_neutral.py](check_mwcc_neutral.py) | Proves that a guard-only change to decomp files (`#ifdef PLATFORM_PC` branches) leaves what MWCC compiles unchanged: MWCC's preprocessed text of every C file, both versions, against the last merged upstream commit ([../docs/game-changes.md](../docs/game-changes.md)) | after a change outside `port/` |
| [bootstrap_checkout.sh](bootstrap_checkout.sh) | Prepares a new clone or worktree: links `orig/`, `.venv` and the decomp's tools from a decomp checkout, sets up Claude Code's rules and write guard, and disables pushes to upstream ([../README.md](../README.md#preparing-a-checkout)) | once per clone or worktree |
| [package_macos.sh](package_macos.sh) | The macOS app, signed ad hoc, in a checked disk image named after it | on a Mac, after a host build |
| [hooks/port_guard.py](hooks/port_guard.py) | Claude Code's PreToolUse guard: refuses commands that would write to the decomp's repositories, link an upstream issue or mention upstream's people ([ADR 0002](../docs/adr/0002-solo-project-no-upstream-posts.md)); `--self-test` runs its cases | by Claude Code, before each command |

## The oracle

[oracle/](oracle/README.md) is the melonDS test oracle, `pbw-oracle`: the player's ROM on melonDS's emulation core,
headless and deterministic, which plays an input file and writes screens and memory for `compare.py` to hold Frigate
against (test level T4), and its sound, the mixer's output sample by sample and the ARM7 sound driver's state after
each run, for `sound.py`. It is a test tool only, never linked into or shipped with Frigate, and built only with
`-DPORT_BUILD_ORACLE=ON`. Its README has the formats (input files, run directories, sound traces), how frames are
aligned, the comparison sequences, saves, the sound and the known differences. `pbwinput.c` and `display3d.c` are
plain C11 that the program's replays and the tests compile too, and `sndtrace.h`, the sound trace's format, is what
the program's own trace includes (`port/app/sound_trace.c`).

## Measurement

Scripts from the 2026-10-16 research, kept for re-measuring. They read the decomp's tree and write reports.

| Directory | Purpose | Run |
| --- | --- | --- |
| [progress/](progress/) | Port readiness of the decomp: per-function tiers (complete C, C not linked, no C), data and pointer coverage, both versions, daily history, GitHub claims, nonmatching rows by kind. Re-run at each sync and record the headline in [../docs/STATUS.md](../docs/STATUS.md) | `sh port/tools/progress/run_all.sh <repo> <outdir>` (about 5 minutes; needs `extract/{b2_us,w2_us}` and `.venv`) |
| [compile-probe/](compile-probe/README.md) | Host-compiler probe: clang over every C file for the DS target and 64-bit hosts, warning counts by kind and module, struct layouts compared between MWCC and each host ABI. The 64-bit warning count is a port metric that must only go down | see its README |
| [research/engine/](research/engine/) | Boot-path milestones, overlay base groups, data coverage per file (one-off) | scripts take the repository path |
| [research/sdk-surface/](research/sdk-surface/) | Library layering, game-side SDK surface, hardware access scan (one-off; `db.py` caches into a scratch directory) | scripts take the repository path |
| [research/assets/](research/assets/) | NitroFS survey, SDAT survey, version diff, and the NitroFS manifest above | scripts take the extracted ROM's path |

Requirements: a configured checkout with `extract/` (from `ninja extract/b2_us/config.yaml extract/w2_us/config.yaml`)
and `build/include/generated` (from `ninja constants_headers`), Homebrew LLVM for the compile probe, and read-only
`gh` for the claims report. The reports they produced are in [../docs/research/](../docs/research/README.md).
