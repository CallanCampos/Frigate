#include "asm/field_script.inc"
#include "text/script/route_4_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Even when I feel sad, looking at my\nPokémon's cute, round eyes[f000]븀\u0000\nmakes my sad feeling melt away."
    ParentActorMsg MSGFILE_SCRIPT, Route46_Text_EvenWhenFeelSad, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 506, 0
    // "Yip! Yip!"
    ParentActorMsg MSGFILE_SCRIPT, Route46_Text_YipYip, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
