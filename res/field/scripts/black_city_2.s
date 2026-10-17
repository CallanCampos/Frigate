#include "asm/field_script.inc"
#include "text/script/black_city_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Listen up!\nThere's nothing wrong with making money![f000]븁\u0000\nBut there are wrong ways to do it...\nYou have to make money the good way!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity2_Text_ListenUpTheresNothing, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "How serious are you willing to get\nin order to get what you want?"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity2_Text_HowSeriousWillingGet, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
