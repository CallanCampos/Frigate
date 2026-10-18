#include "asm/field_script.inc"
#include "text/script/gear_station.h"

// Script plugin 1, from the zones that use this file

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_5:
    ActorsPauseAll
    CallPlaceNameDisp
    ActorCmdExec 255, Movement_006C
    ActorCmdWait
    PlayerSetRailPos 2, 0, 1
    WorkSetConst EVENT_WORK_0x417c, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_006C:
    Move 14, 10
    MoveEnd

Script_4:
    ActorsPauseAll
    PlayerSetRailPos 6, 0, 0
    ActorCmdExec 255, Movement_00A0
    ActorCmdWait
    RTReserveScript 7
    LensFlareRequest
    MapChangeWarp ZONE_NIMBASA_CITY, 422, 459, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_00A0:
    Move 15, 8
    MoveEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can take the Battle Subway from\nGear Station!"
    ParentActorMsg MSGFILE_SCRIPT, GearStation_Text_CanTakeBattleSubway, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Battle Subway is the subway\nwhere Trainers see who's strongest!"
    ParentActorMsg MSGFILE_SCRIPT, GearStation_Text_BattleSubwaySubwayWhere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010B
    // "The Battle Subway has seven lines\nin total![f000]븁\u0000\nIf you can ride a train with “Super\" in\nits name, then you're quite the Trainer!"
    ParentActorMsg MSGFILE_SCRIPT, GearStation_Text_BattleSubwayHasSeven_2, 0, 0
    VMJump L_0115

L_010B:
    // "The Battle Subway has seven lines\nin total![f000]븁\u0000\nBut I heard that the only lines you can\ntake at the beginning are the Single,[f000]븀\u0000\nDouble, or Multi Trains!"
    ParentActorMsg MSGFILE_SCRIPT, GearStation_Text_BattleSubwayHasSeven, 0, 0

L_0115:
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_01A6
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0152
    WorkSetConst 0x8022, 10
    VMJump L_0190

L_0152:
    DebugPrint 0x8020
    DebugPrint 0x8020
    VMStackPush 0x8020
    VMStackPushConst 65535
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0179
    WorkSetConst 0x8022, 12
    VMJump L_0188

L_0179:
    WordSetTrendName 0, 0x8020
    WorkSetConst 0x8022, 9
    DebugPrint 0x8020

L_0188:
    DebugPrint 0x8010
    DebugPrint 0x8022

L_0190:
    ActorMsg MSGFILE_SCRIPT, 0x8022, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01A6:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    // "Hi, hi![f000]븁\u0000\nPlease tell me how you introduce yourself\nbefore a battle and how you feel when[f000]븀\u0000\nyou win or lose."
    ActorMsg MSGFILE_SCRIPT, GearStation_Text_HiHiPleaseTell, 0x8011, 4, 0

L_01C4:
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0372
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32804
    ListMenuAdd 13, 65535, 1
    ListMenuAdd 14, 65535, 2
    ListMenuAdd 15, 65535, 3
    ListMenuAdd 16, 65535, 4
    ListMenuAdd 17, 65535, 0
    ListMenuShow
    VMStackPush 0x8024
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0360
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_034E
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_024F
    VMJump L_0261

L_024F:
    WorkSetConst 0x8026, 5
    WorkSetConst 0x8025, 0
    VMJump L_02D0

L_0261:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_0274
    VMJump L_0286

L_0274:
    WorkSetConst 0x8026, 7
    WorkSetConst 0x8025, 1
    VMJump L_02D0

L_0286:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_0299
    VMJump L_02AB

L_0299:
    WorkSetConst 0x8026, 6
    WorkSetConst 0x8025, 2
    VMJump L_02D0

L_02AB:
    WorkCmpConst 0x8024, 4
    VMJumpIf CMP_EQ, L_02BE
    VMJump L_02D0

L_02BE:
    WorkSetConst 0x8026, 8
    WorkSetConst 0x8025, 3
    VMJump L_02D0

L_02D0:
    ActorMsg MSGFILE_SCRIPT, 0x8026, 0x8011, 4, 0
    ActorMsgClose
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPhraseSelect 0x8025, 0x8020, 0x8021, 0x8010
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033C
    // "Thanks! Will you tell me your other\nfeelings, too?"
    ActorMsg MSGFILE_SCRIPT, GearStation_Text_ThanksWillTellOther, 0x8011, 4, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0336
    WorkSetConst 0x8010, 1
    WorkSetConst 0x8023, 1

L_0336:
    VMJump L_0348

L_033C:
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8023, 1

L_0348:
    VMJump L_035A

L_034E:
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8023, 1

L_035A:
    VMJump L_036C

L_0360:
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8023, 1

L_036C:
    VMJump L_01C4

L_0372:
    VMReturn

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    BSubwayCmd_Tool 43, 0, 0, 32784
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03AB
    // "Under Construction"
    SystemMsg GearStation_Text_UnderConstruction, 2
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03B5

L_03AB:
    BSubwayCmd_Tool 111, 0, 0, 32784

L_03B5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Single Trains"
    SystemMsg GearStation_Text_PlatformSingleTrains, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Double Trains"
    SystemMsg GearStation_Text_PlatformDoubleTrains, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Multi Trains"
    SystemMsg GearStation_Text_PlatformMultiTrains, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Wi-Fi Trains"
    SystemMsg GearStation_Text_PlatformWiFiTrains, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Super Single Trains"
    SystemMsg GearStation_Text_PlatformSuperSingleTrains, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Super Double Trains"
    SystemMsg GearStation_Text_PlatformSuperDoubleTrains, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Super Multi Trains"
    SystemMsg GearStation_Text_PlatformSuperMultiTrains, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Platform for Trains to Anville Town"
    SystemMsg GearStation_Text_PlatformTrainsAnvilleTown, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
