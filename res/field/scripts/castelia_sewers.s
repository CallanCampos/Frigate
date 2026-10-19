#include "asm/field_script.inc"
#include "text/script/castelia_sewers.h"

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
    WorkSetConst 0x8024, 0

Script_1:
    VMStackPush EVENT_WORK_0x40b3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0063
    FlagReset EVENT_FLAG_0x02f3

L_0063:
    VMHalt

Script_2:
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0088
    VMCall L_026B
    VMJump L_008E

L_0088:
    VMCall L_0094

L_008E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0094:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8021, 0x8022
    ActorGetGPos 0, 0x8023, 0x8024
    WorkSub 0x8021, 2
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00DB
    ActorWalkRoute 0, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0918
    ActorCmdWait

L_00DB:
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000!\nI need you to get tougher![f000]븁\u0000\nEven I'm going to have trouble\ntaking them on all by myself...[f000]븁\u0000\nAnyway, it's OK!\nI'll take care of healing our Pokémon![f000]븁\u0000\nI'm counting on you, [f000]Ā\u0001\u0000!\nAre you ready to go?"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_NeedGetTougherEven, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0251
    // "[f000]Ā\u0001\u0001: OK!\nLet's find Team Plasma![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_OkLetsFindTeam, 0, 0, 0
    MsgWinCloseAll
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0127
    PlayerSetSpecialSequence 1

L_0127:
    ActorCmdExec 255, Movement_08D8
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0194
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 0
    WorkSet 0x8001, 6
    WorkSet 0x8002, 1
    WorkSet 0x8003, 588
    WorkSet 0x8004, 10540
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0241

L_0194:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F7
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 0
    WorkSet 0x8001, 6
    WorkSet 0x8002, 1
    WorkSet 0x8003, 589
    WorkSet 0x8004, 10540
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0241

L_01F7:
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 0
    WorkSet 0x8001, 6
    WorkSet 0x8002, 1
    WorkSet 0x8003, 590
    WorkSet 0x8004, 10540
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000

L_0241:
    FlagSet EVENT_FLAG_0x02f3
    HollowRivalCmd_0262 1, 2
    VMJump L_0269

L_0251:
    // "[f000]Ā\u0001\u0001: C'mon!\nHurry up and get ready![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_CmonHurryUpGet, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_08D0
    ActorCmdWait

L_0269:
    VMReturn

L_026B:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029E
    ActorCmdExec 255, Movement_0900
    ActorCmdExec 254, Movement_08E8
    VMJump L_02D7

L_029E:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C7
    ActorCmdExec 255, Movement_0910
    ActorCmdExec 254, Movement_08F8
    VMJump L_02D7

L_02C7:
    ActorCmdExec 255, Movement_0910
    ActorCmdExec 254, Movement_08F8

L_02D7:
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: I suppose.\nGoing to step outside for a minute?"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_SupposeGoingStepOutside, 254, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_036A
    // "[f000]Ā\u0001\u0001: C'mon!\nHurry up and get ready![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_CmonHurryUpGet, 254, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 254, 81, 20, 1, 8, 0
    ActorCmdExec 255, Movement_08D0
    ActorCmdWait
    ActorCmdExec 254, Movement_0918
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 2000
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagReset EVENT_FLAG_0x02f3
    HollowRivalCmd_0262 1, 3
    ActorAdd 0
    ActorDelete 254
    VMJump L_0388

L_036A:
    // "[f000]Ā\u0001\u0001: OK!\nLet's find Team Plasma![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_OkLetsFindTeam, 254, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_08D8
    ActorCmdWait
    ActorPairSetMoveEnable 0

L_0388:
    VMReturn
    .balign 4, 0
    Move 13, 1
    Move 35, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8021, 0x8022
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x338000, 0, 0x65000, 32
    ActorWalkRoute 255, 52, 8, 1, 8, 0
    ActorWalkRoute 254, 50, 8, 1, 8, 0
    ActorCmdWait
    EvCameraWait
    WordSetPlayerName 0
    // "Hey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_Hey, 254, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0908
    ActorCmdExec 2, Movement_0908
    ActorCmdWait
    // "Team Plasma: What do you want?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_TeamPlasmaWhatWant, 1, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Team Plasma!\nYou villains![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_TeamPlasmaVillains, 254, 4, 0
    MsgWinCloseAll
    // "Team Plasma: What?\nDon't treat us like villains![f000]븁\u0000\nAnd don't interfere with our\nplans to liberate Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_TeamPlasmaWhatDont, 1, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Liberate? Ha!\nYou're just ordinary Pokémon thieves![f000]븁\u0000\nAnd what's more, you use those\nstolen Pokémon like they're tools![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_LiberateHaYoureJust, 254, 4, 0
    MsgWinCloseAll
    // "Team Plasma: Shaddap!\nHand over your Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_TeamPlasmaShaddapHand, 1, 3, 0
    MsgWinCloseAll
    ActorCmdExec 254, Movement_0918
    VMSleep 4
    ActorCmdExec 255, Movement_0910
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Let's do this, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_Lets, 254, 4, 0
    MsgWinCloseAll
    ActorCmdExec 254, Movement_0900
    ActorCmdExec 255, Movement_0900
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A4
    CallTrainerMultiBattle 588, 342, 356, 0
    VMJump L_04D1

