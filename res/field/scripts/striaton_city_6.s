#include "asm/field_script.inc"
#include "text/script/striaton_city_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What sort of nicknames\ndo you give your Pokémon?[f000]븁\u0000\nWhat I like to do is give all my Pokémon\nnames that end with “-ington\"!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity6_Text_WhatSortNicknamesGive, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So I plan on going abroad again\nand doing some Pokémon battling.[f000]븁\u0000\nIn other regions, you discover a lot\nabout things you took for granted."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity6_Text_PlanGoingAbroadAgain, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Traveling Trainer, does your family\ncall you often on the Xtransceiver,[f000]븀\u0000\nor do they wait for you to call?"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity6_Text_TravelingTrainerDoesFamily, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
