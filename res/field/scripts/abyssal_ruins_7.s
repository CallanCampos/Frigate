#include "asm/field_script.inc"
#include "text/script/abyssal_ruins_7.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_13:
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005B
    WorkSetConst EVENT_WORK_0x418f, 4

L_005B:
    VMHalt

Script_11:
    VMStackPushFlag EVENT_FLAG_0x00d6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0088
    ActorSetGPos 0, 44, 3, 26, 1
    ActorSetGPos 1, 44, 3, 26, 1

L_0088:
    VMHalt

Script_12:
    VMStackPushFlag EVENT_FLAG_0x00d6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B5
    ActorSetGPos 0, 44, 3, 26, 1
    ActorSetGPos 1, 44, 3, 26, 1

L_00B5:
    VMHalt

L_00B7:
    SEPlay SEQ_SE_MESSAGE
    Cmd_0230 0x8020, 0
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00DC
    MsgPlaceSignClose
    MsgPlaceSign 0x8021, 0

L_00DC:
    MsgPlaceSignClose
    VMReturn

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 7
    VMCall L_00B7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 8
    VMCall L_00B7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    WorkSetConst 0x8021, 9
    VMCall L_00B7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    WorkSetConst 0x8021, 10
    VMCall L_00B7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 4
    WorkSetConst 0x8021, 11
    VMCall L_00B7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    // "It looks like you can climb up here![f000]븁\u0000\nWill you proceed to the upper floor?"
    SystemMsg AbyssalRuins7_Text_LooksLikeCanClimb, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0226
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetGPos 0x8022, 0x8023
    WorkCmpConst 0x8022, 41
    VMJumpIf CMP_EQ, L_01A8
    VMJump L_01B6

L_01A8:
    ActorCmdExec 255, Movement_02D4
    VMJump L_01F8

L_01B6:
    WorkCmpConst 0x8022, 42
    VMJumpIf CMP_EQ, L_01C9
    VMJump L_01D7

L_01C9:
    ActorCmdExec 255, Movement_02E4
    VMJump L_01F8

L_01D7:
    WorkCmpConst 0x8022, 43
    VMJumpIf CMP_EQ, L_01EA
    VMJump L_01F8

L_01EA:
    ActorCmdExec 255, Movement_02EC
    VMJump L_01F8

L_01F8:
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 1
    VMSleep 40
    FadeOutBlack
    FadeWait
    RTReserveScript 1
    MapChangeCore ZONE_ABYSSAL_RUINS_8, 13, 3, 22, 0
    MedalGive 100
    VMJump L_02BC

L_0226:
    WorkSetConst 0x8024, 0
    PlayerGetDir 0x8024
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_0243
    VMJump L_0253

L_0243:
    ActorCmdExec 255, Movement_02FC
    ActorCmdWait
    VMJump L_02BC

L_0253:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_0266
    VMJump L_0276

L_0266:
    ActorCmdExec 255, Movement_0304
    ActorCmdWait
    VMJump L_02BC

L_0276:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_0289
    VMJump L_0299

L_0289:
    ActorCmdExec 255, Movement_030C
    ActorCmdWait
    VMJump L_02BC

L_0299:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_02AC
    VMJump L_02BC

L_02AC:
    ActorCmdExec 255, Movement_0314
    ActorCmdWait
    VMJump L_02BC

L_02BC:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_02D4:
    Move 12, 1
    Move 15, 1
    Move 0, 1
    MoveEnd

Movement_02E4:
    Move 12, 1
    MoveEnd

Movement_02EC:
    Move 12, 1
    Move 14, 1
    Move 0, 1
    MoveEnd

Movement_02FC:
    Move 13, 1
    MoveEnd

Movement_0304:
    Move 12, 1
    MoveEnd

Movement_030C:
    Move 15, 1
    MoveEnd

Movement_0314:
    Move 14, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorCmdExec 255, Movement_0338
    FadeInBlack
    FadeExWait
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0338:
    Move 11, 1
    Move 15, 1
    MoveEnd

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorCmdExec 255, Movement_0360
    FadeInBlack
    FadeExWait
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0360:
    Move 9, 1
    Move 13, 1
    MoveEnd

Script_8:
    ActorsPauseAll
    // "It looks like you can climb down here![f000]븁\u0000\nWill you return to the lower floor?"
    SystemMsg AbyssalRuins7_Text_LooksLikeCanClimb_2, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03BF
    ActorCmdExec 255, Movement_0464
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 0
    VMSleep 40
    FadeOutBlack
    FadeWait
    RTReserveScript 13
    MapChangeCore ZONE_ABYSSAL_RUINS_6, 15, 3, 20, 2
    VMJump L_0455

L_03BF:
    WorkSetConst 0x8025, 0
    PlayerGetDir 0x8025
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_03DC
    VMJump L_03EC

L_03DC:
    ActorCmdExec 255, Movement_02FC
    ActorCmdWait
    VMJump L_0455

L_03EC:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_03FF
    VMJump L_040F

L_03FF:
    ActorCmdExec 255, Movement_0304
    ActorCmdWait
    VMJump L_0455

L_040F:
    WorkCmpConst 0x8025, 2
    VMJumpIf CMP_EQ, L_0422
    VMJump L_0432

L_0422:
    ActorCmdExec 255, Movement_030C
    ActorCmdWait
    VMJump L_0455

L_0432:
    WorkCmpConst 0x8025, 3
    VMJumpIf CMP_EQ, L_0445
    VMJump L_0455

L_0445:
    ActorCmdExec 255, Movement_0314
    ActorCmdWait
    VMJump L_0455

L_0455:
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0464:
    Move 14, 1
    MoveEnd

Script_9:
    ActorsPauseAll
    // "It looks like you can climb down here![f000]븁\u0000\nWill you return to the lower floor?"
    SystemMsg AbyssalRuins7_Text_LooksLikeCanClimb_2, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04BF
    ActorCmdExec 255, Movement_0464
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 0
    VMSleep 40
    FadeOutBlack
    FadeWait
    RTReserveScript 13
    MapChangeCore ZONE_ABYSSAL_RUINS_6, 15, 3, 32, 2
    VMJump L_0555

L_04BF:
    WorkSetConst 0x8026, 0
    PlayerGetDir 0x8026
    WorkCmpConst 0x8026, 0
    VMJumpIf CMP_EQ, L_04DC
    VMJump L_04EC

L_04DC:
    ActorCmdExec 255, Movement_02FC
    ActorCmdWait
    VMJump L_0555

L_04EC:
    WorkCmpConst 0x8026, 1
    VMJumpIf CMP_EQ, L_04FF
    VMJump L_050F

L_04FF:
    ActorCmdExec 255, Movement_0304
    ActorCmdWait
    VMJump L_0555

L_050F:
    WorkCmpConst 0x8026, 2
    VMJumpIf CMP_EQ, L_0522
    VMJump L_0532

L_0522:
    ActorCmdExec 255, Movement_030C
    ActorCmdWait
    VMJump L_0555

L_0532:
    WorkCmpConst 0x8026, 3
    VMJumpIf CMP_EQ, L_0545
    VMJump L_0555

L_0545:
    ActorCmdExec 255, Movement_0314
    ActorCmdWait
    VMJump L_0555

L_0555:
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
