#include "asm/field_script.inc"
#include "text/script/ns_castle_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

Script_3:
    WorkSetConst 0x8026, 0
    RTCGetDate 0x8026, EVENT_WORK_0x400f
    VMStackPushFlag EVENT_FLAG_0x01e3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01e4
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4194
    VMStackPush 0x8026
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0081
    FlagReset EVENT_FLAG_0x0390

L_0081:
    WorkSetConst EVENT_WORK_0x400f, 0
    WorkSetConst 0x8026, 0
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B0
    WorkSetConst EVENT_WORK_0x4020, 365
    VMJump L_00B6

L_00B0:
    WorkSetConst EVENT_WORK_0x4020, 364

L_00B6:
    WorkSetConst 0x8027, 0
    TrainerCardGetSex 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DF
    WorkSetConst EVENT_WORK_0x4021, 231
    VMJump L_00E5

L_00DF:
    WorkSetConst EVENT_WORK_0x4021, 240

L_00E5:
    WorkSetConst 0x8027, 0
    VMHalt

Script_4:
    VMCall L_00F7
    VMHalt

Script_5:
    VMHalt

L_00F7:
    VMStackPushFlag EVENT_FLAG_0x0390
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011C
    ActorSetGPos 0, 16, 0, 36, 1
    VMJump L_011C

L_011C:
    VMReturn

Script_1:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x0390
    FlagReset EVENT_FLAG_0x0391
    FlagReset EVENT_FLAG_0x040b
    ActorAdd 0
    ActorSetGPos 0, 16, 0, 48, 0
    BGMPlay SEQ_BGM_E_N_SWAN
    // "[f000]븉\u0001\u0001That's the place![f000]븉\u0001\u0000[f000]븁\u0000"
    InfoMsg NsCastle5_Text_ThatsPlace, 2
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0A90
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 17
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0176
    WorkSub 0x8021, 1
    VMJump L_017C

L_0176:
    WorkAdd 0x8021, 1

L_017C:
    ActorWalkRoute 0, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 17
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_01BD
    ActorCmdExec 0, Movement_0AA0
    ActorCmdExec 255, Movement_0A98
    ActorCmdWait
    VMJump L_01CF

L_01BD:
    ActorCmdExec 0, Movement_0A98
    ActorCmdExec 255, Movement_0AA0
    ActorCmdWait

L_01CF:
    // "[f000]븉\u0001\u0001It was two years ago.[f000]븁\u0000\nFor the sake of Pokémon...[f000]븁\u0000\nFor my world of truth...[f000]븁\u0000\nI put my beliefs on the line\nand battled a certain Trainer![f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001It was two years ago.[f000]븁\u0000\nFor the sake of Pokémon...[f000]븁\u0000\nFor my ideal world...[f000]븁\u0000\nI put my beliefs on the line\nand battled a certain Trainer![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_TwoYearsAgoSake_2, NsCastle5_Text_TwoYearsAgoSake, 0, 0, 0
    // "[f000]븉\u0001\u0001And I lost...[f000]븁\u0000\nBut at the same time,\nI learned something important.[f000]븁\u0000\nTo make the world better,\nyou must accept different ideas![f000]븁\u0000\nI learned that this is the formula\nfor changing the world.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_LostButSameTime, 0, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x108000, 0, 0x243000, 56
    WorkCmpConst 0x8021, 14
    VMJumpIf CMP_EQ, L_021A
    VMJump L_0234

L_021A:
    ActorCmdExec 0, Movement_03D0
    VMSleep 8
    ActorCmdExec 255, Movement_041C
    VMJump L_02E8

L_0234:
    WorkCmpConst 0x8021, 15
    VMJumpIf CMP_EQ, L_0247
    VMJump L_0261

L_0247:
    ActorCmdExec 0, Movement_03E0
    VMSleep 8
    ActorCmdExec 255, Movement_042C
    VMJump L_02E8

L_0261:
    WorkCmpConst 0x8021, 16
    VMJumpIf CMP_EQ, L_0274
    VMJump L_028E

L_0274:
    ActorCmdExec 0, Movement_03EC
    VMSleep 16
    ActorCmdExec 255, Movement_043C
    VMJump L_02E8

