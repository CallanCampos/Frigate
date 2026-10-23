# port/docs/

Every document about Frigate, one line each. Living documents describe how things are now and change with the code;
records keep what was decided or found at the time, and are added to, not rewritten.

## Start here

| Document | What it covers |
| --- | --- |
| [../README.md](../README.md) | The front door: what Frigate is, the quick start, the directory map, controls, saves, the macOS app, preparing a checkout |
| [../CONTRIBUTING.md](../CONTRIBUTING.md) | External contributions: forks, topic branches, PRs to main, validation and scope |
| [ARCHITECTURE.md](ARCHITECTURE.md) | The layers, how a frame runs, the threads, the exit statuses, the boundary rules, and the feature map from each part of the game to its code and tests |
| [MODDING.md](MODDING.md) | Where the game's logic and data live, how to find, change and test the code for a feature, and what the planned mod platform will add |
| [GLOSSARY.md](GLOSSARY.md) | The DS's, the SDK's, the decomp's and Frigate's words |
| [../CLAUDE.md](../CLAUDE.md) | The rules for working on Frigate: the name, readable code, where to work, changes outside `port/`, sync, the iteration loop, testing |

## Living documents

| Document | What it covers |
| --- | --- |
| [PLAN.md](PLAN.md) | The plan of record: goals, what "native" means, decisions D1 to D11, the architecture, the mod design, verification, the roadmap's phases |
| [REPOSITORY-POLICY.md](REPOSITORY-POLICY.md) | Enforced main rules, reviews, required checks, Actions permissions and audit commands |
| [SYNC.md](SYNC.md) | How this repository follows upstream: the hourly Action, the local sync, conflicts, what to do when either stops |
| [game-changes.md](game-changes.md) | Every change Frigate makes outside `port/`, with its kind, its guard and its proof that the ROMs stay byte for byte |
| [decomp-findings.md](decomp-findings.md) | Where the decomp's C that the ROM doesn't link behaves differently from the ROM, or is unresolved |
| [lessons.md](lessons.md) | What port work taught that the code doesn't show, newest first |
| [sanitizers.md](sanitizers.md) | The sanitized build (T5): how to run it and read its reports, what Frigate tells the sanitizers, the checks it leaves out and why, what it can't see, and what it found |

## Records

| Document | What it covers |
| --- | --- |
| [REVIEW-2026-10-21.md](REVIEW-2026-10-21.md) | Evidence-backed sync and architecture review, field-entry blocker, validation limits and next work |
| [STATUS.md](STATUS.md) | The status log, newest first: one entry per local sync or finished task, with the tests that ran |
| [adr/README.md](adr/README.md) | Architecture decision records: when to write one, the template, and the index |
| [adr/0001](adr/0001-repository-and-upstream-sync.md) | The repository and its upstream sync |
| [adr/0002](adr/0002-solo-project-no-upstream-posts.md) | A solo project, with no posts upstream |
| [adr/0003](adr/0003-ad-hoc-signing-no-apple-developer-account.md) | Ad-hoc signing, no Apple Developer account |
| [adr/0004](adr/0004-ds-addresses-by-header-translation.md) | DS addresses by build-time header translation |
| [adr/0005](adr/0005-platform-boundary-as-built.md) | The platform boundary as built |
| [adr/0006](adr/0006-sound-driver-as-built.md) | The sound driver as built |
| [adr/0007](adr/0007-nitrosystem-files-in-host-structs.md) | NitroSystem's 2D files in host structs |
| [adr/0008](adr/0008-particle-work-from-the-platform-layer.md) | Particle work from the platform layer |
| [adr/0009](adr/0009-gamepads-by-position-one-pad-at-a-time.md) | Gamepads by position, one pad at a time |
| [adr/0010](adr/0010-macos-app-as-a-signed-copy-of-the-program.md) | The macOS app as a signed copy of the program |
| [adr/0011](adr/0011-cartridge-infrared-chip-as-a-register-model.md) | The cartridge's infrared chip as a register model |
| [adr/0012](adr/0012-test-launcher-for-the-field-menus-apps.md) | Frigate's test launcher for the field menu's apps while overlay 36 is claimed |
| [adr/0013](adr/0013-windows-build-with-msys2-clang64.md) | Windows built with MSYS2's CLANG64, its differences behind `port/hal/host` |
| [adr/0014](adr/0014-sanitized-build-game-heaps-poisoned-ds-isms-ignorelisted.md) | The sanitized build: the game's heaps poisoned in place, the DS-isms ignorelisted |
| [adr/0015](adr/0015-universal-macos-app-for-macos-11.md) | A universal macOS app for macOS 11, with the pinned SDL3 linked statically |
| [adr/0017](adr/0017-external-contributions.md) | External contributions through forks and PRs, with maintainer sync and ROM verification |
| [adr/0018](adr/0018-protected-main-and-sync-prs.md) | Protected main and reviewed upstream syncs |
| [adr/0019](adr/0019-pointer-cast-guard-against-a-classified-baseline.md) | The pointer cast guard: the game's casts that cut an address, held to a classified baseline |
| [RESEARCH.md](RESEARCH.md) | The research of 2026-10-16 behind the plan, summarized |
| [research/README.md](research/README.md) | The full research reports, with what each covers |
| [research/decomp-progress.md](research/decomp-progress.md) | The decomp's C coverage per module, data and pointer coverage, pace, claims |
| [research/engine-boot.md](research/engine-boot.md) | The main loop, GFL's systems, the boot to gameplay, overlays, the two screens |
| [research/sdk-surface.md](research/sdk-surface.md) | The libraries' layers, the SDK surface the game uses, hardware access, the boundary's options |
| [research/compile-probe.md](research/compile-probe.md) | The decomp compiled for 64-bit hosts: warnings, struct layouts, undefined behaviour |
| [research/assets-arm7-sound.md](research/assets-arm7-sound.md) | The ROM's layout and formats, the ARM7, sound, saves, the DSi, infrared |
| [research/platinum-port.md](research/platinum-port.md) | The Pokémon Platinum PC port: its changes, history and lessons |
| [research/libntr.md](research/libntr.md) | libntr's architecture, coverage, licensing and gaps for BW2 |
| [research/platinum-mods-alternatives.md](research/platinum-mods-alternatives.md) | Widescreen and single-screen mods, Gen4Recomp, other DS ports, multiplayer |
| [research/port-landscape.md](research/port-landscape.md) | libultraship ports, sm64coopdx, static recompilation, GBA ports, mod architectures |
| [research/bw2-mod-ecosystem.md](research/bw2-mod-ecosystem.md) | Gen 5 projects, randomizers, widescreen, multiplayer and revival services, hacks and tools |
| [research/tech-stack.md](research/tech-stack.md) | SDL3, SDL_GPU, renderers, the build, packaging, mod libraries, networking, testing |

