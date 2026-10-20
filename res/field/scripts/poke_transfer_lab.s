#include "asm/field_script.inc"

// Script plugin 4, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_2:
    FlagGet EVENT_FLAG_0x007b, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003F
    ActorSetGPos 0, 10, 6, 4, 0
    ActorSetGPos 1, 10, 0, 20, 1

L_003F:
    VMHalt

Script_3:
    ActorsPauseAll
    RTCallGlobal 10295
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    VMHalt
    .balign 4, 0
