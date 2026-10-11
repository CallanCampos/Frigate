#include "asm/field_script.inc"
#include "text/script/accumula_town_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 580
    WorkSet 0x8001, 1
    WorkSet 0x8002, 335
    WorkSet 0x8003, 0
    WorkSet 0x8004, 1
    WorkSet 0x8005, 1
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Your Pokédex is full of\nweak Pokémon, strong Pokémon,[f000]븀\u0000\ncool Pokémon, cute Pokémon...[f000]븁\u0000\nAt any rate, there sure are lots\nof Pokémon in there!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown5_Text_PokedexFullWeakPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The moment my Lillipup evolved into\nHerdier, my eyes started watering.[f000]븁\u0000\nI was just so happy somehow..."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown5_Text_MomentLillipupEvolvedInto, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Woo uff!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown5_Text_WooUff, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
