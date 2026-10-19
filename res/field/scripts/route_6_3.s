#include "asm/field_script.inc"
#include "text/script/route_6_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Maybe I should go to\nCelestial Tower on Route 7.[f000]븁\u0000\nI have to ring the bell for my Petilil..."
    // "Maybe I should go to\nCelestial Tower on Route 7.[f000]븁\u0000\nI have to ring the bell for\nmy Cottonee..."
    ActorMsgVersioned 1024, Route63_Text_MaybeShouldGoCelestial, Route63_Text_MaybeShouldGoCelestial_2, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x017f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B5
    // "Mister!\nHere, have this![f000]븁\u0000"
    // "Miss!\nHere, have this![f000]븁\u0000"
    ActorMsgGendered 1024, Route63_Text_MisterHereHave, Route63_Text_MissHereHave, 1, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 107
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Know what? When I gave my Minccino a\nShiny Stone, it evolved and became a[f000]븀\u0000\ndifferent Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, Route63_Text_KnowWhatWhenGave, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x017f
    VMJump L_00C3

L_00B5:
    // "Know what? When I gave my Minccino a\nShiny Stone, it evolved and became a[f000]븀\u0000\ndifferent Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, Route63_Text_KnowWhatWhenGave, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00C3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokePartyGetCount 0x8020, 0

L_00D7:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0144
    PokePartyIsEgg 0x8025, 0x8021
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0138
    PokePartyIsFullHP 0x8022, 0x8021
    PokePartyIsFullPP 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0138
    WorkAdd 0x8024, 1

L_0138:
    WorkAdd 0x8021, 1
    VMJump L_00D7

L_0144:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0199
    // "Oh, dear! Your Pokémon...\nSomehow, they don't seem well.[f000]븁\u0000\nYou should rest here for a little while.\nYou can't go anywhere when you're not[f000]븀\u0000\nfeeling well."
    ParentActorMsg MSGFILE_SCRIPT, Route63_Text_OhDearPokemonSomehow, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Good! Your Pokémon seem to be\nfull of energy!"
    ParentActorMsg MSGFILE_SCRIPT, Route63_Text_GoodPokemonSeemFull, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01A7

L_0199:
    // "Good! Your Pokémon seem to be\nfull of energy!"
    ParentActorMsg MSGFILE_SCRIPT, Route63_Text_GoodPokemonSeemFull, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01A7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
