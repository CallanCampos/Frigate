#include "asm/field_script.inc"
#include "text/script/global_10280.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am researching Pokémon Fossils here.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10280_Text_AmResearchingPokemonFossils, 0, 0
    VMStackPush EVENT_WORK_0x417a
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_004F
    VMCall L_0412
    VMJump L_00B3

L_004F:
    VMCall L_0426
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00A7
    // "You have a Fossil, don't you?\nShall I turn it back into a Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global10280_Text_HaveFossilDontShall, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0095
    VMCall L_00C9
    VMJump L_00A1

L_0095:
    WorkSetConst 0x8020, 4
    VMCall L_00B9

L_00A1:
    VMJump L_00B3

L_00A7:
    WorkSetConst 0x8020, 10
    VMCall L_00B9

L_00B3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00B9:
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_00C9:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0113
    VMCall L_011B
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0101
    VMCall L_031F
    VMJump L_010D

L_0101:
    WorkSetConst 0x8020, 4
    VMCall L_00B9

L_010D:
    VMJump L_0119

L_0113:
    VMCall L_031F

L_0119:
    VMReturn

L_011B:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    // "Which Fossil should I turn back\ninto a Pokémon?"
    ActorMsg MSGFILE_SCRIPT, Global10280_Text_WhichFossilShouldTurn, 6, 2, 0
    WorkSetConst 0x8022, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32804
    WorkSetConst 0x8025, 99
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016B
    ListMenuAdd 13, 65535, 99

L_016B:
    WorkSetConst 0x8025, 100
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0194
    ListMenuAdd 14, 65535, 100

L_0194:
    WorkSetConst 0x8025, 101
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BD
    ListMenuAdd 15, 65535, 101

L_01BD:
    WorkSetConst 0x8025, 102
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E6
    ListMenuAdd 16, 65535, 102

L_01E6:
    WorkSetConst 0x8025, 103
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020F
    ListMenuAdd 17, 65535, 103

L_020F:
    WorkSetConst 0x8025, 104
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0238
    ListMenuAdd 18, 65535, 104

L_0238:
    WorkSetConst 0x8025, 105
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0261
    ListMenuAdd 19, 65535, 105

L_0261:
    WorkSetConst 0x8025, 572
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028F
    WordSetItemName 0, 572
    ListMenuAdd 11, 65535, 572

L_028F:
    WorkSetConst 0x8025, 573
    ItemCheckAmount 0x8025, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BD
    WordSetItemName 1, 573
    ListMenuAdd 12, 65535, 573

L_02BD:
    ListMenuAdd 20, 65535, 0
    ListMenuShow
    VMStackPush 0x8024
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0311
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02FF
    WorkGet 0x8022, 0x8024
    WorkSetConst 0x8010, 1
    VMJump L_030B

L_02FF:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8010, 0

L_030B:
    VMJump L_031D

L_0311:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8010, 0

L_031D:
    VMReturn

L_031F:
    WordSetItemName 0, 0x8022
    // "OK, then![f000]븁\u0000\nI'll turn that [f000]ĉ\u0001\u0000\nback into a Pokémon for you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10280_Text_OkThenIllTurn, 0, 0
    ActorMsgClose
    ItemSub 0x8022, 1, 0x8010
    RecordAdd 89, 1
    Cmd_02C5 15
    ActorCmdExec 6, Movement_06E4
    ActorCmdWait
    VMSleep 90
    ActorCmdExec 6, Movement_06F0
    ActorCmdWait
    VMCall L_05C9
    VMCall L_0368
    VMReturn

L_0368:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WordSetPokeSpecies 0, 0x8023
    // "The Fossil you gave me turned back into\na Pokémon![f000]븁\u0000\nThis is [f000]ā\u0001\u0000!\nPlease take good care of it.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10280_Text_FossilGaveTurnedBack, 0, 0
    PokePartyGetCount 0x8027, 0
    VMStackPush 0x8027
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B4
    WorkGet EVENT_WORK_0x417a, 0x8022
    WorkSetConst 0x8020, 9
    VMCall L_00B9
    VMJump L_0410

L_03B4:
    ActorMsgClose
    WordSetPlayerName 0
    WordSetPokeSpecies 1, 0x8023
    MEPlay SEQ_ME_POKEGET
    // "[f000]Ā\u0001\u0000 received\n[f000]ā\u0001\u0001!"
    SystemMsg Global10280_Text_Received, 2
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    PokePartyAdd 0x8010, 0x8023, 0, 25
    WordSetPokeSpecies 0, 0x8023
    // "Would you like to give a nickname to the\nnewly received [f000]ā\u0001\u0000?"
    SystemMsg Global10280_Text_WouldLikeGiveNickname, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_040A
    WorkGet 0x8026, 0x8027
    CallPokeNameInput 0x8010, 0x8026, 1

L_040A:
    WorkSetConst EVENT_WORK_0x417a, 0

L_0410:
    VMReturn

L_0412:
    WorkGet 0x8022, EVENT_WORK_0x417a
    VMCall L_05C9
    VMCall L_0368
    VMReturn

L_0426:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8028, 105
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_045F
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_045F:
    WorkSetConst 0x8028, 104
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_048C
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_048C:
    WorkSetConst 0x8028, 101
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04B9
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_04B9:
    WorkSetConst 0x8028, 102
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E6
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_04E6:
    WorkSetConst 0x8028, 103
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0513
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_0513:
    WorkSetConst 0x8028, 100
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0540
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_0540:
    WorkSetConst 0x8028, 99
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_056D
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_056D:
    WorkSetConst 0x8028, 572
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_059A
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_059A:
    WorkSetConst 0x8028, 573
    ItemCheckAmount 0x8028, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05C7
    WorkGet 0x8022, 0x8028
    WorkAdd 0x8021, 1

L_05C7:
    VMReturn

L_05C9:
    VMStackPush 0x8022
    VMStackPushConst 105
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05E8
    WorkSetConst 0x8023, 408
    VMJump L_06E0

L_05E8:
    VMStackPush 0x8022
    VMStackPushConst 104
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0607
    WorkSetConst 0x8023, 410
    VMJump L_06E0

L_0607:
    VMStackPush 0x8022
    VMStackPushConst 101
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0626
    WorkSetConst 0x8023, 138
    VMJump L_06E0

L_0626:
    VMStackPush 0x8022
    VMStackPushConst 102
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0645
    WorkSetConst 0x8023, 140
    VMJump L_06E0

L_0645:
    VMStackPush 0x8022
    VMStackPushConst 103
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0664
    WorkSetConst 0x8023, 142
    VMJump L_06E0

L_0664:
    VMStackPush 0x8022
    VMStackPushConst 100
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0683
    WorkSetConst 0x8023, 347
    VMJump L_06E0

L_0683:
    VMStackPush 0x8022
    VMStackPushConst 99
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06A2
    WorkSetConst 0x8023, 345
    VMJump L_06E0

L_06A2:
    VMStackPush 0x8022
    VMStackPushConst 572
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06C1
    WorkSetConst 0x8023, 564
    VMJump L_06E0

L_06C1:
    VMStackPush 0x8022
    VMStackPushConst 573
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06E0
    WorkSetConst 0x8023, 566
    VMJump L_06E0

L_06E0:
    VMReturn
    .balign 4, 0

Movement_06E4:
    Move 15, 2
    Move 69, 1
    MoveEnd

Movement_06F0:
    Move 2, 1
    Move 70, 1
    Move 14, 2
    MoveEnd
