#include "asm/field_script.inc"
#include "text/script/route_21.h"

// Script plugin 15, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

L_0036:
    VMStackPush EVENT_WORK_0x4106
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0053
    ObjInitWarpGPos 2, 787, 65533, 247

L_0053:
    VMReturn

Script_1:
    VMCall L_0036
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x4106
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0082
    ActorSetGPos 9, 797, 65531, 242, 3
    VMJump L_00E6

L_0082:
    VMStackPush EVENT_WORK_0x4106
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0357
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00B7
    ActorSetGPos 9, 798, 65531, 241, 3
    VMJump L_00E6

L_00B7:
    VMStackPush EVENT_WORK_0x4106
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0359
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00E6
    ActorSetGPos 11, 796, 65531, 242, 3

L_00E6:
    VMHalt

Script_3:
    VMCall L_0036
    VMHalt

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    FlagReset EVENT_FLAG_0x0357
    FlagReset EVENT_FLAG_0x0358
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x3219000, 0xfffaffb1, 0xf02000, 48
    EvCameraWait
    VMSleep 80
    EvCameraMoveToDefault 48
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 9
    SEWait
    ActorCmdExec 9, Movement_06B4
    ActorCmdWait
    PlayerGetGPos 0x8022, 0x8023
    WorkSub 0x8022, 1
    ActorWalkRoute 9, 0x8022, 0x8023, 1, 8, 1
    VMSleep 16
    ActorCmdExec 255, Movement_06A4
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: You found them!\nI knew you could do it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_FoundThemKnewCould, 9, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x3204000, 0xfffaffb1, 0xf28000, 26
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 239
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C5
    ActorCmdExec 9, Movement_02D0
    VMSleep 20
    ActorCmdExec 255, Movement_02A4
    VMJump L_0242

L_01C5:
    VMStackPush 0x8023
    VMStackPushConst 240
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F2
    ActorCmdExec 9, Movement_02DC
    VMSleep 20
    ActorCmdExec 255, Movement_02B0
    VMJump L_0242

L_01F2:
    VMStackPush 0x8023
    VMStackPushConst 241
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_021F
    ActorCmdExec 9, Movement_02E8
    VMSleep 20
    ActorCmdExec 255, Movement_02BC
    VMJump L_0242

L_021F:
    VMStackPush 0x8023
    VMStackPushConst 242
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0242
    ActorCmdExec 9, Movement_02F4
    ActorCmdExec 255, Movement_02C4

L_0242:
    ActorCmdWait
    EvCameraWait
    // "But...\nHow do we get in?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_ButHowWeGet, 9, 0, 0
    MsgWinCloseAll
    // "???: Hold up![f000]븁\u0000"
    InfoMsg Route21_Text_HoldUp, 2
    MsgWinCloseAll
    Cmd_02E8 1, 1
    SEPlay SEQ_SE_SW_RENBU_02
    // "A heavy gangplank fell\nwith a solid thud...[f000]븁\u0000"
    InfoMsg Route21_Text_HeavyGangplankFellSolid, 2
    MsgWinCloseAll
    SEWait
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    MapReplaceSetEvent 5, 0, 0
    MapReplaceSetEvent 6, 1, 1
    WorkSetConst EVENT_WORK_0x4106, 2
    RTReserveScript 7
    MapChangeCore ZONE_ROUTE_21, 797, 65531, 241, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_02A4:
    Move 13, 2
    Move 15, 1
    MoveEnd

Movement_02B0:
    Move 13, 1
    Move 15, 1
    MoveEnd

Movement_02BC:
    Move 15, 1
    MoveEnd

Movement_02C4:
    Move 12, 1
    Move 15, 1
    MoveEnd

Movement_02D0:
    Move 13, 3
    Move 15, 2
    MoveEnd

Movement_02DC:
    Move 13, 2
    Move 15, 2
    MoveEnd

Movement_02E8:
    Move 13, 1
    Move 15, 2
    MoveEnd

Movement_02F4:
    Move 15, 2
    MoveEnd

