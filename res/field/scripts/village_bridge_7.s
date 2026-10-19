#include "asm/field_script.inc"
#include "text/script/village_bridge_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokePartyGetCount 0x8020, 0

L_0036:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_008A
    PokePartyIsFullHP 0x8022, 0x8021
    PokePartyIsFullPP 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_007E
    WorkAdd 0x8024, 1

L_007E:
    WorkAdd 0x8021, 1
    VMJump L_0036

L_008A:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00DD
    // "Oh, your Pokémon look pretty tired.\nDon't be shy. Take a nice long rest![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge7_Text_OhPokemonLookPretty, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Both you and your Pokémon are healthy\nand energetic![f000]븁\u0000\nIf you get tired, please talk to me.\nI'll let you take a nice long rest."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge7_Text_BothPokemonHealthyEnergetic, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00EB

L_00DD:
    // "Both you and your Pokémon are healthy\nand energetic![f000]븁\u0000\nIf you get tired, please talk to me.\nI'll let you take a nice long rest."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge7_Text_BothPokemonHealthyEnergetic, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00EB:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 575, 0
    // "Gothoo. ♪"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge7_Text_Gothoo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
