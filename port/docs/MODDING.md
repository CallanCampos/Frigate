# Modding

An orientation for anyone who wants to change how the game plays in Frigate: where the game's logic and data live,
how to find the code for a feature, how a change is made and tested here, and what the mod platform will add. It is
honest about today. Frigate has no mod support yet: no mod loader, no hooks, no scripting, no data overrides. What it
has is the game's whole source, compiled into the program, so a change to the game today is a change to its C.

Two limits shape everything below:

- **Frigate plays as far as the decomp's C goes.** It runs from power-on through the logos, the opening, the title
  screen, the start menu and New Game's intro, and loads a save up to the field. The field engine (overlay 36) is
  still being decompiled upstream, and the game stops there with the name of the first function that has no C
  ([STATUS.md](STATUS.md) has the current state). The overworld, battles and menus past it can't be reached yet.
- **Frigate reads the game's data from the player's ROM, and checks it.** The program accepts only a dump of the
  version it was built for, by its SHA-1 or by every NitroFS file's ([../hal/rom/port_rom.h](../hal/rom/port_rom.h)),
  so a patched ROM is refused. Changing data files (text, species, trainers, maps) needs the layered file system of
  the mod platform, which isn't built yet. Data that the game keeps in its code (tables in C) changes with the C.

## Where the game lives

Everything outside `port/` is the decomp's, as upstream writes it. Its own documents describe it; the ones a modder
needs first:

| Where | What it holds | Read |
| --- | --- | --- |
| `src/system/` | The game's own code in ARM9 main, the part always loaded: the main loop, saves, sound, sprites, the script VM | [code organization](../../docs/code-organization.md) |
| `src/gfl/` | Game Freak's library: heaps, overlays, archives, processes, tasks, backgrounds, cell actors, input, text | as above |
| `src/ovNNN/` | Overlay NNN's files: each screen, mode and system the game loads on demand (the title in `ov162`, the intro in `ov294`, the field engine in `ov036`, battles in `ov167` to `ov170`, the bag in `ov142`) | [source files](../../docs/source-files.md), which lists every overlay's files by name with the evidence for each |
| `lib/` | Nintendo's libraries (NitroSDK, TwlSDK, NitroSystem, SPL, DWC and the rest), built apart from the game | [code organization](../../docs/code-organization.md) |
| `include/` | The game's headers, mirroring `src/` | |
| `res/`, `generated/` | The game's data as sources (species, moves, items, trainers, maps, text, sound), from which the ROM build packs its archives, and the constant lists the C's headers come from. Frigate doesn't read them: it reads the player's ROM | [game data](../../docs/data.md), [scripts](../../docs/scripts.md) |
| `config/<version>/arm9/` | Where every function and data object sits in the ROM: `delinks.txt` (which source file covers which range, and whether it is complete) and `symbols.txt` (every symbol's name and address) | [configs](../../docs/configs.md) |

The decomp writes C from the ROM's code, function by function. A file marked complete compiles to the ROM's exact
bytes; an incomplete one has C that doesn't all match yet, which the ROM build doesn't link but Frigate compiles all
the same. So Frigate runs C that no hash has proven: where it behaves differently from the DS, the finding goes in
[decomp-findings.md](decomp-findings.md).

## Finding the code for a feature

- **Start from the [feature map](ARCHITECTURE.md#feature-map)**, which names the game's files for each part of the game
  and the port's code that serves it.
- **A screen is a process.** Each screen or mode is a `GameProcFunctions` table of init, main and exit functions
  (`include/gfl/proc.h`), in the overlay that holds it, started with `GCTX_ProcMgrQueueProc(overlay, &FUNCS, param)`.
  `grep -rn "GameProcFunctions" src` lists them; the game's first is `EVENT_TITLE_FUNCS` in `src/system/boot_proc.c`,
  which plays the boot screens, the opening and the title over and over.
- **Names come from the ROM where it has them.** Many files pass their own name to an allocation or an assert, so the
  overlays embed strings such as `"startmenu.c"`, and [source files](../../docs/source-files.md) gives each file's
  evidence. `config/<version>/arm9/**/symbols.txt` maps names to addresses and back; a name made of an address,
  such as `func_0204f980`, marks a function nobody has named yet.
- **Run the game to the point you care about** and look: `--screenshot PATH` writes both screens when the run ends,
  `--run-dir DIR --run-dump LABEL=SYMBOL:SIZE@FRAMES` writes a global's memory at chosen frames, and a run that reaches
  code with no C stops with that function's name, address and caller.

## Changing the game's code here

This repository keeps both ROMs building byte for byte from the same tree, so its rules for changes outside `port/` are
strict ([../CLAUDE.md](../CLAUDE.md#changes-outside-port-planmd-41-48), [game-changes.md](game-changes.md)). Only three
kinds of change land:

1. **Neutral:** a portability fix that compiles to the same bytes under MWCC, such as `uptr` for a value that holds an
   address.
2. **Hook:** a hook macro that compiles to nothing, or to the bare expression, in the ROM build (`PORT_EVENT`,
   `PORT_SHOULD`). None exists yet; the first comes with the mod platform.
3. **Platform:** an `#ifdef PLATFORM_PC` block, with a comment saying why. `PLATFORM_PC` is defined only by the host
   build, so the ROM build never sees the block.

Each needs proof that MWCC's output didn't change: `python3 configure.py && ninja` with both SHA1s for complete files,
`port/tools/check_mwcc_neutral.py` for a change that only adds `#ifdef PLATFORM_PC` branches, and the decomp's
`tools/decomp/compiler_probe.py` for anything else in an incomplete file. Each gets a row in
[game-changes.md](game-changes.md) in the same commit. Decomp files are never copied into `port/` to change them.

**Bugs stay the original's.** `BUGFIX`, the decomp's switch for its `// BUG:` fixes, stays off, so the game plays as on
the DS and can be compared with it. The exception is memory safety: an out-of-bounds write or a use after free, which a
64-bit layout turns into a different, worse bug, is fixed under `PLATFORM_PC` with a comment.

**For an experiment of your own**, any change to `src/` builds into Frigate: change the C, `cmake --build build/port`,
and run. Such a change usually breaks the ROM build's SHA1s, so it can't land in this repository, and a change to a
struct that is saved changes the save; until the mod platform exists, that is how to try an idea.

## Adding a program option

Options are read by the module that needs them, not passed down from `main()`:

1. Add the name to `sOptions` in [../app/main.c](../app/main.c), and to `sFlags` as well if it never takes a value.
   The program refuses any option not in the list (status 2).
2. Read it where it is used with `port_app_option("name")` ([../hal/os/port_app.h](../hal/os/port_app.h)): `NULL` when
   it isn't given, `""` for `--name` alone, else its value. Any module of the platform layer, the renderers or the
   program can call it; [../hal/gfl/heap_policy.c](../hal/gfl/heap_policy.c) is a short example.
3. Document it in the usage block at the top of [../app/main.c](../app/main.c) and in `Usage()` (what `--help`
   prints), and in [../README.md](../README.md) if players use it.
4. Test it: a test that runs the program with the option, in the module's fragment in [../cmake/](../cmake/README.md).

A settings file, and menus to change settings in the program, come with the mod platform.

## Testing a change

- **The tests:** `ctest --test-dir build/port -j4 --output-on-failure` runs every test the build has
  ([../tests/README.md](../tests/README.md)); the ones that need the player's ROM skip without it.
- **Replays:** an input file (`--input FILE`, the `pbwinput 1` text format in
  [../tools/oracle/README.md](../tools/oracle/README.md#input-files)) gives the game the same buttons and touches at the
  same moments every run. With `--fixed-time` (or the file's own `time` line) a run is deterministic, since nothing in
  the game reads the host's time: the same frames and the same save every time, which `replay_same_save` checks. The
  replays in [../tests/replay](../tests/replay) go to the start menu and through New Game's intro.
- **The oracle:** melonDS running the same ROM with the same input file is the reference. Configure with
  `-DPORT_BUILD_ORACLE=ON`, and `port/tools/oracle/oracle.py` runs both programs and `compare.py` compares their
  screens and memory frame by frame ([../tools/oracle/README.md](../tools/oracle/README.md)). It tells a change that
  should keep the game as it was from one that didn't.

## What the mod platform will add

[PLAN.md §5](PLAN.md#5-mod-architecture) designs it, and phase 6 of the roadmap
([PLAN.md §7](PLAN.md#phase-6-enhancement-and-mod-platform)) builds it, after the phases that make the whole game run.
**None of it is built yet.** In short:

- **Layered files:** the player's ROM at the bottom, then Frigate's own fixes, then mods in the player's order, then a
  folder that reloads while the game runs. Overrides per file, per NARC member, per text message and per sound; a
  CTRMap-CE project's files mount as they are, and a patched ROM can be turned into a mod folder.
- **Data registries:** the tables compiled into the game's code (TMs, move tutors, the type chart, the starters' cries)
  exposed by their decomp names, so mods change entries without changing code.
- **Hooks in four tiers:** wrapping the game's own tables (processes, script commands, battle event handlers), typed
  events at chosen places (`PORT_EVENT`), switches for vanilla behaviour (`PORT_SHOULD`, as in Ship of Harkinian), and
  later function-level hooks. The macros compile away in the ROM build.
- **Settings and menus:** typed settings saved as TOML, with Dear ImGui menus for them, for mods and for debugging.
- **Lua 5.4** scripting, sandboxed per mod, with bindings generated from the decomp's headers.
- **Mod packages:** a folder or zip with a `manifest.json` and its data, files, text, textures, audio and scripts.
- **Saves:** the DS's save stays as it is; Frigate's and mods' own data go in a sidecar file.

Three features will be built on the same platform:

- **Widescreen** ([PLAN.md §5.2](PLAN.md#52-widescreen)): the 3D scenes widened through the projection and the field's
  camera, with more of the map streamed in, 2D kept centred, field first, then battles. It needs the GPU renderers.
- **Randomizer** ([PLAN.md §5.3](PLAN.md#53-randomizer)): built in, configured by settings, writing to the registries
  and the file layers, with its seed in the save's sidecar.
- **Multiplayer** ([PLAN.md §5.4](PLAN.md#54-multiplayer), phase 7): the game's own wireless protocol carried over the
  network, then the broadcast features, infrared as a pairing code, and Nintendo Wi-Fi Connection through revival
  servers once overlay 11 is C.

Until then, the [plan's decisions](PLAN.md#3-decisions) and [STATUS.md](STATUS.md) say where things stand.
