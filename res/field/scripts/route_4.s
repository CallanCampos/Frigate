#include "asm/field_script.inc"
#include "text/script/route_4.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_9:
    RTCGetWeekDay 0x8023
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_007B
    FlagReset EVENT_FLAG_0x0386
    VMJump L_007F

L_007B:
    FlagSet EVENT_FLAG_0x0386

L_007F:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00AE
    HollowRivalCmd_0262 2, 14
    VMJump L_00D7

L_00AE:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00D7
    HollowRivalCmd_0262 2, 0

L_00D7:
    VMHalt

Script_5:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    PlayerGetDir 0x8020
    VMStackPush 0x8021
    VMStackPushConst 432
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0139
    ActorCmdExec 15, Movement_04A0
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011F
    VMSleep 20
    ActorCmdExec 255, Movement_06B8

L_011F:
    ActorCmdWait
    ActorCmdExec 15, Movement_04AC
    ActorCmdExec 255, Movement_04D0
    ActorCmdWait
    VMJump L_01F9

L_0139:
    VMStackPush 0x8021
    VMStackPushConst 430
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_017C
    ActorWalkRoute 15, 431, 583, 0, 8, 0
    ActorCmdExec 255, Movement_0680
    ActorCmdWait
    ActorCmdExec 15, Movement_06C8
    ActorCmdExec 255, Movement_06D0
    ActorCmdWait
    VMJump L_01F9

L_017C:
    VMStackPush 0x8021
    VMStackPushConst 431
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B1
    ActorCmdExec 15, Movement_04B4
    ActorCmdWait
    ActorCmdExec 15, Movement_06C8
    ActorCmdExec 255, Movement_04C0
    ActorCmdWait
    VMJump L_01F9

L_01B1:
    WorkSub 0x8022, 1
    ActorWalkRoute 15, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 15, Movement_06C0
    ActorCmdWait
    ActorWalkRoute 15, 431, 583, 4, 8, 0
    ActorWalkRoute 255, 430, 583, 4, 8, 1
    ActorCmdWait
    ActorCmdExec 15, Movement_06C8
    ActorCmdWait

L_01F9:
    // "Colress: I've been waiting for you![f000]븁\u0000\nWhat's the matter?\nInterested in what's behind me?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route4_Text_ColressIveBeenWaiting, 15, 0, 0
    // "These are not mere rocks, but\nthe Pokémon known as Crustle.[f000]븁\u0000\nObserve.[f000]븁\u0000\nWith this device I created\nto energize Pokémon, I'll...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route4_Text_TheseNotMereRocks, 15, 0, 0
    MsgWinCloseAll
    ActorCmdExec 15, Movement_06B8
    VMSleep 4
    ActorCmdExec 255, Movement_06B8
    ActorCmdWait
    SEPlay SEQ_SE_SHINKA_W025
    ActorCmdExec 15, Movement_04E0
    ActorCmdWait
    SEWait
    ActorCmdExec 12, Movement_04F4
    ActorCmdExec 14, Movement_04E8
    ActorCmdExec 17, Movement_06C0
    VMSleep 4
    ActorCmdExec 23, Movement_04E8
    ActorCmdExec 13, Movement_04E8
    ActorCmdExec 18, Movement_06C0
    ActorCmdWait
    ActorCmdExec 19, Movement_04E8
    ActorCmdExec 21, Movement_06C0
    VMSleep 4
    ActorCmdExec 20, Movement_06C0
    ActorCmdExec 22, Movement_06C0
    ActorCmdExec 24, Movement_04F4
    ActorCmdWait
    FadeEx 3, 0, 16, 4
    ActorCmdExec 21, Movement_0508
    ActorCmdExec 22, Movement_0508
    VMSleep 4
    ActorCmdExec 19, Movement_0500
    ActorCmdExec 24, Movement_0508
    ActorCmdExec 12, Movement_0500
    ActorCmdExec 13, Movement_0500
    VMSleep 4
    ActorCmdExec 18, Movement_0500
    ActorCmdWait
    FadeExWait
    ActorDelete 12
    ActorDelete 13
    ActorDelete 14
    ActorDelete 17
    ActorDelete 18
    ActorDelete 19
    ActorDelete 20
    ActorDelete 21
    ActorDelete 22
    ActorDelete 23
    ActorDelete 24
    FadeEx 3, 16, 0, 4
    FadeExWait
    // "Colress: Those Crustle...[f000]븁\u0000\nWere they just lying here,\nout of energy, with their[f000]븀\u0000\nboulders on their backs?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route4_Text_ColressThoseCrustleWere, 15, 0, 0
    MsgWinCloseAll
    BGMPlay SEQ_BGM_E_ACHROMA
    ActorCmdExec 15, Movement_06C8
    VMSleep 4
    ActorCmdExec 255, Movement_06D0
    ActorCmdWait
    // "Team Plasma said we should recognize\nthe potential in Pokémon and[f000]븀\u0000\nliberate them from humans.[f000]븁\u0000\nI disagree.[f000]븁\u0000\nConversely, it should be humans who bring\nout the hidden potential in Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route4_Text_TeamPlasmaSaidWe, 15, 0, 0
    MsgWinCloseAll
    ActorCmdExec 15, Movement_04E0
    ActorCmdWait
    WordSetPlayerName 0
    // "Now that I think of it,\nI never asked your name.[f000]븁\u0000\n...\n...[f000]븁\u0000\n[f000]Ā\u0001\u0000...\nI'll remember that name.[f000]븁\u0000\nWell then, I will test you to see if\nyou're a Trainer who can bring out[f000]븀\u0000\nthe hidden potential of Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route4_Text_NowThinkNeverAsked, 15, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_COLRESS, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039B
    CallTrainerBattleEnd
    VMJump L_039D

