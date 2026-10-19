<h1 align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="../port/assets/branding/logo-dark.svg">
    <img alt="Frigate" src="../port/assets/branding/logo.svg" width="400">
  </picture>
</h1>

Frigate is a native PC port of Pokémon Black 2 and Pokémon White 2, built from the
[pokebw2](https://github.com/fuddlesworth/pokebw2) decompilation; you supply your own ROM. It targets macOS on Apple
Silicon first, then Windows and Linux, which CI already builds and tests. It is in progress: it plays from power-on
through the title screen and the start menu, and the overworld waits for the decomp.

Frigate is a fan project, not affiliated with or endorsed by Nintendo, Game Freak, Creatures or The Pokémon Company.

This repository is pokebw2's tree with its full history, plus a `port/` directory, where Frigate lives. It follows the
decomp automatically: a GitHub Action merges upstream's `main` every hour, and each port session verifies the merge
against both ROMs ([how](../port/docs/SYNC.md)). Frigate is developed alongside the decomp, so each part of the game
that upstream turns into C reaches Frigate at the next sync.

## Status

Frigate boots natively through the game's logos, the title screen and the start menu, with sound, and its tests hold it
frame by frame against melonDS; its sound too: the sound driver run by run, the title's stream and the cry to the bit,
the music by its envelope and levels. New Game's intro runs to the field's first script, and CONTINUE loads a save,
which stays interchangeable with melonDS's, up to the field. The field engine (overlay 36) is still being decompiled
upstream; code that isn't C yet stops the game with its name. Meanwhile a test launcher opens the field menu's apps in
the field's place, where the bag, the trainer card, the party list and the Pokédex match melonDS pass for pass in both
versions. The log, newest entry first, is [port/docs/STATUS.md](../port/docs/STATUS.md).

The decomp's progress, from upstream's CI:

[![Code](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=matched_code_percent&label=Code)](https://bytematch.dev/fuddlesworth/pokebw2)
[![Functions](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=matched_functions&label=Functions)](https://bytematch.dev/fuddlesworth/pokebw2)
[![Files](https://bytematch.dev/fuddlesworth/pokebw2.svg?mode=shield&measure=complete_units&label=Complete%20files)](https://bytematch.dev/fuddlesworth/pokebw2)

## What "native" means

- All code runs as host machine code compiled from the decomp's C. No ARM code from the ROM is executed,
  interpreted, recompiled or translated, and there is no CPU emulator.
- The ROM is only a data source. You supply your own dump, and Frigate reads its files at runtime.
- The DS's display and sound hardware will be reimplemented as host renderers (SDL3 and SDL_GPU) and a host sound
  driver, so the game can draw at any resolution.
- An emulator (melonDS) serves only as a test reference during development. It is never linked into or shipped with
  Frigate.

## Documents

| Document | Covers |
| --- | --- |
| [Developer overview](../port/README.md) | Building, running and testing Frigate, the map of `port/`, preparing a checkout, syncing |
| [Architecture](../port/docs/ARCHITECTURE.md) | The layers, how a frame runs, the boundary rules, and the map from each part of the game to its code and tests |
| [Modding](../port/docs/MODDING.md) | Where the game's logic and data live, how to change and test them, and what the planned mod platform adds |
| [Plan](../port/docs/PLAN.md) | Goals, architecture, mod design, testing, roadmap, decisions |
| [Status](../port/docs/STATUS.md) | Progress log |
| [Game changes](../port/docs/game-changes.md) | Every change the port makes outside `port/` |
| [Upstream sync](../port/docs/SYNC.md) | How this repository follows the decomp; the reasons are in [ADR 0001](../port/docs/adr/0001-repository-and-upstream-sync.md) |
| [Research](../port/docs/RESEARCH.md) | The decomp's state, other ports, modding, technology ([reports](../port/docs/research/)) |
| [Artwork](../port/assets/branding/README.md) | Frigate's logo and app icons: the files, their colours, how the build uses them and how to change them |
| [All documents](../port/docs/README.md) | Every document, one line each |

## Relation to pokebw2

All decompilation work and credit belong to [fuddlesworth/pokebw2](https://github.com/fuddlesworth/pokebw2) and its
contributors. This repository adds `port/` and a few files of its own under `.github/`; everything else is upstream's,
changed only where [game-changes.md](../port/docs/game-changes.md) lists. The port's changes to game code keep both
ROMs byte for byte (or sit behind a host-only switch).

## Building

The decomp builds here exactly as upstream: see the [README](../README.md) and [build details](../docs/building.md).
`python3 configure.py && ninja` rebuilds both ROMs from your own dumps and checks both SHA1s. Frigate builds with
CMake, as [port/README.md](../port/README.md#quick-start) says: `frigate-black2` and `frigate-white2`, and on macOS
`Frigate Black 2.app` and `Frigate White 2.app`, which you build yourself from your own checkout, for your Mac or, with
the distribution build, as one universal app for Apple Silicon and Intel Macs, built for macOS 11 or later and so far
run only on macOS 26 ([port/README.md](../port/README.md#the-macos-app)). On Windows it builds in MSYS2's CLANG64
environment ([Building on Windows](../port/README.md#building-on-windows)).

## For decomp contributors

Decompilation and matching happen upstream: decomp claims and contributions go to
[fuddlesworth/pokebw2](https://github.com/fuddlesworth/pokebw2), following its [CONTRIBUTING.md](../CONTRIBUTING.md).
What merges there reaches this repository at the next sync.

Problems with this repository, including building it, go in
[this repository's issues](https://github.com/CallanCampos/Frigate/issues). Its issue and pull request
templates are upstream's, written for decomp work; a port issue or pull request ignores their decomp checklists.

## Legal

No ROMs are included, and none will be. Frigate's builds ship no ROM, ROM files or game assets: Frigate reads them from
your own dump at runtime. The tree carries upstream's `res/`, about 119,000 files, most of them converted from the game
(text, game tables, map models and textures, scripts); Frigate's builds never ship it, and Frigate adds no game data.
Frigate is not affiliated with or endorsed by Nintendo, Game Freak, Creatures or The Pokémon Company.

The [GNU GPL v3.0](../LICENSE) license, as upstream, covers the contributors' work and grants no rights to Nintendo's
data or trademarks. Frigate's logo and icons are its own drawing of the Plasma Frigate, the games' flying ship that
Frigate is named after, under the same license, which covers the drawing and not the ship's design: that belongs to
the games' owners ([port/assets/branding](../port/assets/branding/README.md#license)).
