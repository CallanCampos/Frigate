# Port status

Where the port stands, newest entry first. A session adds an entry after each local sync with
`port/tools/sync_upstream.sh` and each finished task ([PLAN.md §8](PLAN.md#8-how-each-development-iteration-runs)), in
the same commit as the work it records; the hourly `port-sync` Action's merges get no entry of their own
([SYNC.md](SYNC.md)). The plan of record is [PLAN.md](PLAN.md); every change outside `port/` is in
[game-changes.md](game-changes.md).

## How to add an entry

Put the new entry above the newest one, under a heading `## YYYY-MM-DD: title`. Fill every field. A measure or test
level that doesn't exist yet says "not yet"; no field is left out.

| Field | What to write |
| --- | --- |
| Date | The day of the sync or task, `YYYY-MM-DD`, in the heading |
| Upstream | The `fuddlesworth/pokebw2` commit last merged, short SHA, with its PR number when it is a merge |
| Port | The port commit the entry describes. An entry committed with its own work can't name that commit: write `PORT_SHA`, and the next entry's commit replaces it with the SHA from `git log -1 --format=%h -- port/docs/STATUS.md` |
| ROM | Result of `python3 configure.py && ninja`: both SHA1s match, or what failed (T0) |
| Host build | Result per platform and configuration (T1) |
| Tests | Result of each level T2–T7 that ran, with counts; "not yet" for levels that don't exist |
| Tiers | Functions and code bytes in tiers (a), (b) and (c) from `port/tools/progress/`, and the change since the last entry |
| Trap stubs | How many functions the host build still stubs, and which the game hit first |
| 64-bit warnings | The host build's pointer/int warning count, game and `lib/` apart; it must not rise |
| Work done | What changed, with ADRs and game changes by number |
| Next | The next task: the earliest unmet exit criterion of the current phase that the decomp allows |
| Blockers | What waits on upstream or on a decision, with issue numbers |

## 2026-10-18: the card's infrared chip on the SPI registers: the genuine-card check passes as on melonDS

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `PORT_SHA`, on the branch `port-launch` from `5ad9a1e1d`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed, so no
  `check_mwcc_neutral.py` run and no row in `game-changes.md`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. Black 2: all 237 `ctest` tests pass, the oracle's included (`ctest -j4`, 419 s), 3 of them
  new: `hal_card_spi_plain` and `hal_card_spi_sanitized` (the model against melonDS 1.1's `NDSCartSlot` and
  `CartRetailIR` functions and GBATEK: the genuine check's two exchanges, receive's length 0, send, an unknown command,
  the slot or its SPI bus off, the deselect when bit 13 is cleared while held, the busy bit, a byte sent equal to the
  last answer, the low byte), and `rom_game_ir_genuine_card` (the game's own `IrCheck_IsGenuineCard` on the whole
  program's objects returns TRUE twice, as the check passes on melonDS). Mutation: without the two lines of
  `HW_REGISTER_POLICY`, `rom_game_ir_genuine_card` fails. T4 unchanged: `oracle_continue`, `oracle_opening`,
  `oracle_title` and `oracle_port_save` pass, as do `replay_new_game`, `replay_start_menu` and `replay_same_save`.
  By hand, Black 2 under lldb from the oracle's first save through CONTINUE to the field's trap: `func_0202d6a8` and
  `IrCheck_IsGenuineCard` run once, `GameBeaconSys_HideNewHBlank` never (it ran from the HBlank task list before), and
  the save is unchanged. White 2: the 16 `card_backup` tests, `hal_card_spi_*`, `rom_game_ir_genuine_card`,
  `smoke_reaches_game_main` and `replay_same_save` pass; `oracle.py sequence continue` 3,407 of 3,407 (1,312 exact,
  2,095 with 3D within the tolerance), as in the last entry. T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: this task changed no decomp C.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version, against the last entry's 574 and 63:
  the change is upstream's, merged by `5ad9a1e1d`, which had no entry. A continued game still stops on
  `FIELDMAP_PROC_FUNCS` (overlay 36).
- **64-bit warnings:** unchanged: no game file changed.
- **Work done:**
  - **The card's infrared chip** ([ADR 0011](adr/0011-cartridge-infrared-chip-as-a-register-model.md),
    `port/hal/card/card_spi.c`). AUXSPICNT and AUXSPIDATA were plain memory, so AUXSPIDATA read back the last byte
    written, 0, and the game's genuine-card check (`src/ov338/ir_check.c`) failed at every CONTINUE and NEW GAME:
    the game then left `GameBeaconSys_HideNewHBlank` in the HBlank task list for good, and would skip the experience
    of a faint in battle. The two registers are now accessors (`port_hw_auxspicnt`, `port_hw_auxspidata`) that model
    the bus and the chip as melonDS 1.1 does, so the game's own C runs and passes, and the infrared library
    (`lib/irc`, overlay 31) has a chip to talk to; PLAN.md §4.5's stub of the check is dropped. An accessor can't tell
    a load from a store, so each call resolves the store made through the memory the previous call returned:
    AUXSPIDATA's cell keeps a marker in its upper byte that a store clears, a documented deviation (a 16-bit read sees
    it where the hardware gives 0; every reader keeps the low byte).
  - `decomp-findings.md`: `ir_check.c` runs on the port, equivalent as observed; `buildSeed` hashes its digest buffer
    before writing it (stack bytes on the DS, zero on a host), so the boot's and each battle's random numbers can't
    match the oracle's.
- **Next:** the field menu's apps on the port while overlay 36 is claimed: the trainer card's param layout, saves with
  a party made by the oracle, and a test launcher for the apps (ADR 0012, proposed).
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) and overlay 12's script data, as in the last
  entry.

## 2026-10-17: saves, after review: the same save at every run, and the port's save checked on melonDS

- **Upstream:** `615939e25`, as in the last entry; no sync in this task.
- **Port:** `ca2b56b2e`, on the branch `port-saves`, after `3d86232c7`, merged into `main` with the gamepads' branch. The branch's two earlier commits were rebuilt
  before any of it was published, so that the first carries the oracle README's format evidence its links point to
  (`01b182f4b`, `3d86232c7`; their final tree is unchanged).
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). `check_mwcc_neutral.py`: 3,142 of 3,144 file and
  version pairs preprocess as upstream's `615939e25` does, `util_wifiidtool.c` among them; the other two are
  `src/system/vm.c`'s Neutral change already in `game-changes.md`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2, and Black 2 under ASan and UBSan. Not pushed, so
  no CI run.
- **Tests:** T0 as above. Black 2: all 229 `ctest` tests pass, the oracle's included (`ctest -j4`, 345 s), 6 of them
  new: `replay_same_save`, `card_backup_linked_save`, `card_backup_refuse_padded`, `card_backup_refuse_larger`,
  `card_backup_refuse_directory` and `oracle_port_save`; `oracle_compare` has 41 tests (2 new: `port-save` with
  stand-ins for both programs, and its skips; extended: the named stop, the blank `--save-out`, the declarations'
  `stop_trap`). T4: `oracle_continue` 3,415 of 3,415 frames (1,310 exact, 2,105 with 3D within the tolerance), stopping
  on `FIELDMAP_PROC_FUNCS` as `stop_trap` requires; `oracle_opening` 3,800 of 3,800 and `oracle_title` 3,579 of 3,579,
  as before. T6: `oracle_port_save`, 5,400 of 5,400 frames the same, neither save written, the Wi-Fi ID the oracle's,
  the saves 10,712 bytes apart; `replay_same_save`; the card tests. White 2: the 16 `card_backup` tests and
  `replay_same_save` pass; `oracle.py sequence continue` 3,407 of 3,407 (1,312 and 2,095) and `oracle.py port-save`
  5,400 of 5,400, the saves 10,720 bytes apart, run directly. Sanitized (ASan and UBSan, Black 2): the 16 `card_backup`
  tests with no report, `replay_same_save`, and the program on the first save through CONTINUE to its trap: no ASan
  error, from UBSan only the known misaligned read (`g2d_CellAnimation.c:33`), its 3,436 frames' counts and screens
  those of the plain build, the save untouched. Mutations: `replay_same_save` and `oracle.py port-save` fail on the old
  DWC line (another ID at each launch, `e70c0000f3a0b385` and `e7080000f2a0b385`), `card_backup_linked_save` without the
  link's resolution, `card_backup_other_saves` with the old name rule. T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: the one game change is a branch for hosts in an incomplete file.
