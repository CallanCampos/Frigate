#include "asm/field_script.inc"
#include "text/script/driftveil_city_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes you can learn about\nPokémon moves and items[f000]븀\u0000\non TV programs."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity9_Text_SometimesCanLearnAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Some Pokémon evolve by trade![f000]븁\u0000\nCool, right?\nWhy do they evolve?!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity9_Text_SomePokemonEvolveBy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Queak, queak!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity9_Text_QueakQueak, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You might be able to learn something\nif you check out battles between people[f000]븀\u0000\nwho are stronger than you."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity9_Text_MightAbleLearnSomething, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
