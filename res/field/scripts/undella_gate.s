#include "asm/field_script.inc"
#include "text/script/undella_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The longest road in Unova...\nI heard it's Route 13."
    ParentActorMsg MSGFILE_SCRIPT, UndellaGate_Text_LongestRoadUnovaHeard, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I love to clear my path by\nhaving Pokémon use Hidden Moves!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaGate_Text_LoveClearPathBy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
