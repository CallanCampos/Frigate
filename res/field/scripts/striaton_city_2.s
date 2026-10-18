#include "asm/field_script.inc"
#include "text/script/striaton_city_2.h"

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
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_14:
    Random EVENT_WORK_0x418e, 3
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_0075
    VMJump L_0093

L_0075:
    WorkSetConst EVENT_WORK_0x4020, 309
    WorkSetConst EVENT_WORK_0x4021, 309
    WorkSetConst EVENT_WORK_0x4022, 309
    WorkSetConst EVENT_WORK_0x4023, 310
    VMJump L_00F5

L_0093:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_00A6
    VMJump L_00C4

L_00A6:
    WorkSetConst EVENT_WORK_0x4020, 310
    WorkSetConst EVENT_WORK_0x4021, 310
    WorkSetConst EVENT_WORK_0x4022, 310
    WorkSetConst EVENT_WORK_0x4023, 311
    VMJump L_00F5

L_00C4:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_00D7
    VMJump L_00F5

L_00D7:
    WorkSetConst EVENT_WORK_0x4020, 311
    WorkSetConst EVENT_WORK_0x4021, 311
    WorkSetConst EVENT_WORK_0x4022, 311
    WorkSetConst EVENT_WORK_0x4023, 309
    VMJump L_00F5

L_00F5:
    DebugPrint EVENT_WORK_0x418e
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0112
    WorkSetConst EVENT_WORK_0x418d, 0

L_0112:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad9
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012B
    WorkSetConst EVENT_WORK_0x400c, 2

L_012B:
    VMHalt

Script_16:
    VMStackPush EVENT_WORK_0x4196
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015E
    ActorSetGPos 255, 6, 0, 8, 2
    ActorSetGPos 10, 5, 0, 8, 3
    VMJump L_01BA

L_015E:
    VMStackPush EVENT_WORK_0x4196
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018F
    ActorSetGPos 255, 15, 0, 14, 3
    ActorSetGPos 9, 16, 0, 14, 2
    VMJump L_01BA

L_018F:
    VMStackPush EVENT_WORK_0x4196
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BA
    ActorSetGPos 255, 12, 0, 10, 3
    ActorSetGPos 11, 13, 0, 10, 2

L_01BA:
    VMHalt

Script_1:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x400a, 1
    Cmd_02B2 8, EVENT_WORK_0x400f
    DebugPrint EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4045
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01FD
    VMCall L_0BA4
    VMJump L_0203

L_01FD:
    VMCall L_029B

L_0203:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x400a, 2
    Cmd_02B2 8, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4045
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0246
    VMCall L_0BA4
    VMJump L_024C

L_0246:
    VMCall L_029B

L_024C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x400a, 3
    Cmd_02B2 8, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4045
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_028F
    VMCall L_0BA4
    VMJump L_0295

L_028F:
    VMCall L_029B

L_0295:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_029B:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C2
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_02C1
    VMJump L_0308

L_02C1:
    VMStackPush EVENT_WORK_0x418d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EE
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Eeeooow! You're intense![f000]븁\u0000\nBattling together with you\ngot me all fired up, man![f000]븀\u0000\nCome battle again sometime!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_EeeooowYoureIntenseBattling, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0302

L_02EE:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You look like you'd be fun to\nbattle together with![f000]븀\u0000\nAll right! Team up with me tomorrow!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_LookLikeYoudFun, 0, 0
    LastKeyWait
    ActorMsgClose

L_0302:
    VMJump L_03BC

L_0308:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_031B
    VMJump L_0362

L_031B:
    VMStackPush EVENT_WORK_0x418d
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0348
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Battling together with you helped\nme find new potential in myself.[f000]븀\u0000\nThat's what I think.[f000]븁\u0000\nI'd like it if you were to team up\nwith me tomorrow as well."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_BattlingTogetherHelpedFind, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_035C

L_0348:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Losing made me realize something.[f000]븁\u0000\nIf I were to team up with you,\nour onslaught would be like a torrent!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_LosingMadeRealizeSomething, 0, 0
    LastKeyWait
    ActorMsgClose

L_035C:
    VMJump L_03BC

L_0362:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_0375
    VMJump L_03BC

L_0375:
    VMStackPush EVENT_WORK_0x418d
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03A2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "...What a surprise.\nYou...are very strong.[f000]븁\u0000\nWould you team up with me again sometime?\nThere's still much I want to learn."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WhatSurpriseVeryStrong, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_03B6

L_03A2:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "During the battle I was so\ntaken with your fighting style,[f000]븁\u0000\nI almost lost the timing for\ngiving my Pokémon directions![f000]븁\u0000\nNext time, I would like to\nteam up with you."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_DuringBattleTakenFighting, 0, 0
    LastKeyWait
    ActorMsgClose

L_03B6:
    VMJump L_03BC

L_03BC:
    VMJump L_0B3F

L_03C2:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_03DB
    VMJump L_0412

L_03DB:
    VMStackPushFlag EVENT_FLAG_0x0199
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0402
    // "I'm Chili! My Fire-type Pokémon\nand I are too hot to handle![f000]븁\u0000\nWhat it boils down to is\nI want you to be my partner!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_ImChiliFireType, 0, 0
    FlagSet EVENT_FLAG_0x0199
    VMJump L_040C

L_0402:
    // "C'mon! What it boils down to is\nI want you to be my partner!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_CmonWhatBoilsDown, 0, 0

L_040C:
    VMJump L_04A6

L_0412:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0425
    VMJump L_045C

L_0425:
    VMStackPushFlag EVENT_FLAG_0x019a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044C
    // "I'm a Water-type specialist,\nand my name is Cress.[f000]븀\u0000\nPleased to make your acquaintance.[f000]븁\u0000\nYou there. Would you be so kind\nas to be my partner in a battle[f000]븀\u0000\nwith my siblings?"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_ImWaterTypeSpecialist, 0, 0
    FlagSet EVENT_FLAG_0x019a
    VMJump L_0456

