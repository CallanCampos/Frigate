# 0012: Frigate's test launcher for the field menu's apps while overlay 36 is claimed

- **Status:** accepted; superseded: retired when the field opened (2026-10-24)
- **Date:** 2026-10-18
- **Plan:** §7 phase 4 (bag, party, Pokédex, summary), §6 (T4); port/CLAUDE.md, "Code that isn't C yet"
- **Upstream:** `045e97b90` (merged by the port's `5ad9a1e1d`)
- **Note (2026-10-18):** accepted at the launcher track's merge into `port-integrate` (`859e4c1cf`, then `d7849a158`):
  the two documents "On acceptance" names now name the launcher, and its "Until then" no longer holds. Retiring the
  launcher also removes what names it elsewhere: its rows in the app, cmake, hal, include and tests guides,
  ARCHITECTURE.md's feature map and layers, GLOSSARY.md's test launcher, MODDING.md's "Reaching the field menu's
  apps", port/CLAUDE.md's and PLAN.md's sentences, the oracle README's launch checks, `oracle.py`'s `launch` and
  `port --launch`, `check_launch_smoke.py`, and the run directory's `launch:` line.
- **Note (2026-10-18):** the Pokédex's trap in the Consequences is gone since the port's `1f0c184f6` (upstream's C of
  `NNSi_objSizeHTbl` and `NNSi_objSizeWTbl`): `party_menus` launches the Pokédex, and it matches the oracle.
- **Note (2026-10-24):** upstream has since named the table and the function this record calls by address:
  `data_ov036_021d0054` is `FIELD_MENU_ITEMS` and `func_0203d564` `GCTX_HIDSetUsingTouch`.
- **Note (2026-10-24):** superseded: retired when the field opened. `FIELDMAP_PROC_FUNCS` got its C by upstream sync,
  and the field opened on the host with [ADR 0021](0021-root-heaps-in-the-games-own-memory-from-host-memory.md) (PR
  18), so the launcher stepped aside, as its gate meant it to, and its seven tests failed on `main` in both versions.
  The port's `a2fd007bf4` removed what the first note lists, with the `port_launcher` library and its link lines, the
  seam in `src/ov035/event_mapchange.c` and its row of game-changes.md, its declaration in `port/include/port_host.h`,
  `port_trap_is_placeholder`, `--launch`'s option and exit statuses, and its tests (`app_launcher_spec_*`,
  `integrate_launch_*`, `integrate_launcher_off`, `launch_*`, `oracle_launch_*`). Before it, `145d776f17` made the
  routes `field_menus` and `party_menus` comparison sequences, which reach the same apps through the field menu on
  both programs, the clock pinned on both, and `frame_times_party_menus` plays the party_menus file through the field
  menu. `launch_touch_then_party`'s touch on the trainer card's return button, the only touch an input file played,
  is in `field_menus` now, against melonDS.

## Context

