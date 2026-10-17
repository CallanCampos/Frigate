#include "asm/field_script.inc"
#include "text/script/p2_laboratory_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_3:
    FlagReset EVENT_FLAG_0x0290
    VMStackPush EVENT_WORK_0x408f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0066
    WorkSetConst 0x8020, 0
    PokePartyFindEx 649, 0, 0x8020, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005C
    WorkSetConst EVENT_WORK_0x408f, 1
    VMJump L_0060

L_005C:
    FlagSet EVENT_FLAG_0x0290

L_0060:
    WorkSetConst 0x8020, 0

L_0066:
    VMHalt

Script_5:
    VMStackPush EVENT_WORK_0x408f
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0087
    ActorSetGPos 0, 6, 0, 5, 1

L_0087:
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x408f, 2
    VMCall L_012A
    // "Battle the Scientist?"
    SystemMsg P2Laboratory2_Text_BattleScientist, 0
    YesNoWin 0x8010
    InfoMsgClose
    VMCall L_020E
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x408f, 2
    VMJumpIf CMP_EQ, L_00CA
    VMJump L_00FB

L_00CA:
    // "So you want to battle me?!"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_WantBattle, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EF
    ActorMsgClose

L_00EF:
    VMCall L_020E
    VMJump L_0124

L_00FB:
    WorkCmpConst EVENT_WORK_0x408f, 3
    VMJumpIf CMP_EQ, L_010E
    VMJump L_0124

L_010E:
    // "I've given you all of the Drives.\nUse them how you wish..."
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_IveGivenAllDrives, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0124

L_0124:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_012A:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x68000, 0, 0x38000, 16
    EvCameraWait
    ActorCmdExec 0, Movement_03A8
    ActorCmdWait
    // "Oh, it's just a kid...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_OhItsJustKid, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_03BC
    ActorCmdExec 255, Movement_03A0
    ActorCmdWait
    // "You... It couldn't be...[f000]븁\u0000\nThe fact that you are here means you\ncame to learn about the secret[f000]븀\u0000\nof Genesect![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_CouldntFactHereMeans, 0, 0, 0
    ActorMsgClose
    EvCameraMoveTo 9688, 0, 0xed000, 0x68000, 0, 0x48000, 8
    ActorCmdExec 0, Movement_03CC
    ActorCmdWait
    EvCameraWait
    // "We, Team Plasma, revived Genesect from\na Fossil.[f000]븁\u0000\nThen we enhanced the Pokémon with the\npower of science![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_WeTeamPlasmaRevived, 0, 0, 0
    ActorMsgClose
    // "It is the strongest Pokémon in history!![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_StrongestPokemonHistory, 0, 0, 1
    ActorMsgClose
    EvCameraMoveToDefault 12
    ActorCmdExec 0, Movement_03CC
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    // "But, our lord N was not interested in\nthis Genesect that was modified by the[f000]븀\u0000\npower of science![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_ButOurLordN, 0, 0, 0
    ActorMsgClose
    // "“Science damages the natural beauty\nof Pokémon! They're perfect beings!\"[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_ScienceDamagesNaturalBeauty, 0, 0, 1
    ActorMsgClose
    // "That's what he said...[f000]븁\u0000\nSo our research was halted, and this\nfacility was closed...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_ThatsWhatHeSaid, 0, 0, 0
    ActorMsgClose
    // "However!!\nThe Genesect research is all mine![f000]븁\u0000\nSo if you want to know Genesect's\nsecret, you'll have to beat me in battle![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_HoweverGenesectResearchAll, 0, 0, 1
    ActorMsgClose
    VMReturn

L_020E:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0341
    CallTrainerBattle TRAINER_SCIENTIST_DUDLEY, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024E
    VMCall L_0353
    CallTrainerBattleEnd
    VMJump L_0250

L_024E:
    CallTrainerLose

L_0250:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    GameGetVersion 0x8021
    VMStackPush 0x8021
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028B
    WorkSetConst 0x8022, 117
    WorkSetConst 0x8023, 118
    VMJump L_0297

L_028B:
    WorkSetConst 0x8022, 116
    WorkSetConst 0x8023, 119

L_0297:
    // "I've lost everything...[f000]븁\u0000\nI forgot my duty as a Scientist is\nto make the world happy.[f000]븁\u0000\nSo, this must be what I get for trying to\nmake a Pokémon into a tool for fighting...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_IveLostEverythingForgot, 0, 0, 0
    ActorMsgClose
    // "I'm going to wash my hands of this\nGenesect matter...[f000]븁\u0000\nI don't need this anymore...\nI'll give it to you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_ImGoingWashHands, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8022
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "The item I just gave you was made\nfor Genesect.[f000]븁\u0000\nWhen it holds an item like this, it\nchanges the type of the move called[f000]븀\u0000\nTechno Blast, so it can always have[f000]븀\u0000\nan advantage.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_ItemJustGaveMade, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_03D4
    ActorCmdWait
    // "There was another one in my\nother pocket...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_ThereAnotherOneOther, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8023
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 0, Movement_03E8
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x408f, 3
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    VMJump L_0351

L_0341:
    // "There sure are some very cowardly\nTrainers, aren't there!"
    ActorMsg MSGFILE_SCRIPT, P2Laboratory2_Text_ThereSureSomeVery, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0351:
    VMReturn

L_0353:
    ActorSetGPos 0, 6, 0, 5, 1
    ActorSetGPos 255, 6, 0, 6, 0
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "There is a memo stuck to the\nside of the monitor.[f000]븁\u0000\nRead it?"
    SystemMsg P2Laboratory2_Text_ThereMemoStuckSide, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0398
    // "“My lord N apparently rejected\nmy research...[f000]븀\u0000\nBut my research is necessary for[f000]븀\u0000\nTeam Plasma to reach its goal...[f000]븀\u0000\nThe strongest Pokémon...\"[f000]븁\u0000\nThe rest of the memo is torn and\nimpossible to read."
    SystemMsg P2Laboratory2_Text_LordNApparentlyRejected, 0
    LastKeyWait

L_0398:
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_03A0:
    Move 12, 3
    MoveEnd

Movement_03A8:
    Move 75, 1
    Move 62, 1
    Move 33, 1
    Move 63, 1
    MoveEnd

Movement_03BC:
    Move 13, 1
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_03CC:
    Move 13, 1
    MoveEnd

Movement_03D4:
    Move 12, 1
    Move 63, 1
    Move 75, 1
    Move 13, 1
    MoveEnd

Movement_03E8:
    Move 12, 2
    Move 14, 2
    Move 12, 1
    MoveEnd
