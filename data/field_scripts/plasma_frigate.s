#include "asm/field_script.inc"
#include "text/script/plasma_frigate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntry Script_25
    ScriptEntry Script_26
    ScriptEntry Script_27
    ScriptEntry Script_28
    ScriptEntry Script_29
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_3:
    VMCall L_0277
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AD
    WorkSetConst EVENT_WORK_PLASMA_FRIGATE_LOCATION, 3

L_00AD:
    VMStackPushFlag EVENT_FLAG_0x01ad
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ae9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00D8
    FlagReset EVENT_FLAG_0x0405
    FlagSet EVENT_FLAG_0x034e

L_00D8:
    VMHalt

Script_1:
    GameGetVersion 0x8020
    VMStackPush EVENT_WORK_0x40f1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_011D
    DebugPrint 12
    ActorSetGPos 0, 12, 0, 29, 3
    ActorSetGPos 255, 8, 0, 30, 3

L_011D:
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013C
    ActorSetGPos 255, 9, 0, 30, 3

L_013C:
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_020B
    DebugPrint 22
    ActorSetGPos 0, 13, 0, 34, 1
    ActorSetGPos 1, 11, 0, 28, 0
    ActorSetGPos 2, 10, 0, 31, 3
    ActorSetGPos 4, 14, 0, 31, 2
    ActorSetGPos 5, 12, 0, 27, 1
    ActorSetGPos 6, 11, 0, 27, 1
    ActorSetGPos 7, 13, 0, 35, 0
    ActorSetGPos 8, 12, 0, 35, 0
    ActorSetGPos 3, 14, 0, 36, 0
    ActorSetGPos 9, 11, 0, 36, 0
    ActorSetGPos 10, 15, 0, 35, 2
    ActorSetGPos 11, 14, 0, 28, 2
    ActorSetGPos 12, 13, 0, 26, 1
    ActorSetGPos 13, 14, 0, 27, 2

L_020B:
    VMStackPush EVENT_WORK_0x40f1
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0248
    ActorSetGPos 14, 18, 0, 45, 0
    ActorSetGPos 15, 19, 0, 46, 0
    ActorSetGPos 0, 18, 0, 16, 0
    VMJump L_0267

L_0248:
    VMStackPush EVENT_WORK_0x40f2
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0267
    ActorSetGPos 0, 18, 0, 16, 0

L_0267:
    VMCall L_0277
    VMHalt

Script_2:
    VMCall L_0277
    VMHalt

L_0277:
    WorkSetConst 0x8024, 0
    GameGetVersion 0x8024
    VMStackPush 0x8024
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02AE
    ObjInitWarpGPos 2, 0, 0, 0
    ObjInitWarpGPos 3, 0, 0, 0
    VMJump L_02D5

L_02AE:
    VMStackPush 0x8024
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D5
    ObjInitWarpGPos 1, 0, 0, 0
    ObjInitWarpGPos 0, 0, 0, 0

L_02D5:
    VMStackPush EVENT_WORK_0x40c6
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0302
    ObjInitWarpGPos 6, 0, 0, 0
    ObjInitWarpGPos 7, 0, 0, 0
    VMJump L_0343

L_0302:
    VMStackPush EVENT_WORK_0x4106
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_032F
    ObjInitWarpGPos 4, 0, 0, 0
    ObjInitWarpGPos 7, 0, 0, 0
    VMJump L_0343

L_032F:
    ObjInitWarpGPos 4, 0, 0, 0
    ObjInitWarpGPos 6, 0, 0, 0

L_0343:
    WorkSetConst 0x8024, 0
    VMReturn

