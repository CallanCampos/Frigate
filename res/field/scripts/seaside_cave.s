#include "asm/field_script.inc"
#include "text/script/seaside_cave.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40d8
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0041
    VMCall L_00A8
    VMJump L_004F

L_0041:
    // "Hm![f000]븁\u0000\nIf you want past here,\nyou have to defeat me,[f000]븀\u0000\nand my Roggenrola![f000]븀\u0000\nAnd we're as sturdy as rock![f000]븁\u0000\nBut, my Roggenrola and I\nonly battle strong Trainers.[f000]븁\u0000\nThat is my, and my Roggenrola's,\npolicy! It's as sturdy as rock!"
    ParentActorMsg MSGFILE_SCRIPT, SeasideCave_Text_HmIfWantPast, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_004F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4152, 1
    ActorCmdExec 12, Movement_035C
    ActorCmdWait
    WorkSetConst 0x8020, 0
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_008C
    ActorCmdExec 255, Movement_0320

L_008C:
    WorkSetConst 0x8020, 0
    ActorCmdExec 12, Movement_0314
    ActorCmdWait
    VMCall L_00A8
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00A8:
    // "Mmm![f000]븁\u0000\nMy Roggenrola and I\nhave been waiting for[f000]븀\u0000\na strong Trainer like you![f000]븁\u0000\nIf you want to pass, you must beat\nmy Roggenrola and me![f000]븁\u0000\nCan you defeat our rock-hard will?"
    ActorMsg MSGFILE_SCRIPT, SeasideCave_Text_MmmRoggenrolaHaveBeen, 12, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0137
    // "Just to let you know, my Roggenrola and\nI are the sturdiest things that were[f000]븀\u0000\never sturdy![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, SeasideCave_Text_JustLetKnowRoggenrola, 12, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_BLACK_BELT_ROCKY, 0, 0
    VMCall L_0149
    // "Mmm![f000]븁\u0000\nYour strength is the real thing![f000]븁\u0000\nMy Roggenrola and I must\nbecome even sturdier,[f000]븀\u0000\nso we're off to continue our training![f000]븁\u0000\nFarewell!"
    ActorMsg MSGFILE_SCRIPT, SeasideCave_Text_MmmStrengthRealThing, 12, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMSleep 15
    FadeEx 3, 0, 16, 4
    FadeExWait
    ActorDelete 12
    ActorDelete 8
    ActorDelete 10
    ActorDelete 11
    FlagSet EVENT_FLAG_0x0363
    WorkSetConst EVENT_WORK_0x4152, 1
    FadeEx 3, 16, 0, 4
    FadeExWait
    VMSleep 30
    VMJump L_0147

L_0137:
    // "What a flimsy answer!"
    ActorMsg MSGFILE_SCRIPT, SeasideCave_Text_WhatFlimsyAnswer, 12, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0147:
    VMReturn

L_0149:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0168
    CallTrainerBattleEnd
    VMJump L_016A

L_0168:
    CallTrainerLose

L_016A:
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Stur! Stur!"
    ParentActorMsg MSGFILE_SCRIPT, SeasideCave_Text_SturStur, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "De deee!"
    ParentActorMsg MSGFILE_SCRIPT, SeasideCave_Text_DeDeee, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Rorooog!"
    ParentActorMsg MSGFILE_SCRIPT, SeasideCave_Text_Rorooog, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x0170
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FD
    VMCall L_020D
    VMJump L_0207

L_01FD:
    // "It's a big boulder, but it doesn't\nlook like a Pokémon can move it..."
    SystemMsg SeasideCave_Text_ItsBigBoulderBut, 2
    LastKeyWait
    MsgWinCloseAll

L_0207:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_020D:
    // "Use the Colress Machine on the\nbig boulder?"
    SystemMsg SeasideCave_Text_UseColressMachineBig, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EE
    MsgWinCloseAll
    SEPlay SEQ_SE_SHINKA_W025
    SEWait
    WorkSetConst 0x8021, 0
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025D
    ActorCmdExec 9, Movement_033C
    VMJump L_0286

L_025D:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027E
    ActorCmdExec 9, Movement_0344
    VMJump L_0286

L_027E:
    ActorCmdExec 9, Movement_034C

L_0286:
    ActorCmdWait
    WorkSetConst 0x8021, 0
    PVPlay 558, 0
    // "Crus chul!"
    InfoMsg SeasideCave_Text_CrusChul, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    CallWildBattle 558, 42, 128
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02CE
    FlagSet EVENT_FLAG_0x0364
    ActorDelete 9
    CallWildBattleEnd
    VMJump L_02D0

L_02CE:
    CallWildLose

L_02D0:
    // "The Colress Machine broke..."
    SystemMsg SeasideCave_Text_ColressMachineBroke, 2
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8022, 0
    ItemSub ITEM_COLRESS_MCHN, 1, 0x8022
    VMJump L_02F0

L_02EE:
    MsgWinCloseAll

L_02F0:
    VMReturn
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_0314:
    Move 32, 1
    Move 0, 1
    MoveEnd

Movement_0320:
    Move 33, 1
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_033C:
    Move 32, 1
    MoveEnd

Movement_0344:
    Move 33, 1
    MoveEnd

Movement_034C:
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_035C:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
