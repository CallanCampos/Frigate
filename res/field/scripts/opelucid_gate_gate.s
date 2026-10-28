#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    GimmickGateViewBoard 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
