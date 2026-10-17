#include "asm/field_script.inc"
#include "text/script/route_4_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We moved here because I heard\nthis place would be developed more..."
    ParentActorMsg MSGFILE_SCRIPT, Route44_Text_WeMovedHereBecause, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Since construction won't be finished,\nguys like them hang around!"
    ParentActorMsg MSGFILE_SCRIPT, Route44_Text_SinceConstructionWontFinished, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I love this place![f000]븁\u0000\nIt's very convenient, because it's\nclose to both Castelia and Nimbasa!"
    ParentActorMsg MSGFILE_SCRIPT, Route44_Text_LovePlaceItsVery, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
