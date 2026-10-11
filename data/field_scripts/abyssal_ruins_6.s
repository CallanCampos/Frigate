#include "asm/field_script.inc"
#include "text/script/abyssal_ruins_6.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_16:
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0067
    WorkSetConst EVENT_WORK_0x418f, 3

L_0067:
    VMHalt

Script_14:
    VMStackPushFlag EVENT_FLAG_0x00d7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0094
    ActorSetGPos 0, 13, 3, 24, 1
    ActorSetGPos 1, 11, 3, 23, 1

L_0094:
    VMHalt
    .balign 4, 0
    Move 20, 3
    MoveEnd

Script_15:
    VMStackPushFlag EVENT_FLAG_0x00d7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CB
    ActorSetGPos 0, 13, 3, 24, 1
    ActorSetGPos 1, 11, 3, 23, 1

L_00CB:
    VMHalt

L_00CD:
    SEPlay SEQ_SE_MESSAGE
    Cmd_0230 0x8020, 0
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00F2
    MsgPlaceSignClose
    MsgPlaceSign 0x8021, 0

L_00F2:
    MsgPlaceSignClose
    VMReturn

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 10
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 11
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    WorkSetConst 0x8021, 12
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    WorkSetConst 0x8021, 13
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 4
    WorkSetConst 0x8021, 14
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 5
    WorkSetConst 0x8021, 15
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 6
    WorkSetConst 0x8021, 16
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8020, 7
    WorkSetConst 0x8021, 17
    VMCall L_00CD
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    // "It looks like you can climb up here![f000]븁\u0000\nWill you proceed to the upper floor?"
    SystemMsg AbyssalRuins6_Text_LooksLikeCanClimb, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0265
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetGPos 0x8022, 0x8023
    WorkCmpConst 0x8023, 20
    VMJumpIf CMP_EQ, L_020C
    VMJump L_021A

L_020C:
    ActorCmdExec 255, Movement_0314
    VMJump L_023B

L_021A:
    WorkCmpConst 0x8023, 21
    VMJumpIf CMP_EQ, L_022D
    VMJump L_023B

L_022D:
    ActorCmdExec 255, Movement_031C
    VMJump L_023B

L_023B:
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 1
    VMSleep 40
    FadeOutBlack
    FadeWait
    RTReserveScript 7
    MapChangeCore ZONE_ABYSSAL_RUINS_7, 12, 3, 20, 3
    VMJump L_02FB

L_0265:
    WorkSetConst 0x8024, 0
    PlayerGetDir 0x8024
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_0282
    VMJump L_0292

L_0282:
    ActorCmdExec 255, Movement_047C
    ActorCmdWait
    VMJump L_02FB

L_0292:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_02A5
    VMJump L_02B5

L_02A5:
    ActorCmdExec 255, Movement_0484
    ActorCmdWait
    VMJump L_02FB

L_02B5:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_02C8
    VMJump L_02D8

L_02C8:
    ActorCmdExec 255, Movement_048C
    ActorCmdWait
    VMJump L_02FB

L_02D8:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_02EB
    VMJump L_02FB

L_02EB:
    ActorCmdExec 255, Movement_0494
    ActorCmdWait
    VMJump L_02FB

L_02FB:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0314:
    Move 15, 1
    MoveEnd

Movement_031C:
    Move 15, 1
    Move 12, 1
    Move 3, 1
    MoveEnd

Script_10:
    ActorsPauseAll
    // "It looks like you can climb up here![f000]븁\u0000\nWill you proceed to the upper floor?"
    SystemMsg AbyssalRuins6_Text_LooksLikeCanClimb, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CB
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    PlayerGetGPos 0x8025, 0x8026
    WorkCmpConst 0x8026, 32
    VMJumpIf CMP_EQ, L_0372
    VMJump L_0380

L_0372:
    ActorCmdExec 255, Movement_049C
    VMJump L_03A1

L_0380:
    WorkCmpConst 0x8026, 33
    VMJumpIf CMP_EQ, L_0393
    VMJump L_03A1

L_0393:
    ActorCmdExec 255, Movement_04A4
    VMJump L_03A1

L_03A1:
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 1
    VMSleep 40
    FadeOutBlack
    FadeWait
    RTReserveScript 7
    MapChangeCore ZONE_ABYSSAL_RUINS_7, 12, 3, 32, 3
    VMJump L_0461

L_03CB:
    WorkSetConst 0x8027, 0
    PlayerGetDir 0x8027
    WorkCmpConst 0x8027, 0
    VMJumpIf CMP_EQ, L_03E8
    VMJump L_03F8