L_039B:
    CallTrainerLose

L_039D:
    WordSetPlayerName 0
    // "Colress: I see! Just like the\nGym Leaders in each area or the[f000]븀\u0000\nElite Four and Champion in the[f000]븀\u0000\nPokémon League, you bring out the[f000]븀\u0000\npower in Pokémon by being kind to them![f000]븁\u0000\nThat's the kind of person you are.[f000]븁\u0000\nI'm extremely grateful for your help.\nThis is a token of my gratitude.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route4_Text_ColressSeeJustLike, 15, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 46
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    WordSetPlayerName 0
    // "Colress: It's so frustrating![f000]븁\u0000\nIf only we could talk to Pokémon\nin order to bring out their power![f000]븁\u0000\nBut there's no way a person\nlike that could ever exist![f000]븁\u0000\nWell then, [f000]Ā\u0001\u0000,\nI hope to see you again sometime.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route4_Text_ColressItsFrustratingIf, 15, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 15, 431, 572, 4, 8, 0
    VMSleep 8
    ActorCmdExec 255, Movement_06B8
    ActorCmdWait
    ActorDelete 15
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40b6, 3
    FlagSet EVENT_FLAG_0x02f6
    FlagSet EVENT_FLAG_0x02f5
    FlagReset EVENT_FLAG_0x03dc
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40b6
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I never would have guessed\nthey were Crustle...[f000]븁\u0000\nIf you're interested in Crustle,\nyou'll find them in the Desert Resort,[f000]븀\u0000\nwhich is just past here!"
    ParentActorMsg MSGFILE_SCRIPT, Route4_Text_NeverWouldHaveGuessed, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0462

L_044E:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, Trainer![f000]븁\u0000\nThese boulders suddenly\nlined up like this...[f000]븁\u0000\nWhat's more, the HM Strength\nwon't budge them."
    ParentActorMsg MSGFILE_SCRIPT, Route4_Text_HeyTrainerTheseBoulders, 0, 0
    LastKeyWait
    ActorMsgClose

L_0462:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma said we should recognize\nthe potential in Pokémon and[f000]븀\u0000\nliberate them from humans.[f000]븁\u0000\nI disagree.[f000]븁\u0000\nConversely, it should be humans who bring\nout the hidden potential in Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route4_Text_TeamPlasmaSaidWe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "It's a big boulder, but it doesn't\nlook like a Pokémon can move it..."
    SystemMsg Route4_Text_ItsBigBoulderBut, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04A0:
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_04AC:
    Move 14, 1
    MoveEnd

Movement_04B4:
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_04C0:
    Move 14, 1
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_04D0:
    Move 14, 2
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_04E0:
    Move 182, 1
    MoveEnd

Movement_04E8:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_04F4:
    Move 33, 1
    Move 159, 1
    MoveEnd

Movement_0500:
    Move 12, 8
    MoveEnd

Movement_0508:
    Move 12, 7
    MoveEnd
    VMStackAdd
    DebugPrint 254
    VMNop

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Places with ruins are being\ndeveloped one after another.[f000]븁\u0000\nWe end up losing parts of our history..."
    ParentActorMsg MSGFILE_SCRIPT, Route4_Text_PlacesRuinsBeingDeveloped, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh! Trainer, when you see sand...[f000]븁\u0000\nDo you notice how some areas are lighter?\nAnd some of the sand looks...darker...[f000]븁\u0000\nPokémon are hiding in the darker sand!"
    ParentActorMsg MSGFILE_SCRIPT, Route4_Text_OhTrainerWhenSee, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Relic Castle is being buried in sand...\nSomeday, memories of it may be[f000]븀\u0000\nburied, too."
    ParentActorMsg MSGFILE_SCRIPT, Route4_Text_RelicCastleBeingBuried, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nPokémon have a source of energy\nfor using moves.[f000]븁\u0000\nIt's called PP, meaning Power Points.\nThey have PP for each move.[f000]븁\u0000\nWhen a move has no PP remaining,\nthat Pokémon cannot use that move.[f000]븁\u0000\nThat's a good time to head for\nthe Pokémon Center!"
    MsgPlaceSign Route4_Text_TrainerTipsPokemonHave, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a6b
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 4\nPlanned route-expansion area"
    MsgPlaceSign Route4_Text_Route4PlannedRoute, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 4"
    MsgPlaceSign Route4_Text_Route4, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    PVPlay 630, 0
    // "Awwwwk!"
    ScreamMsg Route4_Text_Awwwwk, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 128
    WorkOr 0x8024, 8
    CallWildBattle 630, 25, 0x8024
    WorkSetConst 0x8024, 0
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0620
    FlagSet EVENT_FLAG_0x0386
    FlagSet EVENT_FLAG_DAILY_0x0ad6
    ActorDelete 25
    CallWildBattleEnd
    VMJump L_0622

L_0620:
    CallWildLose

L_0622:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0639
    VMJump L_063F

L_0639:
    VMJump L_066F

L_063F:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_065F
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_065F
    VMJump L_066F

L_065F:
    // "Mandibuzz flew off into the sky..."
    SystemMsg Route4_Text_MandibuzzFlewOffInto, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_066F

L_066F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_0680:
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

Movement_06B8:
    Move 32, 1
    MoveEnd

Movement_06C0:
    Move 33, 1
    MoveEnd

Movement_06C8:
    Move 34, 1
    MoveEnd

Movement_06D0:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
