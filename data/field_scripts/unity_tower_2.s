#include "asm/field_script.inc"
#include "text/script/unity_tower_2.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_10:
    WorkSetConst EVENT_WORK_0x4001, 0
    VMStackPush EVENT_WORK_0x417e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005B
    ActorSetGPos 0, 13, 0, 10, 3
    VMJump L_0061

L_005B:
    VMCall L_078C

L_0061:
    WorkSetConst EVENT_WORK_0x417e, 0
    VMHalt

Script_9:
    ActorsPauseAll
    VMCall L_0707
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello, and welcome to Unity Tower!\nWould you like to go upstairs?"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_HelloWelcomeUnityTower, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0141
    // "Please select a floor.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_PleaseSelectFloor, 0, 0
    ActorMsgClose
    FadeOutBlackQ
    FadeWait
    FieldClose
    UnityTowerCallFloorSelect 1, 0x8021, 0x8022, 0x8010
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F5
    // "Please come again."
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_PleaseComeAgain, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_013B

L_00F5:
    UnityTowerSetFloor 0x8022, 0x8021
    // "Indeed.\nPlease step inside the elevator.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_IndeedPleaseStepInside, 0, 0
    ActorMsgClose
    VMCall L_06C5
    WorkSetConst EVENT_WORK_0x417e, 1
    RTReserveScript 9
    SEPlay SEQ_SE_FLD_23
    FadeOutBlackQ
    FadeWait
    MapChangeCore ZONE_UNITY_TOWER_3, 10, 0, 5, 1
    FadeInBlackQ
    FadeWait
    VMSleep 60
    SEStop
    SEPlay SEQ_SE_FLD_87
    SEWait

L_013B:
    VMJump L_014F

L_0141:
    // "Please come again."
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_PleaseComeAgain, 0, 0
    LastKeyWait
    ActorMsgClose

L_014F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is Unity Tower.[f000]븁\u0000\nTrainers from all over the world cross\nborders and oceans to gather here.[f000]븁\u0000\nVisit whichever floor you like![f000]븁\u0000\nThe more people you know worldwide,\nthe more floors you can visit.[f000]븁\u0000\nPlease enjoy all that we have to offer!"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_UnityTowerTrainersFrom, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8023, 0
    UnityTowerGetVisitorCount 0x8023
    WordSetNumber 11, 0x8023, 2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Right now, the number of visitors in\nUnity Tower is [f000]ȁ\u0001\u000b."
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_RightNowNumberVisitors, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8024, 0
    UnityTowerGetHobby 0x8024
    WordSetHobbyName 6, 0x8024
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wow! So you like\n[f000]ď\u0001\u0006, then!"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_WowLikeThen, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Awesome! The ceiling is sooo high![f000]븁\u0000\nI wonder how many of me\nyou'd have to stack up to reach it?"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_AwesomeCeilingSoooHigh, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Coming to Unity Tower always\ngets me excited![f000]븁\u0000\nBeing here always reminds me of how big\nthe world really is!"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower2_Text_ComingUnityTowerAlways, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8025, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4086, 0
    VMJumpIf CMP_EQ, L_0222
    VMJump L_022E

L_0222:
    WorkSetConst 0x8025, 8
    VMJump L_0424

L_022E:
    WorkCmpConst EVENT_WORK_0x4086, 1
    VMJumpIf CMP_EQ, L_0241
    VMJump L_024D

L_0241:
    WorkSetConst 0x8025, 27
    VMJump L_0424

L_024D:
    WorkCmpConst EVENT_WORK_0x4086, 2
    VMJumpIf CMP_EQ, L_0260
    VMJump L_026C

L_0260:
    WorkSetConst 0x8025, 28
    VMJump L_0424

L_026C:
    WorkCmpConst EVENT_WORK_0x4086, 3
    VMJumpIf CMP_EQ, L_027F
    VMJump L_028B

L_027F:
    WorkSetConst 0x8025, 29
    VMJump L_0424

L_028B:
    WorkCmpConst EVENT_WORK_0x4086, 4
    VMJumpIf CMP_EQ, L_029E
    VMJump L_02AA

L_029E:
    WorkSetConst 0x8025, 30
    VMJump L_0424

