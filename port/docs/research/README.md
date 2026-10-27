# Research reports

The full reports behind [RESEARCH.md](../RESEARCH.md), written by research agents on 2026-10-16 against
`fuddlesworth/pokebw2` at `0c2bbee9d`. Each marks what it verified in code or sources and what it inferred. Paths to
scratch directories inside them refer to that session and no longer exist; the scripts they mention are kept in
[port/tools](../../tools/).

Where a report's recommendation differs from a decision in [PLAN.md §3](../PLAN.md#3-decisions), the plan wins. D10
([ADR 0002](../adr/0002-solo-project-no-upstream-posts.md)) replaces the reports' upstream proposals, such as
[port-landscape.md](port-landscape.md)'s game-code changes that go upstream
([§7.1](port-landscape.md#71-principles-inferred), principle 1), its screen-size constants
([§7.6](port-landscape.md#76-rendering-widescreen-and-dual-screens-inferred)), its upstream portability fixes and hook
sites ([§7.16](port-landscape.md#716-keeping-game-code-close-to-the-decomp-inferred)), and its open questions about
pokebw2 accepting them ([§7.18](port-landscape.md#718-decisions-to-make-explicitly-inferred)). Nothing is proposed
upstream; those changes stay in this repository. D11
([ADR 0003](../adr/0003-ad-hoc-signing-no-apple-developer-account.md)) replaces their Developer ID and notarization
recommendations: macOS builds are ad-hoc signed and never notarized.

| Report | Covers |
| --- | --- |
| [decomp-progress.md](decomp-progress.md) | Tiers of C coverage per module, data and pointer coverage, pace, claims, nonmatching rows |
| [engine-boot.md](engine-boot.md) | Main loop, GFL systems, boot-to-gameplay flow, milestone blockers, overlay semantics, dual-screen use |
| [sdk-surface.md](sdk-surface.md) | Library layering, game-side SDK surface, hardware access, boundary options, HAL table |
| [compile-probe.md](compile-probe.md) | Host compile results, 64-bit sites, struct layouts, undefined behaviour, fix effort |
| [assets-arm7-sound.md](assets-arm7-sound.md) | ROM layout, file formats, `data/`, ARM7, sound, saves, DSi, IR, first-launch design |
| [es-rom-support.md](es-rom-support.md) | The Spanish ROM (IRES) against the US one: header, overlays, NitroFS, text, the US code's English logic, options (2026-10-26) |
| [platinum-port.md](platinum-port.md) | The Pokémon Platinum PC port's game-side changes, history, quality and lessons |
| [libntr.md](libntr.md) | libntr's architecture, coverage, renderer, licensing, macOS blockers and BW2 gaps |
| [platinum-mods-alternatives.md](platinum-mods-alternatives.md) | Widescreen and single-screen mods, Gen4Recomp, other DS ports, multiplayer |
| [port-landscape.md](port-landscape.md) | libultraship ports, sm64coopdx, static recompilation, GBA ports, mod architectures |
| [bw2-mod-ecosystem.md](bw2-mod-ecosystem.md) | Existing Gen 5 projects, randomizer, widescreen, multiplayer and revival services, hacks and tools |
| [tech-stack.md](tech-stack.md) | SDL3, SDL_GPU, renderers, build, packaging, mod libraries, networking, testing |
