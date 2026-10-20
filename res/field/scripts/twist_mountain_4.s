#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    FlagSet EVENT_FLAG_0x02b2
    WorkSetConst 0x8020, 0
    RTCGetSeason 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_002B
    FlagReset EVENT_FLAG_0x02b2

L_002B:
    WorkSetConst 0x8020, 0
    VMHalt
    .balign 4, 0
