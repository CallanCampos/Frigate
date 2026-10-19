#include "asm/field_script.inc"
#include "text/script/floccesy_town_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Here is a little advice!\nKeep a lot of Potions![f000]븁\u0000\nHere is some more advice!\nKeep a lot of Poké Balls, too!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTownPokemonCenter_Text_HereLittleAdviceKeep, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    PokePartyGetMemberByType 0x8020, 2
    WordSetPartyPokeSpecies 0, 0x8020
    WorkSetConst 0x8021, 0
    PokePartyGetParam 0x8021, 0x8020, 110
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0082
    // "Your [f000]ā\u0001\u0000 is male![f000]븁\u0000\nI wonder what the difference is\nbetween male and female Pokémon."
    ActorMsg MSGFILE_SCRIPT, FloccesyTownPokemonCenter_Text_MaleWonderWhatDifference, 9, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00BB

L_0082:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AB
    // "Your [f000]ā\u0001\u0000 is female![f000]븁\u0000\nI wonder what the difference is\nbetween male and female Pokémon."
    ActorMsg MSGFILE_SCRIPT, FloccesyTownPokemonCenter_Text_FemaleWonderWhatDifference, 9, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00BB

L_00AB:
    // "Your [f000]ā\u0001\u0000...\nIts gender is unknown.[f000]븁\u0000\nI wonder what the difference is between\nmale and female Pokémon."
    ActorMsg MSGFILE_SCRIPT, FloccesyTownPokemonCenter_Text_ItsGenderUnknownWonder, 9, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00BB:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I read the Help on the PC.\nI feel I became smarter!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTownPokemonCenter_Text_ReadHelpPcFeel, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

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
    WorkSet 0x8000, 24
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
