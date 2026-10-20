#include "asm/field_script.inc"
#include "text/script/unity_tower_3.h"

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

Script_1:
    WorkSetConst 0x8021, 0
    Cmd_01CE 0, EVENT_WORK_0x4020
    Cmd_01CE 1, EVENT_WORK_0x4021
    Cmd_01CE 2, EVENT_WORK_0x4022
    Cmd_01CE 3, EVENT_WORK_0x4023
    Cmd_01CE 4, EVENT_WORK_0x4024
    FlagSet EVENT_FLAG_0x0260
    FlagSet EVENT_FLAG_0x0261
    FlagSet EVENT_FLAG_0x0262
    FlagSet EVENT_FLAG_0x0263
    FlagSet EVENT_FLAG_0x0264
    UnityTowerGetStateParam 0, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0085
    FlagReset EVENT_FLAG_0x0260

L_0085:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_009C
    FlagReset EVENT_FLAG_0x0261

L_009C:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00B3
    FlagReset EVENT_FLAG_0x0262

L_00B3:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00CA
    FlagReset EVENT_FLAG_0x0263

L_00CA:
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00E1
    FlagReset EVENT_FLAG_0x0264

L_00E1:
    VMHalt

Script_9:
    ActorsPauseAll
    VMCall L_03CC
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorCmdExec 6, Movement_04DC
    ActorCmdWait
    ActorCmdExec 255, Movement_04E4
    ActorCmdWait
    VMCall L_0117
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0117:
    WorkSetConst 0x8022, 0
    // "Return to the entrance?"
    ActorMsg MSGFILE_SCRIPT, UnityTower3_Text_ReturnEntrance, 6, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    ListMenuAdd 65, 65535, 0
    ListMenuAdd 66, 65535, 1
    ListMenuAdd 67, 65535, 2
    ListMenuShow
    ActorMsgClose
    VMStackPush 0x8022
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_024E
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_0174
    VMJump L_01A8

L_0174:
    VMCall L_03FA
    RTReserveScript 9
    SEPlay SEQ_SE_FLD_23
    FadeOutBlackQ
    FadeWait
    MapChangeCore ZONE_UNITY_TOWER_2, 14, 0, 5, 1
    FadeInBlackQ
    FadeWait
    VMSleep 60
    SEStop
    SEPlay SEQ_SE_FLD_87
    SEWait
    VMJump L_024E

L_01A8:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_01BB
    VMJump L_024E

L_01BB:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    UnityTowerGetStateParam 2, 0x8023
    FadeOutBlackQ
    FadeWait
    FieldClose
    UnityTowerCallFloorSelect 0x8023, 0x8024, 0x8025, 0x8010
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0236
    VMCall L_03FA
    UnityTowerSetFloor 0x8025, 0x8024
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

L_0236:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    VMJump L_024E

L_024E:
    VMReturn

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    UnityTowerGetStateParam 0, 0x8026
    UnityTowerGetStateParam 2, 0x8028
    UnityTowerGetStateParam 1, 0x8029
    WordSetCountry 0, 0x8029
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02C5
    VMStackPush 0x8028
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    WorkSetConst 0x8027, 1
    VMJump L_02BF

L_02B9:
    WorkSetConst 0x8027, 0

L_02BF:
    VMJump L_02EA

L_02C5:
    VMStackPush 0x8028
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E4
    WorkSetConst 0x8027, 3
    VMJump L_02EA

L_02E4:
    WorkSetConst 0x8027, 2

L_02EA:
    WordSetNumber 2, 0x8028, 3
    ParentActorMsg MSGFILE_SCRIPT, 0x8027, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0117
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4000, 0
    VMCall L_037D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4000, 1
    VMCall L_037D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4000, 2
    VMCall L_037D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4000, 3
    VMCall L_037D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4000, 4
    VMCall L_037D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_037D:
    WorkSetConst 0x802a, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    UnityTowerGetVisitorParam EVENT_WORK_0x4000, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B4
    UnityTowerCmd_02D8 EVENT_WORK_0x4000
    UnityTowerInitVisitorMessage EVENT_WORK_0x4000, 1, 0x802a
    VMJump L_03BC

L_03B4:
    UnityTowerInitVisitorMessage EVENT_WORK_0x4000, 0, 0x802a

L_03BC:
    ParentActorMsg MSGFILE_SCRIPT, 0x802a, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_03CC:
    BMCreateHandleByGPos 0x8020, 1, 10, 6
    BMHndAudioVisualAnmPlay 0x8020, 0
    BMHndAnmWait 0x8020
    ActorCmdExec 255, Movement_04A4
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8020, 1
    BMHndAnmWait 0x8020
    BMReleaseHandle 0x8020
    VMReturn

L_03FA:
    BMCreateHandleByGPos 0x8020, 1, 10, 6
    BMHndAudioVisualAnmPlay 0x8020, 0
    BMHndAnmWait 0x8020
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    PlayerGetGPos 0x802b, 0x802c
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_0433
    VMJump L_0441

L_0433:
    ActorCmdExec 255, Movement_04B8
    VMJump L_0483

L_0441:
    WorkCmpConst 0x802b, 9
    VMJumpIf CMP_EQ, L_0454
    VMJump L_0462

L_0454:
    ActorCmdExec 255, Movement_04CC
    VMJump L_0483

L_0462:
    WorkCmpConst 0x802b, 10
    VMJumpIf CMP_EQ, L_0475
    VMJump L_0483

L_0475:
    ActorCmdExec 255, Movement_04AC
    VMJump L_0483

L_0483:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8020, 1
    BMHndAnmWait 0x8020
    BMReleaseHandle 0x8020
    VMReturn
    .balign 4, 0

Movement_04A4:
    Move 13, 2
    MoveEnd

Movement_04AC:
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_04B8:
    Move 13, 1
    Move 15, 2
    Move 12, 3
    Move 33, 1
    MoveEnd

Movement_04CC:
    Move 15, 1
    Move 12, 3
    Move 33, 1
    MoveEnd

Movement_04DC:
    Move 3, 1
    MoveEnd

Movement_04E4:
    Move 34, 1
    MoveEnd