Script_22:
    ActorsPauseAll
    DebugPrint 8
    ActorWalkRoute 255, 12, 31, 1, 8, 1
    ActorWalkRoute 0, 13, 32, 1, 8, 1
    ActorWalkRoute 1, 14, 31, 1, 8, 1
    ActorCmdWait
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 0, Movement_1408
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: This ship...[f000]븁\u0000\nWhy is it so cold?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ShipWhyCold, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_13E4
    ActorCmdWait
    // "Cheren: And...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_Cheren, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_063C
    ActorCmdWait
    // "It seems like it's disguised\nas an old sailboat...[f000]븀\u0000\nBut that can't be it, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_SeemsLikeItsDisguised, 1, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 13, 29, 1, 8, 1
    VMSleep 16
    ActorCmdExec 0, Movement_13D4
    ActorCmdExec 1, Movement_13D4
    ActorCmdExec 255, Movement_13D4
    ActorCmdWait
    ActorCmdWait
    // "Team Plasma: Why should you care?[f000]븁\u0000\nBy the time we're done with you,\nit won't matter anyway![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaWhyShould, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 5, 12, 27, 1, 8, 1
    ActorWalkRoute 6, 11, 27, 1, 8, 1
    ActorWalkRoute 11, 14, 28, 1, 8, 1
    ActorWalkRoute 12, 13, 26, 1, 8, 1
    ActorWalkRoute 13, 14, 27, 1, 8, 1
    ActorCmdWait
    WordSetLoadRivalName 1
    BGMPlay SEQ_BGM_SW_D_27_F_AJITO
    FlagReset EVENT_FLAG_0x0a00
    BGMAmbienceResume
    // "[f000]Ā\u0001\u0001: You guys...\nSeriously, this is Team Plasma's base?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_GuysSeriouslyTeamPlasmas, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 4, 14, 33, 1, 8, 1
    VMSleep 32
    ActorCmdExec 1, Movement_13F4
    VMSleep 8
    ActorCmdExec 0, Movement_13F4
    ActorCmdExec 255, Movement_13F4
    ActorCmdWait
    // "Team Plasma: Correct answer![f000]븁\u0000\nSo that's why, as you can see...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaCorrectAnswer, 4, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 8, 12, 35, 1, 8, 1
    ActorWalkRoute 7, 13, 35, 1, 8, 1
    ActorWalkRoute 3, 14, 36, 1, 8, 1
    ActorWalkRoute 9, 11, 36, 1, 8, 1
    ActorWalkRoute 10, 15, 35, 1, 8, 1
    ActorCmdWait
    // "There are so many of us here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ThereManyUsHere, 4, 0, 0
    MsgWinCloseAll
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Is this all?\nI think you need more backup.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_AllThinkNeedMore, 0, 0, 0
    MsgWinCloseAll
    // "Team Plasma: Ha ha ha! Oooh, I'm\nso scared, big tough guy![f000]븁\u0000\nOK!\nPulverize these punks![f000]븀\u0000\nPlasmaaaa![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaHaHa, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_13D4
    ActorCmdExec 1, Movement_13E4
    ActorCmdExec 255, Movement_13EC
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Hey, just to let you know...[f000]븁\u0000\nI'm about to unleash my rage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_HeyJustLetKnow, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0678
    ActorCmdExec 4, Movement_0688
    ActorCmdExec 0, Movement_06A4
    ActorCmdExec 1, Movement_0694
    ActorCmdExec 255, Movement_13DC
    VMSleep 16
    ActorCmdExec 13, Movement_13E4
    ActorCmdExec 10, Movement_13E4
    ActorCmdExec 11, Movement_13E4
    ActorCmdWait
    // "Team Plasma: You're first![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaYoureFirst, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_13A4
    ActorCmdExec 255, Movement_13E4
    ActorCmdWait
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_5, 0, 0
    VMCall L_1268
    ActorCmdExec 2, Movement_06AC
    ActorCmdWait
    // "Team Plasma: Next! Next!\nHurry up already![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaNextNext, 2, 0, 0
    MsgWinCloseAll
    // "Team Plasma: It's my turn!\nActually, it's Trubbish time![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaItsTurn, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_13AC
    ActorCmdExec 255, Movement_13EC
    ActorCmdWait
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_6, 0, 0
    VMCall L_1268
    // "Team Plasma: What's with this Trainer?![f000]븁\u0000\nThis reminds me of that Trainer who\nmessed with us two years ago...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaWhatsTrainer, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_06BC
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40f0, 2
    HollowRivalCmd_0262 0, 0
    HollowRivalCmd_0262 1, 16
    HollowRivalCmd_0262 2, 4
    HollowRivalCmd_0262 4, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_063C:
    Move 63, 1
    Move 32, 1
    Move 63, 1
    Move 33, 1
    Move 63, 1
    Move 34, 1
    MoveEnd
    Move 13, 9
    MoveEnd
    Move 13, 8
    MoveEnd
    Move 12, 9
    MoveEnd
    Move 12, 8
    MoveEnd

