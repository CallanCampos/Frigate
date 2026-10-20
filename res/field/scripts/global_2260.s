#include "asm/field_script.inc"
#include "text/script/global_2260.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

L_0016:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8026, 1

L_003A:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EB
    // "OK, then.\nWhich move should be forgotten?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_OkThenWhichMove, 0, 0
    ActorMsgClose
    CallPokeMoveReplace 0x8025, 0x8022, 0x8020, 0
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DF
    PokePartyGetMove 0x8023, 0x8020, 0x8022
    WordSetMoveName 0, 0x8023
    // "Hm! The move [f000]ć\u0001\u0000?\nShould that move be forgotten?"
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_HmMoveShouldMove, 0, 0
    YesNoWin 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D9
    PokePartyGetMove 0x8023, 0x8020, 0x8022
    WordSetMoveName 0, 0x8023
    // "It worked perfectly![f000]븁\u0000\nYour Pokémon has forgotten the move\n[f000]ć\u0001\u0000 completely."
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_WorkedPerfectlyPokemonHas, 0, 0
    MEPlay SEQ_ME_WASURE
    MEWait
    LastKeyWait
    ActorMsgClose
    PokePartyLearnMove 0x8020, 0x8022, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8021, 0

L_00D9:
    VMJump L_00E5

L_00DF:
    WorkSetConst 0x8026, 0

L_00E5:
    VMJump L_003A

L_00EB:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    VMReturn

L_010B:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

L_0117:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CB
    // "Which Pokémon should forget a move?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_WhichPokemonShouldForget, 0, 0
    ActorMsgClose
    CallPokeSelect 0, 0x8028, 0x8020, 0
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016D
    // "Remember me if there are moves that\nneed to be forgotten."
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_RememberIfThereMoves, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMJump L_01C5

L_016D:
    PokePartyIsEgg 0x8028, 0x8020
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0196
    // "What? That's an Egg.\nNo Egg should know any moves.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_WhatThatsEggNo, 0, 0
    VMJump L_01C5

L_0196:
    PokePartyGetMoveCount 0x8027, 0x8020
    VMStackPush 0x8027
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_01BF
    // "That Pokémon knows only one move,\nso it can't be forgotten...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_PokemonKnowsOnlyOne, 0, 0
    VMJump L_01C5

L_01BF:
    VMCall L_0016

L_01C5:
    VMJump L_0117

L_01CB:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    VMReturn

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8021, 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020E
    // "Er...\nWho was I again?[f000]븁\u0000\n...\n...[f000]븁\u0000\n...Oh, that's right!\nI am the Move Deleter![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_ErWhoAgainOh, 0, 0
    FlagSet EVENT_FLAG_0x0001

L_020E:
    // "You've come to make me force your\nPokémon to forget some moves?"
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_YouveComeMakeForce, 0, 0
    YesNoWin 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023B
    VMCall L_010B
    VMJump L_0249

L_023B:
    // "Remember me if there are moves that\nneed to be forgotten."
    ParentActorMsg MSGFILE_SCRIPT, Global2260_Text_RememberIfThereMoves, 0, 0
    LastKeyWait
    ActorMsgClose

L_0249:
    WorkSetConst 0x8029, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x802a, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x802b, 0
    FlagGet EVENT_FLAG_0x007c, 0x802b
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0292
    FlagSet EVENT_FLAG_0x007c
    // "Everybody calls me the reminder girl.[f000]븁\u0000\nI know every move that Pokémon learn\nwhile they're leveling up.[f000]븁\u0000\nAnd I can make Pokémon remember\nthose moves![f000]븁\u0000\nIf you bring me a Heart Scale, I'll make\na Pokémon remember a move.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2260_Text_EverybodyCallsReminderGirl, 0x8011, 2, 0

L_0292:
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    ItemCheckAmount ITEM_HEART_SCALE, 1, 0x802c
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BF
    VMJump L_0402

L_02BF:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    // "Should a move be remembered?"
    ActorMsg MSGFILE_SCRIPT, Global2260_Text_ShouldMoveRemembered_2, 0x8011, 2, 0
    YesNoWin 0x802d
    VMStackPush 0x802d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F4
    VMJump L_0402

L_02F4:
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    // "Which Pokémon should learn it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2260_Text_WhichPokemonShouldLearn, 0x8011, 2, 0
    ActorMsgClose
    CallPokeSelect 0, 0x802e, 0x802a, 0
    VMStackPush 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0331
    VMJump L_0402

L_0331:
    WorkSetConst 0x802f, 0
    PokePartyIsEgg 0x802f, 0x802a
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0356
    VMJump L_0418

L_0356:
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    MoveReminderCheckPkm 0x8030, 0x802a
    VMStackPush 0x8030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0381
    VMJump L_042E

L_0381:
    WorkSetConst 0x8030, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    WordSetPartyPokeName 0, 0x802a
    // "Which move should\n[f000]Ă\u0001\u0000 remember?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2260_Text_WhichMoveShouldRemember, 0x8011, 2, 0
    ActorMsgClose
    MoveReminderCallMoveSelect 0x8031, 0x802a
    WorkCmpConst 0x8031, 0
    VMJumpIf CMP_EQ, L_03C5
    VMJump L_03D1

L_03C5:
    VMJump L_0444
    .byte 0x1e
    .byte 0x00
    .byte 0x25
    .byte 0x00
    VMNop

L_03D1:
    WorkCmpConst 0x8031, 1
    VMJumpIf CMP_EQ, L_03E4
    VMJump L_03F0

L_03E4:
    VMJump L_0402
    .byte 0x1e
    .byte 0x00
    DebugPrint 0

L_03F0:
    VMJump L_0402
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8031, 0

L_0402:
    // "If any of your Pokémon needs to\nremember a move, bring me a Heart Scale!"
    ActorMsg MSGFILE_SCRIPT, Global2260_Text_IfAnyPokemonNeeds, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_046B

L_0418:
    // "Eggs can't remember moves!"
    ActorMsg MSGFILE_SCRIPT, Global2260_Text_EggsCantRememberMoves, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_046B

L_042E:
    // "This Pokémon hasn't forgotten\nany moves."
    ActorMsg MSGFILE_SCRIPT, Global2260_Text_PokemonHasntForgottenAny, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_046B

L_0444:
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 handed over one\nHeart Scale in exchange."
    SystemMsg Global2260_Text_HandedOverOneHeart, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst 0x8033, 0
    ItemSub ITEM_HEART_SCALE, 1, 0x8033
    WorkSetConst 0x8033, 0
    VMJump L_046B

L_046B:
    WorkSetConst 0x802a, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
