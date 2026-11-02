<!-- Records how Frigate gets mid-game starting points held to melonDS without replaying the story: stations, saves
minted from organic oracle saves by text recipes, placed through the game's own continue-at-spawn path, continued by
both programs and compared as the sequences and the round trips compare them; and the map and trainer sweeps to be
built on them. -->
# 0032: Stations: saves minted from oracle saves, placed by the game's own spawn, held to melonDS

- **Status:** proposed
- **Date:** 2026-11-02
- **Plan:** §6 (verification: T3, T4 the oracle, T5) and §7 (phase 5)
- **Upstream:** `41f42ca1af`, as Frigate's `main` (`e936696a97`) has it
- **Note (2026-11-02):** drafted from the save lab's design of 2026-11-01 and its prototype, both outside the
  repository. PR 71 (L1) committed the minter and reader (`port/tools/lab/savelab.py`), and the pull request that
  proposes this ADR (L2) the stations and the first one, Floccesy Ranch's gate, in both games. It stays proposed until
  the owner confirms the defaults that [Consequences](#consequences) marks as the owner's to confirm; the change that
  records that sets the status line.

## Context

- **Phase 5's story coverage stops at the Route 20 save.** That is `floccesy_save`, zone 446 at `(130,661)`. The
  next leg starts with Terrell's mandatory trainer battle, whose AI the DS's CPU time decides.
  [ADR 0023](0023-oracle-routes-part-where-the-ds-cpu-time-decides.md), accepted on 2026-11-01 (PR 70), parts a route
  at the first decision that time moves and goes on from melonDS's save made after it; past the Route 20 save, no
  sequence holds the story yet.
- **Replay is expensive.** Each organic save is made on melonDS from the one before. `SAVES`, in
  `port/tools/oracle/oracle.py`, chains six saves over 126,300 oracle frames a game. A story sequence costs about
  30 min per game, and no test boots most of the game's 615 zones.
- **Other ports start mid-game without replaying the story.**
  - AKCore/pokeplatinum-port mints saves from recipes and sweeps every map and trainer class: `pc/tests/pc_lab.py`,
    `pc_sweep.py` and `pc_trainers.py` at `3357155cef`.
  - khdays-port walks copies of saves: `regress.py --walk-save` at `b3965470b2`.
  - 2 Ship 2 Harkinian boots to named warp points: `WarpPoint.cpp` at `222acd182a`.
- **The game can already start anywhere.** `FieldSetNextZone` writes `PlayerSave.nextSpawn`
  (`src/ov012/scrcmd_game_state.c:258-270`), and scripts set `EVENT_FLAG_CONTINUE_SCRIPT` before a save. The Hall of
  Fame does this to return the player to their room (`res/field/scripts/global_9600.s:270-271`). On CONTINUE,
  `EventFieldContinue_Create` reads and clears that flag (`src/ov035/event_mapchange.c:381-382`). Its callback then:
  - deletes the actors restored from the save;
  - loads the spawn's zone;
  - runs `FieldMapControl_InitSpawn` (`:305-316`, `:1398-1447`), the same routine every door uses (`:508-524`).

  That routine resolves a warp id through the destination's own warp table (`SetupZoneWarpArrival`,
  `src/ov012/eventdata_system.c:180-204`), falls back to the zone's Fly position (`event_mapchange.c:1390-1396`), and
  spawns the zone's NPCs from its events and flags.
