#include "asm/field_script.inc"
#include "text/script/victory_road.h"

// Script plugin 11, from the zones that use this file

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

L_004C:
    WorkSetConst 0x8023, 0
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0079
    ObjInitWarpGPos 1, 65288, 80, 248
    VMJump L_0083

L_0079:
    ObjInitWarpGPos 0, 65288, 80, 248

L_0083:
    WorkSetConst 0x8023, 0
    VMReturn

Script_1:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8025, 1
    TrainerCardHasBadge 0x8024, 0
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40e4
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00CC
    WorkSetConst EVENT_WORK_0x40e4, 1

L_00CC:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E5
    WorkSetConst 0x8025, 0

L_00E5:
    TrainerCardHasBadge 0x8024, 1
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40e5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0114
    WorkSetConst EVENT_WORK_0x40e5, 1

L_0114:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012D
    WorkSetConst 0x8025, 0

L_012D:
    TrainerCardHasBadge 0x8024, 2
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40e6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_015C
    WorkSetConst EVENT_WORK_0x40e6, 1

L_015C:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0175
    WorkSetConst 0x8025, 0

L_0175:
    TrainerCardHasBadge 0x8024, 3
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40e7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01A4
    WorkSetConst EVENT_WORK_0x40e7, 1

L_01A4:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BD
    WorkSetConst 0x8025, 0

L_01BD:
    TrainerCardHasBadge 0x8024, 4
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40e8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01EC
    WorkSetConst EVENT_WORK_0x40e8, 1

L_01EC:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0205
    WorkSetConst 0x8025, 0

L_0205:
    TrainerCardHasBadge 0x8024, 5
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40e9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0234
    WorkSetConst EVENT_WORK_0x40e9, 1

L_0234:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024D
    WorkSetConst 0x8025, 0

L_024D:
    TrainerCardHasBadge 0x8024, 6
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40ea
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_027C
    WorkSetConst EVENT_WORK_0x40ea, 1

L_027C:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0295
    WorkSetConst 0x8025, 0

L_0295:
    TrainerCardHasBadge 0x8024, 7
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40eb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02C4
    WorkSetConst EVENT_WORK_0x40eb, 1

L_02C4:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DD
    WorkSetConst 0x8025, 0

L_02DD:
    VMStackPush EVENT_WORK_0x40ec
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F6
    WorkSetConst EVENT_WORK_0x40ec, 1

L_02F6:
    WorkGet EVENT_WORK_0x4000, 0x8025
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    VMCall L_004C
    VMHalt

Script_14:
    VMCall L_004C
    VMHalt

Script_2:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 0
    // "Basic Badge confirmed!"
    SystemMsg VictoryRoad_Text_BasicBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40e4, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 1
    // "Toxic Badge confirmed!"
    SystemMsg VictoryRoad_Text_ToxicBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40e5, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 2
    // "Insect Badge confirmed!"
    SystemMsg VictoryRoad_Text_InsectBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40e6, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 3
    // "Bolt Badge confirmed!"
    SystemMsg VictoryRoad_Text_BoltBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40e7, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 4
    // "Quake Badge confirmed!"
    SystemMsg VictoryRoad_Text_QuakeBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40e8, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 5
    // "Jet Badge confirmed!"
    SystemMsg VictoryRoad_Text_JetBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40e9, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 6
    // "Legend Badge confirmed!"
    SystemMsg VictoryRoad_Text_LegendBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40ea, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    BadgeGateCmd_PlayCheck 7
    // "Wave Badge confirmed!"
    SystemMsg VictoryRoad_Text_WaveBadgeConfirmed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x40eb, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047C
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    ActorGetGPos 255, 0x8026, 0x8027
    VMStackPush 0x8026
    VMStackPushConst 31
    VMStackCmp CMP_NE
    VMStackPush 0x8027
    VMStackPushConst 44
    VMStackCmp CMP_NE
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_045E
    ActorWalkRoute 255, 31, 44, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_06A0
    ActorCmdWait

L_045E:
    VMCall L_0498
    BadgeGateCmd_PlayLastGate
    VMCall L_049E
    WorkSetConst EVENT_WORK_0x40ec, 2
    FlagSet EVENT_FLAG_ARRIVED_VICTORY_ROAD
    VMJump L_0486

L_047C:
    ActorCmdExec 255, Movement_06AC
    ActorCmdWait

L_0486:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0498:
    EvCameraInit
    EvCameraUnbind
    VMReturn

L_049E:
    EvCameraWait
    EvCameraMoveToDefault 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn

