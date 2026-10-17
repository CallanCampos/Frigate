#include "asm/field_script.inc"
#include "text/script/pokemon_league_10.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0210
    ActorCmdWait
    VMSleep 8
    SEPlay SEQ_SE_KAIDAN
    ActorNew 8, 19, 0, 251, 298, 0
    SEWait
    ActorCmdExec 251, Movement_01DC
    ActorCmdWait
    WordSetPlayerName 0
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005E
    // "Iris: So, this room is\nthe Hall of Fame![f000]븁\u0000\nYour name and your Pokémon's\nnames will be recorded here[f000]븀\u0000\nso these important things will never[f000]븀\u0000\nbe forgotten:[f000]븁\u0000\nThat you are an excellent and\nkind Trainer![f000]븁\u0000\nThat you are considerate toward\nyour Pokémon, and you give them[f000]븀\u0000\nyour whole heart![f000]븁\u0000\nAnd that your wonderful Pokémon\nbelieve in their Trainer with all[f000]븀\u0000\ntheir hearts, and together you have[f000]븀\u0000\na strength that knows no bounds![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague10_Text_IrisRoomHallFame, 251, 0, 0
    MsgWinCloseAll
    VMJump L_006C

L_005E:
    // "Iris: You've made it here again![f000]븁\u0000\nYour name and your Pokémon's\nnames will be recorded here[f000]븀\u0000\nso these important things will never[f000]븀\u0000\nbe forgotten:[f000]븁\u0000\nThat you are an excellent and\nkind Trainer![f000]븁\u0000\nThat you are considerate toward\nyour Pokémon, and you give them[f000]븀\u0000\nyour whole heart![f000]븁\u0000\nAnd that your wonderful Pokémon\nbelieve in their Trainer with all[f000]븀\u0000\ntheir hearts, and together you have[f000]븀\u0000\na strength that knows no bounds![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague10_Text_IrisYouveMadeHere, 251, 0, 0
    MsgWinCloseAll

L_006C:
    ActorCmdExec 251, Movement_01F8
    ActorCmdExec 255, Movement_01F0
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x88000, 0x2001f, 0x48000, 20
    EvCameraWait
    WorkSetConst 0x8020, 0
    TrainerCardGetSex 0x8020
    ActorCmdExec 251, Movement_0270
    VMSleep 8
    ActorCmdExec 255, Movement_0278
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E3
    // "Hey, c'mon![f000]븁\u0000\nOh, excuse me...\n[f000]ă\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague10_Text_HeyCmonOhExcuse, 251, 0, 0
    ActorMsgClose
    VMJump L_00F1

L_00E3:
    // "Hey, c'mon![f000]븁\u0000\nOh, excuse me...\n[f000]ă\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague10_Text_HeyCmonOhExcuse_2, 251, 0, 0
    ActorMsgClose

L_00F1:
    ActorCmdExec 255, Movement_0260
    VMSleep 8
    ActorCmdExec 251, Movement_0260
    ActorCmdWait
    // "To the Trainer standing before me!\nAnd to the Pokémon at your side![f000]븁\u0000\nYour beautiful bond has\ngrown strong through battle![f000]븁\u0000\nIn order to make this\nbond an eternal treasure,[f000]븀\u0000\nyour names shall be recorded here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague10_Text_TrainerStandingBeforePokemon, 251, 0, 0
    ActorMsgClose
    BMPlayHOFMachineSeq
    EvCameraEnd
    MedalIsObtained EVENT_WORK_0x400e, 253
    PokePartyGetCount EVENT_WORK_0x400f, 0
    PokePartyGetCount EVENT_WORK_0x400d, 3
    WorkSub EVENT_WORK_0x400f, EVENT_WORK_0x400d
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0158
    MedalGive 253

L_0158:
    MedalDiscover 236
    MedalDiscover 237
    MedalDiscover 238
    MedalDiscover 239
    MedalDiscover 240
    MedalDiscover 241
    MedalDiscover 242
    MedalDiscover 243
    MedalDiscover 244
    MedalDiscover 245
    MedalDiscover 246
    MedalDiscover 247
    MedalDiscover 248
    MedalDiscover 249
    MedalDiscover 250
    MedalDiscover 251
    MedalDiscover 252
    MedalDiscover 253
    MedalDiscover 102
    MedalDiscover 136
    MedalDiscover 124
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CD
    FadeOutBlackQ
    FadeWait
    CallGameClear 0
    .byte 0x1e
    .byte 0x00
    VMStackPushConst 0

L_01CD:
    FadeOutBlackQ
    FadeWait
    CallGameClear 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01DC:
    Move 15, 1
    Move 12, 3
    Move 14, 1
    Move 33, 1
    MoveEnd

Movement_01F0:
    Move 12, 13
    MoveEnd

Movement_01F8:
    Move 12, 11
    Move 15, 1
    Move 32, 1
    MoveEnd
    Move 13, 1
    MoveEnd

Movement_0210:
    Move 12, 1
    MoveEnd
    VMStackDiv
    VMNop2
    PokePartyGetSpecies 0, 14
    VMNop2
    PokePartyGetSpecies 0, 0
    VMNop2
    PokePartyGetSpecies 0, 1
    VMNop2
    PokePartyGetSpecies 0, 2
    VMNop2
    PokePartyGetSpecies 0, 3
    VMNop2
    PokePartyGetSpecies 0, 15
    VMHalt
    PokePartyGetSpecies 0, 14
    VMHalt
    .byte 0xfe
    .balign 4, 0
    Move 10, 1
    MoveEnd

Movement_0260:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_0270:
    Move 34, 1
    MoveEnd

Movement_0278:
    Move 35, 1
    MoveEnd
