#include "asm/field_script.inc"
#include "text/script/plasma_frigate_10.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0091
    FlagSet EVENT_FLAG_0x034c

L_0091:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40f4
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMStackPush EVENT_WORK_0x4104
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00C2
    ActorSetGPos 10, 15, 0, 16, 0

L_00C2:
    VMCall L_00D2
    VMHalt

Script_3:
    VMCall L_00D2
    VMHalt

L_00D2:
    VMStackPush EVENT_WORK_0x40fa
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FD
    Plugin12_Cmd1001 0, 1
    Plugin12_Cmd1001 1, 1
    Plugin12_Cmd1001 2, 1
    Plugin12_Cmd1001 3, 1

L_00FD:
    VMReturn

Script_13:
    ActorsPauseAll
    ActorCmdExec 11, Movement_0754
    ActorCmdExec 255, Movement_074C
    ActorCmdWait
    ActorCmdExec 11, Movement_070C
    ActorCmdWait
    // "Team Plasma: At last, we meet again![f000]븁\u0000\nRemember me? Formerly of Team Plasma?\nI've been waiting for you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_TeamPlasmaLastWe, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_073C
    ActorCmdWait
    // "Gah! Spying is such a rotten job.\nI contacted Rood of the Seven Sages,[f000]븀\u0000\nbut nobody showed up to help me.[f000]븁\u0000\nI can't stand it. Everybody around me is\nalways saying bad stuff about Lord N.[f000]븁\u0000\nBut Lord N learned the error of his ways\nand changed course to a better path.[f000]븁\u0000\nYet they call him a betrayer!\nPeople really stink sometimes.[f000]븁\u0000\nThey selfishly counted on him, and\nnow they selfishly make a big fuss[f000]븀\u0000\nabout being betrayed.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_GahSpyingSuchRotten, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_0744
    ActorCmdWait
    // "Oh! I nearly forgot to tell you!\nThis floor is a maze of pipes.[f000]븁\u0000\nYou've got to step on switches\nto connect or disconnect the pipes.[f000]븁\u0000\nIt's a good thing you can walk\non the pipes.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_OhNearlyForgotTell, 11, 0, 0
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4102, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You know what troubles me...[f000]븁\u0000\nHow come all of us who were in\nTeam Plasma together--thinking we[f000]븀\u0000\nknew what was right--are now divided[f000]븀\u0000\ninto former Team Plasma members and[f000]븀\u0000\ncontinuing Team Plasma members,[f000]븀\u0000\nboth with opposing points of view?[f000]븁\u0000\nWhere's the line between friend and foe?\nI spend a lot of time asking myself that.[f000]븁\u0000\nAs for you, on this floor, you'll need\nto enter a password to continue on.[f000]븀\u0000\nAnd that password is...[f000]븁\u0000\nHa! You didn't think I was just going\nto tell you the password, did you?[f000]븁\u0000\nYou'll have to figure it out\nfrom the others!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_KnowWhatTroublesHow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Zinzolin: Beaten again?![f000]븁\u0000\nNo matter!\nTeam Plasma will get the last laugh!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_ZinzolinBeatenAgainNo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x034a
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0xaf000, 28
    EvCameraWait
    // "Zinzolin: The device is indestructible![f000]븁\u0000\nYou will never be able to release Kyurem![f000]븁\u0000"
    InfoMsg PlasmaFrigate10_Text_ZinzolinDeviceIndestructibleWill, 2
    MsgWinCloseAll
    ActorAdd 10
    BGMPlayPush SEQ_BGM_E_7_SAGE
    PlayerGetGPos 0x8022, 0x8023
    WorkAdd 0x8023, 2
    ActorWalkRoute 10, 0x8022, 0x8023, 1, 16, 0
    ActorCmdExec 255, Movement_073C
    EvCameraMoveToDefault 20
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdWait
    // "You don't have the sense to know\nwhen to quit, it seems.[f000]븁\u0000\nIt's an act of mercy on my part\nto bring this to an end now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_DontHaveSenseKnow, 10, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_ZINZOLIN_2, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0235
    CallTrainerBattleEnd
    VMJump L_023B

L_0235:
    FlagSet EVENT_FLAG_0x034a
    CallTrainerLose

L_023B:
    ActorCmdExec 10, Movement_0268
    ActorCmdWait
    // "Zinzolin: Beaten again?![f000]븁\u0000\nNo matter!\nTeam Plasma will get the last laugh!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_ZinzolinBeatenAgainNo, 10, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4104, 1
    WorkSetConst EVENT_WORK_0x40f3, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0268:
    Move 71, 1
    Move 9, 1
    Move 72, 1
    MoveEnd

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It looks like it controls the\ntemperature inside the ship."
    SystemMsg PlasmaFrigate10_Text_LooksLikeControlsTemperature, 2
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A1
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_02A5

L_02A1:
    LastKeyWait
    MsgWinCloseAll

L_02A5:
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C9
    PVPlay 646, 0
    // "Haahra..."
    InfoMsg PlasmaFrigate10_Text_Haahra, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll

L_02C9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a device to control\nthe ship's energy system."
    SystemMsg PlasmaFrigate10_Text_ItsDeviceControlShips, 2
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F8
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_02FC

L_02F8:
    LastKeyWait
    MsgWinCloseAll

L_02FC:
    VMStackPushFlag EVENT_FLAG_0x038d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0320
    PVPlay 646, 0
    // "Haahra..."
    InfoMsg PlasmaFrigate10_Text_Haahra, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll

