#include "asm/field_script.inc"
#include "text/script/plasma_frigate_14.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This Pokémon... I feel sorry for it\nbecause it's been left alone.[f000]븁\u0000\nWhat should I do? Should I ask the guys\nin Driftveil City to take care of it...?"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate14_Text_PokemonFeelSorryBecause, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Squuu..."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate14_Text_Squuu, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
