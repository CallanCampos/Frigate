#include "asm/field_script.inc"
#include "text/script/route_4_16.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    RTCGetWeekDay 0x8023
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_007F
    FlagReset EVENT_FLAG_0x0386
    VMJump L_0083

L_007F:
    FlagSet EVENT_FLAG_0x0386

L_0083:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00B2
    HollowRivalCmd_0262 2, 14
    VMJump L_00DB

L_00B2:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00DB
    HollowRivalCmd_0262 2, 0

L_00DB:
    VMHalt

Script_2:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    PlayerGetDir 0x8020
    VMStackPush 0x8021
    VMStackPushConst 407
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011E
    ActorCmdExec 14, Movement_03DC
    ActorCmdWait
    ActorCmdExec 14, Movement_03E8
    ActorCmdExec 255, Movement_040C
    ActorCmdWait
    VMJump L_01DE

L_011E:
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0161
    ActorWalkRoute 14, 406, 577, 0, 8, 0
    ActorCmdExec 255, Movement_064C
    ActorCmdWait
    ActorCmdExec 14, Movement_0694
    ActorCmdExec 255, Movement_069C
    ActorCmdWait
    VMJump L_01DE

L_0161:
    VMStackPush 0x8021
    VMStackPushConst 406
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0196
    ActorCmdExec 14, Movement_03F0
    ActorCmdWait
    ActorCmdExec 14, Movement_0694
    ActorCmdExec 255, Movement_03FC
    ActorCmdWait
    VMJump L_01DE

L_0196:
    WorkSub 0x8022, 1
    ActorWalkRoute 14, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 14, Movement_068C
    ActorCmdWait
    ActorWalkRoute 14, 406, 577, 4, 8, 0
    ActorWalkRoute 255, 405, 577, 4, 8, 1
    ActorCmdWait
    ActorCmdExec 14, Movement_0694
    ActorCmdWait

L_01DE:
    // "Colress: I've been waiting for you![f000]븁\u0000\nWhat's the matter?\nInterested in what's behind me?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route416_Text_ColressIveBeenWaiting, 14, 0, 0
    // "These are not mere rocks, but\nthe Pokémon known as Crustle.[f000]븁\u0000\nObserve.[f000]븁\u0000\nWith this device I created\nto energize Pokémon, I'll...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route416_Text_TheseNotMereRocks, 14, 0, 0
    MsgWinCloseAll
    ActorCmdExec 14, Movement_0684
    VMSleep 4
    ActorCmdExec 255, Movement_0684
    ActorCmdWait
    SEPlay SEQ_SE_SHINKA_W025
    ActorCmdExec 14, Movement_041C
    ActorCmdWait
    SEWait
    ActorCmdExec 15, Movement_0430
    ActorCmdExec 17, Movement_0424
    ActorCmdExec 18, Movement_068C
    VMSleep 4
    ActorCmdExec 16, Movement_0424
    ActorCmdExec 19, Movement_068C
    VMSleep 4
    ActorCmdExec 21, Movement_0424
    ActorCmdExec 20, Movement_068C
    VMSleep 4
    ActorCmdExec 22, Movement_068C
    ActorCmdWait
    ActorCmdExec 20, Movement_0444
    VMSleep 4
    ActorCmdExec 21, Movement_043C
    ActorCmdExec 15, Movement_043C
    ActorCmdExec 16, Movement_043C
    VMSleep 4
    ActorCmdExec 19, Movement_043C
    VMSleep 4
    ActorCmdExec 17, Movement_043C
    VMSleep 4
    ActorCmdExec 18, Movement_043C
    ActorCmdExec 22, Movement_043C
    ActorCmdWait
    // "Colress: Those Crustle...[f000]븁\u0000\nWere they just lying here,\nout of energy, with their[f000]븀\u0000\nboulders on their backs?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route416_Text_ColressThoseCrustleWere, 14, 0, 0
    MsgWinCloseAll
    BGMPlay SEQ_BGM_E_ACHROMA
    ActorCmdExec 14, Movement_0694
    VMSleep 4
    ActorCmdExec 255, Movement_069C
    ActorCmdWait
    // "Team Plasma said we should recognize\nthe potential in Pokémon and[f000]븀\u0000\nliberate them from humans.[f000]븁\u0000\nI disagree.[f000]븁\u0000\nConversely, it should be humans who bring\nout the hidden potential in Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route416_Text_TeamPlasmaSaidWe, 14, 0, 0
    MsgWinCloseAll
    ActorCmdExec 14, Movement_041C
    ActorCmdWait
    WordSetPlayerName 0
    // "Now that I think of it,\nI never asked your name.[f000]븁\u0000\n...\n...[f000]븁\u0000\n[f000]Ā\u0001\u0000...\nI'll remember that name.[f000]븁\u0000\nWell then, I will test you to see if\nyou're a Trainer who can bring out[f000]븀\u0000\nthe hidden potential of Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route416_Text_NowThinkNeverAsked, 14, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_COLRESS, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0336
    CallTrainerBattleEnd
    VMJump L_0338

L_0336:
    CallTrainerLose

