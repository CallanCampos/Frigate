<!-- The save lab's guide: minting Black 2 and White 2 saves from a recipe, and reading saves back as facts. -->
# port/tools/lab/

The save lab makes and reads Black 2 and White 2 saves, for tests and for starting the game somewhere other than where
a save was made, with nothing in the game or in Frigate changed: a minted save is a save the game loads as its own, on
Frigate and on melonDS alike. It serves the game's saves (the main save's blocks, `include/save/save_control.h`) and,
through the game's own CONTINUE, the field's map changes. Nothing here is linked into Frigate, and no save is ever
committed: a recipe is text, and what it is minted from is a save made on the oracle or the player's own.

| File | What it does | Game functionality it serves | Run |
| --- | --- | --- | --- |
| [savelab.py](savelab.py) | `mint` applies a recipe in the game's constant names to a save and writes the result; `read` prints a save's facts in the same names, the party decoded | The saves: the player's place and the spawn CONTINUE takes (block 0x1c), the event flags and work values (0x2d), the play time (0x1b), the money and badges (0x34), the party (0x1a, read only) | by hand; its test `lab_savelab` ([../../tests/lab/test_savelab.py](../../tests/lab/test_savelab.py)) |

Planned beside it, from the save lab's design: stations, minted saves that the oracle holds Frigate to as it holds a
sequence; a map sweep that places the player in every zone; and verbs for the party, the bag and the Pokédex. ADR 0032
is reserved for the stations' rules.

## Minting a save

A recipe is a text file of lines, one verb each, and `#` starts a comment. Every argument is a number or a name from the
build's generated constants headers, `build/include/generated/constants/*.h`, which the ROM build's
`ninja constants_headers` and the host build's configure write from `generated/*.txt`: the lab numbers zones, flags,
work values and species as the game does, and no table of its own can drift from the game's.

```text
# Floccesy Ranch's gate, with the story of the Route 20 save
base floccesy_save
place ZONE_FLOCCESY_RANCH_2 warp 0
```

```sh
python3 port/tools/lab/savelab.py mint ranch.recipe --base floccesy_save_b2_us.sav --out SAVES/IREO.sav
build/port/frigate-black2 --save-dir SAVES      # the start menu shows Floccesy Ranch; CONTINUE goes there
```

`mint` prints, as JSON, what each line set, the base's and the minted save's SHA-1 and how many bytes changed (38 for
this recipe: 19 in each copy). It never replaces an existing file without `--force`, since `--out` can be the player's
own save, and it writes through a temporary file renamed over the target. A recipe of `-` is read from the standard
input. The same base, recipe and tool give the same bytes, so a test can pin a minted save's SHA-1 as `oracle.py` pins
the saves it makes. The save is named after the ROM's game code: `IREO.sav` for Black 2, `IRDO.sav` for White 2.

