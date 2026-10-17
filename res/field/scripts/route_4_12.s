#include "asm/field_script.inc"
#include "text/script/route_4_12.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
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
    PokePartyGetCount 0x8020, 0

L_003C:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00A9
    PokePartyIsEgg 0x8025, 0x8021
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009D
    PokePartyIsFullHP 0x8022, 0x8021
    PokePartyIsFullPP 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_009D
    WorkAdd 0x8024, 1

L_009D:
    WorkAdd 0x8021, 1
    VMJump L_003C

L_00A9:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00FE
    // "Hmm? Aren't you a bit tired?\nDon't be shy. Take a rest!"
    ParentActorMsg MSGFILE_SCRIPT, Route412_Text_HmmArentBitTired, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Oh! Both you and your Pokémon\nare full of energy!"
    ParentActorMsg MSGFILE_SCRIPT, Route412_Text_OhBothPokemonFull, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_010C

L_00FE:
    // "Oh! Both you and your Pokémon\nare full of energy!"
    ParentActorMsg MSGFILE_SCRIPT, Route412_Text_OhBothPokemonFull, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_010C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Gellooo."
    ParentActorMsg MSGFILE_SCRIPT, Route412_Text_Gellooo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
