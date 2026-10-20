#include "asm/field_script.inc"
#include "text/script/route_20.h"

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

Script_7:
    WorkSetConst 0x8023, 0
    RTCGetSeason 0x8023
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0067
    FlagSet EVENT_FLAG_0x038e
    VMJump L_006B

L_0067:
    FlagReset EVENT_FLAG_0x038e

L_006B:
    VMStackPush EVENT_WORK_0x40ab
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0090
    ObjInitNPCGPos 21, 1, 162, 65533, 668
    VMJump L_00AF

L_0090:
    VMStackPush EVENT_WORK_0x40ab
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AF
    ObjInitNPCGPos 21, 1, 151, 2, 646

L_00AF:
    VMStackPush EVENT_WORK_0x40ab
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00CE
    ObjInitNPCGPos 0, 1, 160, 2, 643

L_00CE:
    WorkSetConst 0x8023, 0
    VMHalt

Script_8:
    VMHalt

Script_1:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 644
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010D
    ActorCmdExec 0, Movement_0170
    VMSleep 46
    ActorCmdExec 255, Movement_07BC
    VMJump L_0134

L_010D:
    VMStackPush 0x8022
    VMStackPushConst 646
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0134
    ActorCmdExec 0, Movement_017C
    VMSleep 46
    ActorCmdExec 255, Movement_07B4

L_0134:
    ActorCmdWait
    // "Come on! A kid without a single\nGym Badge continuing on past here?[f000]븁\u0000\nBattle with the Trainers and Pokémon\nin this area, then battle some more![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_ComeKidWithoutSingle_2, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 645
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_015F
    ActorCmdExec 0, Movement_07C4

L_015F:
    ActorCmdExec 255, Movement_078C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0170:
    Move 75, 1
    Move 32, 1
    MoveEnd

Movement_017C:
    Move 75, 1
    Move 33, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    ActorCmdExec 0, Movement_07D4
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01B5
    VMSleep 20
    ActorCmdExec 255, Movement_07CC

L_01B5:
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 644
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E0
    ActorCmdExec 0, Movement_026C
    ActorCmdWait
    VMJump L_01FD

L_01E0:
    VMStackPush 0x8022
    VMStackPushConst 646
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FD
    ActorCmdExec 0, Movement_0278
    ActorCmdWait

L_01FD:
    // "Hey! That gleaming thing there\nis the Basic Badge![f000]븁\u0000\nBut don't get a swelled head!\nIt's a rough world out there![f000]븀\u0000\nHere, I'll show you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_HeyGleamingThingThere, 0, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_HIKER_JEROME, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0232
    CallTrainerBattleEnd
    VMJump L_0234

L_0232:
    CallTrainerLose

L_0234:
    // "All right... I'm beat![f000]븁\u0000\nWith dependable Pokémon like that,\neven a kid like you can[f000]븀\u0000\nhold your own against an adult. Yup!"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_AllRightImBeat, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 160, 643, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_07BC
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40ab, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_026C:
    Move 12, 1
    Move 34, 1
    MoveEnd

Movement_0278:
    Move 13, 1
    Move 34, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40ab
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_02B3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "All right... I'm beat![f000]븁\u0000\nWith dependable Pokémon like that,\neven a kid like you can[f000]븀\u0000\nhold your own against an adult. Yup!"
    ParentActorMsg MSGFILE_SCRIPT, Route20_Text_AllRightImBeat, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02C7

L_02B3:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Come on! A kid without a single\nGym Badge continuing on past here?[f000]븁\u0000\nBattle with the Trainers and Pokémon\nin this area, then battle some more!"
    ParentActorMsg MSGFILE_SCRIPT, Route20_Text_ComeKidWithoutSingle, 0, 0
    LastKeyWait
    ActorMsgClose