| Verb | What it sets | Where the game keeps it |
| --- | --- | --- |
| `base NAME` | Nothing: it names the oracle save the recipe starts from (`oracle.py`'s `SAVES`), and `mint` refuses a `--base` without one of the SHA-1s `SAVES` pins for it | |
| `place ZONE warp N` | The player arrives through the zone's own warp `N` (`res/field/events/<zone>.json`), as through that door | `PlayerSave.nextSpawn` and `SaveLocation` (block 0x1c), `EVENT_FLAG_CONTINUE_SCRIPT`, `EVENT_WORK_CONTINUE_SCRIPT` (0x2d) |
| `place ZONE at X Z [Y] [facing DIR]` | The player stands on tile (X, Z), at height Y (0 if left out), facing `WARP_DIR_UP`, `_DOWN` (if left out), `_LEFT` or `_RIGHT` | the same |
| `place ZONE fly` | The player stands where Fly lands in the zone (`res/field/zones/<zone>.json`'s `fly`), facing down | the same |
| `flag NAME set`, `flag NAME clear` | A saved event flag, 1 to 3,063 | `EventWork.flags`, bit N % 8 of byte N / 8 |
| `work NAME VALUE` | A saved event work value, `EVENT_WORK_0x4000` to `EVENT_WORK_0x41ae`, to a number or a name (0 to 65,535) | `EventWork.works` |
| `money N` | The money, at most `TRAINER_GAME_INFO_CASH_MAX` (9,999,999) | `TrainerGameInfoSave.cash` (0x34) |
| `badges N` | The first N of the 8 badges | `TrainerGameInfoSave.badges`, bit B for badge B |
| `playtime H:MM:SS` | The play time, at most 999:59:59 | `TrainerDataSave.playTime` (0x1b) |

A recipe places the player once at most and sets each flag, work value and the rest once. `EVENT_FLAG_CONTINUE_SCRIPT`
and `EVENT_WORK_CONTINUE_SCRIPT` belong to `place`, so the other verbs refuse them. The lab has no verb for the clock:
the clock and the DS's random seeds belong to the input file a run plays (`clock pinned`, `time`, `seed`), which Frigate
and the oracle both read.

## How place puts the player there

The game already has a way to start a continued game somewhere other than where it was saved, and `place` uses it, with
no hook in the game. The Hall of Fame puts the player back in their room with it
(`res/field/scripts/global_9600.s`), and the Union Room and the Wi-Fi Club with it too:

1. The script command `FieldSetNextZone` writes `PlayerSave.nextSpawn` (`src/ov012/scrcmd_game_state.c`), and `FlagSet`
   sets `EVENT_FLAG_CONTINUE_SCRIPT`. The next SAVE writes both.
2. On CONTINUE, `EventFieldContinue_Create` reads the flag and clears it (`src/ov035/event_mapchange.c`). Its callback
   deletes the field actors the save restored, loads the spawn's zone and runs `FieldMapControl_InitSpawn`, the routine
   every door runs: a warp is resolved through the destination zone's own warp table (`SetupZoneWarpArrival`,
   `src/ov012/eventdata_system.c`), and the zone's NPCs are spawned from its events as its flags decide.
3. With `EVENT_WORK_CONTINUE_SCRIPT` at 0, no continue script runs, and the field opens with the season's transition.

So `place` writes `nextSpawn` as the game's own constructors write it (`src/system/zone_spawn.c`):
`SetupZoneSpawnInfoWarp` for a warp, which leaves the arrival to the game; `CreateZoneChangeData` for a tile, with the
position as `FieldSetNextZone` makes it from tiles (each `<< 16`); and `LoadZoneSpawnInfoCheckRail` for Fly. It sets the
flag, clears the work value, and writes `SaveLocation`, which the start menu shows, as the player state
`FieldMapControl_InitSpawn` sets from that spawn: the zone, the position, the rotation of the facing
(`ConvWarpDirToAngle`) and not on a rail. The game snaps the player to the tile's centre and writes its own
`SaveLocation` at its next SAVE.

Rewriting `SaveLocation` alone would not do: a plain CONTINUE keeps the actors the save restored, the old zone's NPCs,
in the new map. And it is the continue's season transition that the player sees on arrival, not a door's walk-in, and
the zone left behind runs no exit script; both are the same on Frigate and on melonDS.

- **Rails.** A warp may arrive on a rail (Castelia City's streets, Skyarrow Bridge): the game resolves it from
  `nextSpawn`, and `SaveLocation` then keeps only the zone and the facing, since nothing reads its position before the
  game sets the player on the rail. `at` and `fly` take grid positions, so they refuse the 12 zones whose Fly position
  is on a rail, the game's `ZONES_WITH_RAILS` (`src/system/zonedata.c`, read from the source): place those by warp.
- **The player's state.** `place` refuses a base saved while cycling, surfing or diving
  (`SaveLocation.exState`), which the continue would keep on arrival.
- **Left as the base holds them:** the player's identity, the console the save was last played on (the adventure
  block's clock offset and MAC address), the three blocks the game encrypts with its VBlank count (mystery gifts, the
  trainer card's record, the symbol encounters), DWC's user data, the day care's egg seed, the boxes, the records and
  the C-Gear, and the saved field actors, which the spawn continue deletes. A minted save keeps every byte that a
  program could have made differently, so Frigate and melonDS start it alike.

## Reading a save

```sh
python3 port/tools/lab/savelab.py read floccesy_save_b2_us.sav --work EVENT_WORK_0x40a3
```

prints the save's SHA-1 and facts as JSON:

- `location`: where the player saved (`SaveLocation`): the zone, the tile and height or the rail position, the facing,
  and the state (walking, cycling, surfing, diving);
- `next_spawn`, `now_spawn`: the spawn the next CONTINUE takes when the continue flag is set, and the last arrival;
- `flags`: `EVENT_FLAG_CONTINUE_SCRIPT` and each `--flag NAME`; `flags_set`, how many saved flags are set, and with
  `--all-flags` each one's name (`set_flags`);
- `works`: `EVENT_WORK_CONTINUE_SCRIPT`, each `--work NAME`, and every work value that isn't 0;
- `money`, `badges` (how many, and their bits), `playtime`;
- `party`: each Pokémon's species, level, HP, experience, held item and moves, decrypted as `PokeParty_DecryptPkm` does
  (`src/system/poke_tool.c`), and whether its checksum holds.

For the Route 20 save it gives `ZONE_ROUTE_20` at tile `[130, 661]` and the work values `0x40a3` and `0x40a5` at 2, the
facts the oracle's README records for that save ([../oracle/README.md](../oracle/README.md)).

## What the lab refuses

Both commands take a backup image only as the game would leave it. The layout comes from
[../oracle/saveimage.py](../oracle/saveimage.py): `find_blocks` finds the main save's 73 blocks from their footers and
CRC table, and `validate_image` checks every block's CRC in both copies, the table and the footer. A save is refused,
with the reason and `savelab:` on the error output and status 1, when:

- it isn't 524,288 bytes, or a CRC, the footer's size or its magic fails in either copy;
- its two copies of the main save differ, which the game's SAVE never leaves;
- a block the lab reads doesn't have the size of the game's struct for it (`PlayerSave`, `EventWork`,
  `TrainerDataSave`, `TrainerGameInfoSave`, `PokeParty`, each rounded up to 4 bytes): not a Black 2 or White 2 save;
- the recipe names a base whose SHA-1 the save doesn't have, or a name that isn't a constant of the right kind, a value
  out of the game's range, a verb twice, or a verb that doesn't exist.

Every value is written to both copies. Then each written block's footer CRC and CRC table entry, and the save footer's
CRC of the table, are computed again in each copy, as `SaveData_FillFooter` computes them
(`src/gfl/backup_system.c`); the save counts are left alone. The result must pass `validate_image` against the base's
layout, with its two copies alike, or the minter refuses its own output.

The offsets are the decomp's headers' (`include/save/*.h`, `include/field/zone.h`, `include/pml/poke_party.h`). The test
`lab_savelab` compiles [../../tests/lab/savelab_layout.c](../../tests/lab/savelab_layout.c) as the host build compiles
the game, holds `savelab.py`'s `LAYOUT` to the offsets it prints, builds synthetic saves field by field from them, and
checks the reader, the minter's writes and resealing, and the refusals, each with a control that makes it fail. It needs
no ROM. The reader's facts on the oracle's saves are the ones the oracle's README records for them, independently; the
record of that check is in [STATUS.md](../../docs/STATUS.md).
