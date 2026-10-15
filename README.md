# Pokémon Black 2 and White 2

A work-in-progress decompilation of Pokémon Black 2 and White 2, with a long-term goal of a native PC port.
Both USA/Europe ROMs rebuild byte for byte from the same source tree.

## Progress

[![Code](https://decomp.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=code&label=Code)](https://decomp.dev/fuddlesworth/pokebw2)
[![Functions](https://decomp.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=functions&label=Functions)](https://decomp.dev/fuddlesworth/pokebw2)
[![Files](https://decomp.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=complete_units&label=Complete%20files)](https://decomp.dev/fuddlesworth/pokebw2)

[![Progress](https://decomp.dev/fuddlesworth/pokebw2.svg)](https://decomp.dev/fuddlesworth/pokebw2)

Progress updates automatically from CI. Green areas match; grey and blue areas are still in progress.

## Build

Requires Linux, WSL or macOS, Python 3.11+, ninja, and LLVM (`clang` and `llvm-objcopy` on your `PATH`).

1. Place your own ROM dumps in `orig/`, using the filenames below.
2. Configure and build:

   ```sh
   python3 configure.py
   ninja
   ```

The first run downloads the remaining tools. Every available version is built and checked against its SHA1.
ROMs are not included or provided.

| ROM (USA/Europe, NDSi Enhanced) | Input file | SHA1 |
| --- | --- | --- |
| Black 2 | `orig/baserom_b2_us.nds` | `e51e6dfb8678a3d19dcd2a10691b96a569ca0abb` |
| White 2 | `orig/baserom_w2_us.nds` | `b5d7490be7b415b8f1e672a53e978a9cc667e56a` |

Output: `build/pokeblack2_us.nds` and/or `build/pokewhite2_us.nds`.
See [Build details](docs/building.md) for macOS setup, toolchain options and modified builds.

## Contribute

Read [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow and rules. Claim an overlay's issue before starting work.
CI compiles every C file for both versions; fork PRs must verify ROM matching locally.

## Documentation

| Guide | Covers |
| --- | --- |
| [Build details](docs/building.md) | Setup and build options |
| [Decompiling](docs/decompiling.md) | Workflow and tools |
| [Matching](docs/matching.md) | MWCC behavior and matching techniques |
| [Code organization](docs/code-organization.md) | Source layout, libraries and names |
| [Native port notes](docs/porting-notes.md) | Recovered platform assumptions and open porting questions |
| [Source files](docs/source-files.md) | Overlay files and boundary evidence |
| [Nonmatching functions](docs/nonmatching-functions.md) | Remaining mismatches |
| [Game data](docs/data.md) | Data formats and archives |
| [Scripts](docs/scripts.md) | Trainer AI and field scripts |
| [Configs and ROMs](docs/configs.md) | dsd configuration and ROM structure |
| [Tools](tools/README.md) | Tool reference |

## License

[GNU GPL v3.0](LICENSE), including the symbol names imported from [swan](docs/code-organization.md#names).