L_02C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0000! Wait![f000]븁\u0000"
    InfoMsg Route20_Text_Wait, 2
    MsgWinCloseAll
    ActorSetGPos 21, 160, 65535, 650, 1
    ActorSetGPos 23, 161, 65535, 650, 1
    PlayerGetGPos 0x8021, 0x8022
    BGMPlay SEQ_BGM_E_CHEREN
    ActorWalkRoute 23, 161, 658, 4, 8, 0
    VMSleep 4
    ActorWalkRoute 21, 160, 658, 4, 8, 0
    VMSleep 8
    ActorCmdExec 255, Movement_07B4
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 161
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0359
    ActorWalkRoute 255, 161, 660, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_07B4
    ActorCmdWait

L_0359:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "Cheren: Come with me, you two.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_CherenComeTwo, 23, 0, 0
    MsgWinCloseAll
    ActorCmdExec 23, Movement_04AC
    VMSleep 16
    ActorCmdExec 255, Movement_07CC
    ActorCmdExec 21, Movement_04BC
    ActorCmdWait
    ActorCmdExec 23, Movement_04D0
    VMSleep 4
    ActorCmdExec 21, Movement_04E0
    ActorCmdExec 255, Movement_04E0
    ActorCmdWait
    ActorCmdExec 23, Movement_04C8
    ActorCmdWait
    // "See the dark tall grass here?[f000]븁\u0000\nIt's rare, but sometimes two\nPokémon pop out at the same time.[f000]븁\u0000\nAlso, the Pokémon that hide in\ndark grass are slightly stronger.[f000]븁\u0000\nSo be careful if you walk through.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_SeeDarkTallGrass, 23, 0, 0
    MsgWinCloseAll
    ActorCmdExec 23, Movement_07B4
    ActorCmdWait
    // "If you're going to challenge the\nnext Gym, it's the Virbank Gym.[f000]븀\u0000\nThese might help![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_IfYoureGoingChallenge, 23, 0, 0
    MsgWinCloseAll
    ActorCmdExec 23, Movement_077C
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 151
    WorkSet 0x8001, 3
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 23, Movement_04F0
    ActorCmdWait
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "Here are some for you, [f000]Ā\u0001\u0001![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_HereSome, 23, 0, 0
    MsgWinCloseAll
    // "Both of you, do your best![f000]븁\u0000\nIf you need anything,\ncall me on the Xtransceiver.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_BothBestIfNeed, 23, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 23, 161, 653, 1, 8, 0
    VMSleep 16
    ActorCmdExec 21, Movement_07B4
    ActorCmdExec 255, Movement_07B4
    ActorCmdWait
    BGMChangeMap
    // "[f000]Ā\u0001\u0001: Cheren sure knows a lot.[f000]븁\u0000\nAnd he fought those\nTeam Plasma thugs, too...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_CherenSureKnowsLot, 21, 0, 0
    MsgWinCloseAll
    ActorCmdExec 21, Movement_07CC
    ActorCmdExec 255, Movement_07C4
    ActorCmdWait
    // "I've decided!\nI'm going to get stronger than him![f000]븁\u0000\nYou should...[f000]븁\u0000\nDo your best! Fill up the Pokédex\nand have my back. Got it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_IveDecidedImGoing, 21, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 21, 162, 668, 1, 8, 0
    ActorCmdWait
    ActorDelete 23
    WorkSetConst EVENT_WORK_0x40ab, 3
    FlagSet EVENT_FLAG_0x02bf
    FlagSet EVENT_FLAG_0x0408
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04AC:
    Move 15, 1
    Move 13, 2
    Move 34, 1
    MoveEnd

Movement_04BC:
    Move 13, 2
    Move 35, 1
    MoveEnd

Movement_04C8:
    Move 181, 1
    MoveEnd

Movement_04D0:
    Move 13, 3
    Move 15, 2
    Move 13, 2
    MoveEnd

Movement_04E0:
    Move 13, 3
    Move 15, 3
    Move 33, 1
    MoveEnd

Movement_04F0:
    Move 14, 1
    Move 32, 1
    MoveEnd

