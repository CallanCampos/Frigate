#include "asm/field_script.inc"
#include "text/script/castelia_city_23.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon and I like to laugh![f000]븁\u0000\nWhen either of us starts laughing,\nwe both start laughing! Gwa ha ha!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity23_Text_PokemonLikeLaughWhen, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 572, 0
    // "Gahoohoo!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity23_Text_Gahoohoo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What do you like on TV?\nI like the lady and Watchy Watchog!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity23_Text_WhatLikeTvLike, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Once when I tried to catch a Pokémon,\nthe Poké Ball only shook once![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity23_Text_OnceWhenTriedCatch, 3, 5, 0
    MsgWinCloseAll
    // "Oh! That does happen sometimes![f000]븁\u0000\nI've just been calling it a\ncritical capture![f000]븁\u0000\nThe more Pokémon you catch,\nthe more likely that phenomenon is![f000]븁\u0000\nApparently, when the Poké Ball\nonly rocks once, it's easier[f000]븀\u0000\nto catch a Pokémon."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity23_Text_OhDoesHappenSometimes, 4, 3, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