L_028E:
    WorkCmpConst 0x8021, 17
    VMJumpIf CMP_EQ, L_02A1
    VMJump L_02BB

L_02A1:
    ActorCmdExec 0, Movement_0400
    VMSleep 8
    ActorCmdExec 255, Movement_0444
    VMJump L_02E8

L_02BB:
    WorkCmpConst 0x8021, 18
    VMJumpIf CMP_EQ, L_02CE
    VMJump L_02E8

L_02CE:
    ActorCmdExec 0, Movement_03EC
    VMSleep 8
    ActorCmdExec 255, Movement_0454
    VMJump L_02E8

L_02E8:
    ActorCmdWait
    EvCameraWait
    ActorAdd 2
    ActorCmdExec 255, Movement_0464
    ActorCmdWait
    ActorSetGPos 255, 16, 0, 36, 1
    // "[f000]븉\u0001\u0001Accepting different ideas...[f000]븁\u0000\nI want to see if you're a Trainer whose\nheart is strong enough to do that.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_AcceptingDifferentIdeasWant, 0, 3, 0
    // "[f000]븉\u0001\u0001Reshiram, come![f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001Zekrom, come![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_ReshiramCome, NsCastle5_Text_ZekromCome, 0, 3, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0AA0
    ActorCmdWait
    VMCall L_0474
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0360
    // "Shaaaaaak!"
    ScreamMsg NsCastle5_Text_Shaaaaaak, 1
    PVPlay 644, 0
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMJump L_0371

L_0360:
    // "Baaaaaaaahn!"
    ScreamMsg NsCastle5_Text_Baaaaaaaahn, 1
    PVPlay 643, 0
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039

L_0371:
    ActorCmdExec 0, Movement_0A90
    ActorCmdWait
    // "[f000]븉\u0001\u0001Reshiram also wants to know\nwhich truths you seek[f000]븀\u0000\nand how good a Trainer you are.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001Zekrom also wants to know\nwhat ideals you seek[f000]븀\u0000\nand how good a Trainer you are.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_ReshiramAlsoWantsKnow, NsCastle5_Text_ZekromAlsoWantsKnow, 0, 3, 0
    ActorSetGPos 255, 16, 0, 38, 0
    ActorCmdExec 255, Movement_046C
    ActorCmdWait
    ActorDelete 2
    WorkSetConst EVENT_WORK_0x4114, 2
    WorkSetConst EVENT_WORK_0x4112, 1
    FlagSet EVENT_FLAG_0x03ad
    WorkSetConst EVENT_WORK_0x411d, 2
    FlagSet EVENT_FLAG_0x040b
    WorkSetConst EVENT_WORK_0x400f, 99
    VMCall L_057D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03D0:
    Move 15, 1
    Move 12, 6
    Move 33, 1
    MoveEnd

Movement_03E0:
    Move 12, 6
    Move 33, 1
    MoveEnd

Movement_03EC:
    Move 12, 1
    Move 14, 1
    Move 12, 5
    Move 33, 1
    MoveEnd

Movement_0400:
    Move 12, 6
    Move 33, 1
    MoveEnd
    Move 14, 1
    Move 12, 6
    Move 33, 1
    MoveEnd

Movement_041C:
    Move 15, 2
    Move 12, 4
    Move 32, 1
    MoveEnd

Movement_042C:
    Move 15, 1
    Move 12, 4
    Move 32, 1
    MoveEnd

Movement_043C:
    Move 12, 4
    MoveEnd

Movement_0444:
    Move 14, 1
    Move 12, 4
    Move 32, 1
    MoveEnd

Movement_0454:
    Move 14, 2
    Move 12, 4
    Move 32, 1
    MoveEnd

Movement_0464:
    Move 69, 1
    MoveEnd

Movement_046C:
    Move 70, 1
    MoveEnd

L_0474:
    Cmd_020E 0, 18, 0, 36, 3, 8
    Cmd_020F 0, 18, 0, 36
    Cmd_0211 0
    FadeEx 12, 0, 16, 2
    FadeExWait
    ActorAdd 1
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 0
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4114
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01e0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04E9
    VMCall L_0833
    VMJump L_0530

