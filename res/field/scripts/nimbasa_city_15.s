#include "asm/field_script.inc"
#include "text/script/nimbasa_city_15.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Shiny Krokorok...\nWow. Those colors blew my mind!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity15_Text_ShinyKrokorokWowThose, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hear the Pokémon that\nlook like they're gleaming when[f000]븀\u0000\nthey come out of the grass[f000]븀\u0000\nare called Shiny Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity15_Text_HearPokemonLookLike, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Nuha nuha nuhaha!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity15_Text_NuhaNuhaNuhaha, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
