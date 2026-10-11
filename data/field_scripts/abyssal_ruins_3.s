#include "asm/field_script.inc"

// Script plugin 5, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    AbyssalRuinsCmd_StartUnderwaterEffect
    VMStackPushFlag EVENT_FLAG_TOWN_MAP_ABYSSAL_RUINS
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0027
    FlagSet EVENT_FLAG_TOWN_MAP_ABYSSAL_RUINS

L_0027:
    VMHalt

Script_2:
    AbyssalRuinsCmd_StartUnderwaterEffect
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0050
    ActorCmdWait
    FieldSetNextZone 240, 1, 816, 5, 290
    CallDiving 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0050:
    Move 13, 4
    MoveEnd
