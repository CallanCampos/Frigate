#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

L_002A:
    VMStackPushFlag EVENT_FLAG_0x016c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004D
    ObjInitWarpGPos 1, 23, 0, 1
    VMJump L_0057

L_004D:
    ObjInitWarpGPos 0, 23, 0, 1

L_0057:
    VMReturn

Script_2:
    VMCall L_002A
    VMHalt

Script_3:
    VMHalt

Script_4:
    VMCall L_002A
    VMHalt

Script_1:
    ActorsPauseAll
    GameGetVersion 0x8020
    PlayerGetDir 0x8021
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0098
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_10, 15, 21, 32801
    VMJump L_00A2

L_0098:
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_13, 15, 21, 32801

L_00A2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
