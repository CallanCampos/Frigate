#include "asm/field_script.inc"
#include "text/script/route_3_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon left in the Day Care\nlearn moves as they grow![f000]븁\u0000\nSo if you just leave them there,\nthey might forget an important move, yo!"
    ParentActorMsg MSGFILE_SCRIPT, Route33_Text_PokemonLeftDayCare, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon don't evolve while\nthey're being kept at the Day Care!"
    ParentActorMsg MSGFILE_SCRIPT, Route33_Text_PokemonDontEvolveWhile, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
