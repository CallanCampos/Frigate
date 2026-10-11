#include "asm/field_script.inc"
#include "text/script/striaton_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bill, Lanette, Bebe, and Amanita.[f000]븁\u0000\nThey're all admins of the\nPokémon Storage System."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity4_Text_BillLanetteBebeAmanita, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't really know what it means,\nbut the current C-Gear is version 3.0."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity4_Text_DontReallyKnowWhat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Gwoorf!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity4_Text_Gwoorf, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
