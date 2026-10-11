#include "asm/field_script.inc"
#include "text/script/strange_house_5.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    FlagSet EVENT_FLAG_0x0383
    ActorDelete 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 453
    WorkSet 0x8001, 1
    RTCallGlobal 2814
    VMStackPop 0x8001
    VMStackPop 0x8000
    PlayerGetGPos 0x8020, 0x8021
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_005D
    VMJump L_0069

L_005D:
    WorkAdd 0x8021, 2
    VMJump L_00C6

L_0069:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_007C
    VMJump L_0088

L_007C:
    WorkSub 0x8021, 2
    VMJump L_00C6

L_0088:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_009B
    VMJump L_00A7

L_009B:
    WorkSub 0x8020, 2
    VMJump L_00C6

L_00A7:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_00BA
    VMJump L_00C6

L_00BA:
    WorkAdd 0x8020, 2
    VMJump L_00C6

L_00C6:
    ActorNew 0x8020, 0x8021, 0x8010, 251, 15, 0
    ActorCmdExec 251, Movement_01CC
    ActorCmdWait
    VMSleep 16
    ActorCmdExec 255, Movement_01A4
    ActorCmdWait
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_00FF
    VMJump L_010D

L_00FF:
    ActorCmdExec 255, Movement_01B4
    VMJump L_0170

L_010D:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0120
    VMJump L_012E

L_0120:
    ActorCmdExec 255, Movement_01AC
    VMJump L_0170

L_012E:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0141
    VMJump L_014F

L_0141:
    ActorCmdExec 255, Movement_01BC
    VMJump L_0170

L_014F:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0162
    VMJump L_0170

L_0162:
    ActorCmdExec 255, Movement_01C4
    VMJump L_0170

L_0170:
    ActorCmdWait
    // "Oh... The Lunar Wing...\nI can't take it now...[f000]븀\u0000\nBut it'll be OK...[f000]븁\u0000\nPlease return the wing\nto the Pokémon...[f000]븁\u0000\nI was waiting on the bridge\nso I could return it myself...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StrangeHouse5_Text_OhLunarWingCant, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0220
    ActorCmdWait
    ActorDelete 251
    WorkSetConst EVENT_WORK_0x4073, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 154, 1
    MoveEnd

Movement_01A4:
    Move 75, 1
    MoveEnd

Movement_01AC:
    Move 32, 1
    MoveEnd

Movement_01B4:
    Move 33, 1
    MoveEnd

Movement_01BC:
    Move 34, 1
    MoveEnd

Movement_01C4:
    Move 35, 1
    MoveEnd

Movement_01CC:
    Move 73, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 2
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 3
    Move 70, 1
    Move 61, 3
    Move 69, 1
    Move 61, 4
    Move 70, 1
    MoveEnd

Movement_0220:
    Move 69, 1
    Move 61, 4
    Move 70, 1
    Move 61, 3
    Move 69, 1
    Move 61, 3
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 2
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    MoveEnd
