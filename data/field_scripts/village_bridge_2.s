#include "asm/field_script.inc"
#include "text/script/village_bridge_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My great-great-great-grandfather and\nhis Gurdurr worked together and built[f000]븀\u0000\nthis Village Bridge."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge2_Text_GreatGreatGreatGrandfather, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'll be a stonecutter like my grandpa!\nI can get help from powerful Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge2_Text_IllStonecutterLikeGrandpa, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If Team Plasma had tricked everyone\ninto releasing all their Pokémon,[f000]븀\u0000\nit would have been impossible[f000]븀\u0000\nto build a bridge like this.[f000]븁\u0000\nPeople and Pokémon are inseparable...[f000]븁\u0000\nThe relationship is not more or less\nthan that. That's what I believe."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge2_Text_IfTeamPlasmaHad, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
