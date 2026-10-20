#include "asm/field_script.inc"
#include "text/script/plasma_frigate_19.h"

// Script plugin 12, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Saying yes only so you won't\nhurt another person's feelings...[f000]븀\u0000\nThat is not true kindness.[f000]븁\u0000\nTeam Plasma didn't have true kindness.\nNot even me..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate19_Text_SayingYesOnlyWont, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
