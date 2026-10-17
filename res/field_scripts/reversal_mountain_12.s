#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_001F
    HollowRivalCmd_0262 3, 4

L_001F:
    VMHalt
    .balign 4, 0