Script_7:
    ActorsPauseAll
    Cmd_02E9 1, 1
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 10, Movement_04BC
    ActorCmdWait
    // "Marlon: Eh, this should do it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_MarlonEhShould, 10, 0, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: I thought you weren't\ngoing to fight Team Plasma?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_ThoughtWerentGoingFight, 9, 0, 0
    MsgWinCloseAll
    // "Marlon: Hey, friends!\nTeam Plasma's not botherin' me.[f000]븁\u0000\nThey could be up to no good\nfor reals, and...[f000]븁\u0000\nIt's not my style to go around\ndecidin' Team Plasma's bad[f000]븀\u0000\nwithout thinkin' 'bout it[f000]븀\u0000\njust 'cause everyone says so![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_MarlonHeyFriendsTeam, 10, 0, 0
    // "But, eh, you're havin' trouble,\nso I've got to help out![f000]븀\u0000\nThat's what I want to do![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_ButEhYoureHavin, 10, 0, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Thanks...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_Thanks, 9, 0, 0
    MsgWinCloseAll
    // "Marlon: Listen, 'K.\nBelieve in somethin'![f000]븁\u0000\nSearchin' for stolen\nPokémon is fine![f000]븁\u0000\nKeepin' Unova from bein'\niced over's fine too![f000]븁\u0000\nIt's all good.\nBut think 'bout why you're doin' that.[f000]븁\u0000\nThe strength of your beliefs is what\ngives you and your Pokémon power![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_MarlonListenKBelieve, 10, 0, 0
    MsgWinCloseAll
    ActorCmdExec 10, Movement_06AC
    ActorCmdWait
    // "I made some noise,\nso some people came out![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_MadeSomeNoiseSome, 10, 0, 0
    MsgWinCloseAll
    ActorCmdExec 10, Movement_06A4
    ActorCmdWait
    // "Hey, be careful, 'K![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_HeyCarefulK, 10, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x3198000, 0xfffaffb1, 0xf18000, 40
    ActorCmdExec 10, Movement_04C4
    VMSleep 32
    ActorCmdExec 255, Movement_06A4
    ActorCmdExec 9, Movement_06A4
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 10
    SEWait
    EvCameraWait
    // "[f000]Ā\u0001\u0001: What was that?!\nThe guy sure doesn't follow through![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_WhatGuySureDoesnt, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0694
    ActorCmdExec 255, Movement_069C
    ActorCmdWait
    // "Well, I guess he was being\nkind in his own way...[f000]븁\u0000\nAll right, I'm going in![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_WellGuessHeBeing, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_04D8
    VMSleep 4
    ActorCmdExec 255, Movement_06AC
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 9
    ActorCmdWait
    SEWait
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ObjInitWarpGPos 2, 800, 65531, 241
    WorkSetConst EVENT_WORK_0x4106, 3
    FlagSet EVENT_FLAG_0x0357
    FlagSet EVENT_FLAG_0x0358
    FlagReset EVENT_FLAG_0x033e
    FlagReset EVENT_FLAG_0x0340
    FlagSet EVENT_FLAG_0x033f
    FlagSet EVENT_FLAG_0x033d
    FlagReset EVENT_FLAG_0x03ea
    HollowRivalCmd_0262 1, 30
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_049D
    WorkSetConst EVENT_WORK_0x40f1, 0
    WorkSetConst EVENT_WORK_0x40f2, 1
    WorkSetConst EVENT_WORK_0x4103, 1
    WorkSetConst EVENT_WORK_0x4102, 1
    VMJump L_04B5

L_049D:
    WorkSetConst EVENT_WORK_0x40f1, 1
    WorkSetConst EVENT_WORK_0x40f2, 0
    WorkSetConst EVENT_WORK_0x4103, 1
    WorkSetConst EVENT_WORK_0x4102, 1

L_04B5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04BC:
    Move 14, 3
    MoveEnd

Movement_04C4:
    Move 12, 1
    Move 14, 6
    Move 13, 1
    Move 14, 2
    MoveEnd

Movement_04D8:
    Move 19, 1
    Move 16, 1
    Move 19, 3
    MoveEnd

