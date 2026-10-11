#include "asm/field_script.inc"
#include "text/script/abyssal_ruins_8.h"

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
    WorkSetConst 0x8021, 0

Script_8:
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 5
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_004B
    WorkSetConst EVENT_WORK_0x418f, 6

L_004B:
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorCmdExec 255, Movement_006C
    FadeInBlack
    FadeWait
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_006C:
    Move 8, 1
    Move 12, 1
    MoveEnd

L_0078:
    SEPlay SEQ_SE_MESSAGE
    Cmd_0230 0x8020, 0
    VMStackPush EVENT_WORK_0x418f
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_009D
    MsgPlaceSignClose
    MsgPlaceSign 0x8021, 0

L_009D:
    MsgPlaceSignClose
    VMReturn

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 6
    VMCall L_0078
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8021, 7
    VMCall L_0078
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    WorkSetConst 0x8021, 8
    VMCall L_0078
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    WorkSetConst 0x8021, 9
    VMCall L_0078
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 4
    WorkSetConst 0x8021, 10
    VMCall L_0078
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    // "It looks like you can climb down here![f000]븁\u0000\nWill you return to the lower floor?"
    SystemMsg AbyssalRuins8_Text_LooksLikeCanClimb, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E3
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetGPos 0x8022, 0x8023
    WorkCmpConst 0x8022, 12
    VMJumpIf CMP_EQ, L_0169
    VMJump L_0177

L_0169:
    ActorCmdExec 255, Movement_02B4
    VMJump L_01B9

L_0177:
    WorkCmpConst 0x8022, 13
    VMJumpIf CMP_EQ, L_018A
    VMJump L_0198

L_018A:
    ActorCmdExec 255, Movement_02C4
    VMJump L_01B9

L_0198:
    WorkCmpConst 0x8022, 14
    VMJumpIf CMP_EQ, L_01AB
    VMJump L_01B9

L_01AB:
    ActorCmdExec 255, Movement_02CC
    VMJump L_01B9

L_01B9:
    ActorCmdWait
    PlayerMoveToYAsync 0, 60, 64, 0
    VMSleep 40
    FadeOutBlack
    FadeWait
    RTReserveScript 10
    MapChangeCore ZONE_ABYSSAL_RUINS_7, 42, 3, 24, 1
    VMJump L_0279

L_01E3:
    WorkSetConst 0x8024, 0
    PlayerGetDir 0x8024
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_0200
    VMJump L_0210

L_0200:
    ActorCmdExec 255, Movement_0294
    ActorCmdWait
    VMJump L_0279

L_0210:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_0223
    VMJump L_0233

L_0223:
    ActorCmdExec 255, Movement_029C
    ActorCmdWait
    VMJump L_0279

L_0233:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_0246
    VMJump L_0256

L_0246:
    ActorCmdExec 255, Movement_02A4
    ActorCmdWait
    VMJump L_0279

L_0256:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_0269
    VMJump L_0279

L_0269:
    ActorCmdExec 255, Movement_02AC
    ActorCmdWait
    VMJump L_0279

L_0279:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0294:
    Move 13, 1
    MoveEnd

Movement_029C:
    Move 12, 1
    MoveEnd

Movement_02A4:
    Move 15, 1
    MoveEnd

Movement_02AC:
    Move 14, 1
    MoveEnd

Movement_02B4:
    Move 13, 1
    Move 15, 1
    Move 1, 1
    MoveEnd

Movement_02C4:
    Move 13, 1
    MoveEnd

Movement_02CC:
    Move 13, 1
    Move 14, 1
    Move 1, 1
    MoveEnd

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorDelete 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 590
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x05bd
    WorkSetConst EVENT_WORK_0x418f, 8
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
