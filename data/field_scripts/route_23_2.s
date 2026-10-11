#include "asm/field_script.inc"
#include "text/script/route_23_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
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

L_003A:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_008E
    PokePartyIsFullHP 0x8022, 0x8021
    PokePartyIsFullPP 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0082
    WorkAdd 0x8024, 1

L_0082:
    WorkAdd 0x8021, 1
    VMJump L_003A

L_008E:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00E1
    // "Your Pokémon...\nNot in perfect condition.[f000]븁\u0000\nCome. Don't be so reserved.\nTake a rest![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route232_Text_PokemonNotPerfectCondition, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Yes! The team of you and your Pokémon\ncan go as far as you want!"
    ParentActorMsg MSGFILE_SCRIPT, Route232_Text_YesTeamPokemonCan, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00EF

L_00E1:
    // "Yes! The team of you and your Pokémon\ncan go as far as you want!"
    ParentActorMsg MSGFILE_SCRIPT, Route232_Text_YesTeamPokemonCan, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00EF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 531, 0
    // "Chu!"
    ParentActorMsg MSGFILE_SCRIPT, Route232_Text_Chu, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 610, 0
    // "Achoo!"
    ParentActorMsg MSGFILE_SCRIPT, Route232_Text_Achoo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
