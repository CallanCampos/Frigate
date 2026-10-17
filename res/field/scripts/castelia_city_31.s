#include "asm/field_script.inc"
#include "text/script/castelia_city_31.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Apparently, this is where\nit all began for Castelia City."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity31_Text_ApparentlyWhereAllBegan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
