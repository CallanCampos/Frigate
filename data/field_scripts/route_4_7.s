#include "asm/field_script.inc"
#include "text/script/route_4_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi!\nWelcome![f000]븁\u0000\nWell...\nThere is nothing here."
    ParentActorMsg MSGFILE_SCRIPT, Route47_Text_HiWelcomeWellThere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You know, on Thursdays,\nsome Pokémon fly here."
    ParentActorMsg MSGFILE_SCRIPT, Route47_Text_KnowThursdaysSomePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If I'm with a big Pokémon,\neven I look slim!"
    ParentActorMsg MSGFILE_SCRIPT, Route47_Text_IfImBigPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
