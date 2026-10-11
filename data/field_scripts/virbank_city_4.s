#include "asm/field_script.inc"
#include "text/script/virbank_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I will be a hero and become friends\nwith Reshiram![f000]븁\u0000\nReshiram is a legendary Pokémon!\nBut, I don't know it very well..."
    // "I will be a hero and become friends\nwith Zekrom![f000]븁\u0000\nZekrom is a legendary Pokémon!\nBut, I don't know it very well..."
    ActorMsgVersioned 1024, VirbankCity4_Text_WillHeroBecomeFriends_2, VirbankCity4_Text_WillHeroBecomeFriends, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It was two years ago...\nA bunch of people who identified[f000]븀\u0000\nthemselves as Team Plasma tried[f000]븀\u0000\nto control Unova under the hero[f000]븀\u0000\nwho was with the legendary white[f000]븀\u0000\nPokémon, Reshiram."
    // "It was two years ago...\nA bunch of people who identified[f000]븀\u0000\nthemselves as Team Plasma tried[f000]븀\u0000\nto control Unova under the hero[f000]븀\u0000\nwho was with the legendary black[f000]븀\u0000\nPokémon, Zekrom."
    ActorMsgVersioned 1024, VirbankCity4_Text_TwoYearsAgoBunch_2, VirbankCity4_Text_TwoYearsAgoBunch, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh?\nAre you a Pokémon Trainer?[f000]븁\u0000\nMy grandchild was also visiting\nPokémon Gyms with his Pokémon[f000]븀\u0000\nin various places and collecting Badges."
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity4_Text_OhPokemonTrainerGrandchild, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