L_04A4:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C7
    CallTrainerMultiBattle 589, 342, 356, 0
    VMJump L_04D1

L_04C7:
    CallTrainerMultiBattle 590, 342, 356, 0

L_04D1:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0529
    PokePartyGetCount 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0503
    PokePartyRecoverAll

L_0503:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x338000, 0, 0x65000, 1
    EvCameraWait
    CallTrainerBattleEnd
    VMJump L_052B

L_0529:
    CallTrainerLose

L_052B:
    // "Team Plasma: They're really tough![f000]븁\u0000\nBut whatever!\nWe've got the Pokémon we need![f000]븀\u0000\nNow, we flee![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_TeamPlasmaTheyreReally, 1, 3, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0854
    VMSleep 4
    ActorCmdExec 2, Movement_0864
    VMSleep 8
    ActorCmdExec 255, Movement_0908
    ActorCmdExec 254, Movement_0908
    ActorCmdWait
    ActorDelete 1
    ActorDelete 2
    // "[f000]Ā\u0001\u0001: Tch!\nThey run away like Patrat![f000]븁\u0000\nOK! Let's go farther inside!\nThere might be more in there![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_TchTheyRunAway, 254, 4, 0
    MsgWinCloseAll
    // "???: I don't think that's\nnecessary, you two.[f000]븁\u0000"
    InfoMsg CasteliaSewers_Text_DontThinkThatsNecessary, 1
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0920
    ActorCmdExec 254, Movement_0920
    ActorCmdWait
    ActorWalkRoute 20, 51, 7, 0, 8, 0
    VMSleep 40
    ActorCmdExec 254, Movement_0900
    ActorCmdExec 255, Movement_0900
    ActorCmdWait
    ActorCmdExec 20, Movement_0908
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Could you be\nGym Leader Burgh?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_CouldGymLeaderBurgh, 254, 4, 0
    MsgWinCloseAll
    // "Burgh: Yessir! One and the same!\nI'm Burgh.[f000]븁\u0000\nAnd there was no one suspicious\nbeyond here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_BurghYessirOneSame, 20, 5, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Is that...so?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_IsThatSo, 254, 4, 0
    MsgWinCloseAll
    // "Burgh: Yes indeedy![f000]븁\u0000\nI'm concerned about\nTeam Plasma as well.[f000]븁\u0000\nMore importantly, shall\nwe leave this place?[f000]븁\u0000\nFor some reason,\nthis sewer is bugging me.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_BurghYesIndeedyIm, 20, 5, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Huh...[f000]븁\u0000\nDidn't you and the other Gym Leaders\nfight Team Plasma two years ago?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_HuhDidntOtherGym, 254, 4, 0
    MsgWinCloseAll
    ActorCmdExec 254, Movement_0918
    ActorCmdExec 255, Movement_0910
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Thanks for your help.\nHere, use this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_ThanksHelpHereUse, 254, 4, 0
    MsgWinCloseAll
    ActorCmdExec 254, Movement_08D0
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 423
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "I'll find those other\nTeam Plasma punks![f000]븁\u0000\nI... I'll never forgive Pokémon thieves!\nNo way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_IllFindThoseOther, 254, 4, 0
    MsgWinCloseAll
    ActorCmdExec 254, Movement_0894
    VMSleep 8
    ActorCmdExec 255, Movement_0908
    ActorCmdWait
    ActorWalkRoute 20, 50, 8, 0, 8, 0
    ActorCmdWait
    // "Burgh: He's gone...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_BurghHesGone, 20, 4, 0
    MsgWinCloseAll
    // "???: You there![f000]븁\u0000"
    InfoMsg CasteliaSewers_Text_There, 1
    InfoMsgClose_0039
    ActorCmdExec 255, Movement_0920
    VMSleep 4
    ActorCmdExec 20, Movement_0920
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorSetGPos 19, 51, 0, 0, 1
    ActorCmdExec 20, Movement_0900
    VMSleep 4
    ActorCmdExec 255, Movement_0900
    VMSleep 4
    SEWait
    ActorCmdWait
    ActorCmdExec 19, Movement_08A0
    ActorCmdWait
    // "That was an excellent demonstration\nof battling. The way you brought out[f000]븀\u0000\nthe power of your Pokémon against[f000]븀\u0000\nan opponent like Team Plasma...[f000]븁\u0000\nAstounding. Simply astounding.\nInteresting as well. I see...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_ExcellentDemonstrationBattlingWay, 19, 5, 0
    MsgWinCloseAll
    ActorCmdExec 19, Movement_08A8
    VMSleep 40
    ActorCmdExec 20, Movement_0908
    ActorCmdExec 255, Movement_0908
    ActorCmdWait
    ActorCmdExec 20, Movement_0928
    ActorCmdWait
    // "Burgh: Who...was that?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_BurghWho, 20, 4, 0
    MsgWinCloseAll
    ActorCmdExec 20, Movement_0918
    VMSleep 4
    ActorCmdExec 255, Movement_0910
    ActorCmdWait
    // "Well. No matter.\nWhat are you going to do?[f000]븁\u0000\nYou can stay here\nand train your Pokémon.[f000]븁\u0000\nOr maybe you should come\nchallenge me, the Gym Leader![f000]븁\u0000\nBe seeing you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_WellNoMatterWhat, 20, 4, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 32
    ActorCmdExec 20, Movement_08B4
    VMSleep 12
    ActorCmdExec 255, Movement_0908
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 2000
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorDelete 254
    ActorDelete 19
    ActorDelete 20
    ActorDelete 21
    WorkSetConst EVENT_WORK_0x40b3, 1
    WorkSetConst EVENT_WORK_0x40b2, 3
    FlagSet EVENT_FLAG_0x02f3
    FlagSet EVENT_FLAG_0x02f7
    FlagSet EVENT_FLAG_0x03ed
    FlagSet EVENT_FLAG_0x02f0
    HollowRivalCmd_0262 1, 4
    MedalDiscover 58
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    ActorCmdExec 21, Movement_0848
    VMSleep 40
    ActorCmdExec 255, Movement_0900
    ActorCmdWait
    // "Waitasecond![f000]븁\u0000\nI dropped something here!\nJust wait until I find it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_WaitasecondDroppedSomethingHere_2, 21, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorCmdExec 21, Movement_0918
    ActorCmdExec 255, Movement_08D0
    ActorCmdWait
    ActorPairSetMoveEnable 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Waitasecond![f000]븁\u0000\nI dropped something here!\nJust wait until I find it!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_WaitasecondDroppedSomethingHere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Phew...\nI finally made it back here.[f000]븁\u0000\nAre you going farther inside?\nThere's nothing in there but Trainers."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaSewers_Text_PhewFinallyMadeBack, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0848:
    Move 75, 1
    Move 33, 1
    MoveEnd