Movement_0678:
    Move 14, 3
    Move 13, 2
    Move 35, 1
    MoveEnd

Movement_0688:
    Move 12, 2
    Move 34, 1
    MoveEnd

Movement_0694:
    Move 12, 2
    Move 14, 3
    Move 12, 1
    MoveEnd

Movement_06A4:
    Move 13, 2
    MoveEnd

Movement_06AC:
    Move 71, 1
    Move 14, 1
    Move 72, 1
    MoveEnd

Movement_06BC:
    Move 71, 1
    Move 15, 1
    Move 72, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    WordSetLoadRivalName 1
    WordSetPlayerName 0
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0701
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0001: This is a piece of cake!\nGo back up Cheren!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_PieceCakeGoBack, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08AF

L_0701:
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0852
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0001: I'm gonna end this,\nso back me up![f000]븁\u0000\nOK, Plasma punk, are YOU the thief\nwho stole a Purrloin in Aspertia[f000]븀\u0000\nfive years back?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ImGonnaEndBack, 0, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 34
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_075D
    ActorCmdExec 255, Movement_08B8
    VMJump L_0796

L_075D:
    VMStackPush 0x8022
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 33
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_078E
    ActorCmdExec 255, Movement_08C8
    VMJump L_0796

L_078E:
    ActorCmdExec 255, Movement_13DC

L_0796:
    ActorCmdExec 7, Movement_13B4
    ActorCmdExec 8, Movement_13B4
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07CB
    CallTrainerMultiBattle 368, 376, 377, 0
    VMJump L_07F8

L_07CB:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07EE
    CallTrainerMultiBattle 369, 376, 377, 0
    VMJump L_07F8

L_07EE:
    CallTrainerMultiBattle 370, 376, 377, 0

L_07F8:
    VMCall L_1268
    WordSetLoadRivalName 1
    WordSetPlayerName 0
    VMCall L_0A60
    WorkSetConst EVENT_WORK_0x40f0, 4
    WorkSetConst EVENT_WORK_0x40c6, 4
    FlagSet EVENT_FLAG_0x033e
    FlagSet EVENT_FLAG_0x033f
    FlagSet EVENT_FLAG_0x033d
    FlagSet EVENT_FLAG_0x0341
    FlagSet EVENT_FLAG_0x0342
    FlagReset EVENT_FLAG_0x02c7
    FlagReset EVENT_FLAG_0x02c8
    FlagSet EVENT_FLAG_0x02c6
    MapReplaceSetEvent 4, 0, 0
    RTReserveScript 3
    MapChangeWarp ZONE_PWT, 197, 491, 0
    VMJump L_08AF

L_0852:
    VMStackPush EVENT_WORK_0x40f1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0895
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: Take care of that guy!\nHe's no problem for you, right?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TakeCareGuyHes, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_08AF

L_0895:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: I'm fine...\nLet's try to find a way inside."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ImFineLetsTry, 0, 0
    LastKeyWait
    ActorMsgClose

L_08AF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_08B8:
    Move 12, 1
    Move 14, 2
    Move 13, 1
    MoveEnd

Movement_08C8:
    Move 14, 1
    Move 13, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A28
    SEPlay SEQ_SE_MESSAGE
    // "Team Plasma: Tch!\nIt's two against one now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaTchIts, 5, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0926
    ActorCmdExec 1, Movement_13DC
    VMJump L_094F

L_0926:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0947
    ActorCmdExec 1, Movement_13E4
    VMJump L_094F

