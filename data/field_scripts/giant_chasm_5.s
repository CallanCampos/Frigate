#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

L_0026:
    VMStackPushFlag EVENT_FLAG_0x016c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0049
    ObjInitWarpGPos 0, 25, 0, 25
    VMJump L_0053

L_0049:
    ObjInitWarpGPos 1, 25, 0, 25

L_0053:
    VMReturn

Script_1:
    VMCall L_0026
    VMHalt

Script_3:
    VMCall L_0026
    VMHalt

Script_2:
    VMHalt
    .balign 4, 0