L_02AA:
    WorkCmpConst EVENT_WORK_0x4086, 5
    VMJumpIf CMP_EQ, L_02BD
    VMJump L_02C9

L_02BD:
    WorkSetConst 0x8025, 31
    VMJump L_0424

L_02C9:
    WorkCmpConst EVENT_WORK_0x4086, 6
    VMJumpIf CMP_EQ, L_02DC
    VMJump L_02E8

L_02DC:
    WorkSetConst 0x8025, 32
    VMJump L_0424

L_02E8:
    WorkCmpConst EVENT_WORK_0x4086, 7
    VMJumpIf CMP_EQ, L_02FB
    VMJump L_0307

L_02FB:
    WorkSetConst 0x8025, 33
    VMJump L_0424

L_0307:
    WorkCmpConst EVENT_WORK_0x4086, 8
    VMJumpIf CMP_EQ, L_031A
    VMJump L_0326

L_031A:
    WorkSetConst 0x8025, 34
    VMJump L_0424

L_0326:
    WorkCmpConst EVENT_WORK_0x4086, 9
    VMJumpIf CMP_EQ, L_0339
    VMJump L_0345

L_0339:
    WorkSetConst 0x8025, 35
    VMJump L_0424

L_0345:
    WorkCmpConst EVENT_WORK_0x4086, 10
    VMJumpIf CMP_EQ, L_0358
    VMJump L_0364

L_0358:
    WorkSetConst 0x8025, 36
    VMJump L_0424

L_0364:
    WorkCmpConst EVENT_WORK_0x4086, 11
    VMJumpIf CMP_EQ, L_0377
    VMJump L_0383

L_0377:
    WorkSetConst 0x8025, 37
    VMJump L_0424

L_0383:
    WorkCmpConst EVENT_WORK_0x4086, 12
    VMJumpIf CMP_EQ, L_0396
    VMJump L_03A2

L_0396:
    WorkSetConst 0x8025, 38
    VMJump L_0424

L_03A2:
    WorkCmpConst EVENT_WORK_0x4086, 13
    VMJumpIf CMP_EQ, L_03B5
    VMJump L_03C1

L_03B5:
    WorkSetConst 0x8025, 39
    VMJump L_0424

L_03C1:
    WorkCmpConst EVENT_WORK_0x4086, 14
    VMJumpIf CMP_EQ, L_03D4
    VMJump L_03E0

L_03D4:
    WorkSetConst 0x8025, 40
    VMJump L_0424

L_03E0:
    WorkCmpConst EVENT_WORK_0x4086, 15
    VMJumpIf CMP_EQ, L_03F3
    VMJump L_03FF

L_03F3:
    WorkSetConst 0x8025, 41
    VMJump L_0424

L_03FF:
    WorkCmpConst EVENT_WORK_0x4086, 16
    VMJumpIf CMP_EQ, L_0412
    VMJump L_041E

L_0412:
    WorkSetConst 0x8025, 42
    VMJump L_0424

L_041E:
    WorkSetConst 0x8025, 27

L_0424:
    WordSetPlayerName 3
    ActorMsg MSGFILE_SCRIPT, 0x8025, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 1
    SEPlay SEQ_SE_FLD_124
    SEWait
    ActorCmdExec 4, Movement_0BC0
    ActorCmdWait
    // "Hey, you over there!\nCome here for a second.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UnityTower2_Text_HeyOverThereCome, 4, 6, 0
    ActorMsgClose
    ActorCmdExec 4, Movement_0BC8
    ActorCmdExec 255, Movement_0BD4
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4086
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04AE
    // "You're a new face.\nIs this your first time here?[f000]븁\u0000\nUnity Tower is the place where\nTrainers come from all over the world.[f000]븁\u0000\nThat's why I work hard every day doing\nsecurity checks to protect those[f000]븀\u0000\nyoung Trainers.[f000]븁\u0000\nDon't loiter around here\nand cause trouble.[f000]븀\u0000\n...'Kay, you can pass.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, UnityTower2_Text_YoureNewFaceFirst, 4, 0, 0
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x4086, 1
    VMJump L_06AF

L_04AE:
    WorkCmpConst EVENT_WORK_0x4002, 2
    VMJumpIf CMP_EQ, L_04C1
    VMJump L_04CD