L_04E9:
    VMStackPush EVENT_WORK_0x4114
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01e0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0526
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]븉\u0001\u0001Go to Dragonspiral Tower.[f000]븁\u0000\nI will...[f000]븁\u0000\nI'll search for that Trainer\nI battled two years ago.[f000]븁\u0000\nAnd...[f000]븁\u0000\nI plan to say thank you.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NsCastle5_Text_GoDragonspiralTowerWill, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0530

L_0526:
    BGMPlay SEQ_BGM_E_N_SWAN
    VMCall L_057D

L_0530:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0566
    PVPlay 644, 0
    // "Shaaaaaak!"
    ScreamMsg NsCastle5_Text_Shaaaaaak, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMJump L_0577

L_0566:
    PVPlay 643, 0
    // "Baaaaaaaahn!"
    ScreamMsg NsCastle5_Text_Baaaaaaaahn, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039

L_0577:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_057D:
    // "[f000]븉\u0001\u0001Battle with me.\nAre you prepared?[f000]븉\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_BattlePrepared, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_060F
    // "[f000]븉\u0001\u0001Show me the depth\nof your determination![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_ShowDepthDetermination, 0, 0, 0
    MsgWinCloseAll
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05E4
    PVPlay 644, 0
    // "Bazzazzazzash!"
    ScreamMsg NsCastle5_Text_Bazzazzazzash, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallTrainerBattle TRAINER_N, 0, 0
    VMJump L_05FD

L_05E4:
    PVPlay 643, 0
    // "Preeeeaah!"
    ScreamMsg NsCastle5_Text_Preeeeaah, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallTrainerBattle TRAINER_N_2, 0, 0

L_05FD:
    VMCall L_09C0
    VMCall L_0646
    VMJump L_0644

L_060F:
    // "[f000]븉\u0001\u0001I'm ready whenever you are!\nI'll wait as long as it takes![f000]븉\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_ImReadyWheneverIll, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 99
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0642
    EvCameraMoveToDefault 24
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x400f, 0

L_0642:
    BGMChangeMap

L_0644:
    VMReturn

L_0646:
    WordSetPlayerName 0
    // "[f000]븉\u0001\u0001Reshiram and I were defeated.[f000]븁\u0000\nYour feelings,\nyour desire to pursue ideals--[f000]븀\u0000\nthat's what surpassed us.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001Zekrom and I were defeated.[f000]븁\u0000\nYour feelings,\nyour desire to know the truth--[f000]븀\u0000\nthat's what surpassed us.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_ReshiramWereDefeatedFeelings, NsCastle5_Text_ZekromWereDefeatedFeelings, 0, 3, 0
    TrainerCardGetSex 0x8025
    // "[f000]븉\u0001\u0001Battling with you reminded\nme of two years ago...[f000]븁\u0000\nIt may just be a little,\nbut I know you better...[f000]븀\u0000\nThat's how I feel.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_BattlingRemindedTwoYears, 0, 3, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0AA0
    ActorCmdWait
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_069A
    // "[f000]븉\u0001\u0001And Reshiram...\nThank you for everything.[f000]븁\u0000\nMy journey with you\nhas been truly wonderful![f000]븁\u0000\nFrom now on, I want you to use\nyour power to help this Trainer[f000]븀\u0000\nrealize his dreams.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001And Zekrom...\nThank you for everything.[f000]븁\u0000\nMy journey with you\nhas been truly wonderful![f000]븁\u0000\nFrom now on, I want you to use\nyour power to help this Trainer[f000]븀\u0000\nrealize his dreams.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_ReshiramThankEverythingJourney, NsCastle5_Text_ZekromThankEverythingJourney, 0, 3, 0
    VMJump L_06A8

L_069A:
    // "[f000]븉\u0001\u0001And Reshiram...\nThank you for everything you've done.[f000]븁\u0000\nMy journey with you\nhas been truly wonderful![f000]븁\u0000\nFrom now on, I want you to use\nyour power to help this Trainer[f000]븀\u0000\nrealize her dreams.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001And Zekrom...\nThank you for everything.[f000]븁\u0000\nMy journey with you\nhas been truly wonderful![f000]븁\u0000\nFrom now on, I want you to use\nyour power to help this Trainer[f000]븀\u0000\nrealize her dreams.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_ReshiramThankEverythingYouve, NsCastle5_Text_ZekromThankEverythingJourney_2, 0, 3, 0

