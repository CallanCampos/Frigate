# Pokémon Black 2 and White 2

A work-in-progress decompilation of Pokémon Black 2 and White 2 (NDS, DSi-enhanced). The long-term goal is a PC port.

It builds the following ROMs:

| Version | File | SHA1 |
| --- | --- | --- |
| Black 2, USA/Europe (NDSi Enhanced) | `build/pokeblack2_us.nds` | `e51e6dfb8678a3d19dcd2a10691b96a569ca0abb` |
| White 2, USA/Europe (NDSi Enhanced) | `build/pokewhite2_us.nds` | `b5d7490be7b415b8f1e672a53e978a9cc667e56a` |

## Status

Both ROMs rebuild byte for byte from the same source tree, and everything not yet in C is still delinked code.
[decomp.dev](https://decomp.dev/fuddlesworth/pokebw2) tracks the progress of both versions from CI's report on every
push to main.

[![Code](https://decomp.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=code&label=Code)](https://decomp.dev/fuddlesworth/pokebw2)
[![Functions](https://decomp.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=functions&label=Functions)](https://decomp.dev/fuddlesworth/pokebw2)
[![Files](https://decomp.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=complete_units&label=Complete%20files)](https://decomp.dev/fuddlesworth/pokebw2)

[![Progress](https://decomp.dev/fuddlesworth/pokebw2.svg)](https://decomp.dev/fuddlesworth/pokebw2)

Each rectangle is a source file, or a stretch of code not yet split into files, sized by its code: green when it
matches, and from grey to blue as it gets closer.

- 41,423 functions found by [dsd](https://github.com/AetiasHax/ds-decomp) in the ARM9, its 344 overlays, ITCM, DTCM, and the two TWL autoloads.
- 8,152 functions and 573 data symbols have real names, imported from [swan](docs/code-organization.md#names).
- The DSi-only LTD module in ARM9i is decompressed, analyzed and linked like the other modules. The ARM7i program is
  only extracted (decrypted) and rebuilt.
- The trainer AI and field scripts are built from source, see [Scripts](docs/scripts.md).

## Setup

1. Use Linux (or WSL on Windows) or macOS, with Python 3.11 or later, ninja, and clang and LLVM, whose `clang` and
   `llvm-objcopy` assemble the scripts and must be on the `PATH`. The Metrowerks tools are Windows programs, which
   [wibo](https://github.com/decompals/wibo) runs; `configure.py --wine wine` runs them with Wine instead.

   On macOS, `configure.py` downloads wibo's macOS build, which is x86-64 and runs under Rosetta 2 on Apple silicon
   (`softwareupdate --install-rosetta`). Homebrew no longer offers Wine: its wine casks have been disabled since
   2026-09-01, as they fail the Gatekeeper check. Homebrew's `llvm` has `llvm-objcopy` but isn't linked, so put
   `$(brew --prefix llvm)/bin` on the `PATH`.

2. Place your own dumps at `orig/baserom_b2_us.nds` and/or `orig/baserom_w2_us.nds`. They must match the SHA1s above.
   They are not included and will not be provided. `tools/build/verify_dsi_rom.py` checks a dump against the
   digests in its own header.

3. Configure and build. On first run, `configure.py` downloads [wibo](https://github.com/decompals/wibo), objdiff, the
   Metrowerks CodeWarrior tools, and dsd with DSi hybrid ROM support. Until those changes are upstreamed, dsd is a
   release of these forks' `dsi-hybrid` branches:
   - [`ds-rom`](https://github.com/fuddlesworth/ds-rom/tree/dsi-hybrid): DSi header, digests, modcrypt, TWL autoloads
     and DSi banners.
   - [`ds-decomp`](https://github.com/fuddlesworth/ds-decomp/tree/dsi-hybrid): TWL entrypoint, DS Protect and Thumb
     jump table fixes, the ARM9i LTD module with its sections and DSP images, overlays that run from VRAM, `ldr pc`
     veneers, and overlays linked with a veneer for every call, as the SDK links them.

   ```sh
   python3 configure.py
   ninja
   ```

   `ninja` extracts each base ROM, delinks the code, links it with `mwldarm`, rebuilds the ROM and checks its SHA1.
   `configure.py` builds every version that has a base ROM, or the versions given as arguments. It downloads dsd
   again when `DSD_VERSION` changes, and `--dsd-from-source` builds that release with [cargo](https://rustup.rs)
   instead, as on platforms without a release binary. To work on dsd itself, build it from your own clone of the
   fork and copy it to `tools/dsd`, or pass `--dsd`; a `tools/dsd` without `tools/dsd.rev` is never replaced.

4. Optionally, `python3 configure.py --bugfix` builds the ROMs with the game's bugs fixed, those marked with `BUGFIX`
   in the source. Only files marked `complete` are built from source, so fixes in the others don't apply yet. And
   `python3 configure.py --shift 0x100` builds them with the code moved, to test that mods can change code sizes; see
   [Shifting](docs/configs.md#shifting). These ROMs don't match, so the build skips the checks. Run `configure.py`
   without the option to go back to the matching build.

## Layout

| Path | Contents |
| --- | --- |
| `config/<version>/` | dsd configs: sections (`delinks.txt`), symbols and relocations for every module |
| `config/names.txt`, `config/fixes.txt` | Our own names and fixes to dsd's analysis, applied again after regenerating the configs |
| `src/ovNNN/` | Decompiled C of each overlay, such as `src/ov035/event_mapchange.c` |
| `src/gfl/`, `src/system/` | Decompiled C of the ARM9 main module, by library like the headers, such as `src/gfl/heap.c` |
| `lib/<name>/` | Libraries built apart from the game with their own compiler (`library.toml`), headers and sources, such as `lib/spl/` |
| `include/` | Headers shared by the C code, see [Code organization](docs/code-organization.md) |
| `data/` | The game's data as JSON, one file per species, move, trainer, zone and encounter table, the constant lists, the text, and the scripts, all built into the ROM's files; see [Game data](docs/data.md) and [Scripts](docs/scripts.md) |
| `include/asm/` | Macros for the scripts |
| `tools/` | Our tools, by job: the build's steps, decompiling, the game data, the text and the scripts; see [tools/README.md](tools/README.md) |
| `docs/` | The documentation listed below |
| `CLAUDE.md`, `.claude/` | Rules, skills, agents and hooks for working on the decompilation with Claude Code |
| `extract/`, `build/` | Generated, never committed |

## Decompiling

Each of the game's original source files becomes one C file, written from the assembly until every function compiles
to the original bytes. Matching is checked per function with [objdiff](https://github.com/encounter/objdiff) and
`tools/decomp/compiler_probe.py`, and the ROMs are rebuilt and checked against their SHA1s by every `ninja`.
`ninja progress` prints how much of the game matches.

## Documentation

| Document | Contents |
| --- | --- |
| [Decompiling](docs/decompiling.md) | The workflow, the tools, and the compiler versions |
| [How MWCC compiles](docs/matching.md) | What makes the compiler's output match, by what a diff shows |
| [Code organization](docs/code-organization.md) | Source files, headers, names from swan, and the two versions |
| [Nonmatching functions](docs/nonmatching-functions.md) | Every function in C that doesn't match yet, and why |
| [Source files](docs/source-files.md) | Each overlay's original files, with the evidence for their names |
| [Scripts](docs/scripts.md) | The trainer AI and field scripts built from source, and their macros |
| [Game data](docs/data.md) | The data archives built from source, such as the species data, and their formats |
| [dsd configs and the ROM](docs/configs.md) | Fixing and regenerating the configs, the DSi's differences, known gaps |

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) has the workflow and the rules. CI compiles every C
file for both versions. On pushes to main and on pull requests from branches of this repository, it also builds both
ROMs from private copies, checks them byte for byte, and uploads the progress reports that decomp.dev reads. Pull
requests from forks can't reach the ROMs, so run `ninja` locally before opening one.

## License

This project is licensed under the GNU General Public License v3.0, see [LICENSE](LICENSE). The symbol names imported
from swan are also GPL-3.0.

The repository contains no game code or assets. Building it requires your own dump of the game.