L_0320:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    ActorCmdExec 9, Movement_0754
    ActorCmdWait
    ActorCmdExec 9, Movement_073C
    ActorCmdExec 255, Movement_0734
    ActorCmdWait
    // "Humph! If you intend to continue,\nstep on the other warp panel.[f000]븁\u0000\nKeep in mind that you're going\nto get beaten up if you do!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_HumphIfIntendContinue, 9, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 9, Movement_074C
    ActorCmdExec 255, Movement_0704
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    ActorCmdExec 9, Movement_0754
    ActorCmdWait
    ActorCmdExec 9, Movement_073C
    ActorCmdExec 255, Movement_0734
    ActorCmdWait
    // "What?! You beat Colress?![f000]븁\u0000\nWaaah! I pretended to be strong,\nbut I don't have any Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_WhatBeatColressWaaah, 9, 0, 0
    MsgWinCloseAll
    VMCall L_03FB
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4125
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Humph! If you intend to continue,\nstep on the other warp panel.[f000]븁\u0000\nKeep in mind that you're going\nto get beaten up if you do!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_HumphIfIntendContinue, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_03F5

L_03D3:
    SEPlay SEQ_SE_MESSAGE
    ActorCmdExec 9, Movement_0754
    ActorCmdWait
    // "What?! You beat Colress?![f000]븁\u0000\nWaaah! I pretended to be strong,\nbut I don't have any Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_WhatBeatColressWaaah, 9, 0, 0
    MsgWinCloseAll
    VMCall L_03FB

L_03F5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_03FB:
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0422
    ActorCmdExec 9, Movement_0450
    VMJump L_0430

L_0422:
    ActorWalkRoute 9, 13, 19, 0, 4, 0

L_0430:
    VMSleep 8
    ActorCmdExec 255, Movement_074C
    ActorCmdWait
    ActorDelete 9
    FlagSet EVENT_FLAG_0x0348
    WorkSetConst EVENT_WORK_0x4125, 2
    VMReturn
    .balign 4, 0

Movement_0450:
    Move 17, 1
    Move 19, 5
    Move 17, 7
    MoveEnd

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, poor Kyurem.\nIt was cruelly forced to work.[f000]븁\u0000\nIt must have felt terrible."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_OhPoorKyuremCruelly, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Plasma Frigate is a ship designed\nto be ecological. It runs on[f000]븀\u0000\nKyurem's ice energy and solar panels.[f000]븁\u0000\nYou've got to keep the environment in\nmind when ruling a region like Unova."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_PlasmaFrigateShipDesigned, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Colress doesn't know N.\nI wonder how he'll react if he meets him."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate10_Text_ColressDoesntKnowN, 0, 0
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

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x0164
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05AF
    // "There is a device...[f000]븁\u0000\nIt seems that a card key is necessary\nto enter a password."
    SystemMsg PlasmaFrigate10_Text_ThereDeviceSeemsCard, 2
    LastKeyWait
    MsgWinCloseAll
    VMJump L_067A

L_05AF:
    VMStackPush EVENT_WORK_0x40fa
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0670
    PlasmaFrigateCmd_GetPasswordIndex EVENT_WORK_0x400a
    DebugPrint EVENT_WORK_0x400a
    SEPlay SEQ_SE_SW_PLAZMASHIP_01
    SEWait
    // "There is a device...\nIt seems to be for entering a password.[f000]븁\u0000\nWill you enter a password?"
    SystemMsg PlasmaFrigate10_Text_ThereDeviceSeemsEntering, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0668
    MsgWinCloseAll
    WorkSetConst 0x8024, 0
    PlasmaFrigateCmd_EnterPassword 0x8024
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0652
    SEPlay SEQ_SE_SW_PLAZMASHIP_02
    // "You succeeded in\nentering the password!"
    SystemMsg PlasmaFrigate10_Text_SucceededEnteringPassword, 2
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    Plugin12_Cmd1002 0
    VMSleep 8
    Plugin12_Cmd1002 1
    VMSleep 8
    Plugin12_Cmd1002 2
    VMSleep 8
    Plugin12_Cmd1002 3
    // "All barriers were deactivated,\nand you can proceed now."
    SystemMsg PlasmaFrigate10_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40fa, 1
    FlagSet EVENT_FLAG_0x0165
    VMJump L_0662

L_0652:
    SEPlay SEQ_SE_SW_PLAZMASHIP_03
    SEWait
    // "The password is not correct."
    SystemMsg PlasmaFrigate10_Text_PasswordNotCorrect, 2
    LastKeyWait
    MsgWinCloseAll

L_0662:
    VMJump L_066A

L_0668:
    MsgWinCloseAll

L_066A:
    VMJump L_067A

L_0670:
    // "All barriers were deactivated,\nand you can proceed now."
    SystemMsg PlasmaFrigate10_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_067A:
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    ActorCmdExec 255, Movement_06D4
    ActorCmdWait
    SEPlay SEQ_SE_SW_PLAZMASHIP_08
    ActorCmdExec 255, Movement_06E0
    ActorCmdWait
    SEWait
    // "Be careful!\nThe barriers are electrified!"
    InfoMsg PlasmaFrigate10_Text_CarefulBarriersElectrified, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06D0
    CallTrainerBattleEnd
    VMJump L_06D2

L_06D0:
    CallTrainerLose

L_06D2:
    VMReturn

Movement_06D4:
    Move 0, 1
    Move 75, 1
    MoveEnd

Movement_06E0:
    Move 71, 1
    Move 36, 2
    Move 17, 2
    Move 72, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0704:
    Move 15, 1
    MoveEnd

Movement_070C:
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

Movement_0734:
    Move 32, 1
    MoveEnd

Movement_073C:
    Move 33, 1
    MoveEnd

Movement_0744:
    Move 34, 1
    MoveEnd

Movement_074C:
    Move 35, 1
    MoveEnd

Movement_0754:
    Move 75, 1
    MoveEnd