L_03E8:
    ActorCmdExec 255, Movement_047C
    ActorCmdWait
    VMJump L_0461

L_03F8:
    WorkCmpConst 0x8027, 1
    VMJumpIf CMP_EQ, L_040B
    VMJump L_041B

L_040B:
    ActorCmdExec 255, Movement_0484
    ActorCmdWait
    VMJump L_0461

L_041B:
    WorkCmpConst 0x8027, 2
    VMJumpIf CMP_EQ, L_042E
    VMJump L_043E

L_042E:
    ActorCmdExec 255, Movement_048C
    ActorCmdWait
    VMJump L_0461

L_043E:
    WorkCmpConst 0x8027, 3
    VMJumpIf CMP_EQ, L_0451
    VMJump L_0461

L_0451:
    ActorCmdExec 255, Movement_0494
    ActorCmdWait
    VMJump L_0461

L_0461:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_047C:
    Move 13, 1
    MoveEnd

Movement_0484:
    Move 12, 1
    MoveEnd

Movement_048C:
    Move 15, 1
    MoveEnd

Movement_0494:
    Move 14, 1
    MoveEnd

Movement_049C:
    Move 15, 1
    MoveEnd

Movement_04A4:
    Move 15, 1
    Move 12, 1
    Move 3, 1
    MoveEnd

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorCmdExec 255, Movement_04D0
    FadeInBlack
    FadeWait
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_04D0:
    Move 9, 1
    Move 13, 1
    MoveEnd

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorCmdExec 255, Movement_04F8
    FadeInBlack
    FadeWait
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_04F8:
    Move 10, 1
    Move 14, 1
    MoveEnd

Script_12:
    ActorsPauseAll
    // "It looks like you can climb down here![f000]븁\u0000\nWill you return to the lower floor?"
    SystemMsg AbyssalRuins6_Text_LooksLikeCanClimb_2, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05E5
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    PlayerGetGPos 0x8028, 0x8029
    WorkCmpConst 0x8028, 29
    VMJumpIf CMP_EQ, L_054A
    VMJump L_0558

L_054A:
    ActorCmdExec 255, Movement_0694
    VMJump L_05BB

L_0558:
    WorkCmpConst 0x8028, 30
    VMJumpIf CMP_EQ, L_056B
    VMJump L_0579

L_056B:
    ActorCmdExec 255, Movement_06A4
    VMJump L_05BB

L_0579:
    WorkCmpConst 0x8028, 31
    VMJumpIf CMP_EQ, L_058C
    VMJump L_05BB

L_058C:
    VMStackPush 0x8029
    VMStackPushConst 32
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05AD
    ActorCmdExec 255, Movement_06B0
    VMJump L_05B5

L_05AD:
    ActorCmdExec 255, Movement_06BC

L_05B5:
    VMJump L_05BB

L_05BB:
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 0
    VMSleep 40
    FadeOutBlack
    FadeWait
    RTReserveScript 21
    MapChangeCore ZONE_ABYSSAL_RUINS_5, 81, 3, 75, 1
    VMJump L_067B

L_05E5:
    WorkSetConst 0x802a, 0
    PlayerGetDir 0x802a
    WorkCmpConst 0x802a, 0
    VMJumpIf CMP_EQ, L_0602
    VMJump L_0612

L_0602:
    ActorCmdExec 255, Movement_047C
    ActorCmdWait
    VMJump L_067B

L_0612:
    WorkCmpConst 0x802a, 1
    VMJumpIf CMP_EQ, L_0625
    VMJump L_0635

L_0625:
    ActorCmdExec 255, Movement_0484
    ActorCmdWait
    VMJump L_067B

L_0635:
    WorkCmpConst 0x802a, 2
    VMJumpIf CMP_EQ, L_0648
    VMJump L_0658

L_0648:
    ActorCmdExec 255, Movement_048C
    ActorCmdWait
    VMJump L_067B

L_0658:
    WorkCmpConst 0x802a, 3
    VMJumpIf CMP_EQ, L_066B
    VMJump L_067B

L_066B:
    ActorCmdExec 255, Movement_0494
    ActorCmdWait
    VMJump L_067B

L_067B:
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0694:
    Move 12, 1
    Move 15, 1
    Move 1, 1
    MoveEnd

Movement_06A4:
    Move 12, 1
    Move 1, 1
    MoveEnd

Movement_06B0:
    Move 14, 1
    Move 1, 1
    MoveEnd

Movement_06BC:
    Move 12, 1
    Move 14, 1
    Move 1, 1
    MoveEnd
