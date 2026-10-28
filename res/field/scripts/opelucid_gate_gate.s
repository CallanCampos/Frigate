#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    GimmickGateCmd_ViewBoard 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
