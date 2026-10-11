#include "asm/field_script.inc"
#include "text/script/humilau_city_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 255
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 23
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Humilau City was a resort known to\nonly a limited number of people.[f000]븁\u0000\nBut personally, it's more fun\nif many people come to visit the city."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityPokemonCenter_Text_HumilauCityResortKnown, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    PokePartyHasMoveAny 0x8020, 57
    DebugPrint 0x8020
    VMStackPush 0x8020
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00CE
    WordSetPartyPokeSpecies 0, 0x8020
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wow, seriously?[f000]븁\u0000\nYour [f000]ā\u0001\u0000 can\nuse Surf![f000]븁\u0000\nCool!\nSeriously, I give mad props to you![f000]븁\u0000\nYou ride and [f000]ā\u0001\u0000 is ridden...\nThe vibe between you and your Pokémon[f000]븀\u0000\nis insanely awesome![f000]븁\u0000\n...Me?[f000]븁\u0000\nI sink like a rock, so\nseriously, no thank you[f000]븀\u0000\nto the sea and waves..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityPokemonCenter_Text_WowSeriouslyCanUse, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00E2

L_00CE:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wow, seriously?\nYour Pokémon can't use Surf[f000]븀\u0000\nat all![f000]븁\u0000\nYou can't ride the real wave...\nor feel the vibe.[f000]븀\u0000\nSeriously, no thank you!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCityPokemonCenter_Text_WowSeriouslyPokemonCant, 0, 0
    LastKeyWait
    ActorMsgClose

L_00E2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