L_044C:
    // "What do you think?\nWill you partner up with me[f000]븀\u0000\nand compete against my siblings?"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WhatThinkWillPartner, 0, 0

L_0456:
    VMJump L_04A6

L_045C:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_046F
    VMJump L_04A6

L_046F:
    VMStackPushFlag EVENT_FLAG_0x019b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0496
    // "Um. My name is Cilan.\nI like Grass-type Pokémon.[f000]븁\u0000\nWe were Gym Leaders, but\nin order to improve ourselves further,[f000]븀\u0000\nwe are working at this Restaurant[f000]븀\u0000\nand spending every day with Pokémon.[f000]븁\u0000\nHow about it? Would you team up with me\nand take part in a Double Battle?"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_UmNameCilanLike, 0, 0
    FlagSet EVENT_FLAG_0x019b
    VMJump L_04A0

L_0496:
    // "How about it? Would you team up with me\nand take part in a Double Battle?"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HowAboutWouldTeam, 0, 0

L_04A0:
    VMJump L_04A6

L_04A6:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AD2
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_04D0
    VMJump L_04E0

L_04D0:
    // "Yeeeeooow![f000]븁\u0000\nI'm the strongest of us brothers,\nand I'll make sure you win![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_YeeeeooowImStrongestUs, 0, 0
    VMJump L_0526

L_04E0:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_04F3
    VMJump L_0503

L_04F3:
    // "That's wonderful. With me, Cress,\nas your partner, victory is assured.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_ThatsWonderfulCressPartner, 0, 0
    VMJump L_0526

L_0503:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_0516
    VMJump L_0526

L_0516:
    // "OK. If you're fine with me,\nI will put everything I have into[f000]븀\u0000\nbeing your partner in battle![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OkIfYoureFine, 0, 0
    VMJump L_0526

L_0526:
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xb8000, 0x1000f, 0x28000, 1
    EvCameraWait
    FlagReset EVENT_FLAG_0x03ae
    ActorAdd 1
    ActorAdd 2
    ActorAdd 3
    ActorSetGPos 255, 9, 1, 1, 3
    ActorSetGPos 0, 9, 0, 5, 0
    ActorSetGPos 1, 10, 0, 5, 0
    ActorSetGPos 2, 11, 0, 5, 0
    ActorSetGPos 3, 12, 0, 5, 0
    ActorSetGPos 7, 13, 0, 5, 0
    ActorSetGPos 8, 8, 0, 5, 0
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_05C9
    VMJump L_05F3

L_05C9:
    ActorSetGPos 10, 9, 1, 2, 3
    ActorSetGPos 9, 13, 1, 1, 2
    ActorSetGPos 11, 13, 1, 2, 2
    VMJump L_066D

L_05F3:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0606
    VMJump L_0630

L_0606:
    ActorSetGPos 9, 9, 1, 2, 3
    ActorSetGPos 10, 13, 1, 1, 2
    ActorSetGPos 11, 13, 1, 2, 2
    VMJump L_066D

L_0630:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_0643
    VMJump L_066D

L_0643:
    ActorSetGPos 11, 9, 1, 2, 3
    ActorSetGPos 9, 13, 1, 1, 2
    ActorSetGPos 10, 13, 1, 2, 2
    VMJump L_066D

L_066D:
    VMSleep 30
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0690
    VMJump L_06BA

L_0690:
    // "That's that, then!\nI'm going to show you what[f000]븀\u0000\nme and my blazing Fire types can do![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_ThatsThenImGoing, 0, 0
    MsgWinCloseAll
    ActorCmdExec 10, Movement_1D18
    ActorCmdExec 9, Movement_1D20
    ActorCmdExec 11, Movement_1D20
    VMJump L_0734

L_06BA:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_06CD
    VMJump L_06F7

L_06CD:
    // "That is correct![f000]븁\u0000\nIt shall be I and my esteemed Water\ntypes that you must face in battle![f000]븁\u0000\nLet us begin, then![f000]븁\u0000\nEn garde, user of Grass-type Pokémon--\nCilan--and of Fire-type Pokémon--Chili![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_CorrectShallEsteemedWater, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_1D18
    ActorCmdExec 10, Movement_1D20
    ActorCmdExec 11, Movement_1D20
    VMJump L_0734

L_06F7:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_070A
    VMJump L_0734

L_070A:
    // "OK... So, um,\nI'm Cilan, I like Grass-type Pokémon,[f000]븀\u0000\nand this is my partner here![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OkUmImCilan, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_1D18
    ActorCmdExec 10, Movement_1D20
    ActorCmdExec 9, Movement_1D20
    VMJump L_0734

L_0734:
    ActorCmdExec 255, Movement_1D18
    ActorCmdWait
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0751
    VMJump L_0761

L_0751:
    CallTrainerMultiBattle 494, 496, 495, 0
    VMJump L_07A7

L_0761:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0774
    VMJump L_0784

L_0774:
    CallTrainerMultiBattle 495, 496, 494, 0
    VMJump L_07A7

L_0784:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_0797
    VMJump L_07A7

L_0797:
    CallTrainerMultiBattle 496, 494, 495, 0
    VMJump L_07A7

L_07A7:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07E1
    PokePartyGetCount 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D9
    PokePartyRecoverAll

L_07D9:
    CallTrainerBattleEnd
    VMJump L_07E7

L_07E1:
    FlagSet EVENT_FLAG_0x03ae
    CallTrainerLose

L_07E7:
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_07FA
    VMJump L_0810

