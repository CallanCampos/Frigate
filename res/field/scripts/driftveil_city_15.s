#include "asm/field_script.inc"
#include "text/script/driftveil_city_15.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh? By any chance, was your mother\nworking in a Pokémon Center[f000]븀\u0000\nas a receptionist?[f000]븁\u0000\nYou look very similar to her."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity15_Text_OhByAnyChance, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 559, 0
    // "Garcs!\nGarcs!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity15_Text_GarcsGarcs, 0, 0
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
    // "When things change, I prefer the way\nit was, and when things don't change,[f000]븀\u0000\nI get bored...[f000]븀\u0000\nI have a twisted mind."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity15_Text_WhenThingsChangePrefer, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Pokémon next door...\nI feel like it's intimidating me..."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity15_Text_PokemonNextDoorFeel, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
