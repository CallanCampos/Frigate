#include "asm/field_script.inc"
#include "text/script/plasma_frigate_13.h"

// Script plugin 12, from the zones that use this file

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
    ScriptEntry Script_30
    ScriptEntry Script_31
    ScriptEntry Script_32
    ScriptEntry Script_33
    ScriptEntry Script_34
    ScriptEntry Script_35
    ScriptEntry Script_36
    ScriptEntry Script_37
    ScriptEntry Script_38
    ScriptEntry Script_39
    ScriptEntry Script_40
    ScriptEntry Script_41
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

Script_1:
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E7
    FlagSet EVENT_FLAG_0x034d

L_00E7:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMStackPush EVENT_WORK_0x4105
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0118
    ActorSetGPos 10, 15, 0, 16, 0

L_0118:
    VMCall L_0128
    VMHalt

Script_3:
    VMCall L_0128
    VMHalt

L_0128:
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0147
    Plugin12_Cmd1001 0, 0
    VMJump L_014D

L_0147:
    Plugin12_Cmd1001 0, 1

L_014D:
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016C
    Plugin12_Cmd1001 1, 0
    VMJump L_0172

L_016C:
    Plugin12_Cmd1001 1, 1

L_0172:
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0191
    Plugin12_Cmd1001 2, 0
    VMJump L_0197

L_0191:
    Plugin12_Cmd1001 2, 1

L_0197:
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B6
    Plugin12_Cmd1001 3, 0
    VMJump L_01BC

L_01B6:
    Plugin12_Cmd1001 3, 1

L_01BC:
    VMReturn

Script_16:
    ActorsPauseAll
    ActorCmdExec 9, Movement_0C0C
    ActorCmdExec 255, Movement_0C04
    ActorCmdWait
    ActorCmdExec 9, Movement_0BC4
    ActorCmdWait
    // "Team Plasma: At last, we meet again![f000]븁\u0000\nRemember me? Formerly of Team Plasma?\nI've been waiting for you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_TeamPlasmaLastWe, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0BF4
    ActorCmdWait
    // "Gah! Spying is such a rotten job.\nI contacted Rood of the Seven Sages,[f000]븀\u0000\nbut nobody showed up to help me.[f000]븁\u0000\nI can't stand it. Everybody around me is\nalways saying bad stuff about Lord N.[f000]븁\u0000\nBut Lord N learned the error of his ways\nand changed course to a better path.[f000]븁\u0000\nYet they call him a betrayer!\nPeople really stink sometimes.[f000]븁\u0000\nThey selfishly counted on him, and\nnow they selfishly make a big fuss[f000]븀\u0000\nabout being betrayed.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_GahSpyingSuchRotten, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0BFC
    ActorCmdWait
    // "Oh! I nearly forgot to tell you!\nThis floor is a maze of pipes.[f000]븁\u0000\nYou've got to step on switches\nto connect or disconnect the pipes.[f000]븁\u0000\nIt's a good thing you can walk\non the pipes.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_OhNearlyForgotTell, 9, 0, 0
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4103, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You know what troubles me...[f000]븁\u0000\nHow come all of us who were in\nTeam Plasma together--thinking we[f000]븀\u0000\nknew what was right--are now divided[f000]븀\u0000\ninto former Team Plasma members and[f000]븀\u0000\ncontinuing Team Plasma members,[f000]븀\u0000\nboth with opposing points of view?[f000]븁\u0000\nWhere's the line between friend and foe?\nI spend a lot of time asking myself that.[f000]븁\u0000\nAs for you, on this floor, you'll need\nto deactivate barriers to continue on.[f000]븀\u0000\nDeactivate them by stepping on switches.[f000]븁\u0000\nMove around by stepping on warp panels,\nand you'll find four switches."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_KnowWhatTroublesHow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_41:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Zinzolin: Beaten again?![f000]븁\u0000\nNo matter!\nTeam Plasma will get the last laugh!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_ZinzolinBeatenAgainNo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x034b
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0xaf000, 28
    EvCameraWait
    // "Zinzolin: The device is indestructible![f000]븁\u0000\nYou will never be able to release Kyurem![f000]븁\u0000"
    InfoMsg PlasmaFrigate13_Text_ZinzolinDeviceIndestructibleWill, 2
    MsgWinCloseAll
    ActorAdd 10
    BGMPlayPush SEQ_BGM_E_7_SAGE
    PlayerGetGPos 0x8022, 0x8023
    WorkAdd 0x8023, 2
    ActorWalkRoute 10, 0x8022, 0x8023, 1, 16, 0
    ActorCmdExec 255, Movement_0BF4
    EvCameraMoveToDefault 20
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdWait
    // "You don't have the sense to know\nwhen to quit, it seems.[f000]븁\u0000\nIt's an act of mercy on my part\nto bring this to an end now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_DontHaveSenseKnow, 10, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_ZINZOLIN_2, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F4
    CallTrainerBattleEnd
    VMJump L_02FA