L_06A8:
    MsgWinCloseAll
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06DF
    PVPlay 644, 0
    // "Bazz..."
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_Bazz, 1, 5, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_06F7

L_06DF:
    PVPlay 643, 0
    // "Pree..."
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_Pree, 1, 5, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_06F7:
    ActorCmdExec 0, Movement_0A2C
    ActorCmdWait
    // "[f000]븉\u0001\u0001I know. I'll miss you, too...[f000]븁\u0000\nBut your task is to help\nhumans who seek the truth.[f000]븁\u0000\nI've learned so much from you.[f000]븁\u0000\nI'll do my best to tell everyone\nelse what I learned on my own.[f000]븁\u0000\nI'll be OK!\nI can talk to Pokémon![f000]븁\u0000\nI'll become the bridge\nbetween Pokémon and humans![f000]븀\u0000\nThat's my truth![f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001I know. I'll miss you, too...[f000]븁\u0000\nBut your task is to help\nhumans who seek ideals.[f000]븁\u0000\nI've learned so much from you.[f000]븁\u0000\nI'll do my best to tell everyone\nelse what I learned on my own.[f000]븁\u0000\nI'll be OK!\nI can talk to Pokémon![f000]븁\u0000\nI'll become the bridge\nbetween Pokémon and humans![f000]븀\u0000\nThat's my ideal![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_KnowIllMissToo_2, NsCastle5_Text_KnowIllMissToo, 0, 3, 0
    // "[f000]븉\u0001\u0001So...[f000]븁\u0000\nRest well...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_RestWell, 0, 3, 0
    MsgWinCloseAll
    FadeEx 12, 0, 16, 4
    FadeExWait
    ActorDelete 1
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0758
    ActorNew 18, 36, 1, 251, 143, 0
    VMJump L_0766

L_0758:
    ActorNew 18, 36, 1, 251, 144, 0

L_0766:
    FadeEx 12, 16, 0, 4
    FadeExWait
    VMSleep 8
    ActorCmdExec 0, Movement_0A58
    ActorCmdWait
    VMSleep 16
    ActorDelete 251
    VMSleep 8
    ActorCmdExec 0, Movement_0A3C
    ActorCmdWait
    // "[f000]븉\u0001\u0001[f000]Ā\u0001\u0000![f000]븁\u0000\nI'll entrust you\nwith this Light Stone![f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001[f000]Ā\u0001\u0000![f000]븁\u0000\nI'll entrust you\nwith this Dark Stone![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_IllEntrustLightStone, NsCastle5_Text_IllEntrustDarkStone, 0, 3, 0
    MsgWinCloseAll
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07DF
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 617
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_07FF

L_07DF:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 616
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_07FF:
    // "[f000]븉\u0001\u0001Take that Light Stone\nto Dragonspiral Tower![f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001Take that Dark Stone\nto Dragonspiral Tower![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_TakeLightStoneDragonspiral, NsCastle5_Text_TakeDarkStoneDragonspiral, 0, 3, 0
    MsgWinCloseAll
    BGMChangeMap
    EvCameraMoveToDefault 24
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x4114, 3
    FlagSet EVENT_FLAG_0x0391
    WorkSetConst EVENT_WORK_0x411b, 1
    WorkSetConst EVENT_WORK_0x4148, 1
    VMReturn

L_0833:
    VMStackPushFlag EVENT_FLAG_0x01e6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0858
    // "N: [f000]븉\u0001\u0001How surprising...\nI didn't expect you'd come here.[f000]븁\u0000\nWell, that is the formula for\nunderstanding other Trainers,[f000]븀\u0000\nafter all...[f000]븀\u0000\nYou're OK with a Pokémon battle, right?[f000]븉\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_NHowSurprisingDidnt, 0, 0, 0
    VMJump L_0864

L_0858:
    // "[f000]븉\u0001\u0001Your Pokémon are saying they\nwant to battle with my friend...[f000]븁\u0000\nWhat would you like to do?\nWill you have a Pokémon battle with me?[f000]븉\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_PokemonSayingTheyWant, 0, 0, 0

