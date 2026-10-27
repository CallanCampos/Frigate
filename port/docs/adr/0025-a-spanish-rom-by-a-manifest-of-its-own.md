<!-- Records why Frigate can be built to accept a Spanish Black 2 ROM (IRES) whose files the US code reads, by a
manifest written from the player's dump, and what that leaves wrong. -->
# 0025: A Spanish ROM by a manifest of its own

- **Status:** proposed
- **Date:** 2026-10-26
- **Plan:** §1 (the ROM is a data source the player supplies), §4.5 (the ROM's check), D6
- **Upstream:** `deb044db2d`, as Frigate's `main` has it at `f786fed993`
- **Note (2026-10-26):** a local change in a clone that publishes nothing. The owner of that clone has only the Spanish
  ROM and asked for this, as the first step of [es-rom-support.md](../research/es-rom-support.md).

## Context

Frigate plays the ROM the decomp builds: `port_rom_check` refuses any game code but the version's US one (`IREO`,
`IRDO`), and a dump that isn't byte for byte the decomp's must match the US manifest file by file. The Spanish Black 2
(`IRES`) is refused at the first check. [es-rom-support.md](../research/es-rom-support.md) measured it:

- its NitroFS has the same 318 files (two `dl_rom` children are named `_spa`), 252 of them identical;
- every differing archive keeps its member count, and the text keeps every message count, except the Easy Chat lists by
  initial and 8 messages added at the end of one script bank;
- its overlay table has the same 344 overlays, consistently moved by main's 64 bytes.

So the US code can read its data. The code stays the decomp's US C, with the English logic that §4 of the report lists.

## Decision

A configure option, `PORT_ROM_LANGUAGE` (`us`, the default, or `es`), picks the ROM the program accepts. For `es`,
[rom.cmake](../../cmake/rom.cmake) builds the ROM manifest from the `b2_es` (or `w2_es`) entry of
`port/tools/research/assets/nitrofs_manifest_languages.json`. That entry holds the game code, the whole dump's SHA-1,
the overlay table's size and SHA-1, and every file's size and SHA-1. It is written from the player's own dump by
[rom_manifest.py](../../tools/research/assets/rom_manifest.py), which reads the ROM's tables in place, hashes only.

Configure stops if the entry is missing, if its game code isn't the version's, or if its overlay table doesn't have
`config/<version>`'s count of overlays. The program then accepts exactly that dump, or one whose files match it, with
the title "Pokémon Black 2 (Spain)". It keeps its saves apart (`IRES.sav`). Nothing in the game's code changes. The US
build is unchanged: the same manifest, only messages reworded ("the decomp's" stays for it).

## Alternatives

- **A Spanish version of the decomp (`b2_es`).** It is the exact game, but it means configs and matching outside
  `port/`, which this repository's rules forbid here, and weeks of decomp work.
- **A Spanish manifest committed for everyone.** No clean IRES dump has a published hash to trust. The dump this was
  first made from is a "(Fix)" one of unknown changes. So the entry is written from each player's dump and isn't a
  shared input.
- **Skipping the check for other languages.** That would accept any dump, damaged ones too, against
  [port_rom.h](../../hal/rom/port_rom.h)'s promise that what the program reads is what the manifest has.

## Consequences

- The game runs with Spanish text, fonts and graphics, but with the US code's English logic:
  - Pokémon say English (`region`, `GAME_LANGUAGE`);
  - DS Download Play asks for `_eng.srl`;
  - Easy Chat's word tables by initial are English;
  - the article and plural loaders in `wordset.c` are English.
  Fixing those is the report's option C: a language define for the host build only, one reviewed change each, with
  its row in [game-changes.md](../game-changes.md).
- The replay, frame-hash and oracle tests are US-only. An `es` build has its own build directory and runs the unit
  tests that need a ROM against the Spanish dump.
- Revisit when upstream builds another region, or when a known clean IRES dump gives a hash to commit.
