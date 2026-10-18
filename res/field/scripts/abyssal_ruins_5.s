#include "asm/field_script.inc"
#include "text/script/abyssal_ruins_5.h"

// Script plugin 5, from the zones that use this file

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
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_24:
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0087
    WorkSetConst EVENT_WORK_0x418f, 2

L_0087:
    VMHalt

Script_22:
    VMStackPush EVENT_WORK_0x4094
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B4
    ActorSetGPos 0, 77, 3, 77, 1
    ActorSetGPos 1, 77, 3, 77, 1

L_00B4:
    VMHalt

Script_23:
    VMStackPush EVENT_WORK_0x4094
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E1
    ActorSetGPos 0, 77, 3, 77, 1
    ActorSetGPos 1, 77, 3, 77, 1

L_00E1:
    VMHalt

L_00E3:
    SEPlay SEQ_SE_MESSAGE
    Cmd_0230 0x8020, 0
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0108
    MsgPlaceSignClose
    MsgPlaceSign 0x8021, 0

L_0108:
    MsgPlaceSignClose
    VMReturn

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 22
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 23
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    WorkSetConst 0x8021, 24
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    WorkSetConst 0x8021, 25
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 4
    WorkSetConst 0x8021, 26
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 5
    WorkSetConst 0x8021, 27
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 6
    WorkSetConst 0x8021, 28
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8020, 7
    WorkSetConst 0x8021, 29
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x8020, 8
    WorkSetConst 0x8021, 30
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WorkSetConst 0x8020, 9
    WorkSetConst 0x8021, 31
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8020, 10
    WorkSetConst 0x8021, 32
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8020, 11
    WorkSetConst 0x8021, 33
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst 0x8020, 12
    WorkSetConst 0x8021, 34
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8020, 13
    WorkSetConst 0x8021, 35
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WorkSetConst 0x8020, 14
    WorkSetConst 0x8021, 36
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8020, 15
    WorkSetConst 0x8021, 37
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    WorkSetConst 0x8020, 16
    WorkSetConst 0x8021, 38
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    WorkSetConst 0x8020, 17
    WorkSetConst 0x8021, 39
    VMCall L_00E3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    AbyssalRuinsCmd_GetStepCounter 0x8022
    VMStackPush EVENT_WORK_0x4094
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030B
    VMCall L_0338
    VMJump L_0332

L_030B:
    SEPlay SEQ_SE_MESSAGE
    // "HOJLFWBSCOPPH[f000]븁\u0000"
    Cmd_0230 AbyssalRuins5_Text_Hojlfwbscopph, 0
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0330
    MsgPlaceSignClose
    // "GNIKEVARBNOOG"
    MsgPlaceSign AbyssalRuins5_Text_Gnikevarbnoog, 0

L_0330:
    MsgPlaceSignClose

L_0332:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0338:
    VMStackPush 0x8022
    VMStackPushConst 190
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_03B2
    SEPlay SEQ_SE_MESSAGE
    // "HOJLFWBSCOPPH[f000]븁\u0000"
    Cmd_0230 AbyssalRuins5_Text_Hojlfwbscopph, 0
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0370
    MsgPlaceSignClose
    // "GNIKEVARBNOOG"
    MsgPlaceSign AbyssalRuins5_Text_Gnikevarbnoog, 0

L_0370:
    MsgPlaceSignClose
    EvCameraShake 0, 1, 3, 6, 1, 0, 1, 5
    SEPlay SEQ_SE_FLD_126
    ActorCmdExec 0, Movement_03E0
    ActorCmdExec 1, Movement_03E0
    ActorCmdWait
    SEWait
    // "The wall moved, and you can proceed now!"
    SystemMsg AbyssalRuins5_Text_WallMovedCanProceed, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst EVENT_WORK_0x4094, 1
    VMJump L_03DB

L_03B2:
    SEPlay SEQ_SE_MESSAGE
    // "UTPMTUFHHOJLPO"
    Cmd_0230 AbyssalRuins5_Text_Utpmtufhhojlpo, 0
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_03D7
    MsgPlaceSignClose
    // "TSOLSTEGGNIKON"
    MsgPlaceSign AbyssalRuins5_Text_Tsolsteggnikon, 0

L_03D7:
    LastKeyWait
    MsgPlaceSignClose

L_03DB:
    VMReturn
    .balign 4, 0

Movement_03E0:
    Move 6, 1
    Move 10, 2
    MoveEnd

Script_20:
    ActorsPauseAll
    // "It looks like you can climb up here![f000]븁\u0000\nWill you proceed to the upper floor?"
    SystemMsg AbyssalRuins5_Text_LooksLikeCanClimb, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04AC
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PlayerGetGPos 0x8023, 0x8024
    WorkCmpConst 0x8023, 80
    VMJumpIf CMP_EQ, L_0432
    VMJump L_0440

L_0432:
    ActorCmdExec 255, Movement_0568
    VMJump L_0482

L_0440:
    WorkCmpConst 0x8023, 81
    VMJumpIf CMP_EQ, L_0453
    VMJump L_0461

L_0453:
    ActorCmdExec 255, Movement_0578
    VMJump L_0482

L_0461:
    WorkCmpConst 0x8023, 82
    VMJumpIf CMP_EQ, L_0474
    VMJump L_0482

L_0474:
    ActorCmdExec 255, Movement_0580
    VMJump L_0482

L_0482:
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 1
    VMSleep 20
    FadeOutBlack
    FadeWait
    RTReserveScript 11
    MapChangeCore ZONE_ABYSSAL_RUINS_6, 30, 3, 32, 1
    VMJump L_0542

L_04AC:
    WorkSetConst 0x8025, 0
    PlayerGetDir 0x8025
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_04C9
    VMJump L_04D9

L_04C9:
    ActorCmdExec 255, Movement_0548
    ActorCmdWait
    VMJump L_0542

L_04D9:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_04EC
    VMJump L_04FC

L_04EC:
    ActorCmdExec 255, Movement_0550
    ActorCmdWait
    VMJump L_0542

L_04FC:
    WorkCmpConst 0x8025, 2
    VMJumpIf CMP_EQ, L_050F
    VMJump L_051F

L_050F:
    ActorCmdExec 255, Movement_0558
    ActorCmdWait
    VMJump L_0542

L_051F:
    WorkCmpConst 0x8025, 3
    VMJumpIf CMP_EQ, L_0532
    VMJump L_0542

L_0532:
    ActorCmdExec 255, Movement_0560
    ActorCmdWait
    VMJump L_0542

L_0542:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0548:
    Move 13, 1
    MoveEnd

Movement_0550:
    Move 12, 1
    MoveEnd

Movement_0558:
    Move 15, 1
    MoveEnd

Movement_0560:
    Move 14, 1
    MoveEnd

Movement_0568:
    Move 12, 1
    Move 15, 1
    Move 0, 1
    MoveEnd

Movement_0578:
    Move 12, 1
    MoveEnd

Movement_0580:
    Move 12, 1
    Move 14, 1
    Move 0, 1
    MoveEnd

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_KAIDAN
    SEWait
    FadeInBlackQ
    FadeWait
    ActorCmdExec 255, Movement_05AC
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_05AC:
    Move 9, 1
    Move 13, 1
    MoveEnd
