#include "asm/field_script.inc"
#include "text/script/virbank_city_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When a Pokémon evolves, its appearance\nwill change, and it'll get more powerful![f000]븁\u0000\nIf you keep a Pokémon from evolving,\nit will learn moves more quickly!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity2_Text_WhenPokemonEvolvesIts, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40ac
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0055
    // "Ah! I want to go to Pokéstar Studios\nas soon as possible![f000]븁\u0000\nI want my dear Audino\nto be in a movie!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity2_Text_AhWantGoPokestar, 0, 0
    VMJump L_005F

L_0055:
    // "My dear Audino will make its movie debut\nin Pokéstar Studios!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity2_Text_DearAudinoWillMake, 0, 0

L_005F:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 531, 0
    // "Brrrm...brrrm."
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity2_Text_BrrrmBrrrm, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
