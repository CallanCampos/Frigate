#include "asm/field_script.inc"
#include "text/script/reversal_mountain_11.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0384
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4074
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0057
    FlagSet EVENT_FLAG_0x0384
    WorkSetConst EVENT_WORK_0x4074, 1

L_0057:
    VMHalt

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    ActorCmdExec 254, Movement_02E8
    VMSleep 8
    ActorCmdExec 255, Movement_02D0
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8023
    WorkGet 0x8022, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009F
    WorkSub 0x8021, 1
    VMJump L_00A5

L_009F:
    WorkAdd 0x8021, 1

L_00A5:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xa8000, 0xffff8000, 0xf8000, 32
    ActorWalkRoute 254, 0x8021, 15, 0, 8, 0
    VMSleep 16
    ActorCmdExec 255, Movement_02C8
    ActorCmdWait
    EvCameraWait
    // "What is this place?\nIt feels very strange.[f000]븁\u0000\nCould this be the place where\nReversal Mountain started from--[f000]븀\u0000\nthe lair of the Pokémon Heatran?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain11_Text_WhatPlaceFeelsVery, 254, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 32
    ActorWalkRoute 254, 0x8021, 18, 0, 8, 0
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    PlayerGetGPos 0x8021, 0x8023
    VMStackPush 0x8021
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0136
    ActorCmdExec 254, Movement_02E0
    ActorCmdExec 255, Movement_02D8
    VMJump L_0146

L_0136:
    ActorCmdExec 254, Movement_02D8
    ActorCmdExec 255, Movement_02E0

L_0146:
    ActorCmdWait
    // "Heatran is a Pokémon with\nmagma-like blood flowing through it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, ReversalMountain11_Text_HeatranPokemonMagmaLike, 254, 0, 0
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4121, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    // "The Magma Stone is reacting\nto something...[f000]븀\u0000\nWill you set it down here?"
    SystemMsg ReversalMountain11_Text_MagmaStoneReactingSomething, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BA
    MsgWinCloseAll
    VMSleep 8
    FlagReset EVENT_FLAG_0x0384
    WorkSetConst EVENT_WORK_0x4074, 2
    PVPlay 485, 0
    // "Gwogobo gwobobobo!"
    InfoMsg ReversalMountain11_Text_GwogoboGwobobobo, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    ActorAdd 0
    ActorFallDownToXZ 0, 10, 12, 1
    VMSleep 60
    VMJump L_01C6

L_01BA:
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0288
    ActorCmdWait

L_01C6:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 485, 0
    // "Gwogobo gwobobobo!"
    ScreamMsg ReversalMountain11_Text_GwogoboGwobobobo, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 485, 68, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_021A
    FlagSet EVENT_FLAG_0x0384
    WorkSetConst EVENT_WORK_0x4074, 3
    ActorDelete 0
    CallWildBattleEnd
    VMJump L_021C

L_021A:
    CallWildLose

L_021C:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0233
    VMJump L_023D

L_0233:
    FlagSet EVENT_FLAG_0x017c
    VMJump L_0263

L_023D:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_025D
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_025D
    VMJump L_0263

L_025D:
    VMJump L_0263

L_0263:
    VMStackPushFlag EVENT_FLAG_0x017c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0280
    // "Heatran vanished into the\ndepths of the volcano..."
    SystemMsg ReversalMountain11_Text_HeatranVanishedIntoDepths, 2
    LastKeyWait
    InfoMsgClose

L_0280:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0288:
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_02C8:
    Move 32, 1
    MoveEnd

Movement_02D0:
    Move 33, 1
    MoveEnd

Movement_02D8:
    Move 34, 1
    MoveEnd

Movement_02E0:
    Move 35, 1
    MoveEnd

Movement_02E8:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
