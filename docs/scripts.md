# Scripts

The script VM in `src/system/vm.c` runs four sets of commands: field events, the trainer AI, battle move animations and
musicals. Scripts are files in the ROM's NARC archives. Each command is a 16-bit ID followed by its arguments.

The trainer AI scripts are built from source. Archive `a/1/6/9` holds 14 scripts, one per AI flag, which run in turn
for each flag the trainer has: flag bit N runs script N (`AI_FLAG_*` in `include/constants/tr_ai.h`). They are
written in `data/tr_ai/NN_name.s`, numbered by their flag bit, with the macros in
`include/asm/tr_ai.inc`, in the style of [pokeplatinum](https://github.com/pret/pokeplatinum)'s trainer AI. This
game's scripts grew out of Gen 4's, so most commands, routines and labels are the same as pokeplatinum's, and share
their names. Gen 5's additions, such as the handlers of the new move effects, are named in the same style. Each
script starts with a comment on what it does, and bugs are marked where the code shows them.

```
Basic_CheckForImmunity:
    IfMoveEffectivenessEquals TYPE_EFFECTIVENESS_IMMUNE, ScoreMinus10
    LoadBattlerAbility AI_BATTLER_ATTACKER
    IfLoadedEqualTo ABILITY_MOLD_BREAKER, Basic_CheckSoundproof
    LoadBattlerAbility AI_BATTLER_DEFENDER
    IfLoadedEqualTo ABILITY_VOLT_ABSORB, Basic_CheckElectricAbsorption
```

Each macro is one command of `src/ov170/tr_ai.c`. Its arguments are 32-bit. Jump and table arguments are labels,
which the macros encode relative to the end of the argument. The Load commands set a value that `IfLoadedEqualTo`
and the other comparisons test. Commands that behave differently from Gen 4's have a comment in the macros, and the
commands that do nothing here are named `DummyNN` after their ID, as pokeplatinum does.

Scripts go through the C preprocessor, so they use the same constants as the C code. The constant headers hold
only `#define`s for this reason. The constants come from:

- The lists in `data/constants/`, for moves, abilities, items, species, types, sounds, trainers and zones, which the
  build turns into headers (see [Constant lists](data.md#constant-lists)). They were written from the game's text by
  `tools/data/make_constants.py`, which reads it with `tools/text/msgdata.py`.
- pokeplatinum, for the move effects and held item effects that Gen 4 has, whose IDs this game keeps. Gen 5's move
  effects are named after their first move.
- The move data, for move categories and the conditions that moves inflict.
- The scripts themselves, for stat stages, weather, side and field conditions and genders: where a routine is the same
  as pokeplatinum's, its values show which constant is which.

```sh
python3 tools/data/make_constants.py extract/b2_us/files/a/0/0/2 data/constants
```

`ninja` assembles each script with `clang`, converts it to a binary with `llvm-objcopy`, packs the binaries with
`tools/data/narc.py` into `build/<version>/files/a/1/6/9`, and checks the archive against the extracted one. The
ROM is built from `build/<version>/files`, which `tools/build/files_tree.py` fills with links to the extracted files,
except for the files built from source. `ARCHIVES` in `configure.py` lists them.

The scripts are source, edited by hand. `tools/script/tr_ai_script.py` disassembled them, and writes the macros:

```sh
python3 tools/script/tr_ai_script.py inc include/asm/tr_ai.inc
python3 tools/script/tr_ai_script.py disasm extract/b2_us/files/a/1/6/9 OUTPUT_DIR [--labels NAMES.json]
```

The disassembler follows the jumps from the start of each script. Bytes it does not reach are decoded as commands
where they are valid, then as tables, and otherwise kept as `.byte`. A value compared with the loaded value is written
as a constant when every way to the comparison loads the same kind of value. Labels are named after their offset,
unless `--labels` gives names.

## Field scripts

The field scripts, archive `a/0/5/6`, are built from source in `data/field_scripts/<file>.s`, with the macros in
`include/asm/field_script.inc`. Both versions have the same archive. It holds a script file and a map script table for
each zone, and the global scripts, which zones start by their IDs (from 2000 up). A script file starts with the offsets
of its scripts, followed by the scripts and their movement data. A map script table lists the scripts that the zone runs
at points such as its loading. A script names the messages it shows by their IDs (see [Text](data.md#text)),
`MsgPlaceSign 0x8021, BlackCity_Text_...`, and includes the header of their bank, `text/script/<bank>.h`; the comment
above a command still gives the message.

```
L_015C:
    ItemSub ITEM_POKE_BALL, 1, 0x8010
    ItemAdd ITEM_GREAT_BALL, 1, 0x8010
    WorkSetConst 0x8020, 2
    VMReturn
```

Commands are named after swan's names for their handlers, such as `s0024_FlagReset` for `FlagReset`. Of the ones swan
does not name, a few are named after the function they call, such as `IsFestMissionAvailable`. Those whose calls only
tell the save data they use get its area, such as `MusicalCmd_0165`, and the rest are `Cmd_NNNN`.

Commands from ID 1000 up come from the script plugin, overlays that the zone loads. They take their handler's name once
it has one, such as `BadgeGateCmd_PlayCheck`, and are `PluginN_CmdNNNN` until then. Overlay 12's `SCRIPT_PLUGIN_TABLE`
lists the plugins:

| Plugin | Overlays | For | Files named in the overlays |
|---|---|---|---|
| 1 | 50 | Battle Subway: Gear Station and the trains | |
| 2 | 51 | The Royal Unova, the "pleasure boat" of overlay 36's `pleasure_boat.c` | |
| 3 | 52 | Pokémon League: the Elite Four's rooms, whose gimmicks are overlays 122 (Shauntal), 123 (Grimsley), 124 (Marshal), 125 (Caitlin) and 120 (the Champion) | |
| 4 | 53 | Poké Transfer Lab | `scrcmd_palpark.c` |
| 5 | 54 | Abyssal Ruins | |
| 6, 7 | 55, and 56 or 57 | Pokémon World Tournament | `wbt_*.c` |
| 8 | 58 and 59, with 60 swapped in for the shops | Join Avenue | `scrcmd_resort.c`, `scrcmd_medalinfo.c`, `scrcmd_resort_shop.c` |
| 9 | 61 | Black Tower and White Treehollow | |
| 10 | 62 | Pokéstar Studios | `pokewood_*.c` |
| 11 | 63 | Victory Road's badge gates | |
| 12 | 64 | The Plasma Frigate, with its password device | |
| 13 | 65 | Pokémon Centers: Mr. Medal's Medal Rally, and records of link battles | |
| 14 to 16 | 66 to 68 | Not identified yet: 15 has the DNA Splicers and Terrakion, 16 Meloetta's Relic Song and the Swords of Justice | |

Our files take those names where the overlay has them, and otherwise follow them, as `scrcmd_badge_gate.c`.

Conditions are computed on a stack: `VMStackPush 0x8010`, `VMStackPushConst 0`, `VMStackCmp CMP_EQ`, and then
`VMJumpIf CMP_STACK` jumps if the result is TRUE. `VMJumpIf` can also test the comparison register that
`WorkCmpConst` and the other Cmp commands set (`cmpResult` in `system/vm.h`). The comparisons are in
`include/constants/field_script.h`. Arguments that take a value can take a variable instead: IDs from `0x4000` are
saved event work and from `0x8000` the script's own work, and the scripts write them in hex.

Arguments are written as constants where the handler shows what they are: items, moves, species, abilities and types,
and `MSGFILE_SCRIPT` for the script's own text file. Each command that shows a message has its text as a comment,
from the zone's or the global script's text file in the script message archive (`a/0/0/3`).

The arguments of every command come from its handler. `tools/script/field_command_table.py` follows the reads of the
script in each handler's disassembly: `VM_Read16`, `VM_Read32`, `ScriptReadAny` (a value or a variable),
`ScriptReadVar`, loads through the VM's pc, and the same in the functions the handler calls with the VM. It follows
each value read to the functions it is passed to, and a known function such as `BagSave_AddItem` or
`LoadFieldScriptMessage` tells what the argument is. It writes `tools/script/field_commands.json`, and needs
`dsd dis` output in `build/asm`. Every script file decodes with these arguments.

A script file has the plugin of the zones that use it, or else of the zones that start its scripts. A few global
files get the only plugin whose commands they decode with, and files whose plugin is not known keep plugin commands
as bytes. The Join Avenue shop commands swap one of the plugin's overlays, which changes the arguments of its
commands, and the disassembler follows that.

```sh
python3 tools/script/field_command_table.py tools/script/field_commands.json
python3 tools/script/field_script.py inc include/asm/field_script.inc
python3 tools/script/field_script.py disasm extract/b2_us OUTPUT_DIR
```

The disassembler follows the code from each script. Bytes it does not reach are decoded as code where it ends
cleanly, then as movement data, and otherwise kept as `.byte`: about 2,500 bytes, mostly in global files whose plugin
is not known. The scripts are still written by the disassembler, so improvements to it can be applied by running it
again.