L_0947:
    ActorCmdExec 1, Movement_13EC

L_094F:
    ActorCmdWait
    // "Cheren: [f000]Ā\u0001\u0000![f000]븁\u0000\nI'd like to show them what the two of us\ncan do, so could you help me out?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_CherenIdLikeShow, 1, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 28
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0996
    ActorCmdExec 255, Movement_0A44
    VMJump L_09CF

L_0996:
    VMStackPush 0x8022
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 29
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_09C7
    ActorCmdExec 255, Movement_0A54
    VMJump L_09CF

L_09C7:
    ActorCmdExec 255, Movement_13D4

L_09CF:
    ActorCmdExec 5, Movement_13BC
    ActorCmdExec 6, Movement_13BC
    ActorCmdExec 1, Movement_13D4
    ActorCmdWait
    CallTrainerMultiBattle 371, 374, 375, 0
    VMCall L_1268
    WordSetPlayerName 0
    ActorCmdExec 255, Movement_13E4
    ActorCmdExec 1, Movement_13EC
    ActorCmdWait
    // "Cheren: Thanks!\nYou've really become much stronger![f000]븁\u0000\nHelp out [f000]Ā\u0001\u0001\nnext, OK?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_CherenThanksYouveReally, 1, 0, 0
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40f0, 3
    VMJump L_0A3C

L_0A28:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Cheren: There are so many of them.\nWhat a bother!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_CherenThereManyThem, 0, 0
    LastKeyWait
    ActorMsgClose

L_0A3C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0A44:
    Move 13, 1
    Move 15, 2
    Move 12, 1
    MoveEnd

Movement_0A54:
    Move 15, 1
    Move 12, 1
    MoveEnd

L_0A60:
    WordSetLoadRivalName 1
    WordSetPlayerName 0
    MultiMsg 63, 6, 20, 1
    VMSleep 26
    MultiMsg 64, 4, 11, 2
    VMSleep 26
    MsgWinCloseNo 1
    MultiMsg 65, 13, 2, 3
    VMSleep 26
    MsgWinCloseNo 2
    MultiMsg 66, 18, 14, 4
    VMSleep 26
    MsgWinCloseNo 3
    VMSleep 26
    MsgWinCloseNo 4
    // "[f000]Ā\u0001\u0001: That's the last of 'em![f000]븁\u0000\nWell now... Why don't you tell me\neverything you know about Purrloin?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ThatsLastEmWell, 0, 3, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_13E4
    ActorCmdWait
    // "Cheren: Good idea...[f000]븁\u0000\nThere are a lot of things I want\nto ask about as well.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_CherenGoodIdeaThere, 1, 5, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x0341
    FlagReset EVENT_FLAG_0x0342
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 16
    SEWait
    BGMPlay SEQ_BGM_E_7_SAGE
    // "???: What is going on?[f000]븁\u0000"
    InfoMsg PlasmaFrigate_Text_WhatGoing, 1
    MsgWinCloseAll
    ActorCmdExec 1, Movement_13D4
    ActorWalkRoute 16, 13, 30, 1, 8, 1
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xd8000, 0, 0x1f9000, 30
    ActorCmdExec 5, Movement_0BF8
    ActorCmdExec 12, Movement_0C10
    VMSleep 10
    ActorCmdExec 6, Movement_13EC
    ActorCmdExec 11, Movement_13E4
    ActorCmdExec 13, Movement_13E4
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 16, Movement_0C24
    ActorCmdWait
    // "Cheren: Oh, good grief...[f000]븁\u0000\nYou're the person who was\nshivering in the Cold Storage, right?[f000]븁\u0000\nWasn't your name Zinzolin?[f000]븁\u0000\nYou're going to have to tell me what\nyou're planning to do with this ship.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_CherenOhGoodGrief, 1, 6, 0
    MsgWinCloseAll
    // "Zinzolin: YOU![f000]븁\u0000\nOnce again, we will use the\nlegendary Dragon-type Pokémon[f000]븀\u0000\nand we will rule the Unova region![f000]븁\u0000\nCurious Trainers, we shall not\nlet you run around as you please![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ZinzolinOnceAgainWe, 16, 5, 0
    MsgWinCloseAll
    ActorCmdExec 16, Movement_0C30
    ActorCmdWait
    // "Shadow Triad!\nGet them out of here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ShadowTriadGetThem, 16, 5, 0
    MsgWinCloseAll
    BGMChangeMap
    EvCameraMoveToDefault 24
    ActorAdd 17
    ActorCmdExec 17, Movement_0C50
    ActorAdd 18
    ActorAdd 19
    ActorCmdExec 18, Movement_0C50
    ActorCmdExec 19, Movement_0C50
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    // "Shadow Triad: By the way,\nwe are not your...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ShadowTriadByWay, 17, 4, 0
    MsgWinCloseAll
    ActorCmdExec 16, Movement_13DC
    ActorCmdWait
    // "Zinzolin: I know!\nBut kick them out of here now![f000]븁\u0000"
    InfoMsg PlasmaFrigate_Text_ZinzolinKnowButKick, 1
    MsgWinCloseAll
    // "Shadow Triad: Move.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ShadowTriadMove, 17, 4, 0
    MsgWinCloseAll
    VMReturn
    .balign 4, 0

