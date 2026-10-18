#include "asm/field_script.inc"
#include "text/script/castelia_city_26.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    PokeDexGetCount 0, 0x8020
    WordSetNumber 0, 0x8020, 3
    VMStackPush 0x8020
    VMStackPushConst 40
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00B0
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 538
    WorkSet 0x8001, 1
    WorkSet 0x8002, 244
    WorkSet 0x8003, 1
    WorkSet 0x8004, 2
    WorkSet 0x8005, 2
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_00C4

L_00B0:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am a Pokémon fanatic.\nI am famous in Castelia, too![f000]븁\u0000\nOh, look!\nYou have a Pokédex![f000]븁\u0000\nHow many Pokémon\nhave you found so far?[f000]븁\u0000\n...\n[f000]Ȃ\u0001\u0000 Pokémon![f000]븁\u0000\nIf you have 40 Pokémon or more,\nI'll give you something good!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity26_Text_AmPokemonFanaticAm, 0, 0
    LastKeyWait
    ActorMsgClose

L_00C4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've been thinking about starting\na new business...[f000]븁\u0000\nBut it's quite a chore."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity26_Text_IveBeenThinkingAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Don't you think a service that teaches\nPokémon moves would be successful?[f000]븁\u0000\nWhat?\nThere are already people who do that?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity26_Text_DontThinkServiceTeaches, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What kinds of Abilities do\nyour Pokémon have?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity26_Text_WhatKindsAbilitiesPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Patrat's Ability is Run Away![f000]븁\u0000\nIt can always get away\nfrom wild Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity26_Text_PatratsAbilityRunAway, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hmmm! Fantastic! Excellent!\nBurgh's paintings are magnificent!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity26_Text_HmmmFantasticExcellentBurghs, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Squeak!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity26_Text_Squeak, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
