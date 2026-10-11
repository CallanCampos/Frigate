#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
