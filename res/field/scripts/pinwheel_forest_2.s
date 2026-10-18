#include "asm/field_script.inc"
#include "text/script/pinwheel_forest_2.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_11:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0065
    HollowRivalCmd_0262 2, 14

L_0065:
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0088
    VMCall L_03D8
    VMJump L_008E

L_0088:
    VMCall L_0269

L_008E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 5
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorWalkRoute 255, 43, 30, 0, 8, 1
    ActorCmdExec 21, Movement_01F8
    ActorCmdWait
    ActorNew 43, 41, 0, 251, 190, 0
    ActorWalkRoute 251, 43, 32, 0, 4, 1
    ActorCmdWait
    // "Gorm: Boo![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_GormBoo, 251, 4, 1
    ActorMsgClose
    ActorCmdExec 255, Movement_01EC
    ActorCmdExec 21, Movement_01EC
    ActorCmdWait
    // "I am Gorm. I was once one\nof Team Plasma's Seven Sages.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_AmGormOnceOne, 251, 4, 0
    MsgWinCloseAll
    // "Cheren: Team Plasma's finished.[f000]븁\u0000\nDespite that, you still haven't given up?\nAre you here planning something?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_CherenTeamPlasmasFinished, 21, 5, 0
    MsgWinCloseAll
    // "Gorm: Wait one moment!\nI have no plans to confront you.[f000]븁\u0000\nI don't mean to disappoint you,\nbut I doubt I'm a match for either[f000]븀\u0000\nof you in the first place...[f000]븁\u0000\nHm?\nWhat happened to your glasses?[f000]븁\u0000\nExcuse me, but that's not important.[f000]븁\u0000\nI learned of my old ally's recklessness,\nand I had come here to admonish him...[f000]븁\u0000\nBut the matter had already been\nresolved, and this place made me think...[f000]븁\u0000\nWhat did we believe in that made\nus try to steal the Dragon Skull?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_GormWaitOneMoment, 251, 4, 0
    // "A man who has committed a mistake\nand doesn't correct it[f000]븀\u0000\nis committing another mistake.[f000]븁\u0000\nDo you understand what this means?[f000]븁\u0000\nAvoiding all mistakes is impossible,\nbut not fixing mistakes you've made--[f000]븀\u0000\nthat is truly foolish.[f000]븁\u0000\nThat being said, this doesn't\nreally concern you, does it?[f000]븁\u0000\nWell then, Trainers, may you and your\nPokémon be well.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_ManWhoHasCommitted, 251, 4, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0210
    VMSleep 16
    ActorCmdExec 255, Movement_059C
    ActorCmdExec 21, Movement_059C
    ActorCmdWait
    // "Cheren: You know...[f000]븁\u0000\nIf it wasn't for Ghetsis,\nhe might've chosen another path...[f000]븁\u0000\nOr maybe not. He was the one who\ndecided to follow Ghetsis, after all...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_CherenKnowIfWasnt, 21, 5, 0
    MsgWinCloseAll
    ActorCmdExec 21, Movement_0594
    ActorCmdWait
    // "That aside, thank you![f000]븁\u0000\nYour help made this\ninvestigation go smoothly.[f000]븁\u0000\nThis is my thanks!\nCome on, just take it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_AsideThankHelpMade, 21, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 252
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "OK! Be seeing you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_OkSeeing, 21, 5, 0
    MsgWinCloseAll
    ActorCmdExec 21, Movement_0208
    ActorCmdWait
    ActorDelete 21
    ActorDelete 251
    ActorDelete 24
    WorkSetConst EVENT_WORK_0x4111, 1
    FlagSet EVENT_FLAG_0x038c
    FlagSet EVENT_FLAG_0x03e7
    HollowRivalCmd_0262 2, 0
    FlagReset EVENT_FLAG_0x03e8
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01EC:
    Move 75, 1
    Move 33, 1
    MoveEnd

Movement_01F8:
    Move 12, 2
    Move 15, 1
    Move 12, 2
    MoveEnd

Movement_0208:
    Move 15, 7
    MoveEnd

Movement_0210:
    Move 12, 1
    Move 15, 8
    MoveEnd

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    WorkSetConst 0x8008, 5
    WorkAdd 0x8008, EVENT_WORK_0x418a
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 254, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x418a
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0263
    WorkAdd EVENT_WORK_0x418a, 1

L_0263:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0269:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8021, 0x8022
    ActorGetGPos 21, 0x8023, 0x8024
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02B4
    DebugPrint 99
    ActorWalkRoute 21, 68, 0x8022, 0, 8, 0
    ActorCmdWait

L_02B4:
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02D1
    ActorCmdExec 21, Movement_059C
    ActorCmdWait

L_02D1:
    VMStackPushFlag EVENT_FLAG_0x0197
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F6
    // "Cheren: Hey, nice timing![f000]븁\u0000\nI heard that Team Plasma was seen\nin Pinwheel Forest...[f000]븁\u0000\nCould you help me look for them?"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_CherenHeyNiceTiming, 21, 1, 0
    VMJump L_0302

L_02F6:
    // "Cheren: Team Plasma was seen inside\nPinwheel Forest...[f000]븁\u0000\nBut you already know that.\nWill you help me look for them?"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_CherenTeamPlasmaSeen, 21, 1, 0