L_02F4:
    FlagSet EVENT_FLAG_0x034b
    CallTrainerLose

L_02FA:
    ActorCmdExec 10, Movement_0328
    ActorCmdWait
    // "Zinzolin: Beaten again?![f000]븁\u0000\nNo matter!\nTeam Plasma will get the last laugh!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_ZinzolinBeatenAgainNo, 10, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4105, 1
    WorkSetConst EVENT_WORK_0x40f3, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0328:
    Move 71, 1
    Move 9, 1
    Move 72, 1
    MoveEnd

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It looks like it controls the\ntemperature inside the ship."
    SystemMsg PlasmaFrigate13_Text_LooksLikeControlsTemperature, 2
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0361
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0365

L_0361:
    LastKeyWait
    MsgWinCloseAll

L_0365:
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0389
    PVPlay 646, 0
    // "Haahra..."
    InfoMsg PlasmaFrigate13_Text_Haahra, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll

L_0389:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a device to control\nthe ship's energy system."
    SystemMsg PlasmaFrigate13_Text_ItsDeviceControlShips, 2
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B8
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_03BC

L_03B8:
    LastKeyWait
    MsgWinCloseAll

L_03BC:
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E0
    PVPlay 646, 0
    // "Haahra..."
    InfoMsg PlasmaFrigate13_Text_Haahra, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll

L_03E0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    ActorCmdExec 11, Movement_0C0C
    ActorCmdWait
    ActorCmdExec 11, Movement_0BF4
    ActorCmdExec 255, Movement_0BEC
    ActorCmdWait
    // "Humph! If you intend to continue,\nstep on the other warp panel.[f000]븁\u0000\nKeep in mind that you're going\nto get beaten up if you do!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_HumphIfIntendContinue, 11, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 11, Movement_0C04
    ActorCmdExec 255, Movement_0BBC
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    ActorCmdExec 11, Movement_0C0C
    ActorCmdWait
    ActorCmdExec 11, Movement_0BF4
    ActorCmdExec 255, Movement_0BEC
    ActorCmdWait
    // "What?! You beat Colress?![f000]븁\u0000\nWaaah! I pretended to be strong,\nbut I don't have any Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_WhatBeatColressWaaah, 11, 0, 0
    MsgWinCloseAll
    VMCall L_04BB
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_26:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4125
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0493
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Humph! If you intend to continue,\nstep on the other warp panel.[f000]븁\u0000\nKeep in mind that you're going\nto get beaten up if you do!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_HumphIfIntendContinue, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04B5

L_0493:
    SEPlay SEQ_SE_MESSAGE
    ActorCmdExec 11, Movement_0C0C
    ActorCmdWait
    // "What?! You beat Colress?![f000]븁\u0000\nWaaah! I pretended to be strong,\nbut I don't have any Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_WhatBeatColressWaaah, 11, 0, 0
    MsgWinCloseAll
    VMCall L_04BB

L_04B5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_04BB:
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E2
    ActorCmdExec 11, Movement_0510
    VMJump L_04F0

L_04E2:
    ActorWalkRoute 11, 13, 19, 0, 4, 0

L_04F0:
    VMSleep 8
    ActorCmdExec 255, Movement_0C04
    ActorCmdWait
    ActorDelete 11
    FlagSet EVENT_FLAG_0x0349
    WorkSetConst EVENT_WORK_0x4125, 2
    VMReturn
    .balign 4, 0

Movement_0510:
    Move 17, 1
    Move 19, 5
    Move 17, 7
    MoveEnd

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, poor Kyurem. Ghetsis's device\ncruelly forced it to work.[f000]븁\u0000\nIt must have felt terrible."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_OhPoorKyuremGhetsiss, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Plasma Frigate is a ship designed\nto be ecological. It runs on[f000]븀\u0000\nKyurem's ice energy and solar panels.[f000]븁\u0000\nYou've got to keep the environment in\nmind when ruling a region like Unova."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_PlasmaFrigateShipDesigned, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Colress doesn't know N.\nI wonder how he'll react if he meets him."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate13_Text_ColressDoesntKnowN, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    PlayerGetDir 0x8021
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_3, 11, 5, 32801
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    PlayerGetDir 0x8021
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_4, 7, 14, 32801
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    PlayerGetDir 0x8021
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_6, 8, 9, 32801
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f5, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "A barrier was deactivated!"
    InfoMsg PlasmaFrigate13_Text_BarrierDeactivated, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0612
    MsgWaitAdvance
    VMJump L_0614

L_0612:
    LastKeyWait

L_0614:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1003 0
    Plugin12_Cmd1002 0
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_066C
    // "All barriers were deactivated,\nand you can proceed now."
    InfoMsg PlasmaFrigate13_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_066C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f6, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "A barrier was deactivated!"
    InfoMsg PlasmaFrigate13_Text_BarrierDeactivated, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06CE
    MsgWaitAdvance
    VMJump L_06D0

