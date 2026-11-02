# Scripts

The script VM in `src/system/vm.c` runs four sets of commands: field events, the trainer AI, battle animations and
musicals. Scripts are files in the ROM's NARC archives. Each command is a 16-bit ID followed by its arguments.

The trainer AI scripts are built from source. Archive `a/1/6/9` holds 14 scripts, one per AI flag, which run in turn
for each flag the trainer has: flag bit N runs script N (`AI_FLAG_*` in `include/constants/tr_ai.h`). They are
written in `res/trainers/ai/NN_name.s`, numbered by their flag bit, with the macros in
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

- The lists in `generated/`, for moves, abilities, items, species, types, sounds, trainers and zones, which the
  build turns into headers (see [Constant lists](data.md#constant-lists)). They were written from the game's text by
  `tools/data/make_constants.py`, which reads it with `tools/text/msgdata.py`.
- pokeplatinum, for the move effects and held item effects that Gen 4 has, whose IDs this game keeps. Gen 5's move
  effects are named after their first move.
- The move data, for move categories and the conditions that moves inflict.
- The scripts themselves, for stat stages, weather, side and field conditions and genders: where a routine is the same
  as pokeplatinum's, its values show which constant is which.

```sh
python3 tools/data/make_constants.py extract/b2_us/files/a/0/0/2 generated
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

## Battle animation scripts

The battle effect VM, `src/ov168/btlv_effvm.c`, plays the animations of the moves and of the battle's other effects,
which the encounters' openings and the sending in of Pokémon (`btlv_scu.c`), the server's effect commands
(`btlv_core.c`) and Pokéstar Studios' battles (`btl_client.c`) start. Its scripts are built from source in `res/battle/animations/`, with the macros in `include/asm/btlv_effvm.inc`. An effect number below
`BTLV_EFFECT_FIRST` (0x231) is a move, whose script is the member of archive `a/0/6/5` with the move's number, written
in `moves/<move>.s` after `generated/moves.txt`; the 41 members past the last move are copies of Pound's, numbered
`560.s` to `600.s`, which nothing plays. The other effects are the 115 members of `a/0/6/6`, from `BTLV_EFFECT_FIRST`
up, written in `effects/NNN.s` by their member: the code that starts them (`BtlvEffect_Start` and the others) gives only
their numbers. Both versions have the same archives, and members with the same bytes keep a file
each.

A script file starts with the number of its variants and their code, in `ScriptHeader` and `ScriptVariant`. A variant
is the turn of a two-turn move, or a case of a move that looks different, which `BtlvEffvmParam`'s `variant` picks.

```
Pound:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 165
    EmitterAdd 165, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    End
```

Each macro is one command of `btlv_effvm.c`'s `MOVE_SCRCMD`, named after its handler as the field's are:
`BtlvEffvm_CmdCameraMove` is `CameraMove`. Its arguments are 32-bit, in the order the handler reads them. Particles are
the resources of `ARCID_BTLV_EFFVM_PARTICLES` (`res/particles/btlv_effvm_particles/`) and their emitters, by number;
sounds are `SEQ_*` constants, and `Call` and `ChangeScript` take a move or `BTLV_EFFECT_FIRST + N`, the script of
`effects/NNN.s`. fx32 values are written in hex, 0x1000 for 1. Jumps are labels, relative to the end of the command.
The macros define the VM's own codes, where the handlers show what they are: the attacker, the defender and the other
targets besides the positions 0 to 13 (`EFFVM_ATTACKER`), the emitters' positions (`EFFVM_EMIT_*`), what `Wait` waits
for (`EFFVM_WAIT_*`), the comparisons of `IfVarConst` (`EFFVM_CMP_*`), and the variables that the scripts test and set
(`EFFVM_VAR_*`, with a position added for those of each position, as in `EFFVM_VAR_WEIGHT + 3`); a value compared with
a flag is `TRUE` or `FALSE`.

The scripts are source, edited by hand. `tools/script/btlv_effvm_script.py` disassembled them, and writes the macros
from `MOVE_SCRCMD`. After handlers are renamed, its `sync`, which `sync_handler_names.py` runs too, writes the macros
again and renames the commands in the scripts:

```sh
python3 tools/script/btlv_effvm_script.py inc include/asm/btlv_effvm.inc
python3 tools/script/btlv_effvm_script.py disasm extract/b2_us/files/a/0/6/5 OUTPUT_DIR moves
python3 tools/script/btlv_effvm_script.py disasm extract/b2_us/files/a/0/6/6 OUTPUT_DIR effects
python3 tools/script/btlv_effvm_script.py sync
```

The disassembler follows the code from each variant. Bytes it does not reach are decoded as commands where they trace
cleanly, and otherwise kept as `.byte`; every byte of both archives decodes as commands. `ninja` builds the archives
as it builds the trainer AI's. `configure.py`'s `ARCHIVE_LISTS` orders the move scripts by `generated/moves.txt`, and
the numbered files follow; the effects, which have no list, are all numbered.

## Musical stage scripts

Overlay 209's stage VM (`sta_act_script.c`) plays the musical's performances. Each program's stage scripts are member 2
of its archive (see [Musical programs](data.md#musical-programs)), built from `res/musicals/programs/<n>/script.s`
with the macros in `include/asm/sta_script.inc`. A script file starts with the offsets of its scripts, in
`ScriptHeader` and a `ScriptEntry` each; a program's has 14, of which the game reads the first, `Main`, and the third
to sixth, `Position0` to `Position3`, the scripts of the four positions on the stage, which `StaActing_StartScripts`
runs beside `Main` where they differ from it. Each script runs from its label to its `End`; there are no jumps.

```
    SetPokeMask TRUE, TRUE, TRUE, TRUE
    PokeScript STA_SCRIPT_OWN_POKE, 1
    Wait 700
    StopBgm
    SyncWait 2
    PlayBgm SEQ_BGM_MSL_01
    AudienceFocus 0
    LightAdd 0, 0
    LightFollow 0, 0, 2175, 0, -0x28000, 0
```

Each macro is one command of `script_command.c`'s `STA_SCRIPT_COMMANDS`, named after its handler as the field's are:
`StaScriptCmd_PokeMove` is `PokeMove`. Its arguments are 32-bit, in the order the handler reads them, named after the
handler's variables. Pokémon are positions, 0 to 3, or `STA_SCRIPT_OWN_POKE`, those the script was started for or
that `SetPokeMask` set; a flag the handler compares with `TRUE` or tests is `TRUE` or `FALSE`, as a move's `wait`,
which makes the script wait for it; sequences are `SEQ_*` constants; and fx32 values are written in hex, 0x1000 for 1.
`SyncWait` makes a synced script wait until every synced script waits, so that `Main` and the positions' scripts keep
in step; `PrintMessage` prints a message of the program's `messages.txt` by its line.

The scripts are source, edited by hand. `tools/script/sta_script.py` disassembled them, and writes the macros from
`STA_SCRIPT_COMMANDS`; after handlers are renamed, its `sync`, which `sync_handler_names.py` runs too, writes the
macros again and renames the commands in the scripts:

```sh
python3 tools/script/sta_script.py inc include/asm/sta_script.inc
python3 tools/script/sta_script.py disasm SCRIPT OUTPUT.s --program N
python3 tools/script/sta_script.py sync
```

The build assembles each `script.s` as it does the other scripts and gives the binaries to the programs' packer,
which puts each in its archive among the members of other kinds: `configure.py`'s `PACK_SCRIPTS` names, for a
`DATA_PACKS` packer, the scripts of its data directory that it takes with `--scripts`.

## Field scripts

The field scripts, archive `a/0/5/6`, are built from source in `res/field/scripts/<file>.s`, with the macros in
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

A command's handler is `Scrcmd_<Group>_<Name>`, where `<Group>` is its file, `scrcmd_<group>.c` (or the file's name
without `event_` for the few outside those), and the command is `<Name>`: `Scrcmd_Vm_FlagReset` is `FlagReset`, and
`Scrcmd_Msg_InfoMsgClose` `InfoMsgClose`. swan's handlers (`s0024_FlagReset`) take this form too. Handlers that are
still address-named and whose calls only tell the save data they use get its area, such as `AreaNPCCmd_0218`, and the
rest are `Cmd_NNNN`.

Commands from ID 1000 up come from the script plugin, overlays that the zone loads. They take their handler's `<Name>`
once it has one, as the base commands do (`Scrcmd_BadgeGate_PlayCheck` is `PlayCheck`), with the group in front where
another command has the name already (`BlackTower_GetInfo`, beside other plugins' `GetInfo`), and are `PluginN_CmdNNNN`
until then. A handler that serves commands of two numbers names each with its number, such as `ResetPicks_1023` and
`ResetPicks_1031`. Overlay 12's `SCRIPT_PLUGIN_TABLE` lists the plugins:

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
