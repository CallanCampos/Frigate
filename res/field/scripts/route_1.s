#include "asm/field_script.inc"
#include "text/script/route_1.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_9:
    VMCall L_0081
    VMHalt

Script_8:
    Cmd_02B2 0, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0079
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0079
    VMStackPush EVENT_WORK_0x4136
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0079
    WorkSetConst EVENT_WORK_0x4136, 1

L_0079:
    VMCall L_0081
    VMHalt

L_0081:
    Cmd_02B2 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A0
    WorkSetConst EVENT_WORK_0x4136, 0

L_00A0:
    VMReturn

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 1"
    MsgPlaceSign Route1_Text_Route1, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 1"
    MsgPlaceSign Route1_Text_Route1_2, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nMake an effort to talk to all the\npeople you meet during your journey![f000]븁\u0000\nChances are they will have something\nuseful to tell you."
    MsgPlaceSign Route1_Text_TrainerTipsMakeEffort, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a68
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    WorkSet 0x8002, 425
    WorkSet 0x8003, 0
    WorkSet 0x8004, 1
    WorkSet 0x8005, 1
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    Cmd_02B2 0, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019B
    Cmd_02B5 0, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Two years ago, [f000]Ā\u0001\u0000, a Trainer\nfrom Nuvema did some amazing things,[f000]븀\u0000\nincluding battling Team Plasma[f000]븀\u0000\nand saving Unova![f000]븁\u0000\nI was the one who told that Trainer that\nwild Pokémon are hiding in the tall grass.[f000]븁\u0000\nAnd that you can battle\nor capture wild Pokémon there![f000]븁\u0000\nSo, you could say that I'm one\nof the people who saved Unova, too!"
    ParentActorMsg MSGFILE_SCRIPT, Route1_Text_TwoYearsAgoTrainer_2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01AF

L_019B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Two years ago, a Trainer from Nuvema\ndid some amazing things, including[f000]븀\u0000\nbattling Team Plasma and saving Unova![f000]븁\u0000\nI was the one who told that Trainer that\nwild Pokémon are hiding in the tall grass.[f000]븁\u0000\nAnd that you can battle\nor capture wild Pokémon there![f000]븁\u0000\nSo, you could say that I'm one\nof the people who saved Unova, too!"
    ParentActorMsg MSGFILE_SCRIPT, Route1_Text_TwoYearsAgoTrainer, 0, 0
    LastKeyWait
    ActorMsgClose

L_01AF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'd like to land all of the\nPokémon beyond here, too!"
    ParentActorMsg MSGFILE_SCRIPT, Route1_Text_IdLikeLandAll, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    ActorNew 788, 721, 1, 251, 249, 0
    TrainerCardGetSex 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0204
    VMJump L_020F

L_0204:
    // "Bianca: Heeey![f000]븁\u0000"
    InfoMsg Route1_Text_BiancaHeeey, 1
    VMJump L_022D

L_020F:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0222
    VMJump L_022D

L_0222:
    // "Bianca: Hi there![f000]븁\u0000"
    InfoMsg Route1_Text_BiancaHiThere, 1
    VMJump L_022D

L_022D:
    InfoMsgClose_0039
    BGMPlay SEQ_BGM_E_BERU
    ActorCmdExec 255, Movement_0618
    ActorCmdWait
    ActorGetGPos 255, 0x8021, 0x8022
    PlayerGetDir 0x8010
    ActorWalkRoute 251, 788, 730, 1, 8, 1
    WorkCmpConst 0x8021, 786
    VMJumpIf CMP_EQ, L_026A
    VMJump L_0299

L_026A:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_028B
    ActorCmdExec 255, Movement_0634
    VMJump L_0293

L_028B:
    ActorCmdExec 255, Movement_0648

L_0293:
    VMJump L_03E3

L_0299:
    WorkCmpConst 0x8021, 787
    VMJumpIf CMP_EQ, L_02AC
    VMJump L_02DB

L_02AC:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02CD
    ActorCmdExec 255, Movement_0658
    VMJump L_02D5

L_02CD:
    ActorCmdExec 255, Movement_066C

L_02D5:
    VMJump L_03E3

L_02DB:
    WorkCmpConst 0x8021, 788
    VMJumpIf CMP_EQ, L_02EE
    VMJump L_031D

L_02EE:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_030F
    ActorCmdExec 255, Movement_05D0
    VMJump L_0317

L_030F:
    ActorCmdExec 255, Movement_05B0

L_0317:
    VMJump L_03E3

L_031D:
    WorkCmpConst 0x8021, 789
    VMJumpIf CMP_EQ, L_0330
    VMJump L_035F

L_0330:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0351
    ActorCmdExec 255, Movement_067C
    VMJump L_0359

L_0351:
    ActorCmdExec 255, Movement_0690

L_0359:
    VMJump L_03E3

L_035F:
    WorkCmpConst 0x8021, 790
    VMJumpIf CMP_EQ, L_0372
    VMJump L_03A1

L_0372:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0393
    ActorCmdExec 255, Movement_06A0
    VMJump L_039B

L_0393:
    ActorCmdExec 255, Movement_06B4

