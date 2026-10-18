#include "asm/field_script.inc"
#include "text/script/accumula_town_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Trainers raise Pokémon.\nPokémon make Trainers strong.[f000]븁\u0000\nIt's always been that way\nand always will.[f000]븁\u0000\nIsn't that a solid thought?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown6_Text_TrainersRaisePokemonPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In good times and bad,\nPokémon will stay by your side.[f000]븀\u0000\nYou don't need anything[f000]븀\u0000\nmore than that, right?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown6_Text_GoodTimesBadPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The people of the Unova region really\nlook up to the hero and Pokémon[f000]븀\u0000\nthat founded this place.[f000]븁\u0000\nThat shady Team Plasma tried to use\nthose feelings to their advantage!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown6_Text_PeopleUnovaRegionReally, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Ro gro!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown6_Text_RoGro, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