L_0864:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09AA
    // "[f000]븉\u0001\u0001Good...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_Good, 0, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8028, 0
    RTCGetSeason 0x8028
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08B4
    CallTrainerBattle TRAINER_N_3, 0, 0
    VMJump L_0911

L_08B4:
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08D5
    CallTrainerBattle TRAINER_N_4, 0, 0
    VMJump L_0911

L_08D5:
    VMStackPush 0x8028
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08F6
    CallTrainerBattle TRAINER_N_5, 0, 0
    VMJump L_0911

L_08F6:
    VMStackPush 0x8028
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0911
    CallTrainerBattle TRAINER_N_6, 0, 0

L_0911:
    VMCall L_09C0
    // "[f000]븉\u0001\u0001I remember something Reshiram\ntold me once...[f000]븁\u0000\nReshiram and Zekrom\nare searching for new possibilities[f000]븀\u0000\nby walking alongside humans...[f000]븁\u0000\nMeanwhile, those that live in the wild\ntry to better themselves[f000]븀\u0000\nwithout relying on anyone else.[f000]븁\u0000\nThere are many different Pokémon...[f000]븁\u0000\nAnd their different ways of living...\nThat is the true freedom of Pokémon.[f000]븀\u0000\nThat is what connects Pokémon to us.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001I remember something Zekrom\ntold me once...[f000]븁\u0000\nZekrom and Reshiram\nare searching for new possibilities[f000]븀\u0000\nby walking alongside humans...[f000]븁\u0000\nMeanwhile, those that live in the wild\ntry to better themselves[f000]븀\u0000\nwithout relying on anyone else.[f000]븁\u0000\nThere are many different Pokémon...[f000]븁\u0000\nAnd their different ways of living...\nThat is the true freedom of Pokémon.[f000]븀\u0000\nThat is what connects Pokémon to us.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, NsCastle5_Text_RememberSomethingReshiramTold, NsCastle5_Text_RememberSomethingZekromTold, 0, 0, 0
    MsgWinCloseAll
    // "[f000]븉\u0001\u0001I will set off on another journey.[f000]븁\u0000\nThere are still many Pokémon\nin the world I should talk to.[f000]븁\u0000\nAnd there is also a Trainer\nI want to tell how I feel...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_WillSetOffAnother, 0, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0964
    ActorWalkRoute 0, 17, 44, 1, 8, 0
    VMSleep 24
    VMJump L_0976

L_0964:
    ActorWalkRoute 0, 16, 44, 1, 8, 1
    VMSleep 16

L_0976:
    ActorCmdExec 255, Movement_0A90
    ActorCmdWait
    ActorDelete 0
    RTCGetDate 0x8028, EVENT_WORK_0x400f
    WorkGet EVENT_WORK_0x4194, 0x8028
    FlagSet EVENT_FLAG_0x0390
    FlagReset EVENT_FLAG_0x01e6
    WorkSetConst EVENT_WORK_0x400f, 0
    WorkSetConst 0x8028, 0
    VMJump L_09BE

L_09AA:
    // "[f000]븉\u0001\u0001Very well...\nYou're free to choose that, too.[f000]븉\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle5_Text_VeryWellYoureFree, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01e6

L_09BE:
    VMReturn

L_09C0:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A28
    VMStackPush EVENT_WORK_0x4114
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A20
    ActorSetGPos 255, 16, 0, 38, 0
    ActorSetGPos 0, 16, 0, 36, 1
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x108000, 0, 0x243000, 1
    EvCameraWait

L_0A20:
    CallTrainerBattleEnd
    VMJump L_0A2A

L_0A28:
    CallTrainerLose

L_0A2A:
    VMReturn

Movement_0A2C:
    Move 33, 1
    Move 182, 1
    Move 3, 1
    MoveEnd

Movement_0A3C:
    Move 14, 1
    Move 13, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0A58:
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
    Move 32, 1
    MoveEnd

Movement_0A90:
    Move 33, 1
    MoveEnd

Movement_0A98:
    Move 34, 1
    MoveEnd

Movement_0AA0:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
