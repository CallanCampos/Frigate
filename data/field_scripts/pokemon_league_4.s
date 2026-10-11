#include "asm/field_script.inc"

// Script plugin 14, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x03ef
    ActorAdd 1
    ActorAdd 0
    ActorAdd 3
    ActorAdd 2
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 64216, 0, 0xed000, 0x200000, 0x4892ef, 0x78000, 1
    EvCameraWait
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0061
    FadeEx 1, 16, 0, 2
    VMJump L_006B

L_0061:
    FadeEx 4, 16, 0, 2

L_006B:
    SEPlay SEQ_SE_SW_ENDING_01
    SEPlay SEQ_SE_SW_ENDING_02
    SEPlay SEQ_SE_SW_ENDING_03
    FadeExWait
    Plugin14_Cmd1009
    VMSleep 90
    EvCameraMoveTo 5080, 0, 0xdd000, 0x1f8000, 0x3192ef, 0x78000, 120
    Plugin14Cmd_FadeOutSeq 2406
    Plugin14Cmd_FadeOutSeq 2407
    Plugin14Cmd_FadeOutSeq 2408
    EvCameraWait
    ActorCmdExec 255, Movement_0294
    ActorCmdWait
    VMSleep 16
    ActorAdd 4
    ActorCmdExec 4, Movement_02AC
    VMSleep 16
    ActorCmdExec 255, Movement_02B4
    ActorCmdWait
    ActorCmdExec 4, Movement_02C4
    VMSleep 8
    ActorCmdExec 255, Movement_02CC
    ActorCmdWait
    VMSleep 30
    EvCameraMoveTo 5080, 0, 0xdd000, 0x1f8000, 0x278213, 0xf8000, 120
    ActorWalkRoute 255, 31, 14, 0, 16, 1
    VMSleep 4
    ActorWalkRoute 4, 31, 13, 0, 16, 1
    ActorCmdWait
    ActorCmdExec 4, Movement_02BC
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 0, Movement_02E4
    VMSleep 8
    ActorCmdExec 255, Movement_02C4
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 2, Movement_02EC
    VMSleep 8
    ActorCmdExec 255, Movement_02CC
    ActorCmdWait
    VMSleep 30
    ActorWalkRoute 255, 31, 16, 0, 16, 1
    VMSleep 8
    ActorCmdExec 0, Movement_02BC
    ActorCmdExec 2, Movement_02BC
    ActorCmdWait
    ActorCmdExec 1, Movement_02E4
    VMSleep 8
    ActorCmdExec 255, Movement_02C4
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 3, Movement_02EC
    VMSleep 8
    ActorCmdExec 255, Movement_02CC
    ActorCmdWait
    VMSleep 30
    EvCameraMoveTo 8536, 0, 0xed000, 0x1f8000, 0x260213, 0x11e000, 30
    ActorWalkRoute 255, 31, 18, 0, 16, 1
    VMSleep 8
    ActorCmdExec 1, Movement_02BC
    ActorCmdExec 3, Movement_02BC
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 255, Movement_02B4
    ActorCmdWait
    ActorWalkRoute 0, 29, 15, 0, 16, 0
    ActorWalkRoute 2, 33, 15, 0, 16, 0
    ActorWalkRoute 4, 31, 17, 0, 16, 1
    ActorCmdWait
    VMSleep 45
    EvCameraMoveTo 9688, 0, 0xed000, 0x1f8000, 0x1e8213, 0x188000, 30
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0270
    FadeEx 1, 0, 16, 4
    VMJump L_027A

L_0270:
    FadeEx 4, 0, 16, 4

L_027A:
    ActorCmdExec 255, Movement_0314
    ActorCmdWait
    FadeExWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0294:
    Move 168, 6
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 9, 9
    MoveEnd

Movement_02AC:
    Move 168, 7
    MoveEnd

Movement_02B4:
    Move 32, 1
    MoveEnd

Movement_02BC:
    Move 33, 1
    MoveEnd

Movement_02C4:
    Move 34, 1
    MoveEnd

Movement_02CC:
    Move 35, 1
    MoveEnd
    Move 9, 1
    MoveEnd
    Move 8, 1
    MoveEnd

Movement_02E4:
    Move 11, 1
    MoveEnd

Movement_02EC:
    Move 10, 1
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

Movement_0314:
    Move 164, 10
    MoveEnd