L_07FA:
    ActorCmdExec 9, Movement_1DA0
    ActorCmdExec 11, Movement_1DA0
    VMJump L_0862

L_0810:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0823
    VMJump L_0839

L_0823:
    ActorCmdExec 10, Movement_1DA0
    ActorCmdExec 11, Movement_1DA0
    VMJump L_0862

L_0839:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_084C
    VMJump L_0862

L_084C:
    ActorCmdExec 10, Movement_1DA0
    ActorCmdExec 9, Movement_1DA0
    VMJump L_0862

L_0862:
    ActorCmdWait
    VMSleep 20
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_087B
    VMJump L_0895

L_087B:
    ActorCmdExec 10, Movement_0B48
    ActorCmdWait
    // "I was good, too,\nbut you aren't half bad![f000]븁\u0000\nHow about it,\nCilan and Cress?[f000]븁\u0000\nOut of the three of us,\nI'm the strongest Trainer, right?!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_GoodTooButArent, 0, 0
    VMJump L_08EF

L_0895:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_08A8
    VMJump L_08C2

L_08A8:
    ActorCmdExec 9, Movement_0B48
    ActorCmdWait
    // "My attacks flow like water.[f000]븁\u0000\nYou were blessed with an\namazing partner."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_AttacksFlowLikeWater, 0, 0
    VMJump L_08EF

L_08C2:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_08D5
    VMJump L_08EF

L_08D5:
    ActorCmdExec 11, Movement_0B78
    ActorCmdWait
    // "OK... So, um,\nI'm Cilan, I like Grass-type Pokémon,[f000]븀\u0000\nand this was my partner here!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OkUmImCilan_2, 0, 0
    VMJump L_08EF

L_08EF:
    MsgWaitAdvance
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0912
    VMJump L_0924

L_0912:
    ActorSetGPos 255, 5, 0, 9, 0
    VMJump L_096E

L_0924:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0937
    VMJump L_0949

L_0937:
    ActorSetGPos 255, 15, 0, 14, 3
    VMJump L_096E

L_0949:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_095C
    VMJump L_096E

L_095C:
    ActorSetGPos 255, 13, 0, 11, 0
    VMJump L_096E

L_096E:
    ActorSetGPos 0, 9, 1, 2, 1
    ActorSetGPos 1, 11, 1, 1, 1
    ActorSetGPos 2, 11, 1, 1, 1
    ActorSetGPos 3, 11, 1, 1, 1
    ActorSetGPos 7, 13, 0, 5, 0
    ActorSetGPos 8, 4, 0, 14, 3
    ActorSetGPos 10, 5, 0, 8, 1
    ActorSetGPos 9, 16, 0, 14, 2
    ActorSetGPos 11, 13, 0, 10, 1
    FlagSet EVENT_FLAG_0x03ae
    ActorDelete 1
    ActorDelete 2
    ActorDelete 3
    VMSleep 30
    FadeEx 3, 16, 0, 2
    FadeExWait
    EvCameraRebind
    EvCameraEnd
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0A11
    VMJump L_0A21

L_0A11:
    // "Eeeooow! You're intense![f000]븁\u0000\nBattling together with you\ngot me all fired up, man![f000]븀\u0000\nCome battle again sometime!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_EeeooowYoureIntenseBattling, 0, 0
    VMJump L_0A67

L_0A21:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0A34
    VMJump L_0A44

L_0A34:
    // "Battling together with you helped\nme find new potential in myself.[f000]븀\u0000\nThat's what I think.[f000]븁\u0000\nI'd like it if you were to team up\nwith me tomorrow as well."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_BattlingTogetherHelpedFind, 0, 0
    VMJump L_0A67

L_0A44:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_0A57
    VMJump L_0A67

L_0A57:
    // "...What a surprise.\nYou...are very strong.[f000]븁\u0000\nWould you team up with me again sometime?\nThere's still much I want to learn."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WhatSurpriseVeryStrong, 0, 0
    VMJump L_0A67

L_0A67:
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ad8
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0A82
    VMJump L_0A8E

L_0A82:
    WorkSetConst EVENT_WORK_0x418d, 1
    VMJump L_0ACC

L_0A8E:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0AA1
    VMJump L_0AAD

L_0AA1:
    WorkSetConst EVENT_WORK_0x418d, 2
    VMJump L_0ACC

L_0AAD:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_0AC0
    VMJump L_0ACC

L_0AC0:
    WorkSetConst EVENT_WORK_0x418d, 3
    VMJump L_0ACC

L_0ACC:
    VMJump L_0B3F

L_0AD2:
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0AE5
    VMJump L_0AF5

L_0AE5:
    // "Aww, man!\nI was all fired up, too!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_AwwManAllFired, 0, 0
    VMJump L_0B3B

L_0AF5:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0B08
    VMJump L_0B18

L_0B08:
    // "Oh, what's this?\nI'm amazed you turned me, Cress, down."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OhWhatsImAmazed, 0, 0
    VMJump L_0B3B

L_0B18:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_0B2B
    VMJump L_0B3B

L_0B2B:
    // "...Ah, hmm. I must've timed...\nmy invitation poorly...[f000]븁\u0000\nTrue enough, I did want to...\nto have a Pokémon battle with you."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_AhHmmMustveTimed, 0, 0
    VMJump L_0B3B

L_0B3B:
    LastKeyWait
    MsgWinCloseAll

L_0B3F:
    WorkSetConst EVENT_WORK_0x400a, 0
    VMReturn
    .balign 4, 0

Movement_0B48:
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    MoveEnd

Movement_0B78:
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 100, 1
    MoveEnd

