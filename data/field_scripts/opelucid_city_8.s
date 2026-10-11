#include "asm/field_script.inc"
#include "text/script/opelucid_city_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I was able to get to know\nher through trading Pokémon![f000]븁\u0000\nOur battle styles, tastes,\nand ways of thinking are different,[f000]븀\u0000\nbut it's more interesting that way!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity8_Text_AbleGetKnowHer, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are some Pokémon that evolve by\ntrading parts of their bodies![f000]븁\u0000\nWe might evolve by exchanging\nour thoughts and opinions!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity8_Text_ThereSomePokemonEvolve, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
