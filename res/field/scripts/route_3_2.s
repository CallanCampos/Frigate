#include "asm/field_script.inc"
#include "text/script/route_3_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
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

L_0052:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00A6
    PokePartyIsFullHP 0x8022, 0x8021
    PokePartyIsFullPP 0x8023, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_009A
    WorkAdd 0x8024, 1

L_009A:
    WorkAdd 0x8021, 1
    VMJump L_0052

L_00A6:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00FD
    // "Oh my...\nYour Pokémon...[f000]븁\u0000\nThey don't seem to be healthy.\nLet them rest here a little bit![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route32_Text_OhPokemonTheyDont, 0, 0, 0
    ActorMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    // "Your Pokémon are really energetic!"
    ActorMsg MSGFILE_SCRIPT, Route32_Text_PokemonReallyEnergetic, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_010D

L_00FD:
    // "Your Pokémon are really energetic!"
    ActorMsg MSGFILE_SCRIPT, Route32_Text_PokemonReallyEnergetic, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_010D:
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
    // "We don't raise little ones,\nwe help them grow.[f000]븁\u0000\nThat's what I think, and the Day-Care\nCouple next door feels the same."
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_WeDontRaiseLittle, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Are you aware of the Pokémon Ability\nFlame Body?"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_AwarePokemonAbilityFlame, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018A
    // "Impressive! I often see\nVolcarona on Route 3!"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_ImpressiveOftenSeeVolcarona, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0198

L_018A:
    // "OK! Listen to this, then.[f000]븁\u0000\nWhen a Pokémon with the Flame Body\nAbility is with you, Eggs hatch faster!"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_OkListenThenWhen, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0198:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nice guys from Striaton City\ngave this preschool its Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_NiceGuysFromStriaton, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm gonna be a Pokémon\nwhen I grow up!"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_ImGonnaPokemonWhen, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon Trainers are here\nbecause of Pokémon![f000]븁\u0000\nPokémon are here\nbecause of Pokémon Trainers![f000]븁\u0000\nI wonder which one came first?"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_PokemonTrainersHereBecause, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 511, 0
    // "Ega snap!"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_EgaSnap, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 513, 0
    // "Rae snap!"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_RaeSnap, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 515, 0
    // "Ruo pnap!"
    ParentActorMsg MSGFILE_SCRIPT, Route32_Text_RuoPnap, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