L_0BA4:
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C6A
    VMStackPushFlag EVENT_FLAG_0x01cd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ce
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01cf
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0C0C
    WorkSetConst EVENT_WORK_0x4196, 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Let us tell you why the Trio Badge\nno longer exists![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_LetUsTellWhy, 0, 0
    VMCall L_0DF8
    VMJump L_0C64

L_0C0C:
    VMStackPushFlag EVENT_FLAG_0x01cd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C3D
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey! You've got the...\nOh yeah...[f000]븀\u0000\nThere's no Trio Badge now!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HeyYouveGotOh, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01cd
    VMJump L_0C64

L_0C3D:
    VMStackPushFlag EVENT_FLAG_0x01cd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C64
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey! You've got the...\nOh yeah...[f000]븀\u0000\nThere's no Trio Badge now!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HeyYouveGotOh, 0, 0
    LastKeyWait
    ActorMsgClose

L_0C64:
    VMJump L_0DF0

L_0C6A:
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D30
    VMStackPushFlag EVENT_FLAG_0x01cd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ce
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01cf
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0CD2
    WorkSetConst EVENT_WORK_0x4196, 2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A day that's important to me, Cress...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_DayThatsImportantCress, 0, 0
    VMCall L_0DF8
    VMJump L_0D2A

L_0CD2:
    VMStackPushFlag EVENT_FLAG_0x01ce
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D03
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hope today is a special day for you."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HopeTodaySpecialDay, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01ce
    VMJump L_0D2A

L_0D03:
    VMStackPushFlag EVENT_FLAG_0x01ce
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D2A
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hope today is a special day for you."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HopeTodaySpecialDay, 0, 0
    LastKeyWait
    ActorMsgClose

L_0D2A:
    VMJump L_0DF0

L_0D30:
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DF0
    VMStackPushFlag EVENT_FLAG_0x01cd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ce
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01cf
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0D98
    WorkSetConst EVENT_WORK_0x4196, 3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Well, why don't I tell you\nwhat happened...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WellWhyDontTell, 0, 0
    VMCall L_0DF8
    VMJump L_0DF0

L_0D98:
    VMStackPushFlag EVENT_FLAG_0x01cf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DC9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Striaton Restaurant![f000]븁\u0000\nThis place used to be a\nPokémon Gym, but a lot happened..."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WelcomeStriatonRestaurantPlace, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01cf
    VMJump L_0DF0

L_0DC9:
    VMStackPushFlag EVENT_FLAG_0x01cf
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DF0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Striaton Restaurant![f000]븁\u0000\nThis place used to be a\nPokémon Gym, but a lot happened..."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WelcomeStriatonRestaurantPlace, 0, 0
    LastKeyWait
    ActorMsgClose

L_0DF0:
    WorkSetConst EVENT_WORK_0x400a, 0
    VMReturn

L_0DF8:
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_STRIATON_CITY_10, 7, 0, 3, 1
    VMReturn

Script_15:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    VMStackPush EVENT_WORK_0x4196
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E3B
    // "The unobtainable Trio Badge...[f000]븁\u0000\nNo! One day we will make the\nTrio Badge shine again!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_UnobtainableTrioBadgeNo, 10, 0, 0
    VMJump L_0E7F

L_0E3B:
    VMStackPush EVENT_WORK_0x4196
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E60
    // "Part of my past I want to forget...\nNo, I mustn't forget it."
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_PartPastWantForget, 9, 0, 0
    VMJump L_0E7F

L_0E60:
    VMStackPush EVENT_WORK_0x4196
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E7F
    // "Because of that day, we can\ntruly aim for the top!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_BecauseDayWeCan, 11, 0, 0

L_0E7F:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4045, 1
    WorkSetConst EVENT_WORK_0x4196, 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello! I'm giving out water!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HelloImGivingOut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome![f000]븁\u0000\nThis is a lively restaurant where\nyou can enjoy a show!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WelcomeLivelyRestaurantWhere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_0EEA
    VMJump L_0EF5

L_0EEA:
    WordSetPokeSpecies 1, 511
    VMJump L_0F31

L_0EF5:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_0F08
    VMJump L_0F13

L_0F08:
    WordSetPokeSpecies 1, 513
    VMJump L_0F31

L_0F13:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_0F26
    VMJump L_0F31

L_0F26:
    WordSetPokeSpecies 1, 515
    VMJump L_0F31

L_0F31:
    DebugPrint EVENT_WORK_0x418e
    ActorCmdExec 0, Movement_21F4
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 0, 8, 0
    VMSleep 16
    ActorCmdExec 255, Movement_1D48
    ActorCmdWait
    // "Huh? Aren't you going to guess which\n[f000]ā\u0001\u0001 has the Big Mushroom?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HuhArentGoingGuess, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_102E
    // "I see... Well, come talk to me\nif you change your mind![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_SeeWellComeTalk, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 9, 2, 0, 8, 0
    ActorCmdExec 1, Movement_1D08
    ActorCmdExec 2, Movement_1D08
    ActorCmdExec 3, Movement_1D08
    ActorCmdWait
    ActorSetGPos 1, 10, 1, 1, 1
    ActorSetGPos 2, 11, 1, 1, 1
    ActorSetGPos 3, 12, 1, 1, 1
    ActorCmdExec 1, Movement_1E8C
    ActorCmdExec 3, Movement_1EA4
    VMSleep 16
    ActorCmdExec 2, Movement_1E98
    ActorCmdWait
    ActorDelete 1
    ActorDelete 2
    ActorDelete 3
    ActorCmdExec 0, Movement_1D50
    ActorCmdWait
    FlagSet EVENT_FLAG_0x03ae
    WorkSetConst EVENT_WORK_0x400c, 0
    WorkSetConst EVENT_WORK_0x400b, 0
    VMJump L_1060

L_102E:
    // "OK! Find the [f000]ā\u0001\u0001 that\nI asked you to follow!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OkFindAskedFollow, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 9, 2, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 0, Movement_1D50
    ActorCmdExec 255, Movement_1D10
    ActorCmdWait

L_1060:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_107B
    VMJump L_1086

L_107B:
    WordSetPokeSpecies 1, 511
    VMJump L_10C2

L_1086:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_1099
    VMJump L_10A4

L_1099:
    WordSetPokeSpecies 1, 513
    VMJump L_10C2

L_10A4:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_10B7
    VMJump L_10C2

L_10B7:
    WordSetPokeSpecies 1, 515
    VMJump L_10C2

L_10C2:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad9
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10EF
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You want to see the [f000]ā\u0001\u0001 again?[f000]븁\u0000\nThere are a lot of preparations and\nsuch to make, so come back tomorrow!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WantSeeAgainThere, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_11BF

L_10EF:
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11AB
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_111B
    VMJump L_1126

L_111B:
    WordSetPokeSpecies 1, 511
    VMJump L_1162

L_1126:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_1139
    VMJump L_1144

L_1139:
    WordSetPokeSpecies 1, 513
    VMJump L_1162

L_1144:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_1157
    VMJump L_1162

L_1157:
    WordSetPokeSpecies 1, 515
    VMJump L_1162

L_1162:
    DebugPrint EVENT_WORK_0x418e
    // "Will you come see Striaton Restaurant's\nfamous [f000]ā\u0001\u0001 show?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WillComeSeeStriaton, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1195
    VMCall L_11C5
    VMJump L_11A5

L_1195:
    // "That's too bad...\nI guarantee it's great, though."
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_ThatsTooBadGuarantee, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_11A5:
    VMJump L_11BF

L_11AB:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "OK! Find the [f000]ā\u0001\u0001 that\nI asked you to follow!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OkFindAskedFollow, 0, 0
    LastKeyWait
    ActorMsgClose

L_11BF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_11C5:
    // "OK!\nI'm going to get all fired up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OkImGoingGet, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 255, 11, 3, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_1D48
    ActorCmdExec 0, Movement_1DD0
    ActorCmdWait
    // "Ready all [f000]ā\u0001\u0001!\nEveryone, gather up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_ReadyAllEveryoneGather, 0, 0, 1
    ActorMsgClose
    ActorCmdExec 0, Movement_1DD0
    VMSleep 10
    VMCall L_1389
    FlagReset EVENT_FLAG_0x03ae
    ActorAdd 1
    ActorAdd 2
    ActorAdd 3
    ActorCmdExec 1, Movement_1E5C
    ActorCmdExec 2, Movement_1E6C
    ActorCmdExec 3, Movement_1E78
    PVWait
    ActorCmdWait
    ActorCmdExec 1, Movement_1E14
    VMSleep 8
    ActorCmdExec 2, Movement_1E14
    VMSleep 8
    ActorCmdExec 3, Movement_1E14
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xdd000, 0xb8000, 0x1000f, 49152, 30
    EvCameraWait
    // "The [f000]ā\u0001\u0001 that I will have you\nfollow today is...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_WillHaveFollowToday, 0, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8023, 0
    Random 0x8023, 3
    WordSetItemName 0, 87
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_12B3
    VMJump L_12E9

L_12B3:
    ActorCmdExec 1, Movement_1D70
    ActorCmdWait
    // "This--the [f000]ā\u0001\u0001 that's\nabsolutely raring to go![f000]븁\u0000\nIt's holding a [f000]ĉ\u0001\u0000!\nReady, set, go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_ThatsAbsolutelyRaringGo, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_1D88
    ActorCmdWait
    VMCall L_1389
    PVWait
    WorkSetConst EVENT_WORK_0x400b, 10
    VMJump L_137B

L_12E9:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_12FC
    VMJump L_1332

L_12FC:
    ActorCmdExec 2, Movement_1D70
    ActorCmdWait
    // "This--the [f000]ā\u0001\u0001 with the\ncute round eyes![f000]븁\u0000\nIt's holding a [f000]ĉ\u0001\u0000!\nReady, set, go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_CuteRoundEyesIts, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_1D88
    ActorCmdWait
    VMCall L_1389
    PVWait
    WorkSetConst EVENT_WORK_0x400b, 20
    VMJump L_137B

L_1332:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_1345
    VMJump L_137B

L_1345:
    ActorCmdExec 3, Movement_1D70
    ActorCmdWait
    // "This--the [f000]ā\u0001\u0001 with the\npretty tail![f000]븁\u0000\nIt's holding a [f000]ĉ\u0001\u0000!\nReady, set, go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_PrettyTailItsHolding, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_1D88
    ActorCmdWait
    VMCall L_1389
    PVWait
    WorkSetConst EVENT_WORK_0x400b, 30
    VMJump L_137B

L_137B:
    VMCall L_1447
    WorkSetConst EVENT_WORK_0x400c, 1
    VMReturn

L_1389:
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_139C
    VMJump L_13A8

L_139C:
    PVPlay 511, 0
    VMJump L_13E6

L_13A8:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_13BB
    VMJump L_13C7

L_13BB:
    PVPlay 513, 0
    VMJump L_13E6

L_13C7:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_13DA
    VMJump L_13E6

L_13DA:
    PVPlay 515, 0
    VMJump L_13E6

L_13E6:
    VMReturn

L_13E8:
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_13FB
    VMJump L_1407

L_13FB:
    PVPlay 513, 0
    VMJump L_1445

L_1407:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_141A
    VMJump L_1426

L_141A:
    PVPlay 515, 0
    VMJump L_1445

L_1426:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_1439
    VMJump L_1445

L_1439:
    PVPlay 511, 0
    VMJump L_1445

L_1445:
    VMReturn

L_1447:
    WorkSetConst 0x8024, 0
    Random 0x8024, 3
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_1466
    VMJump L_1556

L_1466:
    ActorCmdExec 1, Movement_1F18
    ActorCmdExec 2, Movement_1F18
    ActorCmdExec 3, Movement_1EF0
    ActorCmdWait
    ActorCmdExec 1, Movement_1FB0
    ActorCmdExec 2, Movement_1EF0
    ActorCmdExec 3, Movement_1EC8
    ActorCmdWait
    ActorCmdExec 1, Movement_1F64
    ActorCmdExec 2, Movement_1EC8
    ActorCmdExec 3, Movement_1F64
    ActorCmdWait
    ActorCmdExec 1, Movement_2044
    ActorCmdExec 2, Movement_208C
    ActorCmdExec 3, Movement_1FF4
    ActorCmdWait
    ActorCmdExec 1, Movement_211C
    ActorCmdExec 2, Movement_1FF4
    ActorCmdExec 3, Movement_20D4
    ActorCmdWait
    ActorCmdExec 1, Movement_1FF4
    ActorCmdExec 2, Movement_2044
    ActorCmdExec 3, Movement_208C
    ActorCmdWait
    ActorCmdExec 1, Movement_21D0
    ActorCmdExec 2, Movement_21D0
    ActorCmdExec 3, Movement_2164
    ActorCmdWait
    ActorCmdExec 1, Movement_2164
    ActorCmdExec 2, Movement_21D0
    ActorCmdExec 3, Movement_21D0
    ActorCmdWait
    ActorCmdExec 1, Movement_21D0
    ActorCmdExec 2, Movement_2164
    ActorCmdExec 3, Movement_21D0
    ActorCmdWait
    VMJump L_175C

L_1556:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_1569
    VMJump L_1659

L_1569:
    ActorCmdExec 1, Movement_1FB0
    ActorCmdExec 2, Movement_1F18
    ActorCmdExec 3, Movement_1F64
    ActorCmdWait
    ActorCmdExec 1, Movement_1EC8
    ActorCmdExec 2, Movement_1F64
    ActorCmdExec 3, Movement_1F64
    ActorCmdWait
    ActorCmdExec 1, Movement_1F64
    ActorCmdExec 2, Movement_1F64
    ActorCmdExec 3, Movement_1EC8
    ActorCmdWait
    ActorCmdExec 1, Movement_1FF4
    ActorCmdExec 2, Movement_2044
    ActorCmdExec 3, Movement_208C
    ActorCmdWait
    ActorCmdExec 1, Movement_211C
    ActorCmdExec 2, Movement_1FF4
    ActorCmdExec 3, Movement_20D4
    ActorCmdWait
    ActorCmdExec 1, Movement_2044
    ActorCmdExec 2, Movement_208C
    ActorCmdExec 3, Movement_1FF4
    ActorCmdWait
    ActorCmdExec 1, Movement_21D0
    ActorCmdExec 2, Movement_2164
    ActorCmdExec 3, Movement_21D0
    ActorCmdWait
    ActorCmdExec 1, Movement_21D0
    ActorCmdExec 2, Movement_21D0
    ActorCmdExec 3, Movement_2164
    ActorCmdWait
    ActorCmdExec 1, Movement_2164
    ActorCmdExec 2, Movement_21D0
    ActorCmdExec 3, Movement_21D0
    ActorCmdWait
    VMJump L_175C

L_1659:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_166C
    VMJump L_175C

L_166C:
    ActorCmdExec 1, Movement_1EC8
    ActorCmdExec 2, Movement_1FB0
    ActorCmdExec 3, Movement_1EF0
    ActorCmdWait
    ActorCmdExec 1, Movement_1EF0
    ActorCmdExec 2, Movement_1F18
    ActorCmdExec 3, Movement_1F18
    ActorCmdWait
    ActorCmdExec 1, Movement_1EC8
    ActorCmdExec 2, Movement_1F64
    ActorCmdExec 3, Movement_1F64
    ActorCmdWait
    ActorCmdExec 1, Movement_208C
    ActorCmdExec 2, Movement_1FF4
    ActorCmdExec 3, Movement_2044
    ActorCmdWait
    ActorCmdExec 1, Movement_2044
    ActorCmdExec 2, Movement_1FF4
    ActorCmdExec 3, Movement_208C
    ActorCmdWait
    ActorCmdExec 1, Movement_208C
    ActorCmdExec 2, Movement_1FF4
    ActorCmdExec 3, Movement_2044
    ActorCmdWait
    ActorCmdExec 1, Movement_21AC
    ActorCmdExec 2, Movement_21AC
    ActorCmdExec 3, Movement_2188
    ActorCmdWait
    ActorCmdExec 1, Movement_21AC
    ActorCmdExec 2, Movement_2188
    ActorCmdExec 3, Movement_21AC
    ActorCmdWait
    ActorCmdExec 1, Movement_2188
    ActorCmdExec 2, Movement_21AC
    ActorCmdExec 3, Movement_21AC
    ActorCmdWait
    VMJump L_175C

L_175C:
    ActorCmdExec 1, Movement_1DB0
    ActorCmdExec 2, Movement_1DB0
    ActorCmdExec 3, Movement_1DB0
    ActorCmdWait
    ActorCmdExec 1, Movement_1E14
    VMSleep 8
    ActorCmdExec 2, Movement_1E14
    VMSleep 8
    ActorCmdExec 3, Movement_1E14
    ActorCmdWait
    // "OK! Find the [f000]ā\u0001\u0001 that\nI asked you to follow!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OkFindAskedFollow, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    EvCameraMoveToDefault 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 10
    VMCall L_17F0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8020, 20
    VMCall L_17F0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x8020, 30
    VMCall L_17F0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_17F0:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1919
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_181C
    VMJump L_1827

L_181C:
    WordSetPokeSpecies 1, 511
    VMJump L_1863

L_1827:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_183A
    VMJump L_1845

L_183A:
    WordSetPokeSpecies 1, 513
    VMJump L_1863

L_1845:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_1858
    VMJump L_1863

L_1858:
    WordSetPokeSpecies 1, 515
    VMJump L_1863

L_1863:
    WordSetItemName 0, 87
    ActorCmdExec 0, Movement_1D28
    ActorCmdWait
    // "That [f000]ā\u0001\u0001 is holding\na [f000]ĉ\u0001\u0000, right?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_HoldingRight, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1917
    MsgWinCloseAll
    ActorWalkRoute 255, 11, 3, 0, 8, 1
    VMSleep 16
    ActorCmdExec 0, Movement_1D50
    ActorCmdExec 1, Movement_1D50
    ActorCmdExec 2, Movement_1D50
    ActorCmdExec 3, Movement_1D50
    ActorCmdWait
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_18EC
    ActorCmdExec 255, Movement_1D48
    ActorCmdWait

L_18EC:
    VMStackPush EVENT_WORK_0x400b
    VMStackPush 0x8020
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_190B
    VMCall L_191B
    VMJump L_1911

L_190B:
    VMCall L_1A04

L_1911:
    VMJump L_1919

L_1917:
    MsgWinCloseAll

L_1919:
    VMReturn

L_191B:
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_192E
    VMJump L_1939

L_192E:
    WordSetPokeSpecies 1, 513
    VMJump L_1975

L_1939:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_194C
    VMJump L_1957

L_194C:
    WordSetPokeSpecies 1, 515
    VMJump L_1975

L_1957:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_196A
    VMJump L_1975

L_196A:
    WordSetPokeSpecies 1, 511
    VMJump L_1975

L_1975:
    WordSetItemName 0, 87
    // "You are...\n...[f000]븀\u0000\ntotally correct![f000]븁\u0000\nReceive your prize, a [f000]ĉ\u0001\u0000,\nfrom that [f000]ā\u0001\u0001![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_TotallyCorrectReceivePrize, 0, 0, 0
    MsgWinCloseAll
    ActorAdd 13
    VMCall L_13E8
    ActorWalkRoute 13, 10, 3, 0, 4, 1
    VMSleep 16
    ActorCmdExec 255, Movement_1D58
    ActorCmdWait
    PVWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 87
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMCall L_13E8
    ActorCmdExec 13, Movement_1EB4
    VMSleep 16
    ActorCmdExec 255, Movement_1D48
    ActorCmdWait
    PVWait
    ActorDelete 13
    WorkSetConst EVENT_WORK_0x400b, 77
    VMCall L_1AF7
    FlagSet EVENT_FLAG_DAILY_0x0ad9
    VMReturn

L_1A04:
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_1A17
    VMJump L_1A22

L_1A17:
    WordSetPokeSpecies 1, 511
    VMJump L_1A5E

L_1A22:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_1A35
    VMJump L_1A40

L_1A35:
    WordSetPokeSpecies 1, 513
    VMJump L_1A5E

L_1A40:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_1A53
    VMJump L_1A5E

L_1A53:
    WordSetPokeSpecies 1, 515
    VMJump L_1A5E

L_1A5E:
    // "You are...\n...[f000]븀\u0000\ninconceivably incorrect! I'm sorry...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_InconceivablyIncorrectImSorry, 0, 0, 0
    MsgWinCloseAll
    WorkCmpConst EVENT_WORK_0x400b, 10
    VMJumpIf CMP_EQ, L_1A7F
    VMJump L_1A97

L_1A7F:
    ActorCmdExec 1, Movement_1D08
    ActorCmdWait
    ActorCmdExec 1, Movement_1D70
    VMJump L_1AED

L_1A97:
    WorkCmpConst EVENT_WORK_0x400b, 20
    VMJumpIf CMP_EQ, L_1AAA
    VMJump L_1AC2

L_1AAA:
    ActorCmdExec 2, Movement_1D08
    ActorCmdWait
    ActorCmdExec 2, Movement_1D70
    VMJump L_1AED

L_1AC2:
    WorkCmpConst EVENT_WORK_0x400b, 30
    VMJumpIf CMP_EQ, L_1AD5
    VMJump L_1AED

L_1AD5:
    ActorCmdExec 3, Movement_1D08
    ActorCmdWait
    ActorCmdExec 3, Movement_1D70
    VMJump L_1AED

L_1AED:
    ActorCmdWait
    VMCall L_1AF7
    VMReturn

L_1AF7:
    WorkCmpConst EVENT_WORK_0x418e, 0
    VMJumpIf CMP_EQ, L_1B0A
    VMJump L_1B15

L_1B0A:
    WordSetPokeSpecies 1, 511
    VMJump L_1B51

L_1B15:
    WorkCmpConst EVENT_WORK_0x418e, 1
    VMJumpIf CMP_EQ, L_1B28
    VMJump L_1B33

L_1B28:
    WordSetPokeSpecies 1, 513
    VMJump L_1B51

L_1B33:
    WorkCmpConst EVENT_WORK_0x418e, 2
    VMJumpIf CMP_EQ, L_1B46
    VMJump L_1B51

L_1B46:
    WordSetPokeSpecies 1, 515
    VMJump L_1B51

L_1B51:
    // "That marks the grand finale of\nour [f000]ā\u0001\u0001 show![f000]븀\u0000\nThank you, one and all![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_MarksGrandFinaleOur, 0, 0, 0
    MsgWinCloseAll
    WorkCmpConst EVENT_WORK_0x400b, 10
    VMJumpIf CMP_EQ, L_1B72
    VMJump L_1B88

L_1B72:
    ActorCmdExec 2, Movement_1D08
    ActorCmdExec 3, Movement_1D08
    VMJump L_1C0B

L_1B88:
    WorkCmpConst EVENT_WORK_0x400b, 20
    VMJumpIf CMP_EQ, L_1B9B
    VMJump L_1BB1

L_1B9B:
    ActorCmdExec 1, Movement_1D08
    ActorCmdExec 3, Movement_1D08
    VMJump L_1C0B

L_1BB1:
    WorkCmpConst EVENT_WORK_0x400b, 30
    VMJumpIf CMP_EQ, L_1BC4
    VMJump L_1BDA

L_1BC4:
    ActorCmdExec 1, Movement_1D08
    ActorCmdExec 2, Movement_1D08
    VMJump L_1C0B

L_1BDA:
    WorkCmpConst EVENT_WORK_0x400b, 77
    VMJumpIf CMP_EQ, L_1BED
    VMJump L_1C0B

L_1BED:
    ActorCmdExec 1, Movement_1D08
    ActorCmdExec 2, Movement_1D08
    ActorCmdExec 3, Movement_1D08
    VMJump L_1C0B

L_1C0B:
    ActorCmdWait
    ActorSetGPos 1, 10, 1, 1, 1
    ActorSetGPos 2, 11, 1, 1, 1
    ActorSetGPos 3, 12, 1, 1, 1
    ActorCmdExec 1, Movement_1E14
    ActorCmdExec 2, Movement_1E14
    ActorCmdExec 3, Movement_1E14
    ActorCmdWait
    ActorCmdExec 1, Movement_1E8C
    ActorCmdExec 3, Movement_1EA4
    VMSleep 16
    ActorCmdExec 2, Movement_1E98
    ActorCmdWait
    ActorDelete 1
    ActorDelete 2
    ActorDelete 3
    FlagSet EVENT_FLAG_0x03ae
    WorkSetConst EVENT_WORK_0x400c, 0
    WorkSetConst EVENT_WORK_0x400b, 0
    VMReturn

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Waaatch!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_Waaatch, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Tch-hooog!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_TchHooog, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh? This show doesn't\nfeature Patrat..."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_OhShowDoesntFeature, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Striaton City's triplets\nmake a fantastic combination![f000]븁\u0000\nThe user of Grass-type Pokémon, Cilan,\ncan pick the finest leaves.[f000]븁\u0000\nThe user of Water-type Pokémon, Cress,\ncan bring the finest water.[f000]븁\u0000\nThe user of Fire-type Pokémon, Chili, can\nheat water to the perfect temperature.[f000]븁\u0000\nNow I get it!\nThey can make the perfect tea!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity2_Text_StriatonCitysTripletsMake, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_1D08:
    Move 13, 1
    MoveEnd

Movement_1D10:
    Move 12, 1
    MoveEnd

Movement_1D18:
    Move 15, 1
    MoveEnd

Movement_1D20:
    Move 14, 1
    MoveEnd

Movement_1D28:
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

Movement_1D48:
    Move 32, 1
    MoveEnd

Movement_1D50:
    Move 33, 1
    MoveEnd

Movement_1D58:
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 48, 1
    MoveEnd

Movement_1D70:
    Move 49, 1
    MoveEnd
    Move 50, 1
    MoveEnd
    Move 51, 1
    MoveEnd

Movement_1D88:
    Move 49, 2
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_1DA0:
    Move 71, 1
    Move 170, 1
    Move 72, 1
    MoveEnd

Movement_1DB0:
    Move 12, 1
    Move 49, 1
    MoveEnd
    // "Yeeeeooow![f000]븁\u0000\nI'm the strongest of us brothers,\nand I'll make sure you win![f000]븁\u0000"
    CheckerMsg StriatonCity2_Text_YeeeeooowImStrongestUs, 254, 0, 0
    VMStackMul
    VMHalt
    Move 33, 1
    MoveEnd

Movement_1DD0:
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    MoveEnd

Movement_1E14:
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 49, 1
    MoveEnd

Movement_1E5C:
    Move 63, 2
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_1E6C:
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_1E78:
    Move 63, 1
    Move 15, 1
    Move 59, 1
    Move 33, 1
    MoveEnd

Movement_1E8C:
    Move 14, 1
    Move 69, 1
    MoveEnd

Movement_1E98:
    Move 14, 2
    Move 69, 1
    MoveEnd

Movement_1EA4:
    Move 58, 1
    Move 14, 1
    Move 69, 1
    MoveEnd

Movement_1EB4:
    Move 18, 2
    Move 16, 2
    Move 19, 1
    Move 69, 1
    MoveEnd

Movement_1EC8:
    Move 33, 2
    Move 59, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_1EF0:
    Move 33, 2
    Move 58, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_1F18:
    Move 33, 2
    Move 55, 1
    Move 33, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_1F64:
    Move 33, 2
    Move 54, 1
    Move 33, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_1FB0:
    Move 33, 4
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_1FF4:
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 13, 1
    MoveEnd

Movement_2044:
    Move 13, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 71, 1
    Move 15, 2
    Move 72, 1
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 1, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    MoveEnd

Movement_208C:
    Move 13, 1
    Move 2, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 71, 1
    Move 14, 2
    Move 72, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    MoveEnd

Movement_20D4:
    Move 13, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 71, 1
    Move 15, 1
    Move 72, 1
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 1, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    MoveEnd

Movement_211C:
    Move 13, 1
    Move 2, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 71, 1
    Move 14, 1
    Move 72, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    MoveEnd

Movement_2164:
    Move 59, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_2188:
    Move 58, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_21AC:
    Move 55, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_21D0:
    Move 54, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Movement_21F4:
    Move 75, 1
    MoveEnd