L_04C1:
    WorkSetConst 0x8027, 10
    VMJump L_0685

L_04CD:
    WorkCmpConst EVENT_WORK_0x4002, 3
    VMJumpIf CMP_EQ, L_04E0
    VMJump L_04EC

L_04E0:
    WorkSetConst 0x8027, 11
    VMJump L_0685

L_04EC:
    WorkCmpConst EVENT_WORK_0x4002, 4
    VMJumpIf CMP_EQ, L_04FF
    VMJump L_050B

L_04FF:
    WorkSetConst 0x8027, 12
    VMJump L_0685

L_050B:
    WorkCmpConst EVENT_WORK_0x4002, 5
    VMJumpIf CMP_EQ, L_051E
    VMJump L_052A

L_051E:
    WorkSetConst 0x8027, 13
    VMJump L_0685

L_052A:
    WorkCmpConst EVENT_WORK_0x4002, 6
    VMJumpIf CMP_EQ, L_053D
    VMJump L_0549

L_053D:
    WorkSetConst 0x8027, 14
    VMJump L_0685

L_0549:
    WorkCmpConst EVENT_WORK_0x4002, 7
    VMJumpIf CMP_EQ, L_055C
    VMJump L_0568

L_055C:
    WorkSetConst 0x8027, 15
    VMJump L_0685

L_0568:
    WorkCmpConst EVENT_WORK_0x4002, 8
    VMJumpIf CMP_EQ, L_057B
    VMJump L_0587

L_057B:
    WorkSetConst 0x8027, 16
    VMJump L_0685

L_0587:
    WorkCmpConst EVENT_WORK_0x4002, 9
    VMJumpIf CMP_EQ, L_059A
    VMJump L_05A6

L_059A:
    WorkSetConst 0x8027, 17
    VMJump L_0685

L_05A6:
    WorkCmpConst EVENT_WORK_0x4002, 10
    VMJumpIf CMP_EQ, L_05B9
    VMJump L_05C5

L_05B9:
    WorkSetConst 0x8027, 18
    VMJump L_0685

L_05C5:
    WorkCmpConst EVENT_WORK_0x4002, 11
    VMJumpIf CMP_EQ, L_05D8
    VMJump L_05E4

L_05D8:
    WorkSetConst 0x8027, 19
    VMJump L_0685

L_05E4:
    WorkCmpConst EVENT_WORK_0x4002, 12
    VMJumpIf CMP_EQ, L_05F7
    VMJump L_0603

L_05F7:
    WorkSetConst 0x8027, 20
    VMJump L_0685

L_0603:
    WorkCmpConst EVENT_WORK_0x4002, 13
    VMJumpIf CMP_EQ, L_0616
    VMJump L_0622

L_0616:
    WorkSetConst 0x8027, 21
    VMJump L_0685

L_0622:
    WorkCmpConst EVENT_WORK_0x4002, 14
    VMJumpIf CMP_EQ, L_0635
    VMJump L_0641

L_0635:
    WorkSetConst 0x8027, 22
    VMJump L_0685

L_0641:
    WorkCmpConst EVENT_WORK_0x4002, 15
    VMJumpIf CMP_EQ, L_0654
    VMJump L_0660

L_0654:
    WorkSetConst 0x8027, 23
    VMJump L_0685

L_0660:
    WorkCmpConst EVENT_WORK_0x4002, 16
    VMJumpIf CMP_EQ, L_0673
    VMJump L_067F

L_0673:
    WorkSetConst 0x8027, 24
    VMJump L_0685

L_067F:
    WorkSetConst 0x8026, 0

L_0685:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06AF
    WordSetPlayerName 3
    ActorMsg MSGFILE_SCRIPT, 0x8027, 4, 0, 0
    ActorMsgClose
    WorkGet EVENT_WORK_0x4086, EVENT_WORK_0x4002

L_06AF:
    ActorCmdExec 4, Movement_0BDC
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x4001, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_06C5:
    ActorCmdExec 0, Movement_074C
    ActorCmdWait
    ActorCmdExec 255, Movement_075C
    ActorCmdWait
    BMCreateHandleByGPos 0x8020, 1, 14, 6
    BMHndAudioVisualAnmPlay 0x8020, 0
    BMHndAnmWait 0x8020
    ActorCmdExec 255, Movement_0764
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8020, 1
    BMHndAnmWait 0x8020
    BMReleaseHandle 0x8020
    VMReturn

