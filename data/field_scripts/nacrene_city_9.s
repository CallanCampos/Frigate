#include "asm/field_script.inc"
#include "text/script/nacrene_city_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Burgh used this warehouse up until\nfour years ago.[f000]븁\u0000\nWhen Burgh gets artist's block,\nhe comes back here to Nacrene City!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity9_Text_BurghUsedWarehouseUp, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Burgh is an artist.\nHe's also the Gym Leader in Castelia City.[f000]븁\u0000\nWe want to be like him![f000]븁\u0000\nI'll never stop admiring him![f000]븁\u0000\nYup, I'm only going to keep on admiring!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity9_Text_BurghArtistHesAlso, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