Script_9:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush EVENT_WORK_0x40ab
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0531
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: Help me check Route 20!"
    ParentActorMsg MSGFILE_SCRIPT, Route20_Text_HelpCheckRoute20, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0545

L_0531:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I have to get stronger\nthan Team Plasma!"
    ParentActorMsg MSGFILE_SCRIPT, Route20_Text_HaveGetStrongerThan, 0, 0
    LastKeyWait
    ActorMsgClose

L_0545:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorSetGPos 21, 146, 2, 663, 3
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_48
    // "Yeesh! I really hate it when\npeople won't let things go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_YeeshReallyHateWhen, 22, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_48, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0598
    CallTrainerBattleEnd
    VMJump L_059A

L_0598:
    CallTrainerLose

L_059A:
    // "Now that I think about it...[f000]븁\u0000\nIf I run this way, no one can help me!\nI have to head toward the ocean![f000]븁\u0000\nSo, with that in mind...[f000]븁\u0000\nI'm going to flee again!\nGood-bye, Virbank City![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_NowThinkAboutIf, 22, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_05D3
    ActorWalkRoute 22, 155, 653, 1, 4, 0
    VMJump L_05E1

L_05D3:
    ActorWalkRoute 22, 156, 653, 1, 4, 0

L_05E1:
    VMSleep 8
    ActorCmdExec 255, Movement_07B4
    ActorCmdWait
    FlagReset EVENT_FLAG_0x09fe
    BGMChangeMap
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0630
    ActorWalkRoute 21, 156, 666, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_07CC
    ActorCmdExec 21, Movement_07C4
    ActorCmdWait
    VMJump L_06DB

L_0630:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_066B
    ActorWalkRoute 21, 155, 667, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_07C4
    ActorCmdExec 21, Movement_07CC
    ActorCmdWait
    VMJump L_06DB

L_066B:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06A6
    ActorWalkRoute 21, 156, 668, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_07CC
    ActorCmdExec 21, Movement_07C4
    ActorCmdWait
    VMJump L_06DB

L_06A6:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06DB
    ActorWalkRoute 21, 155, 667, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_07CC
    ActorCmdExec 21, Movement_07C4
    ActorCmdWait

L_06DB:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Well, they're\nreally good at running, anyway...[f000]븁\u0000\nStill, she just said something useful![f000]븁\u0000\nThey must be moving around by boat![f000]븁\u0000\nCastelia City has a big port...\nI wonder if they headed there.[f000]븁\u0000\nOK! Let’s go back to Virbank\nand look for someone who looks like[f000]븀\u0000\nthey know a lot about ships.[f000]븁\u0000\nIf I remember right, there was a place\nto board boats on the docks.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route20_Text_WellTheyreReallyGood, 21, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 21, 155, 654, 1, 8, 1
    VMSleep 8
    ActorCmdExec 255, Movement_07B4
    ActorCmdWait
    ActorDelete 21
    ActorDelete 22
    FlagSet EVENT_FLAG_0x02bd
    FlagSet EVENT_FLAG_0x02be
    WorkSetConst EVENT_WORK_0x40ab, 6
    FlagReset EVENT_FLAG_0x02d6
    WorkSetConst EVENT_WORK_0x40ac, 7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon battles begin\nwhen eyes meet![f000]븀\u0000\nThat's a rule for Trainers!"
    ParentActorMsg MSGFILE_SCRIPT, Route20_Text_PokemonBattlesBeginWhen, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 20"
    MsgPlaceSign Route20_Text_Route20, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    CallLeafPileStuck
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_077C:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_078C:
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

Movement_07B4:
    Move 32, 1
    MoveEnd

Movement_07BC:
    Move 33, 1
    MoveEnd

Movement_07C4:
    Move 34, 1
    MoveEnd

Movement_07CC:
    Move 35, 1
    MoveEnd

Movement_07D4:
    Move 75, 1
    MoveEnd