Movement_0BF8:
    Move 13, 1
    Move 3, 1
    Move 71, 1
    Move 14, 1
    Move 72, 1
    MoveEnd

Movement_0C10:
    Move 2, 1
    Move 71, 1
    Move 15, 1
    Move 72, 1
    MoveEnd

Movement_0C24:
    Move 75, 1
    Move 159, 1
    MoveEnd

Movement_0C30:
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 34, 1
    Move 35, 1
    Move 34, 1
    Move 35, 1
    MoveEnd

Movement_0C50:
    Move 184, 1
    MoveEnd
    Move 185, 1
    Move 69, 1
    MoveEnd

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: Go! Go![f000]븁\u0000\nSomebody avenge me\nand take these guys out!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaGoGo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: What's with this Trainer?![f000]븁\u0000\nThis reminds me of that Trainer who\nmessed with us two years ago..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaWhatsTrainer_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CCB
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: The Gym Leader is\nirritating, but you...[f000]븀\u0000\nYou are a horrible little brat!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaGymLeader, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0CDF

L_0CCB:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: Hey, I remember you!\nYou're gonna pay for Virbank![f000]븁\u0000\nYou and the Gym Leader are going\ndown, so you'd better brace yourself!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaHeyRemember, 0, 0
    LastKeyWait
    ActorMsgClose

L_0CDF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: Tch!\nIt's two against one now![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaTchIts, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Team Plasma: This guy...\nMore than strong, he's scary![f000]븀\u0000\nHe's really cheesed off!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaGuyMore, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: Look around you![f000]븁\u0000\nSee how many of us there are?\nJust surrender already!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaLookAround, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: Ooogz...\nStrong for such a little brat!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaOoogzStrong, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: That little punk\nwas too intense![f000]븁\u0000\nEspecially for someone whose head\nlooks like a Qwilfish!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaLittlePunk, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: I'm not losing![f000]븁\u0000\nWell, actually, I haven't\nreceived a Pokémon yet!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaImNot, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: What's this?[f000]븁\u0000\nAre the Gym Leaders\ngoing to interfere with us again?"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaWhatsGym, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: I didn't stand a chance...[f000]븁\u0000\nAnd I bullied--I mean trained--my\nPokémon for two whole years, too!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaDidntStand, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: Sniff...\nOur plans might fail again..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaSniffOur, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Intruders!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_Intruders, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "For now, let them through!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_NowLetThemThrough, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_27:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Don't try to get out of your\nwatch with stupid talk like that!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_DontTryGetOut, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_28:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "And, um, did you find the swimsuit\nguy who lowered the gangplank?"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_UmDidFindSwimsuit, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 0, Movement_13E4
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Take care of that guy!\nHe's no problem for you, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TakeCareGuyHes, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_13EC
    ActorWalkRoute 14, 10, 30, 1, 8, 1
    ActorCmdWait
    // "Team Plasma: You![f000]븁\u0000\nYou must be friends of Smiley Swimsuit,\nthat guy who lowered the gangplank![f000]븁\u0000\nAaargh! You even got the roadblock\nCrustle out of the way![f000]븁\u0000\nHere I come!\nPlasmaaaa![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaMustFriends, 14, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_40, 0, 0
    VMCall L_1268
    ActorCmdExec 14, Movement_0FC0
    ActorCmdWait
    // "Team Plasma: Oh no!\nThe hatch is still open![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaOhNo, 14, 0, 0
    MsgWinCloseAll
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F04
    ActorWalkRoute 14, 13, 20, 1, 4, 0
    ActorWalkRoute 15, 13, 20, 1, 4, 0
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40f2, 2
    ActorDelete 14
    ActorDelete 15
    ActorDelete 23
    ActorDelete 24
    VMJump L_0F40