- **Trap stubs:** unchanged, 574 function stubs and 63 data placeholders in each version; a continued game still stops
  on `FIELDMAP_PROC_FUNCS` (overlay 36), which `oracle_continue` now requires by name.
- **64-bit warnings:** unchanged: the game change is a `PLATFORM_PC` branch, which the compile probe doesn't define.
- **Work done** (the two reviews of the last entry's work):
  - **The port's saves differed at every run.** DWC seeds the console's pending Wi-Fi ID with the clock plus the
    address of `OS_GetTick` (`util_wifiidtool.c`'s `BUG`), a constant on the DS and on a host wherever ASLR loaded the
    program, and New Game copies the ID into both copies of the save. The host build now adds the DS's address
    (`PORT_DS_OS_GETTICK_ADDRESS`, `0x0207bb0d` and `0x0207bb39` with the Thumb bit, from the ROM's literal pools):
    a game change of the Platform kind, row in `game-changes.md`. The port's New Game now writes the same save at every
    run, with the oracle's Wi-Fi ID (`280c0000f2a0b315` in Black 2, `69080000f2a0b3ff` in White 2): ctest
    `replay_same_save` runs it twice and holds the ID to the oracle's, and fails on the old code.
  - **The port to melonDS is a test now**, where the last entry's T6 claim rested on a run by hand: `oracle.py
    port-save`, ctest `oracle_port_save`, boots the port's New Game save on the oracle against the oracle's own save of
    that moment, frame for frame, checks that neither is written and that the Wi-Fi ID is the oracle's.
  - **The two formatted saves' difference, traced** (`port/tools/oracle/README.md`, "Saves"): 10,712 bytes in Black 2
    (10,720 in White 2), the same at every run, all from when each program set the blocks up: three blocks encrypted
    with a key seeded by the VBlank count, DWC's player ID from `OS_GetLowEntropyData`, the day care's egg seed, five
    small fields, and the CRCs over them. The last entry's 10,718 was one run's.
  - **The card layer** (`port/hal/card/backup.c`): a save path that is a symbolic link stays one (the image goes to the
    file it links to, so a link to melonDS's `.sav` keeps one save for both programs); a padded image of this chip
    and a directory are refused with what they are; a size is said to be another chip's save's size, not that save;
    the hint for a melonDS save under another name leaves out only this game's saves as the port names them
    (`IRE?.sav`, `IRD?.sav`), so a `6149.sav` is named. The `.dsv` check is DeSmuME's own (`src/mc.cpp`), and its test
    builds the footer as DeSmuME writes it.
  - **`oracle.py`:** a sequence that allows a stop names its trap (`stop_trap`), so `oracle_continue` passes only on
    overlay 36's `FIELDMAP_PROC_FUNCS`; `port --save-out` never leaves an earlier run's file (the blank chip when the
    game wrote nothing). White 2's known press frame in the continue sequence is described from the oracle's own
    frames: the cursor's pulse step the port shows there is one the DS shows at counts 3274, 3296, 3338 and 3360.
  - **The tests' save directories:** `smoke_reaches_game_main` and `integrate_boot_logos` ran the program without
    `--save-dir`, so with the player's own save directory; they have one in the build tree now.
  - **D7 corrected:** the save keeps the console's clock offset, MAC and birthday, which CONTINUE checks
    (`EventFieldContinue_Create`, which the port reaches before its trap) and overwrites with the console's. Saves from
    the port, the oracle and melonDS's generated firmware move as they are (offset 0), but one from a DS whose clock
    was set takes the changed clock's path on the port, and back. Taking a save's console on import, with the
    console's Wi-Fi ID kept between launches, is the sidecar's first job; not built.
- **Next:** as in the last entry: a save with an adventure once overlay 36 lets the port into the field; then the
  import of a save's console (D7), DeSmuME's `.dsv`, and a save with the Pokédex for the start menu's last line.
- **Blockers:** overlay 36 (issue #83) and overlay 12's script data, as in the last entry.

## 2026-10-17: saves with melonDS, and CONTINUE from the oracle's first save

- **Upstream:** `615939e25`, which `main` merged before this task (`e1b68f578`); no sync in this task.
- **Port:** `3d86232c7`, on the branch `port-saves` from `f52dad0b2`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed, so no
  `check_mwcc_neutral.py` run and no row in `game-changes.md`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 (everything) and White 2 (the program and the backup tests),
  and Black 2 under ASan and UBSan (the program and the backup tests). Not pushed, so no CI run.
- **Tests:** T0 as above. T2: Black 2, 219 of 219 `ctest` tests pass with the four long oracle tests left out
  (`oracle_boot`, `oracle_opening`, `oracle_title`, `oracle_continue`, run apart), 223 in all; new or extended:
  `card_backup_round_trip`, `card_backup_other_saves`, `card_backup_refuse_eeprom`, `card_backup_refuse_dsv` and
  `card_backup_bad_file`'s message, and `oracle_compare`'s 39 tests (4 new: the port's save directory with `--save`
  and `--save-out`, runs a sequence lets stop on a trap, saves held to their SHA-1, the save command's skips; and the
  sequences' declarations, saves included). White 2: the
  12 `card_backup` tests pass. Sanitized: the 12 `card_backup` tests, and the program on the first save through
  CONTINUE to its trap: no ASan error, from UBSan only the known misaligned read (`g2d_CellAnimation.c:33`), every
  frame's count and screens those of the plain build, the save file untouched. T6, for the first time: a save of the
  4 Mbit flash round-trips through the card layer byte for byte (unit), melonDS's save continues in the port and the
  port's own file boots in melonDS (below). T4: `oracle.py sequence continue`, the port against melonDS 1.1 on the
  game's count, from power-on with the first save to the port's trap after CONTINUE:
  - Black 2: 3,415 of 3,415 compared frames match (1,310 exact, 2,105 with 3D within the tolerance), the title's 21
    known frames and oracle frame 4920 left out; frames 3,244-3,436, the start menu with the saved game, CONTINUE and
    the wipe out to the field's black, all exact;
  - White 2: 3,407 of 3,407 (1,312 and 2,095), the title's 28 known frames, port frame 3402 (the cursor's blink at
    the press, a step apart since the cry's lag) and oracle frames 1260 and 4852 left out.

  T3, T5's nightly runs, T7 not yet.
- **Tiers:** not measured: this task changed no decomp C.
- **Trap stubs:** 574 function stubs and 63 data placeholders in each version. A continued game stops after port
  frame 3,436, in the black after the start menu, on `FIELDMAP_PROC_FUNCS` (overlay 36's .data, `0x021c9eb8` in Black
  2, `0x021c9eec` in White 2): overlay 36 gates CONTINUE, before overlay 12's script tables, which still stop New Game.
- **64-bit warnings:** unchanged (no game C changed).
- **Work done:**
  - The save format, settled with evidence (`port/tools/oracle/README.md`, "Saves"): the port's file and melonDS's
    `.sav` of BW2 are both the 4 Mbit flash's 524,288 bytes with nothing before or after; melonDS 1.1 gives `IREO`
    and `IRDO` save type 6, a 512 KiB flash, and reads and writes the `.sav` whole, padding or cutting another size
    without a word. Only the names differ (the ROM's file for melonDS, the game code for the port). D7's sidecar isn't
    needed yet: the port keeps nothing of its own, and the one thing a save would need from it, the console's clock
    offset, is 0 on the port, the oracle and melonDS's generated firmware alike.
  - The card layer (`port/hal/card/backup.c`): a file of another size is refused with what it seems to be (another
    chip's save by melonDS's sizes, DeSmuME's `.dsv` by its footer), and a `.sav` of the chip's size under another
    name in the save directory, such as a melonDS save copied in as it was, is named with how to play it. The save's
    names and both directions in `port/README.md`, "Saves".
  - The oracle makes saves: `oracle.py save first_save` plays `port/tests/oracle/first_save_<version>.pbwinput`, New
    Game through the intro into the field, the scripts to the player's room and the field menu's SAVE (18,800
    frames), and checks the save against the SHA-1 `SAVES` keeps; nothing the game wrote is committed. `oracle.py run`
    and `port` take `--save` and `--save-out`; a sequence can start from a save and allow a trap stub from a frame on.
  - The continue sequence (`continue_<version>.pbwinput`, ctest `oracle_continue`): the title's presses from the first
    save and A on CONTINUE.
  - The port's own save to melonDS: the file the game wrote when New Game formatted the blank backup on the port boots
    in melonDS to the same 5,400 frames as melonDS's own file of that moment. Lessons in `port/docs/lessons.md`.
- **Next:** a save with an adventure made by the port, once overlay 36 lets it into the field (phase 4's exit: catch,
  save, quit, reload; a melonDS save continues in the port and back); import of DeSmuME's `.dsv` and the clock offset
  a save brings, with D7's sidecar, when a save from another console needs it.
- **Blockers:** overlay 36 (issue #83) stops CONTINUE at the field's process table and gates the overworld; overlay
  12's script data stops New Game at the field's first script.
## 2026-10-17: gamepads, and the macOS app in a disk image

- **Upstream:** `615939e25`, as merged into the port's `main` by `e1b68f578`.
- **Port:** `3ad982e8c`, on the branch `port-controller-pkg` from `f52dad0b2` (merged into `main`, with fixes in `df3a77ccd`, `cfd1cf911` and `4a0fa7d89`): the work in
  `78758202d` and `c424bf45c`, and the fixes from two reviews of it in `01240f928` and `31814aaf2`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `ninja` has no work after the commits. Nothing
  outside `port/` changed but `.github/workflows/port-ci.yml`, a port file, so no game-changes row and no
  `check_mwcc_neutral.py` run.
- **Host build:** arm64 macOS, at `31814aaf2`: Black 2 RelWithDebInfo and its app, Black 2 under ASan and UBSan (the
  alignment check recoverable), White 2 RelWithDebInfo and its app; not pushed, so no CI run (Linux unbuilt).
- **Tests:** T0 as above. Every result below is from `31814aaf2` with nothing uncommitted (the apps say version
  `0.0.2544`, revision `31814aaf2f46`); the proofs this entry first gave for the launch through LaunchServices, the disk
  image and all of White 2 were from apps built before the work was committed (`0.0.2540` and `0.0.2541`, `-dirty`), and
  are replaced.
  - T2: Black 2, 220 of 223 `ctest` tests pass and 3 are skipped without the oracle (`oracle_boot`, `oracle_opening`,
    `oracle_title`); White 2, 218 of 221 pass and the same 3 are skipped, `replay_start_menu` and `rom_game_overlays`
    among the passes (the last entry had them failing in White 2). New: `app_gamepad_plain` and `app_gamepad_sanitized`
    (424 checks each: the stick's dead zone, sectors and hysteresis, the map by position and by label, the pad that
    drives, the description, and SDL's virtual joysticks as pads that come and go, a noisy desk pad and nine pads at
    once), and on macOS `app_make_app` (19 checks), `app_message_log` (11) and `app_package_macos_out` (13).
  - Sanitized (`build/port-asan`): the app, integration, smoke and `replay_start_menu` tests, 20 of 20, and a windowed
    run of 120 frames with SDL's dummy video and sound drivers, with no report.
  - T3: `replay_start_menu` and `replay_new_game` pass (among the 220).
  - The app, each version: `codesign --verify --deep --strict` passes (ad hoc, no team, no hardened runtime); its
    program, run from inside the app with `--headless --frames 300 --rom orig/baserom_<version>.nds`, loads the app's
    own SDL3 (`DYLD_PRINT_LIBRARIES`) and stops after 300 frames, with the plain program's picture at frame 300
    (`d66b7bf8`, the same picture in both versions), also from the mounted disk image. Black 2's, opened through
    LaunchServices (`open -n -W --env HOME=<scratch>`, headless, no `--rom`), finds the checkout's ROM, shows the same
    picture and writes its messages to `<scratch>/Library/Logs/pokebw2-native/pokeblack2.log`; `spctl --assess`
    rejects it, as Gatekeeper does any app that isn't notarized. The disk images verify (`hdiutil verify`) and hold
    the app and a link to `/Applications` only. `package_macos.sh` refuses an image path in another repository and
    takes one under an ignored directory or outside every work tree.
  - T4, T5's nightly runs, T6, T7 not yet.
- **Tiers** (`port/tools/progress`, upstream `615939e25`): (a) 32,231 functions, 2,881,954 code bytes (74.9%); (b)
  5,195, 619,954 bytes; (c) 3,592, 348,336 bytes (9.0%). Since the last entry that measured them ("the title screen and
  the start menu", `e484d5011`): (a) +138 functions and 22,912 bytes, from (b); (c) unchanged. The change is upstream's
  matching merged by `e1b68f578`; the entries since then called the tiers unchanged without measuring them.
- **Trap stubs:** 574 function stubs and 63 data placeholders (`gen_stubs`, each build above), down from the 581 and 67
  the last entry carried over unmeasured, by the same upstream C. Overlay 12's script data, which New Game stops on,
  are still placeholders; the two replays reach none.
- **64-bit warnings:** game / `lib/` on arm64 (`port/tools/compile-probe`, its arm64 host pass at `615939e25`):
  `-Wpointer-to-int-cast` 865 / 184, `-Wvoid-pointer-to-int-cast` 52 / 117, `-Wint-to-pointer-cast` 76 / 144,
  `-Wint-to-void-pointer-cast` 69 / 132. The game's `-Wvoid-pointer-to-int-cast` rose by 3 since the last measurement
  (49): `src/ov168/btlv_input.c` lines 3021, 3025 and 3029, `(u32)vram` in upstream's newly matched battle code, merged
  by `e1b68f578` before this branch. This branch changes no game or `lib/` code.
- **Work done:** phase 5's controller support and macOS packaging.
  - Gamepads through SDL3's gamepad API (`port/app/gamepad.c`,
    [ADR 0009](adr/0009-gamepads-by-position-one-pad-at-a-time.md)): every pad connected, hot-plugged and remapped
    (eight open at once, a ninth opened when a place frees); the DS's A, B, X and Y by position (a Nintendo pad's
    layout), L and R on the shoulders, Start and Select, the D-pad and the left stick as the D-pad with a circular dead
    zone of half its travel and hysteresis at the dead zone's edge and at the sectors' edges; one pad drives at a time;
    one table, `PortGamepadMap`, for a settings file to replace. Each pad names its DS buttons by SDL's labels when it
    connects. The keyboard works at the same time. The controls are in `port/app/present.c`'s header and
    port/README.md.
  - The macOS app ([ADR 0010](adr/0010-macos-app-as-a-signed-copy-of-the-program.md)): the target
    `pokeblack2_app` (`pokewhite2_app` for White 2; `-DPORT_MACOS_APP=ON` builds it every time) copies the program into
    `<program>.app` with the libraries it loads from outside macOS in `Contents/Frameworks`, however they are named
    (absolute, `@rpath`, `@loader_path`, `@executable_path`), and an Info.plist (a placeholder bundle identifier,
    `com.example.pokebw2-native.<program>`, the version from git, the minimum macOS from the binaries, high DPI, a game
    category with Game Mode); checks that the app loads nothing from outside macOS and itself, signs it ad hoc, and
    starts its program once. Opened from the Finder, the program writes its messages to
    `~/Library/Logs/pokebw2-native/<program>.log` and shows a missing or refused ROM in an alert
    (`port/app/message_log.c`, `port_app_fatal_notice`). `port/tools/package_macos.sh` makes the disk image, checks it,
    and refuses a path that git would track in any work tree. port/README.md says where the ROM goes, where the log is,
    and how to get past Gatekeeper ("Open Anyway"). CI builds the app on its macOS runner and uploads nothing.
  - The reviews' fixes, each with a test: make_app.sh had left a program that loads SDL3 by `@rpath` with no library and
    no search path, signed and verified but unable to start; the stick flickered at the dead zone's and the sectors'
    edges (30 presses in 60 frames of noise; now one); a ninth pad was dropped for good; a Finder launch lost every
    reason it stopped; `package_macos.sh` accepted an image path in another work tree.
- **Next:** as the last entry (overlay 12's field script data, upstream). For the app: a first-launch ROM picker; an app
  for older macOS (the pinned SDL3 built statically with a deployment target, not tried yet). The 3 new 64-bit
  warnings in `btlv_input.c` want a portability fix (`uptr`) once phase 4 reaches the battle screens.
- **Blockers:** unchanged: overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download
  Play) only matter from phase 7.

## 2026-10-17: oracle comparisons from power-on to the start menu

- **Upstream:** `e484d5011` (merge of PR #353), as in the last entry.
- **Port:** `f3f285b1c`, merged with the intro's branch into `main` (below), which supplies the libfx fix.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: all 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2, and Black 2 under ASan and UBSan; not pushed,
  so no CI run (Linux unbuilt: the program now links with `ENABLE_EXPORTS` and `CMAKE_DL_LIBS` there).
- **Tests:** T0 as above. T2: Black 2, 211 of 214 `ctest` tests pass and 3 are skipped without the oracle
  (`oracle_boot`, `oracle_opening`, `oracle_title`); White 2, 209 pass, the same 3 are skipped, and 2 fail as they did
  before this work (`replay_start_menu`, whose input file is Black 2's, and `rom_game_overlays`, on overlay 12's
  address). Among them:
  - the run directory's 93 checks, plain and sanitized (memory dumps added), and `oracle_compare`'s 35 tests;
  - the 3D renderer's 777 checks, 13 of them new on edge interpolation (they fail on the old interpolation);
  - `oracle_opening_hashes` (337 frames and 638 screens without 3D in each version) and `oracle_title_hashes` (400
    and 393 frames, 544 and 541 screens, 321 and 314 of the frames on the count).

  Sanitized: `app_run_dir`, `oracle_compare`, `replay_start_menu`, `g3d_ds3d`, `g3d_hal` and the smoke test
  pass; both sequences under ASan and UBSan (3,600 and 3,800 frames) give the plain build's hashes and
  counts at every frame and its `NNS_G3dGlb` where dumped, with the one misaligned read already known
  (`g2d_CellAnimation.c:33`, NitroSystem reading packed file data). T4: `oracle.py sequence`, the port against
  melonDS 1.1, every port frame on the game's count (the movies by their pictures in order, no oracle picture
  skipped) but the known differences, and NNS G3D's state on the count through the title:
  - opening (power-on to the title's first 274 frames, nothing pressed): Black 2, 3,800 of 3,800 frames match (3,526
    exact at 6 bits, 274 with 3D within the tolerance); White 2, the same, with the DS's torn first frame of the
    opening declared; `NNS_G3dGlb` equal at all 274 frames compared in each;
  - title (the opening skipped with START, the title's 3D on both screens, START to the start menu): Black 2, 3,579
    of 3,579 compared match (1,474 exact, 2,105 with 3D within the tolerance), 21 known differences left out; White 2,
    3,572 of 3,572 (1,477 and 2,095), 28 left out; `NNS_G3dGlb` equal at 2,470 of 2,470 frames in each, 15 left out;
  - 3D beyond the pixel tolerance: a median of 17 pixels a screen in Black 2's title and 7 in White 2's, at most 80 of
    the 245 allowed (before the rasterizer fix, a median of 79 and at most 172).

  T3, T5's nightly runs, T6, T7 not yet.
- **Tiers:** unchanged since the last entry (no decomp C changed).
- **Trap stubs:** unchanged, 581 function stubs and 67 data placeholders; neither sequence reaches one in either
  version.
- **64-bit warnings:** unchanged by this branch.
- **Work done:**
  - Phase 2's last exit criterion: oracle comparisons pass within tolerance, for both versions, in two sequences
    (`port/tests/oracle/<name>_<version>.pbwinput`: opening and title; `oracle.py sequence` and `sequence-check`;
    ctest `oracle_<name>` and `oracle_<name>_hashes`).
  - compare.py pairs on the game's count (`--align counter`), the movies by their pictures (`--clock`), with declared
    count lags (`--lag`), and fails any oracle picture no port frame shows (`--known-oracle` for the DS's torn
    frames); `--align sequence` alone, which only holds pictures and their order, passed a port that showed pictures
    late or held a still. The references keep the count each row expects; every known difference is declared, with
    its evidence in `port/tools/oracle/README.md`.
  - The program writes its own run directory for compare.py (`--run-dir`, `--run-screens`, `--run-dump`,
    `port/app/run_dir.c`): `frames.tsv` with GFL's frame count and the game frame at every frame hook, PNG files at
    the frames of a list, `memory.txt` with globals by name (`dlsym`), through a pointer with `*`. `oracle.py port`
    runs the port once.
  - Two 3D fixes. The decomp's `MTX_Concat33`, `MTX_Concat43`, `MTX_Concat44` and `MTX_MultVec43` read an argument
    after storing over it when the result replaces it, which the original never does, so on a host the title's camera
    looked elsewhere (`MAT3_RotationEulerZYX`). This branch had host versions in `port/hal/fx`; the intro's branch
    fixed the same functions in place, with `#ifdef PLATFORM_PC` blocks and an asm oracle test of the decomp's C, and
    the merge keeps that fix alone (a decomp finding). And the rasterizer takes values along an edge by line, as
    melonDS does, not along an X-major edge's run, which had put whole spans' colours a step apart.
  - Settled with evidence (README, "Game frames and video frames" and "Known differences"): START held doesn't skip
    the opening on melonDS or on the port (a press does, on the same pass on both); the oracle's start-up gives
    oracle frame = port frame + 242 from count 5 on through the boot screens, and game frames part where the game
    waits on a clock (from the black after the copyright screen) and line up again at a press; the DS's camera leads
    after START (NNS G3D's state a pass ahead), and White 2's cry ends a pass later against the DS's passes because
    the DS starts it later (traced by the sound driver's shared work on both programs).
- **Next:** overlay 12's field script data (`g_ActiveFieldScriptSubEvents` and the script tables), which New Game
  stops on after the intro, as decomp work upstream; the White 2 failures of `replay_start_menu` and
  `rom_game_overlays`; `--screen-hashes` folded into `--run-dir`.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7.
## 2026-10-17: the intro as the DS draws it: libfx's products in place, and a replay checked against the oracle

- **Upstream:** `e484d5011` (merge of PR #353), as in the last entry; no sync in this task.
- **Port:** `e3d170b17`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). `check_mwcc_neutral.py`: this task's three game
  files preprocess identically to the last entry's commit in both versions; over every file, 3,138 of 3,140 file and
  version pairs give upstream `e484d5011`'s token stream, the other two being `src/system/vm.c`'s cast from the last
  entry, which is complete.
- **Host build:** arm64 macOS, RelWithDebInfo, and the sanitized build (ASan and UBSan, alignment recoverable). CI not
  run: the work is on a branch, not pushed.
- **Tests:** T0 as above. T2: 210 `ctest` tests, 209 pass and `oracle_boot` is skipped (no oracle build in this
  worktree). New: `fx_libfx`, libfx's products and transforms into their own inputs and GFL's Euler rotation, by hand;
  the asm oracle, which now runs here, with `test_fx_decomp.py`: the decomp's libfx C against the ROM's code, 88,500
  cases in each version, apart and in place; `integrate_screens` checks the screens' hashes. T3: `replay_new_game` is
  the first replay with goldens: it must show nine screens, from the start menu through the intro to the opening's
  season banner, exactly as the DS does by the oracle's hashes, and run 10,600 frames; it fails on a run that never
  takes NEW GAME (1 of 9) and on the code before this entry's fix (5 of 9). T4, by hand (pbw-oracle, melonDS 1.1,
  `new_game.pbwinput` on both): the DS plays the same file through the intro and the opening into the field's first
  script. From START to the port's trap, 3,378 of the port's 6,856 frames show exactly a DS frame's two screens, in
  order, MCSS's Juniper and Minccino included. In the others a sprite's animation meets a line of text at another moment
  than on the DS (the two reach each scene at another point between presses), or the 3D flight shows. That follows the
  DS's camera frame for frame, 346 frames later, and differs from it in 0.1 to 0.7 % of the top screen's pixels by more
  than 2 steps, at polygon edges (the 3D renderer, not the game). The title screen's 3D Kyurem, about 35,000 of its
  49,152 pixels off at 5 bits a channel before the fix, differs in 500 to 1,000 now. T5: `new_game.pbwinput` in the
  sanitized build to its trap: no ASan error; from UBSan only the two misaligned NitroSystem reads already known
  (`g2d_CellAnimation.c:33`, `g2d_MultiCellAnimation.c:119`); every frame's screens identical to the release build's. T6
  and T7 not yet.
- **Tiers:** unchanged (no upstream merge, no decomp C added).
- **Trap stubs:** unchanged, 574 function stubs and 65 data placeholders. `new_game.pbwinput` now reaches the trap
  on `g_ActiveFieldScriptSubEvents` (overlay 12) at video frame 10,756.
- **64-bit warnings:** unchanged: this task's game changes are inside `#ifdef PLATFORM_PC`, which the compile probe
  doesn't define.
- **Work done:**
  - The decomp's `MTX_Concat33`, `MTX_Concat43`, `MTX_Concat44` and `MTX_MultVec43` read their first input after storing
    part of the result, where the ROM loads each row (the vector) first, so the game's products into their own input
    were wrong on hosts: the intro drew Juniper and Minccino mirrored and in the wrong places (`MCSSSys_Draw`), the
    title's Kyurem from another angle, and every other product into its own input (GFL's Euler rotations, SPL's
    particles, NitroSystem's SBC) came out wrong. Platform blocks route those products through `tmp` and read an
    in-place vector from a copy (`port/docs/game-changes.md`, `port/docs/decomp-findings.md`). The rest of libfx's C is
    safe in place: the scalings, `MTX_MultVec33` and the vector arithmetic by the asm oracle, the inverses (through
    `tmp` in both) and `VEC_Normalize` by reading.
  - `--screen-hashes PATH` writes every frame's screens and GFL's frame count in the oracle's `frames.tsv` format,
    with the platform layer's SHA-1; `compare.py align` reads it. `port/tests/replay/check_screens.py` checks a run
    against a table of the oracle's screen hashes (`new_game.screens.tsv`).
  - `new_game.pbwinput` rewritten so that its presses land the same way on the DS: START at video frame 3,900, then A
    twice a second from 4,400 to 11,000. The 200-pass gap between START and the start menu that the review saw was
    the input, not the game: keyed to game frames, START reached the DS after its title screen had restarted its
    animation, since the DS's game count runs about 2,500 passes behind through the logo movie, where it drops frames.
    With START on a video frame both take 155 passes to the start menu. The DS then falls behind at each load, 70 to
    346 frames by the opening, all of it frames it drops and presses it waits for.
  - [ADR 0008](adr/0008-particle-work-from-the-platform-layer.md) records the last entry's particle work design. The
    last entry's four intermediate commits were not built one by one when made; the review built the host and ran
    `replay_start_menu` and the integration tests at each, and their game changes are token-identical under MWCC but
    for `vm.c`, which is the same at each as at the end.
  - Lessons in `port/docs/lessons.md`.
- **Next:** as in the last entry: overlay 12's field script data, which is decomp work (issue #73 has no active
  claim), then overlay 36. The 3D renderer's edge differences in the opening's flight are worth a look when phase 3's
  oracle comparison closes.
- **Blockers:** as in the last entry: overlay 12's script data stops New Game at the field's first script; overlay 36
  (issue #83, claimed by another contributor) gates the overworld after it.

## 2026-10-17: New Game's intro natively, to the field's first script

- **Upstream:** `e484d5011` (merge of PR #353), as in the last entry; no sync in this task.
- **Port:** `cbec17cb7`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: 3,138 of 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`. The other two are `src/system/vm.c` in each
  version, which differs by one cast and is complete, so the SHA1s prove it.
- **Host build:** arm64 macOS, RelWithDebInfo, and a sanitized build (ASan and UBSan, alignment recoverable). CI not
  run: the work is on a branch, not pushed.
- **Tests:** T0 as above. T2: 209 `ctest` tests, 207 pass and 2 are skipped (`asm_oracle` and `oracle_boot`, which need
  the oracle build), among them the new `replay_new_game`, which plays an input file from power-on through NEW GAME and
  the intro to video frame 12,000. T5, by hand: the same replay in the sanitized build, to frame 12,100: no ASan error,
  and from UBSan only the two misaligned reads already known, NitroSystem reading a 2-byte element of a file through a
  larger struct (`g2d_CellAnimation.c:33`, `g2d_MultiCellAnimation.c:119`). T3 (frame hashes), T4, T6 and T7 not yet.
- **Tiers:** unchanged since the last entry (no upstream merge, no decomp C added).
- **Trap stubs:** 574 function stubs (355 in overlay 36, 113 in overlay 11) and 65 data placeholders: DS Protect's seven
  functions and two data objects are `port/hal/stubs/dsprot.c` now. New Game stops just after video frame 12,060 of
  `new_game.pbwinput`, on `g_ActiveFieldScriptSubEvents` (overlay 12's .bss), which `ScriptWork_Create` reads as the
  opening starts the field's first script. Overlay 12's script tables have no C either: `GLOBAL_SCRIPT_TABLE`,
  `EVCMD_TABLE`, `EVCMD_PERM_TABLE`, `EVCMD_MAX`, `VM_CMP_LUT`, `FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS`. Until this entry
  no placeholder was protected on macOS: ld64 placed them all before the paddings that were to bracket them, so the
  first run here read `GLOBAL_SCRIPT_TABLE` as zeros and hung in an assert's error screen.
- **64-bit warnings:** game / `lib/` on arm64 unchanged: `-Wpointer-to-int-cast` 865 / 184, `-Wvoid-pointer-to-int-cast`
  49 / 117, `-Wint-to-pointer-cast` 76 / 144, `-Wint-to-void-pointer-cast` 69 / 132. The probe preprocesses without
  `PLATFORM_PC`, where `uptr` is `u32`, so this task's `uptr` fixes don't show in it.
- **Work done:**
  - New Game plays natively from the start menu through the whole intro (overlay 294): Professor Juniper and Minccino
    with the intro's particle effects, the player's sex, the name entries for the player and the rival, then the
    opening's season banner and its 3D flight over Aspertia City, which fades out at the player's house.
  - The particle systems' work comes from the platform layer, twice the size the 13 creating files give for the DS's
    structs (`port/hal/gfl/particle_work.c`).
  - Pointers carried in 32-bit values on the way, with each API's siblings: the Pokémon parameter setters (47 call sites
    in 23 files), NitroSystem's animation callback parameter with ClAct's and MCSS's (14 callbacks), the sound preload
    handle, the script VM's stack and loaded file; the opening's raw LCDC VRAM address as the SDK's constant.
  - DS Protect as a genuine cartridge: `port/hal/stubs/dsprot.c`, and the game's own checks of DS Protect's code skipped
    on hosts. `EventData_Reset`'s placeholder pointer, past an array, worked out without undefined behaviour.
  - Trap stubs: the data placeholders' range is worked out from where the linker put each one, so every one faults.
    `--screenshot-every N` writes the screens every N frames, for playtests.
  - Game changes, all proven neutral, in `port/docs/game-changes.md`; the lessons in a new `port/docs/lessons.md`.
- **Next:** overlay 12's field script data (the tables above and the .bss), which is decomp work (issue #73 has no
  active claim) and lets the opening run the field's first script; then overlay 36. Meanwhile, phase 2's oracle
  comparison, and the intro's. The same 32-bit pointer shape waits further on: `JoinAvenue_GetParam` and
  `JoinAvenuePerson_SetParam`, `BmpMenuList_GetParam`, and `NameEntryParam`'s `unkC` and `unk10` (`pokemonNameEntry`).
- **Blockers:** overlay 12's script data stops New Game at the field's first script; overlay 36 (issue #83, claimed by
  another contributor) gates the overworld after it.

## 2026-10-17: the title screen and the start menu, natively

- **Upstream:** `e484d5011` (merge of PR #353), which brings the ITCM's ClAct renderers (PR #352, decomp work this
  session did to unblock the title screen).
- **Port:** `570db24f4`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: all 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`.
- **Host build:** arm64 macOS, RelWithDebInfo and unoptimized (`-fgnu89-inline` made the unoptimized build link); CI
  green on macOS and Linux for every commit pushed.
- **Tests:** T0 as above. T2: 207 of 207 `ctest` tests pass, among them the 3D engine's (1,400 hand-derived checks,
  plain and sanitized), the sound driver's 31 and its two ROM scenarios (all 1,390 sequences), the G2D loaders' ROM
  equivalence (every one of the 30,078 G2D files of each ROM against a simulation of the DS's unpack), display capture,
  and `replay_start_menu`, which plays an input file from power-on to the start menu. T4: the melonDS oracle builds on
  request (`-DPORT_BUILD_ORACLE=ON`); a comparison of the boot sequence before this entry's renderer fix matched 33 of 33
  sampled frames at 5 bits per channel, and 11 of 11 at 6 bits in a scratch build with this entry's fix. T3, T5-T7 not yet.
- **Tiers:** (a) 32,093 functions, 2,859,042 code bytes (74.3%); (b) 5,333, 642,866 bytes; (c) 3,592, 348,336 bytes
  (9.0%). Since the research measurement: (a) +209 functions and 26,318 bytes, (c) -3,024 bytes, the ITCM renderers.
- **Trap stubs:** 581 function stubs (355 in overlay 36, 113 in overlay 11) and 67 data placeholders. The game runs
  4,400 frames to the title screen, and through START to the start menu, without reaching one.
- **64-bit warnings:** game / `lib/` on arm64: `-Wpointer-to-int-cast` 865 / 184, `-Wvoid-pointer-to-int-cast` 49 /
  117, `-Wint-to-pointer-cast` 76 / 144, `-Wint-to-void-pointer-cast` 69 / 132 (`port/tools/compile-probe`); as in the
  research measurement but for three void-pointer casts in new upstream C.
- **Work done:**
  - Phase 1's exit, the boot logos in the repository build, and most of phase 2: the boot screens, the Game Freak logo,
    the opening movie and the title screen with its 3D Black Kyurem play as on the DS, and the start menu responds.
  - NitroSystem's 2D files in host structs with a registry tied to heap frees
    ([ADR 0007](adr/0007-nitrosystem-files-in-host-structs.md)); the DS's geometry engine and a CPU 3D renderer;
    display capture; the ARM7's sound driver in host C with SDL3 audio output and `--audio-dump`
    ([ADR 0006](adr/0006-sound-driver-as-built.md)); the melonDS test oracle (`port/tools/oracle`); `--input` replays
    in the oracle's input format, counting game frames by GFL's own frame count.
  - Game changes, all proven neutral (`port/docs/game-changes.md`): the sound archive's file table, wave archive links
    and the sound heap's data as 32-bit handles or `uptr`; GFL's allocation head padded to 32 bytes on hosts. NNS FND's
    heaps are aligned for 64-bit structs by the platform layer: a strict ASan and UBSan build runs the 4,400 frames with
    two misaligned reads left, both NitroSystem reading packed file data.
  - The 2D renderer follows melonDS 1.1 in ignoring a colour's bit 15; the overlay-sections check accepts a program
    whose `__eh_frame` ld64 dropped.
- **Next:** phase 2's oracle comparison through the start menu with one input file for both programs, then New Game's
  intro (overlay 294): 64-bit fixes for the particle system's work buffers and for the Pokémon parameter setters,
  which take pointers as `u32`.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7.

## 2026-10-17: phase 1 platform layer, and the game changes it needs

- **Upstream:** `4aa041311` (merge of PR #348); unchanged.
- **Port:** `e49c96554`.
- **ROM:** both SHA1s match; `check_mwcc_neutral.py`: all 3,138 file and version pairs give the same MWCC token stream
  as upstream's `4aa041311`.
- **Host build:** arm64 macOS; CI checks macOS and Linux.
- **Tests:** T0 as above. T2: 144 of 144 `ctest` tests pass (OS scheduler and interrupt scenarios, the 2D renderer's
  16,318 pixel checks, PXI services, ROM and card, overlay data sections, VRAM banks, DMA, integration, the asm
  oracle, the smoke test, which now runs the first frame). T3–T7 not yet.
- **Tiers:** not re-measured (upstream unchanged).
- **Trap stubs:** 587 function stubs and 69 data placeholders.
- **64-bit warnings:** not yet counted by the host build.
- **Work done:**
  - The phase 1 platform layer ([ADR 0005](adr/0005-platform-boundary-as-built.md)): the OS on fibers with the
    decomp's own scheduler and a virtual clock (`port/hal/os`); the PXI bus and the ARM7's services (power, touch,
    firmware settings, reset, cartridge slot, mic, a sound driver); the ROM from the player's dump, the backup chip as an
    atomic save file, and resident overlays whose statics reset on load (`port/hal/rom`, `card`, `fs`); VRAM banks
    aliased by page mapping (`port/hal/gx`); DMA done at once (`port/hal/dma`); the CPU 2D renderer (`port/gfx/ref`),
    screens, input and an SDL window (`port/gfx/screens.c`, `port/app`, `port/hal/input`), `--screenshot`.
  - Game changes, all proven neutral (`port/docs/game-changes.md`): `uptr`, `sptr` and `ulong32`, and the sites that
    needed them (NNS FND heaps, `HW_DTCM`, GX register-address parameters, the sound heap, two alignment computations,
    the RTC date and the misc save block); the heap size hook.
  - `port/hal/gfl/heap_policy.c` (root heaps x4, child heaps x2, `--heap-scale-root`/`--heap-scale-child`);
    `port/hal/dwc/ov011_pending.c`, the four overlay 11 symbols DWC's boot-time init reaches before upstream's claim
    writes them; `port/docs/decomp-findings.md`.
  - The generator routes `reg_CP_DIV_RESULT` (a plain `u64` register) through its accessor; before, every `FX_Div`
    returned 0. Crashes outside the trap pages now print their address and a backtrace, on their own signal stack.
  - In an exploration tree with the remaining fix (below) the game played the boot screens and the whole Game Freak
    logo movie for 1,100 frames, deterministically, and reached the title screen.
- **Next:** NitroSystem G2D's in-place relocation (PLAN.md §4.8 rule 5), in progress as a workflow: a relocation
  registry tied to heap frees and host unpackers for every G2D format, proven on every such file in the ROM. Then the
  boot logos in the repository build (phase 1's exit), then phase 2.
- **Blockers:** the repository build crashes in the boot screens (`src/ov162/boot_screens.c`, the first character
  data), on the G2D relocation above. Overlay 36 (issue #83) still gates phase 3.

## 2026-10-17: phase 0 complete; DS addresses, asm replacements and 64-bit layouts

- **Upstream:** `4aa041311` (merge of PR #348); upstream hasn't moved since this session's sync.
- **Port:** `d63819fc5`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`) after a full `ninja` with this commit's
  `include/types.h` change; `check_mwcc_neutral.py`, now comparing token streams: all 3,138 file and version pairs give the same MWCC token stream as upstream's `4aa041311`.
- **Host build:** arm64 macOS and x86-64 Linux build in CI (`port-ci.yml` green), which closes phase 0's exit criteria
  (links, starts, opens a window, reaches the game's `main()`, both ROMs match, CI green).
- **Tests:** T0 as above. T2: 4 of 4 `ctest` tests pass: the smoke test, C unit tests of the host MI and FX/MATH/RC4
  functions, and `asm_oracle`, which runs the ROM's own machine code for each replaced asm function in a test-only CPU
  emulator and compares it with the host C on thousands of seeded inputs, both versions (it skips without the ROM
  extract, as in CI). T3–T7 not yet.
- **Tiers** (`port/tools/progress`, `b1369e48f`): (a) 31,884 functions (77.7%), 2,832,724 bytes (73.6%); (b) 5,538
  (13.5%), 666,160 bytes (17.3%); (c) 3,596 (8.8%), 351,360 bytes (9.1%), unchanged since 2026-10-16 (overlay 36's C
  hasn't landed upstream yet).
- **Trap stubs:** 664 function stubs and 72 data placeholders; the game stops first at `OS_Init`.
- **64-bit warnings:** not yet counted by the host build.
- **Work done:**
  - DS addresses through build-time header translation ([ADR 0004](adr/0004-ds-addresses-by-header-translation.md)):
    static 16 MiB windows per memory-map region in `port/hal/hw`, accessors for the registers whose accesses act
    (geometry command log, bit-exact divider and square root, VCOUNT, timers, GXSTAT, matrix readbacks), and
    `port_hw_g3_send` for SDK copies to the geometry FIFO.
  - Host C for 38 SDK asm functions (MI copies, fills, compare, swap and LZ decompression; FX matrices; MATH CLZ,
    QSort, SHA-1 block; RC4), each proven against the ROM's code by `port/tests/asm_oracle`.
  - `include/types.h`: `u64`, `s64` and `f64` keep the DS's 4-byte alignment on hosts, so structs keep the DS's
    layout (a bag sort entry stays 12 bytes, so `MATH_QSort` orders equal keys as on the DS; save blocks).
  - The three phase 0 spikes kept in `port/spikes/` (overlay data sections, VRAM aliasing, fibers); the host build
    compiles `port/hal` against the decomp's headers with the game's ABI flags, `port/gfx` as its own target, and
    `port/cmake/*.cmake` fragments.
- **Next:** phase 1, running as a workflow: the OS core on fibers, the PXI bus and host ARM7 services, the ROM, file
  system, saves and overlay data resets, VRAM banks, and the CPU 2D renderer, then integration toward the boot logos.
  Later: fast paths for the byte-wise MI copies (the game's most-called functions), under the same differential tests.
- **Blockers:** none for phase 1. Overlay 36 (issue #83) still gates phase 3.

## 2026-10-17: phase 0, the native build links and enters the game's main()

- **Upstream:** `4aa041311` (merge of PR #348), merged in `0f65946be` by this session's sync, which built both ROMs and
  checked both SHA1s.
- **Port:** `577554b0b`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`) after a full `ninja` with this commit's guards;
  `port/tools/check_mwcc_neutral.py`: all 3,138 file and version pairs preprocess under MWCC exactly as upstream's
  `4aa041311` does.
- **Host build:** arm64 macOS, Apple clang, RelWithDebInfo: 1,480 of 1,518 C files of `src/` and `lib/` compile
  natively (38 are replaced by the platform layer, `port/hal/replaced.txt`) and link into `pokeblack2` (10 MB) with
  SDL3 3.4.18. Linux and Windows not yet; CI (`port-ci.yml`) builds macOS and Linux from this commit on.
- **Tests:** T0 as above; T1: builds; T2: the smoke test (`ctest`) passes: the program reaches the game's `main()`.
  T3–T7 not yet.
- **Tiers:** not re-measured since 2026-10-16.
- **Trap stubs:** 692 function stubs and 72 data placeholders. The game stops first at `OS_Init` (called by
  `SystemInit`), the first SDK module the platform layer replaces.
- **64-bit warnings:** not yet counted by the host build (silenced there); the compile probe baseline of the
  2026-10-16 entry stands.
- **Work done:**
  - `port/CMakeLists.txt`: the host build, with MWCC's language model (signed char, 16-bit `wchar_t`, wrapping
    arithmetic, no strict aliasing, zeroed locals) and SDL3 (Homebrew's, or the pinned release).
  - `port/tools/gen_host_tree.py`: the source list, an `include/` mirror without MSL's C library headers, the generated
    constants. `port/tools/gen_stubs.py`: a trap stub for each DS function the C references and nothing defines, and a
    placeholder on a page with no access for each such data object, so the first use is reported by name.
  - `port/hal/trap.c`, `port/hal/os/memory_map.c`, `port/hal/stubs/unresolved.c`, `port/include/port_host.h`,
    `port/app/main.c` (SDL3 window, the game on its own thread, `--headless`).
  - Game changes (`port/docs/game-changes.md`): `OVERLAY_ID(n)` is `n` on a host; `MATH_CountLeadingZeros` uses the
    compiler's builtin; the 18 MWCC `asm` functions of seven mostly-C NitroSDK files are behind `#ifndef PLATFORM_PC`.
  - `port/tools/check_mwcc_neutral.py`, which proves a guard-only change neutral for every file at once.
  - `.github/workflows/port-ci.yml`: the host build and tests on macOS and Linux.
- **Next:** the phase 0 spikes (overlay data sections, VRAM aliasing, fibers), then phase 1: the platform layer in the
  order the traps ask for it, starting with `OS_Init`, threads, IRQs, arenas, MI and FX.
- **Blockers:** none for phase 0 or 1.

## 2026-10-17: solo project, ad-hoc signing and upstream guards

- **Upstream:** `462e90193` (merge of PR #345), still the last merged. Upstream `main` is now `69e82e80c` (merge of
  PR #347), four commits ahead, and not yet merged by this commit; the next sync merges it.
- **Port:** `5b1102e5d`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); this commit changes nothing outside `port/` and
  `.github/`.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Tiers:** not re-measured; no upstream commit has been merged since the 2026-10-16 entry.
- **Trap stubs:** not yet.
- **64-bit warnings:** not yet from a host build; the compile probe baseline in the 2026-10-16 entry stands.
- **Work done:**
  - Two decisions by the user: the port is a solo project, so nothing about it is posted on `fuddlesworth/pokebw2`
    and its portability changes stay here (D10, [ADR 0002](adr/0002-solo-project-no-upstream-posts.md), which amends
    [ADR 0001](adr/0001-repository-and-upstream-sync.md)); and there is no Apple Developer account, so macOS builds
    are ad-hoc signed and never notarized (D11, [ADR 0003](adr/0003-ad-hoc-signing-no-apple-developer-account.md)).
  - The reference guard: whatever the repository's visibility, `port/tools/sync_upstream.sh` stops with exit code 5
    and the `port-sync` Action with the `references` outcome before pushing commits whose messages reference issues;
    `--allow-references` pushes them once the user agrees. The sync also checks strictly that `origin` is
    `CallanCampos/pokebw2-native`.
  - The write guard: `port/tools/hooks/port_guard.py`, a PreToolUse hook that `port/tools/bootstrap_checkout.sh`
    installs in the clone's `.claude/settings.local.json`, which Claude Code also reads in the clone's worktrees. The
    same script sets the `upstream` remote's push URL to `DISABLED-port-never-pushes-upstream`.
  - `port/CLAUDE.md`: `gh` writes only to the port, no @-mentions of upstream's people, port commit messages name
    only the port's own issues, and the root `CLAUDE.md`'s overlay claim and progress
    rules and the `decomp-file`, `finish-file` and `match-function` skills never run in the port clone. The plan,
    ADRs, landing page, game-change ledger and research index say the same. The scripts in
    `port/tools/research/sdk-surface/` take the repository as an argument, or find it from their own location,
    instead of a hardcoded worktree path, and write their outputs to `build/port-research/sdk-surface/`, which git
    ignores.
  - No branch of the decomp checkout holds the port commit `0835ef06c` any more, so no decomp session there can push
    it upstream. The `port-sync` Action was paused while these guards were written and is re-enabled after this push.
- **Next:** the session sync with `port/tools/sync_upstream.sh`, merging `69e82e80c` (none of its four new commits
  references an issue), then phase 0: CMake skeleton with SDL3, generated source lists and trap stubs,
  `include/platform.h`, the overlay `.data`/`.bss` spike, then the first arm64 macOS link.
- **Blockers:** none for phase 0. Overlay 36 (issue #83, claimed by mariano54) still gates phase 3. The four ITCM
  ClAct renderers (phase 2) have no owner; writing them is ordinary decomp work for a decomp session, which the user
  starts. D10, which the 2026-10-16 entry listed as waiting, is decided.

## 2026-10-17: local clone and first syncs

- **Upstream:** `462e90193` (merge of PR #345); unchanged since the repository was created.
- **Port:** `ecb9e5f65`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`), from the first run of
  `port/tools/sync_upstream.sh` in the new clone, which configured from scratch (downloading wibo, sjiswrap and dsd
  v0.12.1-dsi.4) and built both ROMs. 22 compiler warnings, the same upstream ov036 ones as before.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Tiers:** not re-measured; upstream hasn't moved since the 2026-10-16 entry.
- **Trap stubs:** not yet.
- **64-bit warnings:** not yet from a host build; the compile probe baseline in the 2026-10-16 entry stands.
- **Work done:**
  - The public repository `CallanCampos/pokebw2-native` exists with upstream's full history and the port commit
    `0835ef06c`. Upstream's `build.yml` is disabled here (it needs upstream's ROM deploy key); `compile.yml` and
    `standards.yml` pass on `main`.
  - The `port-sync` Action ran once by hand and reported upstream already merged. Its merge path runs at the next
    upstream push.
  - The local clone `~/dev/decomp/pokebw2-native` is set up: `origin` is the port, `upstream` is the decomp, gh's
    default repository is the port, `port/tools/bootstrap_checkout.sh` linked the inputs from the decomp checkout,
    and `CLAUDE.local.md` (`@port/CLAUDE.md`) with `.worktreeinclude` loads the port rules in every session.
  - `port/tools/sync_upstream.sh` ran end to end on the real build: fetch, already-merged check, configure, ninja,
    both SHA1s, nothing to push.
- **Next:** phase 0 — CMake skeleton with SDL3, generated source lists and trap stubs, `include/platform.h`, the
  overlay `.data`/`.bss` spike, then the first arm64 macOS link.
- **Blockers:** none for phase 0. Overlay 36 (issue #83, claimed by mariano54) still gates phase 3.

## 2026-10-16: repository created, research and plan complete

- **Upstream:** `462e90193` (merge of PR #345). The repository was created from upstream's full history at this
  commit.
- **Port:** `0835ef06c`.
- **ROM:** both SHA1s match (Black 2 `e51e6dfb…`, White 2 `b5d7490b…`), full `ninja` on `462e90193` with `port/`
  present. 22 MWCC "function has no prototype" warnings, all in upstream's new `src/ov036/field_g3dobj.c` and
  `src/ov036/fld_wipe_3dobj.c`, none from the port.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Trap stubs:** not yet (no generator). The functions they will cover, those with no C, numbered 3,596 on
  `0c2bbee9d`.
- **64-bit warnings:** not yet from a host build. The compile probe's baseline on arm64 macOS (clang 23.1.3,
  `0c2bbee9d`): 1,164 unique pointer/int truncation sites in game code (`src/` and `include/`), 700 of them the
  `OVERLAY_ID(n)` macro, leaving 464 sites in 154 of 1,170 game files; 678 sites in 117 `lib/` files.

### Tiers

Measured on `0c2bbee9d`, three upstream merges (PRs #343–#345) before the seed commit, Black 2; White 2 has the same
tiers function for function ([decomp-progress.md](research/decomp-progress.md)). The next sync re-measures.

| Tier | Functions | Code bytes |
| --- | ---: | ---: |
| (a) complete file, matching C, linked | 31,068 (75.7%) | 2,751,648 (71.5%) |
| (b) C written, file incomplete, ROM code still linked | 6,354 (15.5%) | 747,236 (19.4%) |
| (c) no C at all | 3,596 (8.8%) | 351,360 (9.1%) |
| C exists, (a) + (b) | 37,422 (91.2%) | 3,498,884 (90.9%) |

Totals: 41,018 ARM9 functions and 3,850,244 code bytes, without 2,384 linker veneers. Of tier (b), 432 functions
(145 KB) have a row in `docs/nonmatching-functions.md`. About 84% of initialized data is C; 3,707 of 13,063 pointer
words in data are not (2,789 in overlay 36, 674 in overlay 12's event-script command tables).

When tier (c) reaches zero, at each area's own recent rate:

| Area | Left (bytes) | Rate, last 7 days | ETA | Rate, last 3 days | ETA | Who |
| --- | ---: | ---: | --- | ---: | --- | --- |
| ov036 field | 110,808 | 12.2 KB/day | ~Oct 25 | 27.1 KB/day | ~Oct 20 | mariano54 (claim; re-landing a fork branch) |
| ov011 Wi-Fi stack | 143,146 | 10.3 KB/day | ~Oct 30 | 16.4 KB/day | ~Oct 25 | mariano54 (claim; same fork) |
| ov182 Wi-Fi utility | 79,342 | 3.0 KB/day | ~Nov 11 | 2.9 KB/day | ~Nov 12 | mariano54 (claim) |
| LTD (TwlSDK, DSi) | 11,164 | lumpy | days | | | no issue tracks it |
| ITCM (ClAct renderers, IRQ) | 4,788 | 200 B in 20 days | none | | | no issue tracks it |
| main (crt0, SVC stubs, `exit`) | 2,112 | flat | none | | | no issue tracks it |

Overlays 36 and 11 could be C within one to two weeks at their recent rates. The rest of tier (c) only by
mid-November, if overlay 182 keeps its pace and someone takes the untracked ITCM, crt0 and DSi code. These are
extrapolations from a 20-day-old project whose progress comes in large merges.

### Compile probe

From [RESEARCH.md §9](RESEARCH.md#9-the-native-c-compile-probe): clang accepts all 1,170 `src/` files for 32-bit
targets, and for arm64 macOS once `OVERLAY_ID(n)` becomes `n` on hosts. Every remaining failure is in `lib/`: 43 files
with MWCC `asm` and 6 with dialect issues. i386 SysV matches MWCC's layouts exactly; save blocks are pointer-free and
keep the DS layout on 64-bit hosts with `aligned(4)` typedefs for `s64`/`u64`/`f64` and a typedef for two
`unsigned long` fields. The 64-bit work is estimated at 500–700 edited lines in about 160 game files.

### Work done

- Repository created from upstream's history; upstream's `build.yml`, which needs a ROM deploy key this repository
  doesn't have, disabled here with `gh workflow disable build.yml -R CallanCampos/pokebw2-native`.
- Research ([RESEARCH.md](RESEARCH.md) and 11 reports) and the plan ([PLAN.md](PLAN.md)).
- Measurement tools in `port/tools/` (progress tracker, compile probe, research scripts).
- Port rules (`port/CLAUDE.md`), the landing page (`.github/README.md`), `port/README.md`, the hourly sync Action
  (`.github/workflows/port-sync.yml`), the local sync and bootstrap scripts, the sync guide ([SYNC.md](SYNC.md)),
  [ADR 0001](adr/0001-repository-and-upstream-sync.md), this log, and the game-change ledger (empty).

### Current phase: 0, foundations

Exit criteria ([PLAN.md §7](PLAN.md#phase-0-foundations)): the executable links on macOS, starts, opens a window and
reaches `main()`; both ROMs still match; CI green.

### Next

1. CI for the host build, and the progress tracker and compile probe wired in as metrics for this log.
2. CMake skeleton with SDL3 in `port/third_party/`.
3. `port/tools/gen_sources.py`: source lists generated from `config/`, minus `hal/replaced.txt`; trap stubs; and
   placeholder definitions for data symbols with no C (zero-filled or loaded from the ROM at startup), so the host
   build links.
4. `include/platform.h` with `PLATFORM_PC`, `uptr`/`sptr` and the host typedefs; `OVERLAY_ID(n)` as `n` on hosts.
   These are game changes: each gets a row in [game-changes.md](game-changes.md).
5. Compile every game and library C file for arm64 macOS and fix 64-bit sites far enough to link: the 9 incompatible
   function-pointer casts, the 5 missing returns and the save-struct typedefs first.
6. Spikes: overlay `.data`/`.bss` sections on Mach-O, VRAM page aliasing on macOS, fibers with DS priority
   scheduling. Each ends in an ADR.

### Blockers

- None for phase 0: code and data with no C get trap stubs and placeholders (Next, item 3).
- Phase 3 (the overworld) needs overlay 36's C and data: issue #83, claimed by mariano54 on 2026-10-16 and being
  re-landed from a fork. Overlay 12's event-script command tables (674 pointer words) also need to become C.
- Phase 2 needs the four ITCM ClAct renderers as C; no issue tracks them and nobody owns them. Opening an issue for
  them is a public post that needs the user's OK, and writing them is decomp work for a decomp session in the decomp
  checkout, never for a port session.
- Decision D10 (proposing the portability conventions upstream) waits on the user's OK.