Movement_0854:
    Move 19, 1
    Move 17, 4
    Move 19, 9
    MoveEnd

Movement_0864:
    Move 18, 1
    Move 17, 4
    Move 19, 9
    MoveEnd
    VMHalt
    .byte 0x01
    .balign 4, 0
    Move 71, 1
    Move 15, 1
    Move 72, 1
    MoveEnd
    Move 14, 1
    Move 32, 1
    MoveEnd

Movement_0894:
    Move 13, 2
    Move 15, 9
    MoveEnd

Movement_08A0:
    Move 13, 5
    MoveEnd

Movement_08A8:
    Move 13, 5
    Move 15, 10
    MoveEnd

Movement_08B4:
    Move 13, 2
    Move 15, 9
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_08D0:
    Move 15, 1
    MoveEnd

Movement_08D8:
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_08E8:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0

Movement_08F8:
    Move 3, 1
    MoveEnd

Movement_0900:
    Move 32, 1
    MoveEnd

Movement_0908:
    Move 33, 1
    MoveEnd

Movement_0910:
    Move 34, 1
    MoveEnd

Movement_0918:
    Move 35, 1
    MoveEnd

Movement_0920:
    Move 75, 1
    MoveEnd

Movement_0928:
    Move 159, 1
    MoveEnd