L_0707:
    BMCreateHandleByGPos 0x8020, 1, 14, 6
    BMHndAudioVisualAnmPlay 0x8020, 0
    BMHndAnmWait 0x8020
    ActorCmdExec 255, Movement_0770
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8020, 1
    ActorCmdExec 255, Movement_0778
    ActorCmdWait
    BMHndAnmWait 0x8020
    BMReleaseHandle 0x8020
    ActorCmdExec 0, Movement_0780
    ActorCmdWait
    VMReturn
    .balign 4, 0

Movement_074C:
    Move 12, 1
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_075C:
    Move 12, 5
    MoveEnd

Movement_0764:
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_0770:
    Move 13, 2
    MoveEnd

Movement_0778:
    Move 13, 5
    MoveEnd

Movement_0780:
    Move 15, 1
    Move 13, 1
    MoveEnd

L_078C:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    VMStackPush EVENT_WORK_0x4086
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07BD
    WorkSetConst EVENT_WORK_0x4001, 1
    VMJump L_0BBE

L_07BD:
    WorkCmpConst EVENT_WORK_0x4086, 1
    VMJumpIf CMP_EQ, L_07D0
    VMJump L_07DC

L_07D0:
    WorkSetConst 0x8029, 3
    VMJump L_09AF

L_07DC:
    WorkCmpConst EVENT_WORK_0x4086, 2
    VMJumpIf CMP_EQ, L_07EF
    VMJump L_07FB

L_07EF:
    WorkSetConst 0x8029, 5
    VMJump L_09AF

L_07FB:
    WorkCmpConst EVENT_WORK_0x4086, 3
    VMJumpIf CMP_EQ, L_080E
    VMJump L_081A

L_080E:
    WorkSetConst 0x8029, 10
    VMJump L_09AF

L_081A:
    WorkCmpConst EVENT_WORK_0x4086, 4
    VMJumpIf CMP_EQ, L_082D
    VMJump L_0839

L_082D:
    WorkSetConst 0x8029, 15
    VMJump L_09AF

L_0839:
    WorkCmpConst EVENT_WORK_0x4086, 5
    VMJumpIf CMP_EQ, L_084C
    VMJump L_0858

L_084C:
    WorkSetConst 0x8029, 20
    VMJump L_09AF

L_0858:
    WorkCmpConst EVENT_WORK_0x4086, 6
    VMJumpIf CMP_EQ, L_086B
    VMJump L_0877

L_086B:
    WorkSetConst 0x8029, 25
    VMJump L_09AF

L_0877:
    WorkCmpConst EVENT_WORK_0x4086, 7
    VMJumpIf CMP_EQ, L_088A
    VMJump L_0896

L_088A:
    WorkSetConst 0x8029, 30
    VMJump L_09AF

L_0896:
    WorkCmpConst EVENT_WORK_0x4086, 8
    VMJumpIf CMP_EQ, L_08A9
    VMJump L_08B5

L_08A9:
    WorkSetConst 0x8029, 35
    VMJump L_09AF

L_08B5:
    WorkCmpConst EVENT_WORK_0x4086, 9
    VMJumpIf CMP_EQ, L_08C8
    VMJump L_08D4

L_08C8:
    WorkSetConst 0x8029, 40
    VMJump L_09AF

L_08D4:
    WorkCmpConst EVENT_WORK_0x4086, 10
    VMJumpIf CMP_EQ, L_08E7
    VMJump L_08F3

L_08E7:
    WorkSetConst 0x8029, 50
    VMJump L_09AF

L_08F3:
    WorkCmpConst EVENT_WORK_0x4086, 11
    VMJumpIf CMP_EQ, L_0906
    VMJump L_0912

L_0906:
    WorkSetConst 0x8029, 60
    VMJump L_09AF

L_0912:
    WorkCmpConst EVENT_WORK_0x4086, 12
    VMJumpIf CMP_EQ, L_0925
    VMJump L_0931

L_0925:
    WorkSetConst 0x8029, 70
    VMJump L_09AF