- **What is committed.** The minter and reader came with PR 71: `place ZONE_FLOCCESY_RANCH_2 warp 0` changes 38 bytes
  of the Route 20 save, 19 in each copy with the CRCs. The pull request that proposes this ADR adds the first station,
  Floccesy Ranch's gate, held in both games:
  - each version's Route 20 save mints to a pinned SHA-1 (Black 2 `72e7ab2a3fa0`, White 2 `d82663c39d7c`), whose
    recipe reads back from it;
  - on both programs the start menu names Floccesy Ranch, and CONTINUE opens the ranch's field at the gate, the player
    facing up;
  - Frigate's frames from power-on to the field menu's first press are melonDS's, 4,269 of 4,269 compared in Black 2
    and 4,261 of 4,261 in White 2, with NNS G3D's and GFL's random state, but the title's and CONTINUE's differences,
    each traced, at the frames the floccesy_continue sequence declares for the same presses;
  - both programs SAVE at the gate, and their saves read back there and differ only in the play time's seconds and the
    CRCs over them, as the Route 20 save's round trip does
    ([the oracle's README](../../tools/oracle/README.md#floccesy-ranchs-gate-floccesy_ranch)).
- **The design's sweep prototype, measured outside the repository on Black 2:** a save minted per zone booted all 615
  zones, port-alone, in about 32 min wall on 3–5 workers; 611 were `ok`. The other four were three link rooms that stop
  on the watchdog, and one zone whose load ended Frigate with status 1 and no message, Castelia City's café
  (`ZONE_CASTELIA_CITY_22`), which loads since PR 73. No map load reached a trap stub.
- **Rewriting the saved place alone is wrong.** A normal CONTINUE restores the saved field actors and spawns none
  (`event_mapchange.c:317-331`; `GameData_LoadBackup`, `src/system/game_data.c`, from `:675`), so the old zone's NPCs
  would appear in the new map.

## Decision

1. **A station is a text recipe, an input file and a spec. A minted `.sav` is never committed.**
   - The recipe's verbs take numbers or the game's own constant names, resolved from the port's generated
     `build/include/generated/constants/*.h`, as `savelab.py` takes them:
     - `place ZONE warp N`, `place ZONE at X Z [Y] [facing DIR]`, `place ZONE fly`; `at` and `fly` are refused where
       a rail system or an empty Fly position needs a warp;
     - `flag`, `work`, `money`, `badges`, `playtime`;
     - later `item`, `party`, `dex` and `option`.
   - Its `base` is an oracle-made save: a `SAVES` name, or a station's own melonDS `--save-out`, each pinned by
     SHA-1. A port-made save never bases a station.
   - Minting is deterministic. The station pins the minted save's SHA-1 for each version, so a change to the minter
     or a recipe shows in review.
   - A station's files: its spec in `port/tools/lab/stations.py` (`STATIONS`), its recipe and an input file per
     version in `port/tests/stations/`.
2. **Placement uses the game's continue-at-spawn path, and nothing else.** `place` writes three things:
   - `PlayerSave.nextSpawn`, as a warp spawn (zone and warp id) or a position spawn: where CONTINUE goes and, while the
     flag is set, the place the start menu names (`src/ov162/startmenu.c:1117-1127`);
   - `EVENT_FLAG_CONTINUE_SCRIPT` set, with `EVENT_WORK_CONTINUE_SCRIPT` at 0;
   - `SaveLocation` as the arrival's player state: what `Continue_Exit` (`src/ov162/game_start.c:411-431`) and
     `GameData_LoadBackup` read before the spawn replaces the place, written so that the two agree. It is not what the
     start menu shows while the flag is set.

   No location-only mint is allowed, and no game or port code is changed for placement.
3. **The minter writes only the fields its verbs name, in unencrypted blocks.** It writes both copies of the main save
   alike. It recomputes each changed block's CRC in the block's footer and in the CRC table, and the footer's CRC of
   that table. Everything else stays as the base holds it:
   - the console data;
   - the blocks encrypted with VBlank-seeded keys;
   - the field actors;
   - the save counts.

   It refuses a base that fails `saveimage.validate_image`, and checks its own output with the same function. Pokémon
   are minted, when needed, by a Python codec of the game's format, proved by decoding and re-encoding every organic
   save's Pokémon byte for byte.
4. **A station is held at every level, each gating the next.** A failure at any level fails the station.
   1. The minted save's structure is checked, and its SHA-1 is the pinned one.
   2. The recipe's facts are read back from it, and every other fact is the base's.
   3. melonDS boots it to the recipe's place, without a damaged-save message.
   4. Frigate's run is held to melonDS's frame by frame, by the sequences' rules: known differences are only frames
      whose cause is traced.
   5. A station that ends with SAVE has its resulting saves checked:
      - **The facts:** each program's save is read back and its facts asserted (zone, tile, the continue flag
        cleared, the party, money, flags and work values the minted save's).
      - **The bytes:** the two saves are compared byte by byte, as `oracle.py round-trip` compares them, the mystery
        gift blocks decrypted; the facts are compared whatever the bytes declare.

   The tests: `oracle_station_<name>` holds the five levels (`oracle.py station`); `oracle_station_<name>_check` holds
   the port alone, levels 1, 2 and 5 on its own resulting save (`oracle.py station-check`); `lab_stations` holds the
   stations' specs and checks without a ROM. The two station tests carry the label `station`, with `route` and
   `oracle`.
5. **Stations form a breadth spine beside the organic story spine, and never replace it.**
   - Phase 5's exit criterion stays organic: story progression as replay routes, cut by ADR 0023 where the DS's CPU
     time decides.
   - A minted state may be one that play can't produce. The recipe says so, and the station is never cited as story
     evidence.
   - ADR 0023's cut applies inside stations: frames are held up to the first CPU-time decision, and the rest of the
     run is port-alone.
   - A station continues from another only through melonDS's resulting save.
   - When the organic spine reaches a station's place, the station is re-based on the organic save or retired.
   - They run on demand, never on a schedule. A pull request that changes the lab or a station's own files runs them
     (T4); otherwise someone runs them when they ask (`ctest -L station`), as the sequences are run. They are not a
     requirement of every pull request whose code a station runs through: Frigate runs the whole game.
6. **Sweeps are stations generated from the warp graph and the trainer data.**
   - The map sweep places one arrival per zone. Arrivals come from the inverted warps of `res/field/events/*.json`,
     and the Fly position is used where no door leads.
   - It runs one port process per zone, in parallel, headless. The sanitized build gives a sanitized variant, run on
     demand.
   - Each zone gets a verdict, read from its exit status and last message: `ok`, `trap`, `terminate`, `deadlock`,
     `watchdog`, `reset`, `crash`, a sanitizer report, `not-idle`, `misplaced` (the player's zone at the end isn't the
     recipe's), or `refused` with a written reason.
   - The verdict baselines are text files of verdicts and hashes. A row that moves from `ok` to a failure fails the
     sweep. A changed picture hash is reported but never fails it.
   - A deterministic oracle sample is held to melonDS as stations are.
   - The trainer sweep battles one trainer per class, for the first turn only. Frames are held only to the AI's
     first decision.
7. **Developer commands are host-only debug paths, if they are added.** These are a one-process `--dev-warp-list` and
   a `--dev-battle` that install the game's own map-change or trainer-battle event at a pass boundary on the game's
   main fiber. They are:
   - off by default;
   - named in `run.txt`;
   - refused with `--record` and with the oracle comparison;
   - never evidence of organic behaviour.

## Alternatives

- **Only replay the story:** keep the organic chain as the sole coverage.
  - It is the most faithful method. Nothing past a trainer battle can be covered until ADR 0023's saves or timing work
    reach it, and each new place costs a full replay leg.
  - It stays the exit criterion's method, but not the only coverage.
- **melonDS savestates as the port's starting points:** the port can't load an emulator's machine state. A save is the
  boundary both programs already share byte for byte. Savestates stay useful on the oracle side only, as checkpoints
  for bisecting a divergence.
- **A location-only mint:** rewriting `SaveLocation` and nothing else is wrong for any zone with NPCs, because a normal
  CONTINUE restores the previous zone's actors (Context).
- **Minting in the port with the game's own functions** (AKCore's `--lab`): exact by construction, but it needs port C
  at a safe point in a running game. A port bug would then mint a wrong save that both programs continue alike, caught
  only by read-back. It is kept for anything the Python codec can't express.
- **One process for the whole map sweep first** (AKCore's `--sweep`): one boot saves Frigate only about 1,560 frames
  per zone (2–4 s unloaded), and it needs a dev command and restart-on-crash logic. A process per zone needs no port C
  and isolates failures. One process is the later optimisation.
- **Commit minted saves:** binary, ROM-derived game data in the repository. A recipe regenerates the save
  deterministically, so a save can't drift from the code that made it.
- **Commit the oracle's hashes for each station, as the sequences' `_hashes`:** a station's base needs the oracle
  anyway, which is why the sequences from saves have no `_hashes` either (`port/cmake/oracle.cmake`). ADR 0031's
  cache, keyed by the start save, takes the minted save's SHA-1 instead.
- **The sweep's fast boot for every station** (CONTINUE at game frame 1,500 instead of 3,400): deferred. Its table of
  known differences is measured on Black 2 only, and L3 measures it in both games.

## Consequences

- **New files.** L2's: `port/tools/lab/stations.py`, `port/tests/stations/` (recipes and inputs) and
  `port/tests/lab/test_stations.py`, beside L1's `port/tools/lab/savelab.py` and `port/cmake/lab.cmake`. The sweep's
  come with L3.
- **New test names.** `lab_stations` (every build, no ROM); `oracle_station_<name>` and `oracle_station_<name>_check`
  (`route`, `oracle`, `station`); with L3, `sweep_maps` (`sweep`, `route`) and `sweep_maps_oracle_sample` (`sweep`,
  `oracle`).
- **Coverage past Route 20.** The Ranch, the Town Map, Herdier and Virbank get frame-by-frame melonDS coverage before
  the story's routes reach them, and every zone's load is checked when the sweep runs.
- **Same rules as today.** Each station's known differences need tracing, as each sequence's do. Stations and sweeps
  are per version.
- **Minter changes are visible.** A change to the minter moves pinned minted SHA-1s, and shows in review.
- **Revisit** if the organic spine reaches a station's place, or if a later decision models the DS's CPU time. Also
  revisit when ADR 0031's part 5 covers the sequences from saves: a station's oracle run is then cached by its minted
  SHA-1, and stations become port-alone checks by key.
- **The owner's to confirm**, written here as defaults that the save lab's pull requests follow until the owner says
  otherwise:
  1. *D1, the breadth spine:* stations are Phase 5 coverage beside the story's routes, never the exit criterion, never
     story evidence. (The owner's to confirm.)
  2. *D2:* the continue at a spawn is the only placement; no location-only mint, no game hook. (The owner's to
     confirm.)
  3. *D3, the sweep's tier:* on demand, with no scheduled run (the owner's rule of 2026-11-01): the map sweep runs
     port-alone when someone runs it; a pull request that changes the field's load path runs it for the zones it
     touches; its oracle sample runs on demand too; the stations' tests are on demand the same way
     (`ctest -L station`), run as T4 by a pull request that changes the lab or a station's files. (The owner's to
     confirm.)
  4. *D4:* Pokémon are minted by a Python codec of the game's format, proved on every organic save's Pokémon. The
     port-side `--lab` is only for what the codec can't express. (The owner's to confirm.)
  5. *D5:* developer commands are host-only and off by default, refused with `--record` and the oracle comparison,
     named in `run.txt`, and listed under developer options in `--help`. (The owner's to confirm.)
  6. *D6:* a sweep row going from `ok` to `trap`, `crash`, a sanitizer report or `watchdog` fails that run of the
     sweep, and is fixed or explained in STATUS.md. A changed hash is reported only. (The owner's to confirm.)
  7. *D7:* the sweep's verdict baselines (verdicts and hashes, no ROM bytes) are committed, as the sequences'
     references are. (The owner's to confirm.)
