#include "asm/field_script.inc"
#include "text/script/driftveil_city_12.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We can live unchanged,\nbecause we keep changing.[f000]븁\u0000\nI mean, Pokémon also evolve,\nbut their Natures stay the same."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity12_Text_WeCanLiveUnchanged, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Driftveil City that I remembered\nhad sort of a dowdy, you know,[f000]븀\u0000\nshabby look..."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity12_Text_DriftveilCityRememberedHad, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0140
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A9
    // "Oh! The Pokémon has something\nin its mouth.[f000]븁\u0000"
    SystemMsg DriftveilCity12_Text_OhPokemonHasSomething, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 197
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    PVPlay 610, 0
    // "Roooooar!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity12_Text_Roooooar, 2, 0, 0
    PVWait
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0140
    VMJump L_00C5

L_00A9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 610, 0
    // "Roooooar!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity12_Text_Roooooar, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose

L_00C5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
