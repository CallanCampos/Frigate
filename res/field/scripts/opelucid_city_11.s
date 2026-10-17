#include "asm/field_script.inc"
#include "text/script/opelucid_city_11.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if the Elite Four in the\nPokémon League like Pokémon more[f000]븀\u0000\nthan anyone else.[f000]븁\u0000\nIf you don't like Pokémon,\nyou'll never get good at battling, right?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity11_Text_WonderIfEliteFour, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You need to have eight Gym Badges\nto take on the pinnacle of Pokémon,[f000]븀\u0000\nthe Pokémon League!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity11_Text_NeedHaveEightGym, 0, 0
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
    // "Raaah!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity11_Text_Raaah, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