Phase 4 asks for the party, the Pokédex, the bag and the summary. Each is an app proc whose C is complete (overlays
165, 302, 142 and 207, the trainer card's 186), and the game opens them from the field menu through overlay 12's app
call (`EventFieldAppCall`, `src/ov012/event_field_proclink.c`), which is C too. The menu itself, and the field it
belongs to, are overlay 36's: its process table, `FIELDMAP_PROC_FUNCS`, is a trap placeholder, so a continued game
stops when CONTINUE asks for the field (`EventFieldContinue_Callback`'s state 2, `src/ov035/event_mapchange.c`).
Overlay 36 is under another contributor's active claim upstream (issue #83, 2026-10-16), and port/CLAUDE.md leaves
work under a claim to its holder: the port waits, or uses a documented stand-in for a deferred subsystem, as
`port/hal/dwc/ov011_pending.c` is for overlay 11. Without a way past the field, none of the apps can run on the port,
and none can be compared with the oracle, though the oracle reaches each through the field menu.

Overlay 36 also holds about 170 other stubs on the field's load path (`g_FieldHeapMemory`, the subscreen, the field's
events), so writing its process table alone would not open the field.

## Decision

- **A test launcher, off by default.** `--launch APP[@COUNT][,APP[@COUNT]]...` (`port/app/launcher.c`,
  `launcher_spec.c`) names apps of the field menu (`party`, `pokedex`, `bag`, `card`, `summary`). When a continued
  game asks for the field, the launcher chains its own event in place of `EventFieldOpen` and opens each app in turn,
  when GFL's frame count reaches COUNT (at once without one), then asks the program to stop. Without `--launch`
  nothing changes: a continued game stops on the trap.
- **It stands in for three things, each through the game's own functions and setters**, with their effects read from
  overlay 36's C, never called:
  - **The menu's offer and its choice.** It opens only the apps the save's field menu offers: the kind of menu
    `FieldMenu_GetMenuType` gives (`src/ov036/field_menu.c:480-496`: the Union Room's, the Royal Unova's, a season's
    change, no Pokémon by event flag 0x961, no Pokédex by 0x962, the Black Tower's) from the same zone functions,
    event flags and `GameData` it asks, and the items its table gives each kind (`data_ov036_021d0054`; the Black
    Tower's has no BAG). Before each app it sets the UI's touch flag FALSE (`func_0203d564`), as the menu's choice of
    an item by a key does (`field_menu.c:709`, after `EventFieldMenu_Create`'s TRUE), whatever the last app left it
    at.
  - **The field's close.** Before each app it records the menu's subscreen as the last one
    (`GameData_SetLastSubscreen(gameData, 1)`), and once it unloads overlay 35 (`UnloadFieldGlueOverlay()`,
    `src/system/field_event.c:86`). The game records what `FieldSubscreen_Free` returns (`fieldmap.c:632`), overlay
    36's code; the launcher's 1 is a constant, the menu's subscreen (`event_field_menu.c:91` changes to it). The
    oracle shows both values at the bag's first frame: the UI's touch flag 0 (1 while the menu was open), the last
    subscreen 1 (0 before the menu).
  - **The app call's field-free states.** It fills the app call's input and work as `EventFieldMenu_Create` and
    `EventFieldAppCall_Create` fill them, but for the two calls that need the field (`PlayerActionPerms_Create`,
    `CalcPlayerActionPossibilities`): every field action is blocked and the possibilities are zero, so no field event
    can follow. It then runs `EventFieldAppCall`'s three field-free states (11: create the param and queue the proc;
    12: wait for it; 13: read the result, open the next app it asks for or free the param), one pass each, through
    the game's own table and functions. It reports each app's result and event type on stderr instead of opening the
    field again.
- **Its exit status says whether the launch finished** (`port/app/launcher.h`): 0 when every app opened and closed,
  each showing the menu again or closing it (event type 0 or 1); 1 when it didn't: the run ended first (`--frames`, a
  closed window), the game never asked for the field, the launcher stepped aside, or an app wanted the field (an app
  that runs in it, or event type 2, 3 or 5, which end the launch); 2 when the save's field menu doesn't offer an app,
  before anything opens. A trap or another stop keeps its own status.
- **The seam** is a guard-only `#ifdef PLATFORM_PC` block before state 2's two unchanged lines in
  `src/ov035/event_mapchange.c` (a Platform row of [game-changes.md](../game-changes.md)): MWCC sees the same tokens,
  which `check_mwcc_neutral.py` proves for every file in both versions. The state stays at 2: the launcher's event
  never ends, so the parent never reaches state 3, which would read a field that doesn't exist.
- **It steps aside by itself.** It acts only while `&FIELDMAP_PROC_FUNCS` is a trap placeholder
  (`port_trap_is_placeholder`, `port/hal/trap.c`); once the table has C, it prints a notice, the field opens as on the
  DS, and the launch tests fail on that notice, which forces its retirement. A rename of the symbol breaks the build,
  not the check.
- **It never calls or queues overlay 36's code or data**, and never defines `FIELDMAP_PROC_FUNCS`.
- **It is how the apps are tested until overlay 36 lands**: the oracle reaches each app through the field menu with an
  input file, and the port plays the same file with `--launch` at the oracle's counts (`oracle.py launch`).
- **On acceptance**, the status here becomes accepted, and two documents name the launcher: port/CLAUDE.md's "Code
  that isn't C yet", after `ov011_pending.c`, says that Frigate's test launcher (`port/app/launcher.c`, this ADR) is
  the other documented stand-in, test-only and off by default, opening the field menu's apps while overlay 36 is
  claimed; and PLAN.md's phase 4 says that until overlay 36 lands the field menu's apps are tested through it against
  the oracle's route through the field menu. Until then neither does, and the launcher's commits stay on a branch of
  their own, `port-launch-adr0012`, which isn't pushed.

## Alternatives

- **Writing or defining `FIELDMAP_PROC_FUNCS`, or a placeholder of it, in `port/`**: overlay 36's data under its
  holder's claim; it loads overlay 36, whose range covers every app overlay; and it makes a trap's "no C" message
  false.
- **Swapping the event from a frame hook between passes**: it needs `GameEvent`'s private layout (`game_event.c`) to
  recognise `EventFieldOpen`, whose callback is static.
- **Redirecting `EventFieldOpen_Create`** with a per-file `-D` or a linker alias: a hidden source transformation,
  which port/CLAUDE.md allows only for ADR 0004's headers; ld64 has no `--wrap` either.
- **A hook in the HAL's overlay loader**: it runs before `functions->init` is read, too late to stand in for the event.
- **A `PORT_SHOULD` hook macro**: the first one would fix the hook bus's API (a shared header, an ID namespace, an
  argument shape) that PLAN.md §5.1 leaves to phase 6, for a seam that goes when overlay 36 lands.
- **`GSYS_StartFieldmapProc`** as the seam: it runs inside `EventFieldOpen`'s state 0, which increments its state
  after the call (a use after free if the event were replaced), and would need a stand-in proc on top of the seam.
- **Waiting for overlay 36**: phase 4's apps would go untested against the oracle meanwhile, though their C is
  complete.

## Consequences

- The apps run natively and are compared with melonDS app screen by app screen (`oracle_launch_*`). As built, the bag,
  the trainer card and the party list with its Pokémon's menu show on the port pass for pass as on the oracle, from
  their first frame to their last; the summary's background is a step behind (it scrolls by VBlanks, which the DS
  spends loading it), and the Pokédex stops on a trap some 395 passes after it opens, still in its opening
  (`NNSi_objSizeWTbl`, ARM9 main's NitroSystem G2D, no C yet): only a Pokédex closed before then runs through. What
  the field leaves in VRAM, palettes, OAM, the random numbers and the clock before an app opens, the launcher can't
  give: such a difference is declared with its cause, never hidden by a changed hash.
- It stretches the exception: `ov011_pending.c` gives overlay 11's effect, while the launcher replaces a menu the
  player can't reach. It is a test tool, off by default, and the user decides whether it stays.
- **Retirement**, when the gate steps aside (the field opens): delete `port/app/launcher.c`, `launcher_spec.c`,
  `launcher.h`, `launcher_spec.h`, the `port_launcher` library and its link lines, the seam in `event_mapchange.c` and
  its row in `game-changes.md`, the declaration in `port/include/port_host.h`, `port_trap_is_placeholder` if nothing
  else uses it, `--launch` in `main.c` and the README, and the launcher's tests; turn the oracle's launch routes into
  plain sequences that walk the field menu on both programs.
- Risks: `FIELDMAP_PROC_FUNCS` could land at a sync before the rest of the field's load path; the launcher then steps
  aside, the run traps on the next stub, and the launch tests and `oracle_continue` fail until someone retires it, which
  blocks that sync's tests. Upstream edits to `event_mapchange.c` can conflict with the seam (the row's resolution
  rule applies).