Script_11:
    ActorsPauseAll
    ActorCmdExec 255, Movement_06C4
    ActorCmdWait
    ActorWalkRoute 255, 80, 50, 1, 8, 0
    ActorCmdWait
    VMSleep 16
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x508000, 0x5004f, 0x308000, 30
    ActorWalkRoute 0, 80, 47, 1, 14, 1
    ActorCmdWait
    EvCameraWait
    // "N: [f000]븉\u0001\u0001You came...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad_Text_NCame, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06DC
    ActorCmdWait
    // "[f000]븉\u0001\u0001The Pokémon League is\njust past Victory Road.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad_Text_PokemonLeagueJustPast, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06EC
    ActorCmdWait
    // "[f000]븉\u0001\u0001Pokémon battles do nothing\nmore than hurt Pokémon...[f000]븁\u0000\nThat's how I understood it,\nand that's why I hated battles.[f000]븁\u0000\nBut it's not that simple.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad_Text_PokemonBattlesNothingMore, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06BC
    ActorCmdWait
    ActorCmdExec 0, Movement_06CC
    ActorCmdWait
    // "[f000]븉\u0001\u0001Pokémon battles decide\nwinners and losers, it's true.[f000]븁\u0000\nYet they do so much more.[f000]븁\u0000\nYour Pokémon! You!\nYour opponents! And their Pokémon![f000]븁\u0000\nEveryone can see what wonderful\nthings the others have to contribute![f000]븁\u0000\nThat's right! Accepting different ideas--\ndifferent beings--changes the world[f000]븀\u0000\nlike a chemical reaction![f000]븁\u0000\nPokémon battles are like a catalyst:\na small component that leads to[f000]븀\u0000\nbig changes![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad_Text_PokemonBattlesDecideWinners, 0, 0, 0
    // "[f000]븉\u0001\u0001My friend Reshiram taught me that...[f000]븁\u0000\nAnd it's the formula I've\nderived from traveling the world.[f000]븁\u0000\nI want you to think for\nyourself about what it means.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001My friend Zekrom taught me that...[f000]븁\u0000\nAnd it's the formula I've derived\nfrom traveling the world.[f000]븁\u0000\nI want you to think for\nyourself about what it means.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, VictoryRoad_Text_FriendReshiramTaughtIts, VictoryRoad_Text_FriendZekromTaughtIts, 0, 0, 0
    VMCall L_05F8
    // "[f000]븉\u0001\u0001Here!\nTake this with you![f000]븁\u0000\nThe new Victory Road has areas that\nare only accessible with Waterfall.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad_Text_HereTakeNewVictory, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06CC
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 424
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "[f000]븉\u0001\u0001Yes!\nIf it's your Pokémon and you...[f000]븁\u0000\nYou will get past the Pokémon League\nand recognize your own truth![f000]븁\u0000\nThat's what I see in store for you![f000]븁\u0000"
    // "[f000]븉\u0001\u0001Yes!\nIf it's your Pokémon and you...[f000]븁\u0000\nYou will get past the Pokémon League\nand recognize your own ideals![f000]븁\u0000\nThat's what I see in store for you![f000]븁\u0000"
    ActorMsgVersioned 1024, VictoryRoad_Text_YesIfItsPokemon_2, VictoryRoad_Text_YesIfItsPokemon, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 30
    ActorWalkRoute 0, 81, 56, 1, 8, 0
    VMSleep 16
    ActorCmdExec 255, Movement_06EC
    ActorCmdWait
    ActorDelete 0
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FlagSet EVENT_FLAG_0x0385
    WorkSetConst EVENT_WORK_0x410d, 1
    HollowRivalCmd_0262 1, 37
    FlagSet EVENT_FLAG_0x0407
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_05F8:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    PokePartyGetCount 0x8028, 0

L_061C:
    VMStackPush 0x8028
    VMStackPush 0x8029
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0667
    PokePartyGetParam 0x802b, 0x8029, 178
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_065B
    PokePartyGetSpecies 0x802a, 0x8029
    WordSetPokeSpecies 0, 0x802a
    WorkSetConst 0x802c, 1

L_065B:
    WorkAdd 0x8029, 1
    VMJump L_061C

L_0667:
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0694
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0698
    ActorCmdWait
    // "[f000]븉\u0001\u0001What's this?\nWhy, you're the...[f000]븁\u0000\nYou're the [f000]ā\u0001\u0000 that helped me\nthat time, aren't you?[f000]븀\u0000\nHey, thanks![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad_Text_WhatsWhyYoureYoure, 0, 0, 0
    MsgWinCloseAll

L_0694:
    VMReturn
    .balign 4, 0

Movement_0698:
    Move 75, 1
    MoveEnd

Movement_06A0:
    Move 32, 1
    Move 0, 1
    MoveEnd

Movement_06AC:
    Move 33, 1
    Move 1, 1
    Move 13, 1
    MoveEnd

Movement_06BC:
    Move 182, 1
    MoveEnd

Movement_06C4:
    Move 12, 1
    MoveEnd

Movement_06CC:
    Move 13, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_06DC:
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_06EC:
    Move 33, 1
    MoveEnd

Script_12:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Badge Check Gate Ahead"
    MsgPlaceSign VictoryRoad_Text_BadgeCheckGateAhead, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "The Pokémon League\nis through this tunnel!"
    MsgPlaceSign VictoryRoad_Text_PokemonLeagueThroughTunnel, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
