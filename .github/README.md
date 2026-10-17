# Pokémon Black 2 and White 2: native port

A native port of Pokémon Black 2 and White 2, for macOS on Apple Silicon first and then Windows and Linux, built from
the [pokebw2](https://github.com/fuddlesworth/pokebw2) decompilation. It is in progress and does not run the game yet.

This repository is pokebw2's tree with its full history, plus a `port/` directory. It follows the decomp
automatically: a GitHub Action merges upstream's `main` every hour, and each port session verifies the merge against
both ROMs ([how](../port/docs/SYNC.md)). The port is developed alongside the decomp, so each part of the game that
upstream turns into C reaches the port at the next sync.

## Status

The research and the plan are done. Phase 0 is next: CI, the host build skeleton, generated source lists, trap stubs
for code that isn't C yet, and 64-bit fixes far enough to link and open a window on macOS. The log, newest entry
first, is [port/docs/STATUS.md](../port/docs/STATUS.md).

The decomp's progress, from upstream's CI:

[![Code](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=matched_code_percent&label=Code)](https://bytematch.dev/fuddlesworth/pokebw2)
[![Functions](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=matched_functions&label=Functions)](https://bytematch.dev/fuddlesworth/pokebw2)
[![Files](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=complete_units&label=Complete%20files)](https://bytematch.dev/fuddlesworth/pokebw2)

## What "native" means

- All code runs as host machine code compiled from the decomp's C. No ARM code from the ROM is executed,
  interpreted, recompiled or translated, and there is no CPU emulator.
- The ROM is only a data source. You supply your own dump, and the port reads its files at runtime.
- The DS's display and sound hardware will be reimplemented as host renderers (SDL3 and SDL_GPU) and a host sound
  driver, so the game can draw at any resolution.
- An emulator (melonDS) serves only as a test reference during development. It is never linked into or shipped with
  the port.

## Documents

| Document | Covers |
| --- | --- |
| [Plan](../port/docs/PLAN.md) | Goals, architecture, mod design, testing, roadmap, decisions |
| [Research](../port/docs/RESEARCH.md) | The decomp's state, other ports, modding, technology ([reports](../port/docs/research/)) |
| [Upstream sync](../port/docs/SYNC.md) | How this repository follows the decomp; the reasons are in [ADR 0001](../port/docs/adr/0001-repository-and-upstream-sync.md) |
| [Status](../port/docs/STATUS.md) | Progress log |
| [Game changes](../port/docs/game-changes.md) | Every change the port makes outside `port/` |
| [Developer overview](../port/README.md) | The layout of `port/`, preparing a checkout, syncing |

## Relation to pokebw2

All decompilation work and credit belong to [fuddlesworth/pokebw2](https://github.com/fuddlesworth/pokebw2) and its
contributors. This repository adds `port/` and a few files of its own under `.github/`; everything else is upstream's,
changed only where [game-changes.md](../port/docs/game-changes.md) lists. The port's changes to game code keep both
ROMs byte for byte (or sit behind a host-only switch), and the matching-neutral ones are offered upstream.

## Building

The decomp builds here exactly as upstream: see the [README](../README.md) and [build details](../docs/building.md).
`python3 configure.py && ninja` rebuilds both ROMs from your own dumps and checks both SHA1s. The port itself doesn't
build yet; that is phase 0.

## For decomp contributors

Decompilation and matching happen upstream. Claim overlays, open pull requests and report decomp issues at
[fuddlesworth/pokebw2](https://github.com/fuddlesworth/pokebw2), following its
[CONTRIBUTING.md](../CONTRIBUTING.md). What merges there reaches this repository at the next sync. Issues and pull
requests here are for the port only. The issue and pull request templates are upstream's; file decomp claims and
build problems at [fuddlesworth/pokebw2](https://github.com/fuddlesworth/pokebw2/issues).

## Legal

No ROMs are included, and none will be. Port builds will ship no ROM, ROM files or game assets: the port reads them
from your own dump at runtime. The tree carries upstream's `data/`, about 99,500 files converted from the game (text,
game tables, map models and textures, scripts); port builds never ship it, and the port adds no game data. This
project is not affiliated with or endorsed by Nintendo, Game Freak, Creatures or The Pokémon Company.

The [GNU GPL v3.0](../LICENSE) license, as upstream, covers the contributors' work and grants no rights to Nintendo's
data or trademarks.