L_0302:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03A6
    // "Thank you![f000]븁\u0000\nThis is a good opportunity for me\nto see up close what you can really do.[f000]븀\u0000\nI suppose I'll follow your lead.[f000]븁\u0000\nLeave recovery to me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_ThankGoodOpportunitySee, 21, 1, 0
    MsgWinCloseAll
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0342
    PlayerSetSpecialSequence 1

L_0342:
    ActorCmdExec 255, Movement_055C
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 21
    WorkSet 0x8001, 5
    WorkSet 0x8002, 0
    WorkSet 0x8003, 632
    WorkSet 0x8004, 5
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0197
    WorkSetConst EVENT_WORK_0x400f, 0
    VMJump L_03D6

L_03A6:
    ActorCmdExec 21, Movement_053C
    ActorCmdWait
    // "We don't know how many there are,\nso splitting up doesn't seem like[f000]븀\u0000\na very good tactic.[f000]븁\u0000\nGot it! I'll wait here until\nyou're ready to go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_WeDontKnowHow, 21, 1, 0
    MsgWinCloseAll
    ActorCmdExec 21, Movement_057C
    ActorCmdExec 255, Movement_0554
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x400f, 1

L_03D6:
    VMReturn

L_03D8:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 255, Movement_0594
    ActorCmdExec 254, Movement_057C
    ActorCmdWait
    // "Cheren: Stop![f000]븁\u0000\nIf you go past here, we'll leave\nthe Pinwheel Forest.[f000]븁\u0000\nWe still haven't found Team Plasma,\nbut do you need to leave for a minute?"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_CherenStopIfGo, 254, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046F
    // "We don't know how many there are,\nso splitting up doesn't seem like[f000]븀\u0000\na very good tactic.[f000]븁\u0000\nGot it! I'll wait here until\nyou're ready to go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_WeDontKnowHow, 254, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 5
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 21, 67, 69, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 21, Movement_059C
    ActorCmdExec 255, Movement_0554
    ActorCmdWait
    VMJump L_048D

L_046F:
    // "Thank you![f000]븁\u0000\nThis is a good opportunity for me\nto see up close what you can really do.[f000]븀\u0000\nI suppose I'll follow your lead.[f000]븁\u0000\nLeave recovery to me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_ThankGoodOpportunitySee, 254, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_055C
    ActorCmdWait
    ActorPairSetMoveEnable 0

L_048D:
    VMReturn

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The legendary Pokémon...\nIs it true it was really beyond here?"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_LegendaryPokemonTrueReally, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 24, Movement_059C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm going to do a lap around Unova\nclockwise from Nimbasa City[f000]븀\u0000\nwithout healing my Pokémon![f000]븁\u0000\nIt's the Unova Spartan Marathon,\nand next time, I'm going to race!"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_ImGoingLapAround, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We're thinning trees to\nprotect the forest.[f000]븁\u0000\nThat's why we're having Pokémon\ncut down trees.[f000]븁\u0000\nWhen there are too many trees,\nthe whole forest gets weaker...[f000]븁\u0000\nThese trees are being cut down\nso the whole forest will thrive..."
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_WereThinningTreesProtect, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ah, wouldn't it be nice if the Pokémon\nliving in the forest liked the sunbeams[f000]븀\u0000\nfiltering through the leaves, too!"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_AhWouldntNiceIf, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The surface is covered with moss.\nTouching it feels good somehow."
    InfoMsg PinwheelForest2_Text_SurfaceCoveredMossTouching, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nA forest is likely to contain many\nwell-hidden items![f000]븁\u0000\nThey may be hard to find,\nso look carefully!"
    MsgPlaceSign PinwheelForest2_Text_TrainerTipsForestLikely, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a67
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_053C:
    Move 181, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0554:
    Move 15, 1
    MoveEnd

Movement_055C:
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

Movement_057C:
    Move 3, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_0594:
    Move 34, 1
    MoveEnd

Movement_059C:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01d6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0694
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi, Trainer.[f000]븁\u0000\nIf you have a Pokédex, could you show me\nyour Habitat List?[f000]븁\u0000\nI want to know about the Pokémon\nthat live in Pinwheel Forest.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_HiTrainerIfHave, 0, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    PokeDexCheckHabitatList 154, 0, 0, 0x8025
    PokeDexCheckHabitatList 154, 1, 0, 0x8026
    PokeDexCheckHabitatList 154, 2, 0, 0x8027
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0680
    // "Perfect!\nThis is my thanks![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_PerfectThanks, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 8
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01d6
    // "Finding the Pokémon that can\nonly be found in the rustling grass[f000]븀\u0000\nis really amazing!"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_FindingPokemonCanOnly, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_068E

L_0680:
    // "You still have many meetings\nwaiting for you...[f000]븁\u0000\nTell me when you've encountered\nall of the Pokémon in Pinwheel Forest."
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_StillHaveManyMeetings, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_068E:
    VMJump L_06A8

L_0694:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Finding the Pokémon that can\nonly be found in the rustling grass[f000]븀\u0000\nis really amazing!"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest2_Text_FindingPokemonCanOnly, 0, 0
    LastKeyWait
    ActorMsgClose

L_06A8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
