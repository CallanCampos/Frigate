#include "asm/field_script.inc"
#include "text/script/pokemon_league.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    FlagReset EVENT_FLAG_0x0967
    FlagReset EVENT_FLAG_0x0968
    FlagReset EVENT_FLAG_0x0969
    FlagReset EVENT_FLAG_0x096a
    VMStackPush EVENT_WORK_0x4109
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004D
    ObjInitNPCGPos 1, 1, 32, 1, 46

L_004D:
    FlagSet EVENT_FLAG_0x016f
    VMHalt

Script_2:
    VMHalt

Script_3:
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokémon League is a place where you\nboth pursue strength and express it.[f000]븁\u0000\nThe way to express it is simple...[f000]븁\u0000\nYou just have to beat the Elite Four and\nthe Champion![f000]븁\u0000\nYou can start your challenge by battling\nany of the Elite Four, and if you defeat[f000]븀\u0000\nthem all, you can challenge the Champion![f000]븁\u0000\nHowever! I warn you, once you start\nyour challenge, there's no turning back.[f000]븁\u0000\nIf you enter, you must keep battling\nuntil you defeat them all...[f000]븀\u0000\nor are defeated yourself."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague_Text_PokemonLeaguePlaceWhere_2, 1, 1, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokémon League is every Trainer's\ngreatest challenge![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague_Text_PokemonLeagueEveryTrainers, 0, 1, 0
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00AC
    MsgWinCloseAll
    ActorCmdExec 0, Movement_01F0
    ActorCmdWait

L_00AC:
    // "You might want to prepare here.\nBecause if you lose even once,[f000]븀\u0000\nyou have to start all over again!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague_Text_MightWantPrepareHere, 0, 1, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_01E8
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    ActorCmdExec 1, Movement_01D8
    ActorCmdWait
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00F9
    ActorCmdExec 255, Movement_01E0
    ActorCmdWait

L_00F9:
    VMStackPushFlag EVENT_FLAG_0x016e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0128
    // "The Pokémon League is a place where you\nboth pursue strength and express it.[f000]븁\u0000\nThe way to express it is simple...[f000]븁\u0000\nYou just have to beat the Elite Four and\nthe Champion![f000]븁\u0000\nYou can start your challenge by battling\nany of the Elite Four, and if you defeat[f000]븀\u0000\nthem all, you can challenge the Champion![f000]븁\u0000\nHowever! I warn you, once you start\nyour challenge, there's no turning back.[f000]븁\u0000\nYou must keep battling until you defeat\nthem all...or are defeated yourself.[f000]븁\u0000\nDo you want to go in?"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague_Text_PokemonLeaguePlaceWhere, 1, 5, 0
    FlagSet EVENT_FLAG_0x016e
    HollowRivalCmd_0262 1, 39
    VMJump L_0134

L_0128:
    // "Once you start your challenge,\nyou cannot leave until you win against all[f000]븀\u0000\nor lose! Do you want to go in?"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague_Text_OnceStartChallengeCannot, 1, 5, 0

L_0134:
    YesNoWin 0x8010
    PlayerGetGPos 0x8008, 0x8009
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B7
    // "Then, proceed![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague_Text_ThenProceed, 1, 5, 0
    MsgWinCloseAll
    VMStackPush 0x8008
    VMStackPushConst 31
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0180
    ActorCmdExec 1, Movement_0200
    VMJump L_01A9

L_0180:
    VMStackPush 0x8008
    VMStackPushConst 32
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A1
    ActorCmdExec 1, Movement_0210
    VMJump L_01A9

L_01A1:
    ActorCmdExec 1, Movement_0224

L_01A9:
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x4109, 1
    VMJump L_01CF

L_01B7:
    // "Don't neglect preparation![f000]븁\u0000\nGo through the entrance there, and\nprepare as much as possible. Make sure[f000]븀\u0000\nyour Pokémon are fully recovered![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague_Text_DontNeglectPreparationGo, 1, 5, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0234
    ActorCmdWait

L_01CF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01D8:
    Move 75, 1
    MoveEnd

Movement_01E0:
    Move 32, 1
    MoveEnd

Movement_01E8:
    Move 33, 1
    MoveEnd

Movement_01F0:
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0200:
    Move 13, 2
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_0210:
    Move 14, 1
    Move 13, 2
    Move 14, 1
    Move 33, 1
    MoveEnd

Movement_0224:
    Move 13, 2
    Move 14, 2
    Move 33, 1
    MoveEnd

Movement_0234:
    Move 13, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 1055, 0, 0x150000, 0x208000, 0x4a6a0, 0x398000, 1
    EvCameraWait
    FadeInBlack
    EvCameraMoveTo 1055, 0, 0x150000, 0x208000, 0, 0x398000, 110
    CallPlaceNameDisp
    VMSleep 96
    FadeWait
    FadeEx 3, 0, 16, 4
    EvCameraWait
    FadeExWait
    EvCameraMoveTo 6563, 0, 0x160000, 0x208000, 0x186a0, 0x398000, 1
    EvCameraWait
    FadeEx 3, 16, 0, 4
    EvCameraMoveToDefault 60
    VMSleep 40
    ActorCmdExec 255, Movement_02D4
    EvCameraWait
    ActorCmdWait
    FadeExWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_02D4:
    Move 12, 3
    MoveEnd
