#include "asm/field_script.inc"
#include "text/script/humilau_city_5.h"

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
    WorkSetConst 0x8025, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0061
    // "The flat part of Stunfisk is\nvery charming, isn't it?"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_FlatPartStunfiskVery, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01E3

L_0061:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0156
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01B2
    // "The flat part of Stunfisk is\nvery charming, isn't it?[f000]븁\u0000\nDo you know other Water-type Pokémon\nwith a slender body?"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_FlatPartStunfiskVery_2, 0, 0
    MsgWaitAdvance
    PokePartyGetCount 0x8020, 0

L_0096:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00EA
    PokePartyGetSpecies 0x8022, 0x8021
    PokePartyIsEgg 0x8025, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 370
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00DE
    WorkSetConst 0x8023, 1

L_00DE:
    WorkAdd 0x8021, 1
    VMJump L_0096

L_00EA:
    ItemCheckSpace ITEM_HEART_SCALE, 5, 0x8024
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_015B
    // "Oh!\nThe heart-shaped Pokémon![f000]븁\u0000\nWow! It's called Luvdisc!\nGreat! So slender!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_OhHeartShapedPokemon, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 93
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Stunfisk and Luvdisc both\nhave slender bodies![f000]븁\u0000\nBut whether it is vertically slender\nor horizontally slender[f000]븀\u0000\ngives a different impression!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_StunfiskLuvdiscBothHave, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0156
    VMJump L_01AC

L_015B:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_019E
    // "Oh!\nThe heart-shaped Pokémon![f000]븁\u0000\nWow! It's called Luvdisc!\nGreat! So slender!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_OhHeartShapedPokemon, 0, 0
    MsgWaitAdvance
    // "Oh? Your Bag is full of\nHeart Scales![f000]븁\u0000\nCome visit me again\nwith Luvdisc!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_OhBagFullHeart, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01AC

L_019E:
    // "The Pokémon's name starts with “L\"\nand it looks like a heart! I think."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_PokemonsNameStartsL, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01AC:
    VMJump L_01E3

L_01B2:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0156
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01E3
    // "Stunfisk and Luvdisc both\nhave slender bodies![f000]븁\u0000\nBut whether it is vertically slender\nor horizontally slender[f000]븀\u0000\ngives a different impression!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_StunfiskLuvdiscBothHave, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01E3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you sleep on Stunfisk,\nits electricity relaxes the body!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_IfSleepStunfiskIts, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 618, 0
    // "Stuun!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity5_Text_Stuun, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
