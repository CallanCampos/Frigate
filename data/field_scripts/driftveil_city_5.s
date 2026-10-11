#include "asm/field_script.inc"
#include "text/script/driftveil_city_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the\nDriftveil Continental Hotel![f000]븁\u0000\nThere are no open rooms,\nbut feel free to relax in the lobby."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity5_Text_WelcomeDriftveilContinentalHotel, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My guilty pleasure is the newest films\nfrom Pokéstar Studios."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity5_Text_GuiltyPleasureNewestFilms, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Driftveil's gone and changed so much,\nI thought I'd been hornswoggled!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity5_Text_DriftveilsGoneChangedMuch, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm going to watch... ♪\nThe Pokémon World Tournament! ♪[f000]븁\u0000\nHuh? The Pokémon World Tournament\nis in the southernmost part of town."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity5_Text_ImGoingWatchPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A terrific Trainer!\nLet me share a training tip with you.[f000]븁\u0000\nDuring a battle with a Trainer, even if\nyour Pokémon's items are taken away,[f000]븀\u0000\ndon't worry.[f000]븁\u0000\nThey will be returned to you safely\nafter the battle.[f000]븁\u0000\n...But disposable items seem\nto disappear."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity5_Text_TerrificTrainerLetShare, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
