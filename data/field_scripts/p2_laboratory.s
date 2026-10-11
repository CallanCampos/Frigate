#include "asm/field_script.inc"
#include "text/script/p2_laboratory.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh... It's just you...[f000]븁\u0000\nNeither Ghetsis nor Colress\nchanged me...[f000]븁\u0000\nI guess I'm the only one\nwho can change myself..."
    ParentActorMsg MSGFILE_SCRIPT, P2Laboratory_Text_OhItsJustNeither, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
