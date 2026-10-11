#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkGet 0x8022, 0x8000
    VMStackPush 0x8022
    VMStackPushConst 15
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_004D
    WorkSetConst 0x8024, 4
    VMJump L_005F

L_004D:
    WorkGet 0x8024, 0x8022
    WorkSub 0x8024, 15
    WorkAdd 0x8024, 5

L_005F:
    SystemMsg 0x8024, 2
    Cmd_01AF 0x8022, 0x8023
    WorkSetConst 0x8024, 13
    WorkAdd 0x8024, 0x8023
    SystemMsg 0x8024, 2
    InfoMsgClose
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    RTEndGlobal
    VMHalt
    WorkSetConst 0x8021, 0

Script_2:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkGet 0x8025, 0x8000
    Cmd_01AF 0x8025, 0x8026
    WorkSetConst 0x8027, 13
    WorkAdd 0x8027, 0x8026
    SystemMsg 0x8027, 2
    InfoMsgClose
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    RTEndGlobal
    VMHalt
    .balign 4, 0