## Directory guides

Each directory of `port/` has a guide to its files: what each does, the part of the game it touches, its tests.

| Guide | What it covers |
| --- | --- |
| [../app/README.md](../app/README.md) | The program: `main()` and its options, the window, the controls, replays, the run directory, the sound driver's trace, the test launcher (`--launch`), the macOS app's scripts |
| [../hal/README.md](../hal/README.md) | The platform layer: one row per module with the SDK module it replaces, how decomp files are adapted, the names |
| [../gfx/README.md](../gfx/README.md) | The renderers: 2D, the geometry engine, 3D, capture, the scanline hook |
| [../audio/README.md](../audio/README.md) | The sound output |
| [../include/README.md](../include/README.md) | `port_host.h`, force-included into the decomp's C |
| [../cmake/README.md](../cmake/README.md) | The build's fragments, their tests, and the build's options |
| [../tests/README.md](../tests/README.md) | The tests by directory, which need the ROM, how to write one |
| [../tests/asm_oracle/README.md](../tests/asm_oracle/README.md) | The differential tests of the SDK's assembly in C against the ROM's code |
| [../tools/README.md](../tools/README.md) | The generators, the sync, the packaging, the oracle, the measurement scripts |
| [../tools/oracle/README.md](../tools/oracle/README.md) | The melonDS oracle: building and running it, input files, run directories, comparing, saves, the sound, the launch checks of the field menu's apps, known differences |
| [../tools/compile-probe/README.md](../tools/compile-probe/README.md) | The host-compiler probe: 64-bit warnings and struct layouts (the 64-bit metric is the pointer cast guard's, [SYNC.md](SYNC.md#when-the-pointer-cast-guard-fails)) |
| [../assets/README.md](../assets/README.md) | Files that aren't code |
| [../assets/branding/README.md](../assets/branding/README.md) | Frigate's logo and app icons: the files, colours, how the build uses them, how to change them, the license |
| [../spikes/README.md](../spikes/README.md) | Phase 0's frozen experiments and where their live code is: [os-fibers](../spikes/os-fibers/README.md), [overlay-sections](../spikes/overlay-sections/README.md), [vram-alias](../spikes/vram-alias/README.md) |

The repository's landing page on GitHub is [../../.github/README.md](../../.github/README.md). The decomp's own
documents are in [../../docs/](../../docs/): its [code organization](../../docs/code-organization.md),
[source files](../../docs/source-files.md), [game data](../../docs/data.md) and [scripts](../../docs/scripts.md) are the
ones Frigate's documents point to.
