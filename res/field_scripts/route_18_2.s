#include "asm/field_script.inc"
#include "text/script/route_18_2.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PokePartyGetCount 0x8020, 0

L_0032:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0086
    PokePartyIsFullHP 0x8022, 0x8021
    PokePartyIsFullPP 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_007A
    WorkAdd 0x8024, 1

L_007A:
    WorkAdd 0x8021, 1
    VMJump L_0032

L_0086:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00D9
    // "Wait! Wait![f000]븁\u0000\nAren't your Pokémon a bit tired?\nHere, don't be shy! Let them rest![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route182_Text_WaitWaitArentPokemon, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Your Pokémon are so energetic!"
    ParentActorMsg MSGFILE_SCRIPT, Route182_Text_PokemonEnergetic, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00E7

L_00D9:
    // "Your Pokémon are so energetic!"
    ParentActorMsg MSGFILE_SCRIPT, Route182_Text_PokemonEnergetic, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00E7:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
