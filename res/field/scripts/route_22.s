#include "asm/field_script.inc"
#include "text/script/route_22.h"

// Script plugin 15, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    // "The one radiating such a\ntremendous presence before you[f000]븀\u0000\nis none other than Terrakion![f000]븁\u0000"
    InfoMsg Route22_Text_OneRadiatingSuchTremendous, 1
    InfoMsgClose_0039
    ActorCmdExec 255, Movement_0550
    ActorCmdWait
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_004A
    VMJump L_0058

L_004A:
    ActorCmdExec 255, Movement_0558
    VMJump L_0079

L_0058:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_006B
    VMJump L_0079

L_006B:
    ActorCmdExec 255, Movement_0570
    VMJump L_0079

L_0079:
    ActorCmdWait
    FlagReset EVENT_FLAG_0x032b
    ActorAdd 7
    PlayerGetGPos 0x8021, 0x8022
    BGMPlay SEQ_BGM_E_ACHROMA
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_00A0
    VMJump L_00B2

L_00A0:
    WorkAdd 0x8021, 2
    WorkSetConst 0x8022, 138
    VMJump L_00DD

L_00B2:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_00C5
    VMJump L_00DD

L_00C5:
    ActorSetGPos 7, 723, 0, 146, 0
    WorkAdd 0x8022, 2
    VMJump L_00DD

L_00DD:
    ActorWalkRoute 7, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0106
    VMJump L_0139

L_0106:
    VMStackPush 0x8022
    VMStackPushConst 138
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0133
    ActorWalkRoute 7, 729, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 7, Movement_0560
    ActorCmdWait

L_0133:
    VMJump L_016F

L_0139:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_014C
    VMJump L_016F

L_014C:
    VMStackPush 0x8021
    VMStackPushConst 724
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0169
    ActorCmdExec 7, Movement_0568
    ActorCmdWait

L_0169:
    VMJump L_016F

L_016F:
    // "Colress: It's been a long time.[f000]븁\u0000\nTerrakion is one of the three Pokémon\nwho protected Pokémon from[f000]븀\u0000\nthe flames of a human conflict![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route22_Text_ColressItsBeenLong, 7, 0, 0
    MsgWinCloseAll
    WorkCmpConst 0x8021, 727
    VMJumpIf CMP_EQ, L_0190
    VMJump L_019E

L_0190:
    ActorCmdExec 7, Movement_0570
    VMJump L_01CC

L_019E:
    WorkCmpConst 0x8021, 723
    VMJumpIf CMP_EQ, L_01BE
    WorkCmpConst 0x8021, 724
    VMJumpIf CMP_EQ, L_01BE
    VMJump L_01CC

L_01BE:
    ActorCmdExec 7, Movement_0558
    VMJump L_01CC

L_01CC:
    ActorCmdWait
    // "It seems likely that Terrakion has\npicked up the scent of danger that[f000]븀\u0000\nemanates from Team Plasma.[f000]븁\u0000\nIt's probably seeking a Pokémon\nTrainer who has the strength[f000]븀\u0000\nto stand up to them.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route22_Text_SeemsLikelyTerrakionHas, 7, 0, 0
    MsgWinCloseAll
    WorkCmpConst 0x8021, 727
    VMJumpIf CMP_EQ, L_01EF
    VMJump L_01FD

L_01EF:
    ActorCmdExec 7, Movement_0560
    VMJump L_022B

L_01FD:
    WorkCmpConst 0x8021, 723
    VMJumpIf CMP_EQ, L_021D
    WorkCmpConst 0x8021, 724
    VMJumpIf CMP_EQ, L_021D
    VMJump L_022B

L_021D:
    ActorCmdExec 7, Movement_0568
    VMJump L_022B

L_022B:
    ActorCmdWait
    // "It's very interesting indeed that\nTerrakion appeared before you![f000]븁\u0000\nThat aside, do you plan\nto confront Team Plasma?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route22_Text_ItsVeryInterestingIndeed, 7, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0264
    // "I see. If that's the case,\nyou must have the power to protect[f000]븀\u0000\nyour own Pokémon![f000]븁\u0000\nBeing protected by Pokémon\nalone doesn't make you a Trainer![f000]븁\u0000\nIt's because Trainers are strong,\nbecause they care about their Pokémon,[f000]븀\u0000\nthat these Pokémon can also be strong![f000]븁\u0000\nHere!\nThis is from me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route22_Text_SeeIfThatsCase, 7, 0, 0
    MsgWinCloseAll
    VMJump L_0272

L_0264:
    // "I see!\nThat works as well.[f000]븁\u0000\nYou and your Pokémon\ncan proceed down your own path![f000]븁\u0000\nWhat you should do is care\nabout your Pokémon to bring out[f000]븀\u0000\ntheir power to its fullest potential![f000]븁\u0000\nHere!\nThis is from me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route22_Text_SeeWorksWellPokemon, 7, 0, 0
    MsgWinCloseAll

L_0272:
    WorkCmpConst 0x8021, 727
    VMJumpIf CMP_EQ, L_0285
    VMJump L_0293

L_0285:
    ActorCmdExec 7, Movement_0590
    VMJump L_02C1

