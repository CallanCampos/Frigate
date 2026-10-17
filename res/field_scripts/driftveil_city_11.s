#include "asm/field_script.inc"
#include "text/script/driftveil_city_11.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "I gave the Pokémon a nickname!\nI also let it hold an item![f000]븀\u0000\nSo, please take good care of it!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity11_Text_GavePokemonNicknameAlso, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Great! We can trade Pokémon using\nwireless communications[f000]븀\u0000\neven when we're apart!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity11_Text_GreatWeCanTrade, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "He's trading Pokémon right after\nhe came to a foreign place...[f000]븁\u0000\nChildren have the gift to make friends."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity11_Text_HesTradingPokemonRight, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