Script_5:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    FlagReset EVENT_FLAG_0x0359
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Team Plasma![f000]븁\u0000\nNo matter where you fly,\nI won't let you get away![f000]븁\u0000\nBut, what's over there anyway?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_TeamPlasmaNoMatter, 9, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 11
    ActorCmdWait
    SEWait
    ActorWalkRoute 11, 796, 242, 1, 8, 1
    VMSleep 8
    ActorCmdExec 9, Movement_06A4
    ActorCmdExec 255, Movement_06A4
    ActorCmdWait
    // "Cheren: Sorry I'm late...[f000]븁\u0000\nI suspect that they're flying\ntoward the Giant Chasm.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_CherenSorryImLate, 11, 0, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: The Giant Chasm!\nThat's deep in Route 22, right?[f000]븁\u0000\nOK, I'm going!\n[f000]Ā\u0001\u0000! You come too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_GiantChasmThatsDeep, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_069C
    ActorCmdExec 255, Movement_0694
    ActorCmdWait
    // "That Pokémon...[f000]븁\u0000\nKyurem, was it?[f000]븁\u0000\nIts cry sounded\nlonely somehow.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_PokemonKyuremItsCry, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0624
    VMSleep 8
    ActorCmdExec 11, Movement_06A4
    ActorCmdExec 255, Movement_06A4
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 9
    SEWait
    ActorCmdExec 11, Movement_06AC
    ActorCmdWait
    // "Cheren: Kyurem![f000]븁\u0000\nTeam Plasma is using Kyurem's\npower for evil?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route21_Text_CherenKyuremTeamPlasma, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_060C
    ActorCmdWait
    // "Then how should we stop\nTeam Plasma?[f000]븁\u0000\nIf Kyurem is, like Drayden said,\na legendary Dragon-type Pokémon,[f000]븀\u0000\nthe only ones who could stand up[f000]븀\u0000\nto it are Reshiram or Zekrom![f000]븁\u0000\nBut...\nNeither of them is here right now...[f000]븁\u0000\nWhere are the ones the legendary\nPokémon recognized as heroes, anyway?[f000]븀\u0000\nI guess I just have to find one of them!"
    // "Then how should we stop\nTeam Plasma?[f000]븁\u0000\nIf Kyurem is, like Drayden said,\na legendary Dragon-type Pokémon,[f000]븀\u0000\nthe only ones who could stand up[f000]븀\u0000\nto it are Zekrom or Reshiram![f000]븁\u0000\nBut...\nNeither of them is here right now...[f000]븁\u0000\nWhere are the ones the legendary\nPokémon recognized as heroes, anyway?[f000]븀\u0000\nI guess I just have to find one of them!"
    ActorMsgVersioned 1024, Route21_Text_ThenHowShouldWe, Route21_Text_ThenHowShouldWe_2, 11, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4106, 5
    WorkSetConst EVENT_WORK_0x4070, 1
    FlagSet EVENT_FLAG_0x0357
    FlagSet EVENT_FLAG_0x016c
    HollowRivalCmd_0262 2, 9
    HollowRivalCmd_0262 1, 32
    HollowRivalCmd_0262 3, 7
    HollowRivalCmd_0262 0, 6
    HollowRivalCmd_0262 4, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_060C:
    Move 33, 1
    Move 161, 1
    Move 181, 1
    Move 66, 2
    Move 1, 1
    MoveEnd

Movement_0624:
    Move 18, 7
    MoveEnd
    Move 12, 1
    Move 35, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Cheren: Yeah...[f000]븁\u0000\nYou can get to the Giant Chasm\nfrom Route 22."
    ParentActorMsg MSGFILE_SCRIPT, Route21_Text_CherenYeahCanGet, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
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

Movement_0694:
    Move 32, 1
    MoveEnd

Movement_069C:
    Move 33, 1
    MoveEnd

Movement_06A4:
    Move 34, 1
    MoveEnd

Movement_06AC:
    Move 35, 1
    MoveEnd

Movement_06B4:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
