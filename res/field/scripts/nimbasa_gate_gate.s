#include "asm/field_script.inc"
#include "text/script/nimbasa_gate_gate.h"

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
    // "There are performers gathering on\nRoute 5!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaGateGate_Text_TherePerformersGatheringRoute, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if the news on the bulletin\nboard is just someone's mutterings..."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaGateGate_Text_WonderIfNewsBulletin, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