L_0F04:
    ActorWalkRoute 14, 13, 38, 1, 4, 0
    ActorWalkRoute 15, 14, 38, 1, 4, 0
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40f1, 2
    ActorSetGPos 14, 18, 0, 45, 0
    ActorSetGPos 15, 19, 0, 46, 0

L_0F40:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 0, Movement_13E4
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: OK!\nThis time we'll check out the ship![f000]븁\u0000\nWhen we were in Driftveil City,\nZinzolin came from this direction![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_OkTimeWellCheck, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 12, 20, 1, 4, 0
    ActorCmdExec 255, Movement_13D4
    ActorCmdWait
    ActorSetGPos 0, 18, 0, 16, 0
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F99
    FlagSet EVENT_FLAG_0x0340

L_0F99:
    HollowRivalCmd_0262 0, 6
    HollowRivalCmd_0262 1, 31
    HollowRivalCmd_0262 2, 7
    HollowRivalCmd_0262 3, 7
    HollowRivalCmd_0262 4, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0FC0:
    Move 71, 1
    Move 15, 1
    Move 72, 1
    Move 63, 1
    Move 75, 1
    MoveEnd
    Move 15, 4
    Move 12, 10
    Move 15, 2
    Move 12, 4
    Move 15, 3
    Move 12, 1
    MoveEnd
    Move 12, 1
    Move 15, 4
    Move 12, 10
    Move 15, 2
    Move 12, 3
    Move 15, 3
    Move 32, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 16
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_104B
    ActorCmdExec 0, Movement_13E4
    VMJump L_1053

L_104B:
    ActorCmdExec 0, Movement_13EC

L_1053:
    ActorCmdWait
    // "This is the place...[f000]븁\u0000\nOK! I'm going![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_PlaceOkImGoing, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_10A8
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    ActorWalkRoute 255, 18, 15, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_139C
    ActorCmdWait
    RTReserveScript 6
    MapChangeWarp ZONE_PLASMA_FRIGATE_12, 11, 15, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_10A8:
    Move 12, 2
    MoveEnd

Script_5:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 16
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10DF
    ActorCmdExec 0, Movement_13E4
    VMJump L_10E7

L_10DF:
    ActorCmdExec 0, Movement_13EC

L_10E7:
    ActorCmdWait
    // "This is the place...[f000]븁\u0000\nOK! I'm going![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_PlaceOkImGoing, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_10A8
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    ActorWalkRoute 255, 18, 15, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_139C
    ActorCmdWait
    RTReserveScript 9
    MapChangeWarp ZONE_PLASMA_FRIGATE_2, 11, 15, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorCmdExec 22, Movement_1410
    ActorCmdWait
    // "Zzz...\nZzz...[f000]븁\u0000\nLooks like he's asleep."
    InfoMsg PlasmaFrigate_Text_ZzzZzzLooksLike, 1
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_29:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ae9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_121C
    VMStackPushFlag EVENT_FLAG_0x01ed
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_119A
    // "Even the same Pokémon\ncan exhibit different strengths[f000]븀\u0000\ndepending on the Trainer![f000]븁\u0000\nIt's so interesting!\nVery, very interesting![f000]븁\u0000\nWell, now!\nGive me but a moment of your time!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_EvenSamePokemonCan, 0, 0
    FlagSet EVENT_FLAG_0x01ed
    VMJump L_11A4