L_06CE:
    LastKeyWait

L_06D0:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1003 1
    Plugin12_Cmd1002 1
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0728
    // "All barriers were deactivated,\nand you can proceed now."
    InfoMsg PlasmaFrigate13_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_0728:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f7, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "A barrier was deactivated!"
    InfoMsg PlasmaFrigate13_Text_BarrierDeactivated, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_078A
    MsgWaitAdvance
    VMJump L_078C

L_078A:
    LastKeyWait

L_078C:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1003 2
    Plugin12_Cmd1002 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07E4
    // "All barriers were deactivated,\nand you can proceed now."
    InfoMsg PlasmaFrigate13_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_07E4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f8, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "A barrier was deactivated!"
    InfoMsg PlasmaFrigate13_Text_BarrierDeactivated, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0846
    MsgWaitAdvance
    VMJump L_0848

L_0846:
    LastKeyWait

L_0848:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1003 3
    Plugin12_Cmd1002 3
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_08A0
    // "All barriers were deactivated,\nand you can proceed now."
    InfoMsg PlasmaFrigate13_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_08A0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4000, 1
    WorkSetConst EVENT_WORK_0x4001, 0
    WorkSetConst EVENT_WORK_0x4002, 0
    WorkSetConst EVENT_WORK_0x4003, 0
    Plugin12_Cmd1000 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4001, 1
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst EVENT_WORK_0x4002, 0
    WorkSetConst EVENT_WORK_0x4003, 0
    Plugin12_Cmd1000 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4002, 1
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst EVENT_WORK_0x4001, 0
    WorkSetConst EVENT_WORK_0x4003, 0
    Plugin12_Cmd1000 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4003, 1
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst EVENT_WORK_0x4001, 0
    WorkSetConst EVENT_WORK_0x4002, 0
    Plugin12_Cmd1000 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0955
    CallTrainerBattleEnd
    VMJump L_0957

L_0955:
    CallTrainerLose

L_0957:
    VMReturn

Script_11:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0B8C
    ActorCmdWait
    SEPlay SEQ_SE_SW_PLAZMASHIP_08
    ActorCmdExec 255, Movement_0B98
    ActorCmdWait
    SEWait
    // "Be careful!\nThe barriers are electrified!"
    InfoMsg PlasmaFrigate13_Text_CarefulBarriersElectrified, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_27:
    ActorsPauseAll
    WorkSetConst 0x8024, 11
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 48
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_28:
    ActorsPauseAll
    WorkSetConst 0x8024, 7
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 16
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_29:
    ActorsPauseAll
    WorkSetConst 0x8024, 5
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 41
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_30:
    ActorsPauseAll
    WorkSetConst 0x8024, 7
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 21
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_31:
    ActorsPauseAll
    WorkSetConst 0x8024, 25
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 41
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_32:
    ActorsPauseAll
    WorkSetConst 0x8024, 23
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 16
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_33:
    ActorsPauseAll
    WorkSetConst 0x8024, 19
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 48
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_34:
    ActorsPauseAll
    WorkSetConst 0x8024, 23
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 21
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_35:
    ActorsPauseAll
    WorkSetConst 0x8024, 5
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 54
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_36:
    ActorsPauseAll
    WorkSetConst 0x8024, 11
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 41
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_37:
    ActorsPauseAll
    WorkSetConst 0x8024, 25
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 54
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_38:
    ActorsPauseAll
    WorkSetConst 0x8024, 11
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 54
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_39:
    ActorsPauseAll
    WorkSetConst 0x8024, 19
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 54
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_40:
    ActorsPauseAll
    WorkSetConst 0x8024, 19
    WorkSetConst 0x8025, 65533
    WorkSetConst 0x8026, 41
    VMCall L_0B44
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0B44:
    WorkSetConst 0x8027, 0
    DebugPrint 0x8024
    DebugPrint 0x8026
    SEPlay SEQ_SE_SW_PLAZMASHIP_10
    PlayerGetDir 0x8027
    FadeEx 3, 0, 16, 2
    FadeExWait
    VMSleep 6
    ActorSetGPos 255, 0x8024, 0x8025, 0x8026, 0x8027
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkSetConst 0x8027, 0
    VMReturn
    .balign 4, 0

Movement_0B8C:
    Move 0, 1
    Move 75, 1
    MoveEnd

Movement_0B98:
    Move 71, 1
    Move 36, 2
    Move 17, 2
    Move 72, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0BBC:
    Move 15, 1
    MoveEnd

Movement_0BC4:
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

Movement_0BEC:
    Move 32, 1
    MoveEnd

Movement_0BF4:
    Move 33, 1
    MoveEnd

Movement_0BFC:
    Move 34, 1
    MoveEnd

Movement_0C04:
    Move 35, 1
    MoveEnd

Movement_0C0C:
    Move 75, 1
    MoveEnd