L_0293:
    WorkCmpConst 0x8021, 723
    VMJumpIf CMP_EQ, L_02B3
    WorkCmpConst 0x8021, 724
    VMJumpIf CMP_EQ, L_02B3
    VMJump L_02C1

L_02B3:
    ActorCmdExec 7, Movement_0580
    VMJump L_02C1

L_02C1:
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 635
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "This is a prototype of my device\nthat energizes Pokémon![f000]븁\u0000\nIt doesn't work on battling Pokémon,\nbut you may find it useful for something![f000]븁\u0000\nWell then, I wish you and your Pokémon\na safe journey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route22_Text_PrototypeDeviceEnergizesPokemon, 7, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    WorkCmpConst 0x8021, 727
    VMJumpIf CMP_EQ, L_0308
    VMJump L_033E

L_0308:
    EvCameraMoveTo 9688, 0, 0xed000, 0x2db8000, 0xfffefff1, 0x8a8000, 40
    ActorWalkRoute 7, 730, 138, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 7, Movement_05A8
    VMJump L_0394

L_033E:
    WorkCmpConst 0x8021, 723
    VMJumpIf CMP_EQ, L_035E
    WorkCmpConst 0x8021, 724
    VMJumpIf CMP_EQ, L_035E
    VMJump L_0394

L_035E:
    EvCameraMoveTo 9688, 0, 0xed000, 0x2d48000, 0, 0x918000, 40
    ActorWalkRoute 7, 724, 144, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 7, Movement_0598
    VMJump L_0394

L_0394:
    ActorCmdWait
    EvCameraWait
    // "Now that I think about it![f000]븁\u0000\nIn the Seaside Cave on Route 21,\nI saw something that reminded[f000]븀\u0000\nme of when we met on Route 4.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route22_Text_NowThinkAboutSeaside, 7, 0, 0
    MsgWinCloseAll
    ActorCmdExec 7, Movement_05D8
    ActorCmdWait
    EvCameraReturn 30
    WorkCmpConst 0x8021, 727
    VMJumpIf CMP_EQ, L_03C7
    VMJump L_03DB

L_03C7:
    ActorWalkRoute 7, 737, 138, 1, 8, 0
    VMJump L_040F

L_03DB:
    WorkCmpConst 0x8021, 723
    VMJumpIf CMP_EQ, L_03FB
    WorkCmpConst 0x8021, 724
    VMJumpIf CMP_EQ, L_03FB
    VMJump L_040F

L_03FB:
    ActorWalkRoute 7, 724, 150, 1, 8, 1
    VMJump L_040F

L_040F:
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdWait
    ActorDelete 7
    BGMChangeMap
    MapReplaceSetEvent 5, 1, 1
    MapReplaceSetEvent 6, 0, 0
    FlagSet EVENT_FLAG_0x032b
    FlagSet EVENT_FLAG_0x0170
    WorkSetConst EVENT_WORK_PLASMA_FRIGATE_LOCATION, 1
    WorkSetConst EVENT_WORK_0x4106, 1
    WorkSetConst EVENT_WORK_0x40e3, 2
    HollowRivalCmd_0262 1, 29
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 639, 0
    // "Gurooooohhh!"
    ScreamMsg Route22_Text_Gurooooohhh, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMStackPushFlag EVENT_FLAG_0x0149
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_049F
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8023, 1
    CallWildBattle 639, 45, 0x8023
    WorkSetConst 0x8023, 0
    VMJump L_04B9

L_049F:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 1
    CallWildBattle 639, 65, 0x8024
    WorkSetConst 0x8024, 0

L_04B9:
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E0
    FlagSet EVENT_FLAG_0x032a
    ActorDelete 6
    CallWildBattleEnd
    VMJump L_04E2

L_04E0:
    CallWildLose

L_04E2:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_04F9
    VMJump L_0503

L_04F9:
    FlagSet EVENT_FLAG_0x014a
    VMJump L_0529

L_0503:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0523
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0523
    VMJump L_0529

L_0523:
    VMJump L_0529

L_0529:
    VMStackPushFlag EVENT_FLAG_0x014a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0546
    // "Terrakion ran away past\nthe end of the route..."
    SystemMsg Route22_Text_TerrakionRanAwayPast, 2
    MsgWaitAdvance
    InfoMsgClose

L_0546:
    FlagSet EVENT_FLAG_0x0149
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0550:
    Move 75, 1
    MoveEnd

Movement_0558:
    Move 35, 1
    MoveEnd

Movement_0560:
    Move 34, 1
    MoveEnd

Movement_0568:
    Move 32, 1
    MoveEnd

Movement_0570:
    Move 33, 1
    MoveEnd
    Move 13, 1
    MoveEnd

Movement_0580:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_0590:
    Move 14, 1
    MoveEnd

Movement_0598:
    Move 9, 1
    MoveEnd
    Move 8, 1
    MoveEnd

Movement_05A8:
    Move 11, 1
    MoveEnd
    Move 10, 1
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

Movement_05D8:
    Move 100, 1
    MoveEnd
    Move 13, 3
    Move 9, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 22"
    MsgPlaceSign Route22_Text_Route22, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