L_0338:
    WordSetPlayerName 0
    // "Colress: I see! Just like the\nGym Leaders in each area or the[f000]븀\u0000\nElite Four and Champion in the[f000]븀\u0000\nPokémon League, you bring out the[f000]븀\u0000\npower in Pokémon by being kind to them![f000]븁\u0000\nThat's the kind of person you are.[f000]븁\u0000\nI'm extremely grateful for your help.\nThis is a token of my gratitude.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route416_Text_ColressSeeJustLike, 14, 0, 0
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
    ActorMsg MSGFILE_SCRIPT, Route416_Text_ColressItsFrustratingIf, 14, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 14, 406, 563, 4, 8, 0
    VMSleep 8
    ActorCmdExec 255, Movement_0684
    ActorCmdWait
    BGMChangeMap
    ActorDelete 14
    ActorDelete 15
    ActorDelete 16
    ActorDelete 17
    ActorDelete 18
    ActorDelete 19
    ActorDelete 21
    ActorDelete 22
    ActorDelete 20
    WorkSetConst EVENT_WORK_0x40b6, 3
    FlagSet EVENT_FLAG_0x02f6
    FlagSet EVENT_FLAG_0x02f5
    FlagReset EVENT_FLAG_0x03dc
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03DC:
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_03E8:
    Move 14, 1
    MoveEnd

Movement_03F0:
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_03FC:
    Move 14, 1
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_040C:
    Move 14, 2
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_041C:
    Move 182, 1
    MoveEnd

Movement_0424:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_0430:
    Move 33, 1
    Move 159, 1
    MoveEnd

Movement_043C:
    Move 12, 12
    MoveEnd

Movement_0444:
    Move 12, 11
    MoveEnd

Script_3:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40b6
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I never would have guessed\nthey were Crustle...[f000]븁\u0000\nIf you're interested in Crustle,\nyou'll find them in the Desert Resort,[f000]븀\u0000\nwhich is just past here!"
    ParentActorMsg MSGFILE_SCRIPT, Route416_Text_NeverWouldHaveGuessed, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_048F

L_047B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, Trainer![f000]븁\u0000\nThese boulders suddenly\nlined up like this...[f000]븁\u0000\nWhat's more, the HM Strength\nwon't budge them."
    ParentActorMsg MSGFILE_SCRIPT, Route416_Text_HeyTrainerTheseBoulders, 0, 0
    LastKeyWait
    ActorMsgClose

L_048F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma said we should recognize\nthe potential in Pokémon and[f000]븀\u0000\nliberate them from humans.[f000]븁\u0000\nI disagree.[f000]븁\u0000\nConversely, it should be humans who bring\nout the hidden potential in Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route416_Text_TeamPlasmaSaidWe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "It's a big boulder, but it doesn't\nlook like a Pokémon can move it..."
    SystemMsg Route416_Text_ItsBigBoulderBut, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "All I do is look at the cars\ndriving down the freeway."
    ParentActorMsg MSGFILE_SCRIPT, Route416_Text_AllLookCarsDriving, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh! Trainer, take a look at the sand.[f000]븁\u0000\nDo you see how some areas are lighter?\nAnd some of the sand looks...darker...[f000]븁\u0000\nPokémon are hiding in the darker sand!"
    ParentActorMsg MSGFILE_SCRIPT, Route416_Text_OhTrainerTakeLook, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When the ruins were discovered,\nnew construction was stopped...[f000]븁\u0000\nGuess you can't compete\nagainst the weight of history."
    ParentActorMsg MSGFILE_SCRIPT, Route416_Text_WhenRuinsWereDiscovered, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 4"
    MsgPlaceSign Route416_Text_Route4, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nPokémon have a source of energy\nfor using moves.[f000]븁\u0000\nIt's called PP, meaning Power Points.\nThey have PP for each move.[f000]븁\u0000\nWhen a move has no PP remaining,\nthat Pokémon cannot use that move.[f000]븁\u0000\nThat's a good time to head for\nthe Pokémon Center!"
    MsgPlaceSign Route416_Text_TrainerTipsPokemonHave, 0
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Desert Resort Ahead\nRight: Nimbasa City"
    MsgPlaceSign Route416_Text_DesertResortAheadRight, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nPokémon have a source of energy\nfor using moves.[f000]븁\u0000\nIt's called PP, meaning Power Points.\nThey have PP for each move.[f000]븁\u0000\nWhen a move has no PP remaining,\nthat Pokémon cannot use that move.[f000]븁\u0000\nThat's a good time to head for\nthe Pokémon Center!"
    MsgPlaceSign Route416_Text_TrainerTipsPokemonHave, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a6b
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    PVPlay 628, 0
    // "Ra ra ra ra!"
    ScreamMsg Route416_Text_RaRaRaRa, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 128
    WorkOr 0x8024, 8
    CallWildBattle 628, 25, 0x8024
    WorkSetConst 0x8024, 0
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05EC
    FlagSet EVENT_FLAG_0x0386
    FlagSet EVENT_FLAG_DAILY_0x0ad6
    ActorDelete 12
    CallWildBattleEnd
    VMJump L_05EE

L_05EC:
    CallWildLose

L_05EE:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0605
    VMJump L_060B

L_0605:
    VMJump L_063B

L_060B:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_062B
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_062B
    VMJump L_063B

L_062B:
    // "Braviary flew off into the sky..."
    SystemMsg Route416_Text_BraviaryFlewOffInto, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_063B

L_063B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_064C:
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

Movement_0684:
    Move 32, 1
    MoveEnd

Movement_068C:
    Move 33, 1
    MoveEnd

Movement_0694:
    Move 34, 1
    MoveEnd

Movement_069C:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
