#include "asm/field_script.inc"
#include "text/script/anville_town_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Why do people love trains?[f000]븁\u0000\nMaybe it's the design--the way trains can\nrun so fast and still be so comfortable.[f000]븁\u0000\nAll the essential parts work together...\nSimple, but so beautiful!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown2_Text_WhyPeopleLoveTrains, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Isn't it amazing how,\njust by riding a train,[f000]븀\u0000\nyou can see such a variety of scenery?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown2_Text_IsntAmazingHowJust, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 596, 0
    // "Tulaaa..."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown2_Text_Tulaaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
