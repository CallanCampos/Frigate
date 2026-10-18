#include "asm/field_script.inc"
#include "text/script/route_gate.h"

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
    // "The Relic Castle in the Desert Resort.[f000]븁\u0000\nYou can tell just by looking at it\nthat time has been cruel."
    ParentActorMsg MSGFILE_SCRIPT, RouteGate_Text_RelicCastleDesertResort, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm a School Kid, and I'll tell you this.[f000]븁\u0000\nJust like in caves, you can run into\nPokémon almost anywhere in the desert.[f000]븁\u0000\nBut they don't like hiding in the pale\nsand. So if you walk there, you won't[f000]븀\u0000\nhave to worry about battling."
    ParentActorMsg MSGFILE_SCRIPT, RouteGate_Text_ImSchoolKidIll, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