L_039B:
    VMJump L_03E3

L_03A1:
    WorkCmpConst 0x8021, 791
    VMJumpIf CMP_EQ, L_03B4
    VMJump L_03E3

L_03B4:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_03D5
    ActorCmdExec 255, Movement_06C4
    VMJump L_03DD

L_03D5:
    ActorCmdExec 255, Movement_06D8

L_03DD:
    VMJump L_03E3

L_03E3:
    ActorCmdWait
    ActorCmdExec 251, Movement_0628
    ActorCmdWait
    // "I have fond memories of this place...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route1_Text_HaveFondMemoriesPlace, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_05D8
    ActorCmdWait
    Cmd_02B5 0, 1
    // "One day...[f000]븁\u0000\n[f000]Ā\u0001\u0001, Cheren, and I\nall gathered right here and took[f000]븀\u0000\nthe first step of our adventure.[f000]븁\u0000\nIt's a very special spot.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route1_Text_OneDayCherenAll, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 789, 731, 1, 8, 0
    VMSleep 15
    ActorCmdExec 255, Movement_05E8
    ActorCmdWait
    Cmd_02B5 0, 1
    ActorCmdExec 251, Movement_05E0
    ActorCmdWait
    // "Bianca: Hey, [f000]Ā\u0001\u0001![f000]븁\u0000\nLet's all take our first step\non Route 1 together![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route1_Text_BiancaHeyLetsAll, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_05D0
    ActorCmdExec 251, Movement_05D0
    ActorCmdWait
    // "One, two![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route1_Text_OneTwo, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0598
    ActorCmdExec 251, Movement_0598
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 251, Movement_05E0
    VMSleep 10
    ActorCmdExec 255, Movement_05E8
    ActorCmdWait
    Cmd_02B5 0, 1
    // "Ha ha! That's what I said.[f000]븁\u0000\nHey, while we're here,\nhave a Pokémon battle with me![f000]븁\u0000\nTalking about [f000]Ā\u0001\u0001 put me\nin the mood for a Pokémon battle![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route1_Text_HaHaThatsWhat, 251, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    Cmd_02B3 0, 0x8023
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_04DA
    VMJump L_04E6

L_04DA:
    WorkSetConst 0x8024, 706
    VMJump L_0524

L_04E6:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_04F9
    VMJump L_0505

L_04F9:
    WorkSetConst 0x8024, 707
    VMJump L_0524

L_0505:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_0518
    VMJump L_0524

L_0518:
    WorkSetConst 0x8024, 708
    VMJump L_0524

L_0524:
    CallTrainerBattle 0x8024, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_054B
    CallTrainerBattleEnd
    VMJump L_054D

L_054B:
    CallTrainerLose

L_054D:
    Cmd_02B5 0, 1
    // "Tee-hee! You're so tough!\nYou're just like [f000]Ā\u0001\u0001![f000]븁\u0000\nOK![f000]븁\u0000\nI just have to remember what\nI felt like back then and work hard, too![f000]븁\u0000\nThanks!\nSee you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route1_Text_TeeHeeYoureTough, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 789, 721, 4, 8, 1
    VMSleep 20
    ActorCmdExec 255, Movement_05D0
    ActorCmdWait
    ActorDelete 251
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x4136, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_0598:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_05B0:
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

Movement_05D0:
    Move 32, 1
    MoveEnd

Movement_05D8:
    Move 33, 1
    MoveEnd

Movement_05E0:
    Move 34, 1
    MoveEnd

Movement_05E8:
    Move 35, 1
    MoveEnd
    FinishAllEvents
    VMNop2
    PokePartyGetSpecies 0, 49
    VMNop2
    PokePartyGetSpecies 0, 50
    VMNop2
    PokePartyGetSpecies 0, 51
    VMNop2
    PokePartyGetSpecies 0, 49
    VMHalt
    .byte 0xfe
    .balign 4, 0

Movement_0618:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_0628:
    Move 35, 1
    Move 34, 1
    MoveEnd

Movement_0634:
    Move 32, 1
    Move 65, 1
    Move 15, 2
    Move 32, 1
    MoveEnd

Movement_0648:
    Move 65, 1
    Move 15, 2
    Move 32, 1
    MoveEnd

Movement_0658:
    Move 32, 1
    Move 65, 1
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_066C:
    Move 65, 1
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_067C:
    Move 32, 1
    Move 65, 1
    Move 14, 1
    Move 32, 1
    MoveEnd

Movement_0690:
    Move 65, 1
    Move 14, 1
    Move 32, 1
    MoveEnd

Movement_06A0:
    Move 32, 1
    Move 65, 1
    Move 14, 2
    Move 32, 1
    MoveEnd

Movement_06B4:
    Move 65, 1
    Move 14, 2
    Move 32, 1
    MoveEnd

Movement_06C4:
    Move 32, 1
    Move 65, 1
    Move 14, 3
    Move 32, 1
    MoveEnd

Movement_06D8:
    Move 65, 1
    Move 14, 3
    Move 32, 1
    MoveEnd
    Move 12, 10
    MoveEnd