L_0931:
    WorkCmpConst EVENT_WORK_0x4086, 13
    VMJumpIf CMP_EQ, L_0944
    VMJump L_0950

L_0944:
    WorkSetConst 0x8029, 85
    VMJump L_09AF

L_0950:
    WorkCmpConst EVENT_WORK_0x4086, 14
    VMJumpIf CMP_EQ, L_0963
    VMJump L_096F

L_0963:
    WorkSetConst 0x8029, 100
    VMJump L_09AF

L_096F:
    WorkCmpConst EVENT_WORK_0x4086, 15
    VMJumpIf CMP_EQ, L_0982
    VMJump L_098E

L_0982:
    WorkSetConst 0x8029, 130
    VMJump L_09AF

L_098E:
    WorkCmpConst EVENT_WORK_0x4086, 16
    VMJumpIf CMP_EQ, L_09A1
    VMJump L_09AD

L_09A1:
    WorkSetConst 0x8029, 300
    VMJump L_09AF

L_09AD:
    VMReturn

L_09AF:
    UnityTowerCmd_02D7 1, 0x8028
    VMStackPush 0x8028
    VMStackPushConst 130
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_09D4
    WorkSetConst 0x802a, 16
    VMJump L_0B8C

L_09D4:
    VMStackPush 0x8028
    VMStackPushConst 100
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_09F3
    WorkSetConst 0x802a, 15
    VMJump L_0B8C

L_09F3:
    VMStackPush 0x8028
    VMStackPushConst 85
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0A12
    WorkSetConst 0x802a, 14
    VMJump L_0B8C

L_0A12:
    VMStackPush 0x8028
    VMStackPushConst 70
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0A31
    WorkSetConst 0x802a, 13
    VMJump L_0B8C

L_0A31:
    VMStackPush 0x8028
    VMStackPushConst 60
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0A50
    WorkSetConst 0x802a, 12
    VMJump L_0B8C

L_0A50:
    VMStackPush 0x8028
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0A6F
    WorkSetConst 0x802a, 11
    VMJump L_0B8C

L_0A6F:
    VMStackPush 0x8028
    VMStackPushConst 40
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0A8E
    WorkSetConst 0x802a, 10
    VMJump L_0B8C

L_0A8E:
    VMStackPush 0x8028
    VMStackPushConst 35
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0AAD
    WorkSetConst 0x802a, 9
    VMJump L_0B8C

L_0AAD:
    VMStackPush 0x8028
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0ACC
    WorkSetConst 0x802a, 8
    VMJump L_0B8C

L_0ACC:
    VMStackPush 0x8028
    VMStackPushConst 25
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0AEB
    WorkSetConst 0x802a, 7
    VMJump L_0B8C

L_0AEB:
    VMStackPush 0x8028
    VMStackPushConst 20
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0B0A
    WorkSetConst 0x802a, 6
    VMJump L_0B8C

L_0B0A:
    VMStackPush 0x8028
    VMStackPushConst 15
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0B29
    WorkSetConst 0x802a, 5
    VMJump L_0B8C

L_0B29:
    VMStackPush 0x8028
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0B48
    WorkSetConst 0x802a, 4
    VMJump L_0B8C

L_0B48:
    VMStackPush 0x8028
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0B67
    WorkSetConst 0x802a, 3
    VMJump L_0B8C

L_0B67:
    VMStackPush 0x8028
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0B86
    WorkSetConst 0x802a, 2
    VMJump L_0B8C

L_0B86:
    WorkSetConst 0x802a, 1

L_0B8C:
    VMStackPush 0x8028
    VMStackPush 0x8029
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0BBE
    WorkSetConst EVENT_WORK_0x4001, 1
    VMStackPush 0x802a
    VMStackPush EVENT_WORK_0x4086
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0BBE
    WorkGet EVENT_WORK_0x4002, 0x802a

L_0BBE:
    VMReturn

Movement_0BC0:
    Move 33, 1
    MoveEnd

Movement_0BC8:
    Move 14, 2
    Move 33, 1
    MoveEnd

Movement_0BD4:
    Move 12, 2
    MoveEnd

Movement_0BDC:
    Move 15, 2
    Move 34, 1
    MoveEnd
