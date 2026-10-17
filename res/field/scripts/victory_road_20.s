#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

L_0014:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0053
    ObjInitWarpGPos 6, 65288, 80, 248
    ObjInitWarpGPos 7, 65288, 80, 232
    ObjInitWarpGPos 8, 65288, 80, 216
    FlagSet EVENT_FLAG_0x036b
    VMJump L_0075

L_0053:
    ObjInitWarpGPos 3, 65288, 80, 248
    ObjInitWarpGPos 4, 65288, 80, 232
    ObjInitWarpGPos 5, 65288, 80, 216
    FlagSet EVENT_FLAG_0x036a

L_0075:
    VMReturn

Script_1:
    VMCall L_0014
    VMHalt

Script_3:
    VMCall L_0014
    VMHalt

Script_2:
    VMHalt
    .balign 4, 0