L_119A:
    // "Well, now!\nGive me but a moment of your time!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_WellNowGiveBut, 0, 0

L_11A4:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1208
    // "The invisible force that exists between\nyou and Pokémon...[f000]븀\u0000\nThis time I will determine what it is![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_InvisibleForceExistsBetween, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_COLRESS_2, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11EE
    CallTrainerBattleEnd
    VMJump L_11F0

L_11EE:
    CallTrainerLose

L_11F0:
    // "The things you and your Pokémon\nhave seen and felt...[f000]븀\u0000\nDo they belong to you and you alone?[f000]븁\u0000\nIf you would, please have another\nPokémon battle with me.[f000]븁\u0000\nBy facing you, I feel as if I can see\nwhat I should do from now on."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ThingsPokemonHaveSeen, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ae9
    VMJump L_1216

L_1208:
    // "And I went to all the\ntrouble to come here, too...[f000]븀\u0000\nYou're an unkind person."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_WentAllTroubleCome, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_1216:
    VMJump L_122A

L_121C:
    // "The things you and your Pokémon\nhave seen and felt...[f000]븀\u0000\nDo they belong to you and you alone?[f000]븁\u0000\nIf you would, please have another\nPokémon battle with me.[f000]븁\u0000\nBy facing you, I feel as if I can see\nwhat I should do from now on."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_ThingsPokemonHaveSeen, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_122A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: The sky sure is vast."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaSkySure, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_26:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: I knew...[f000]븁\u0000\nI knew that Ghetsis\nwas using me...[f000]븁\u0000\nBut I had friends here..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate_Text_TeamPlasmaKnewKnew, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_1268:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1371
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1298
    VMCall L_1375

L_1298:
    VMStackPush EVENT_WORK_0x40f0
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1369
    VMCall L_1375
    ActorSetGPos 2, 10, 0, 30, 3
    ActorSetGPos 4, 10, 0, 32, 3
    ActorSetGPos 5, 12, 0, 27, 1
    ActorSetGPos 6, 11, 0, 27, 1
    ActorSetGPos 7, 13, 0, 35, 0
    ActorSetGPos 8, 12, 0, 35, 0
    ActorSetGPos 3, 14, 0, 36, 0
    ActorSetGPos 9, 11, 0, 36, 0
    ActorSetGPos 10, 15, 0, 35, 0
    ActorSetGPos 11, 14, 0, 28, 1
    ActorSetGPos 12, 13, 0, 26, 1
    ActorSetGPos 13, 14, 0, 27, 1
    ActorSetGPos 0, 12, 0, 32, 0
    ActorSetGPos 1, 14, 0, 32, 0
    ActorSetGPos 255, 13, 0, 33, 0
    FlagSet EVENT_FLAG_0x033f

L_1369:
    CallTrainerBattleEnd
    VMJump L_1373

L_1371:
    CallTrainerLose

L_1373:
    VMReturn

L_1375:
    PokePartyGetCount 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1390
    PokePartyRecoverAll

L_1390:
    VMReturn
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_139C:
    Move 12, 1
    MoveEnd

Movement_13A4:
    Move 15, 1
    MoveEnd

Movement_13AC:
    Move 14, 1
    MoveEnd

Movement_13B4:
    Move 0, 1
    MoveEnd

Movement_13BC:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_13D4:
    Move 32, 1
    MoveEnd

Movement_13DC:
    Move 33, 1
    MoveEnd

Movement_13E4:
    Move 34, 1
    MoveEnd

Movement_13EC:
    Move 35, 1
    MoveEnd

Movement_13F4:
    Move 33, 1
    Move 75, 1
    MoveEnd
    Move 75, 1
    MoveEnd

Movement_1408:
    Move 159, 1
    MoveEnd

Movement_1410:
    Move 161, 1
    MoveEnd
