#include "asm/field_script.inc"
#include "text/script/ns_castle_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_1:
    VMHalt

Script_2:
    VMStackPushFlag EVENT_FLAG_0x03ad
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    ActorSetGPos 0, 31, 0, 18, 2

L_0051:
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0074
    VMCall L_01BC
    VMJump L_007A

L_0074:
    VMCall L_0080

L_007A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0080:
    PlayerGetGPos 0x8021, 0x8022
    ActorGetGPos 0, 0x8023, 0x8024
    VMStackPush 0x8022
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00C1
    ActorCmdExec 0, Movement_0268
    ActorCmdWait
    VMJump L_0187

L_00C1:
    VMStackPush 0x8022
    VMStackPushConst 18
    VMStackCmp CMP_LE
    VMStackPush 0x8024
    VMStackPushConst 19
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00F4
    ActorCmdExec 0, Movement_0268
    ActorCmdWait
    VMJump L_0187

L_00F4:
    VMStackPush 0x8022
    VMStackPushConst 19
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0127
    ActorCmdExec 0, Movement_0270
    ActorCmdWait
    VMJump L_0187

L_0127:
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMStackPush 0x8022
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_015A
    ActorCmdExec 0, Movement_0268
    ActorCmdWait
    VMJump L_0187

L_015A:
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMStackPush 0x8022
    VMStackPushConst 19
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0187
    ActorCmdExec 0, Movement_0270
    ActorCmdWait

L_0187:
    // "[f000]븉\u0001\u0001I want you to go inside.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle4_Text_WantGoInside, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01B0
    ActorCmdExec 0, Movement_0278

L_01B0:
    ActorCmdExec 255, Movement_0240
    ActorCmdWait
    VMReturn

L_01BC:
    FlagReset EVENT_FLAG_0x03ad
    WorkSetConst EVENT_WORK_0x4113, 4
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 4
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8021, 1
    ActorAdd 0
    ActorSetGPos 0, 0x8021, 0, 0x8022, 2
    ActorDelete 254
    // "[f000]븉\u0001\u0001This is our destination...\nGo inside.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle4_Text_OurDestinationGoInside, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0240
    ActorCmdWait
    VMReturn

Script_4:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_0240:
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

Movement_0268:
    Move 32, 1
    MoveEnd

Movement_0270:
    Move 33, 1
    MoveEnd

Movement_0278:
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
