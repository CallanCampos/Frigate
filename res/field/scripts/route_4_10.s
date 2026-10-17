#include "asm/field_script.inc"
#include "text/script/route_4_10.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "People say they are brats,\nbut they are just hanging out.[f000]븀\u0000\nNever judge a book by its cover."
    ParentActorMsg MSGFILE_SCRIPT, Route410_Text_PeopleSayTheyBrats, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
