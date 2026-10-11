#include "asm/field_script.inc"
#include "text/script/global_10785.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8023, 0
    WorkGet 0x8023, 0x8000
    VMStackPush 0x8000
    WorkSet 0x8000, 0x8023
    RTCallGlobal 2816
    VMStackPop 0x8000
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMCall L_0106
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMCall L_0144
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMCall L_01F7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMCall L_04AE
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMCall L_05B3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    VMCall L_08DE
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    VMCall L_0A3B
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    VMCall L_0AFF
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    VMCall L_0B90
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00E6:
    FadeEx 3, 0, 16, 2
    FadeExWait
    VMSleep 8
    FunfestActorDelete
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMReturn

L_0106:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Waaah! Waaaaah!\nI got lost! Waaaah![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_WaaahWaaaaahGotLost, 0, 0
    MsgWinCloseAll
    // "Showed him your Town Map![f000]븁\u0000"
    SystemMsg Global10785_Text_ShowedHimTownMap, 0
    MsgWinCloseAll
    // "...\n...Sniff. Thank you.[f000]븀\u0000\nNow I know where my home is...[f000]븁\u0000\nI'll go home![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_SniffThankNowKnow, 0, 0
    MsgWinCloseAll
    FunfestMissionBroadcast 30, 0
    VMCall L_00E6
    // "The boy went home..."
    SystemMsg Global10785_Text_BoyWentHome, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_0144:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    FunfestGetGenericInfo 0, 0x8024
    WordSetItemName 0, 0x8024
    // "Waaah! Waaah!\n[f000]ĉ\u0001\u0000! I want one![f000]븀\u0000\nGive one to me!"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_WaaahWaaahWantOne, 0, 0
    YesNoWin 0x8025
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0192
    // "Stingy! You're stingy!"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_StingyYoureStingy, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_0192:
    ItemSub 0x8024, 1, 0x8025
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BD
    // "You don't have one!"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_DontHaveOne, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_01BD:
    // "Yay!\nThank you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_YayThank, 0, 0
    MsgWinCloseAll
    FunfestMissionBroadcast 31, 0x8024
    // "Gave the [f000]ĉ\u0001\u0000 to him.[f000]븁\u0000"
    SystemMsg Global10785_Text_GaveHim, 0
    MsgWinCloseAll
    // "I'll take good care of it![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_IllTakeGoodCare, 0, 0
    MsgWinCloseAll
    VMCall L_00E6
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    VMReturn

L_01F7:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0220
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0218
    VMReturn

L_0218:
    VMCall L_00E6
    VMReturn

L_0220:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    FunfestDispSalesmanMessage 15, 9, 2
    Random 0x8029, 6
    Random 0x802a, 6
    WorkSetConst 0x8026, 0

L_025E:
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_03F8
    FunfestGetItemExchangeInfo 0x8029, 0x802a, 0x8027, 0x8028
    WordSetItemName 0, 0x8027
    WordSetItemName 1, 0x8028
    FunfestDispSalesmanMessage 16, 9, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32811
    ListMenuAdd 12, 65535, 2
    ListMenuAdd 13, 65535, 1
    ListMenuAdd 14, 65535, 0
    ListMenuShow
    VMStackPush 0x802b
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02ED
    FunfestDispSalesmanMessage 17, 9, 2
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    VMReturn
    VMJump L_0398

L_02ED:
    VMStackPush 0x802b
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0398
    ItemCheckAmount 0x8027, 1, 0x802b
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_032F
    FunfestDispSalesmanMessage 20, 9, 2
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    VMReturn

L_032F:
    ItemAdd 0x8028, 1, 0x802b
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035E
    FunfestDispSalesmanMessage 21, 9, 2
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    VMReturn

L_035E:
    FunfestDispSalesmanMessage 22, 9, 2
    MsgWinCloseAll
    ItemSub 0x8027, 1, 0x802b
    FunfestMissionBroadcast 32, 0
    MEPlay SEQ_ME_ITEM
    // "Gave the [f000]ĉ\u0001\u0000 in exchange for\nthe [f000]ĉ\u0001\u0001!"
    SystemMsg Global10785_Text_GaveExchange, 0
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll
    FunfestDispSalesmanMessage 23, 9, 2
    MsgWinCloseAll
    WorkSetConst 0x8020, 1
    VMReturn

L_0398:
    WorkGet 0x8021, 0x8026
    VMCall L_041E
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C5
    FunfestDispSalesmanMessage 19, 9, 2
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_03C5:
    FunfestDispSalesmanMessage 18, 9, 2
    WorkAdd 0x802a, 1
    VMStackPush 0x802a
    VMStackPushConst 6
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_03EC
    WorkSetConst 0x802a, 0

L_03EC:
    WorkAdd 0x8026, 1
    VMJump L_025E

L_03F8:
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    VMReturn

L_041E:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkGet 0x802c, 0x8021
    WorkSetConst 0x8020, 0
    Random 0x802d, 10000
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046A
    VMStackPush 0x802d
    VMStackPushConst 6000
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0464
    VMReturn

L_0464:
    VMJump L_049A

L_046A:
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0498
    VMStackPush 0x802d
    VMStackPushConst 3000
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0492
    VMReturn

L_0492:
    VMJump L_049A

L_0498:
    VMReturn

L_049A:
    WorkSetConst 0x8020, 1
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    VMReturn

L_04AE:
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    FunfestGetItemSaleInfo 0x802e, 0x802f
    MoneyWinDisp 31, 1
    WordSetItemName 0, 0x802e
    WordSetNumber 1, 0x802f, 5
    FunfestDispSalesmanMessage 34, 6, 2
    YesNoWin 0x8030
    VMStackPush 0x8030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050D
    FunfestDispSalesmanMessage 35, 6, 2
    LastKeyWait
    MsgWinCloseAll
    MoneyWinClose
    VMReturn

L_050D:
    MoneyCheck 0x8030, 0x802f
    VMStackPush 0x8030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_053C
    FunfestMissionBroadcast 39, 0
    FunfestDispSalesmanMessage 36, 6, 2
    LastKeyWait
    MsgWinCloseAll
    MoneyWinClose
    VMReturn

L_053C:
    ItemAdd 0x802e, 1, 0x8030
    VMStackPush 0x8030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0567
    FunfestDispSalesmanMessage 37, 6, 2
    LastKeyWait
    MsgWinCloseAll
    MoneyWinClose
    VMReturn

L_0567:
    FunfestDispSalesmanMessage 38, 6, 2
    MsgWinCloseAll
    MoneySub 0x802f
    FunfestMissionBroadcast 33, 0x802e
    SEPlay SEQ_SE_SYS_22
    MoneyWinUpdate
    // "Bought the [f000]ĉ\u0001\u0000\nfor $[f000]ȅ\u0001\u0001."
    SystemMsg Global10785_Text_Bought, 2
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    FunfestDispSalesmanMessage 39, 6, 2
    MsgWinCloseAll
    MoneyWinClose
    VMCall L_00E6
    WorkSetConst 0x8030, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    VMReturn

L_05B3:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0602
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05E8
    FunfestMissionBroadcast 40, 0
    // "Yay!\nYou lose."
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_YayLose, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_05E8:
    FunfestMissionBroadcast 34, 0
    // "Whaa. I lost...\nYou won![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_WhaaLostWon, 0, 0
    MsgWinCloseAll
    VMCall L_00E6
    VMReturn

L_0602:
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    // "Hi!\nPlay rock-paper-scissors![f000]븁\u0000\nRock, paper, scissors..."
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_HiPlayRockPaper, 2, 0
    WorkSetConst 0x8031, 1

L_0636:
    VMStackPush 0x8031
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0760
    Random 0x8035, 3
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32820
    ListMenuAdd 46, 65535, 0
    ListMenuAdd 47, 65535, 1
    ListMenuAdd 48, 65535, 2
    ListMenuShow
    MsgWinCloseAll
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0692
    // "Shoot!"
    ScreamMsg Global10785_Text_Shoot, 2
    VMJump L_0697

L_0692:
    // "Shoot!"
    ScreamMsg Global10785_Text_Shoot_2, 2

L_0697:
    WorkGet 0x8021, 0x8034
    WorkGet 0x8022, 0x8035
    VMCall L_0786
    InfoMsgClose_0039
    WorkCmpConst 0x8034, 0
    VMJumpIf CMP_EQ, L_06BE
    VMJump L_06CA

L_06BE:
    WorkSetConst 0x8036, 1
    VMJump L_0708

L_06CA:
    WorkCmpConst 0x8034, 2
    VMJumpIf CMP_EQ, L_06DD
    VMJump L_06E9

L_06DD:
    WorkSetConst 0x8036, 0
    VMJump L_0708

L_06E9:
    WorkCmpConst 0x8034, 1
    VMJumpIf CMP_EQ, L_06FC
    VMJump L_0708

L_06FC:
    WorkSetConst 0x8036, 2
    VMJump L_0708

L_0708:
    VMStackPush 0x8034
    VMStackPush 0x8035
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0731
    // "Rock, paper, scissors..."
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_RockPaperScissors, 2, 0
    WorkSetConst 0x8032, 1
    VMJump L_075A

L_0731:
    VMStackPush 0x8035
    VMStackPush 0x8036
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0752
    WorkSetConst 0x8020, 1
    VMReturn
    VMJump L_075A

L_0752:
    WorkSetConst 0x8020, 0
    VMReturn

L_075A:
    VMJump L_0636

L_0760:
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8031, 0
    VMReturn

L_0786:
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8039, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803c, 0
    WorkSetConst 0x803d, 0
    WorkGet 0x8038, 0x8021
    WorkAdd 0x8038, 46
    WorkGet 0x8039, 0x8022
    WorkAdd 0x8039, 46
    PlayerGetDir 0x8037
    WorkCmpConst 0x8037, 0
    VMJumpIf CMP_EQ, L_07DF
    VMJump L_07FD

L_07DF:
    WorkSetConst 0x803a, 8
    WorkSetConst 0x803b, 14
    WorkSetConst 0x803c, 22
    WorkSetConst 0x803d, 8
    VMJump L_0890

L_07FD:
    WorkCmpConst 0x8037, 1
    VMJumpIf CMP_EQ, L_0810
    VMJump L_082E

L_0810:
    WorkSetConst 0x803a, 22
    WorkSetConst 0x803b, 8
    WorkSetConst 0x803c, 8
    WorkSetConst 0x803d, 14
    VMJump L_0890

L_082E:
    WorkCmpConst 0x8037, 2
    VMJumpIf CMP_EQ, L_0841
    VMJump L_085F

L_0841:
    WorkSetConst 0x803a, 22
    WorkSetConst 0x803b, 8
    WorkSetConst 0x803c, 7
    WorkSetConst 0x803d, 8
    VMJump L_0890

L_085F:
    WorkCmpConst 0x8037, 3
    VMJumpIf CMP_EQ, L_0872
    VMJump L_0890

L_0872:
    WorkSetConst 0x803a, 7
    WorkSetConst 0x803b, 8
    WorkSetConst 0x803c, 22
    WorkSetConst 0x803d, 8
    VMJump L_0890

L_0890:
    MultiMsg 0x8038, 0x803a, 0x803b, 0
    MultiMsg 0x8039, 0x803c, 0x803d, 1
    VMSleep 16
    MsgWaitAdvance
    MsgWinCloseNo 0
    MsgWinCloseNo 1
    WorkSetConst 0x803d, 0
    WorkSetConst 0x803c, 0
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x8039, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8037, 0
    VMReturn

L_08DE:
    WorkSetConst 0x803e, 0
    WorkSetConst 0x803f, 0
    WorkSetConst 0x8040, 0
    WorkSetConst 0x8041, 0
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8043, 0
    WorkSetConst 0x8044, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "OK, here's the question![f000]븁\u0000\nPlease remember the names\nof these Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_OkHeresQuestionPlease, 2, 0
    FunfestGetPokemonQuizInfo 0x8040, 0x8041, 0x8042
    WorkSetConst 0x803f, 0

L_0926:
    VMStackPush 0x803f
    VMStackPush 0x8040
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0964
    FunfestGetPokemonQuizSpecies 0x803f, 0x8044
    WordSetPokeSpecies 0, 0x8044
    PVPlay 0x8044, 0
    // "[f000]ā\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_Empty_2, 2, 0
    PVWait
    MsgWaitAdvance
    WorkAdd 0x803f, 1
    VMJump L_0926

L_0964:
    WorkGet 0x8043, 0x8041
    WorkAdd 0x8043, 1
    WordSetNumber 6, 0x8043, 2
    // "Now!\nWhat is the name of the Pokémon[f000]븀\u0000\nin position [f000]ȁ\u0001\u0006?"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_NowWhatNamePokemon, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32830
    WorkSetConst 0x803f, 0

L_0990:
    VMStackPush 0x803f
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_09C2
    FunfestGetPokemonQuizBogusSpecies 0x803f, 0x8044
    WordSetPokeSpecies 1, 0x8044
    ListMenuAdd 57, 65535, 0x803f
    WorkAdd 0x803f, 1
    VMJump L_0990

L_09C2:
    ListMenuShow
    VMStackPush 0x803e
    VMStackPush 0x8042
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_09ED
    FunfestMissionBroadcast 41, 0
    // "Too bad! Incorrect!"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_TooBadIncorrect, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_09ED:
    FunfestMissionBroadcast 35, 0
    // "Correct!\nExcellent![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_CorrectExcellent, 2, 0
    // "Then, see you again somewhere...\nGood-bye![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_ThenSeeAgainSomewhere, 2, 0
    MsgWinCloseAll
    VMCall L_00E6
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8043, 0
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8041, 0
    WorkSetConst 0x8040, 0
    WorkSetConst 0x803f, 0
    WorkSetConst 0x803e, 0
    VMReturn

L_0A3B:
    WorkSetConst 0x8045, 0
    WorkSetConst 0x8046, 0
    WorkSetConst 0x8047, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    FunfestGetGenericInfo 0, 0x8046
    FunfestDispSalesmanMessage 67, 6, 0
    YesNoWin 0x8045
    VMStackPush 0x8045
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A86
    FunfestDispSalesmanMessage 68, 6, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_0A86:
    FunfestDispSalesmanMessage 69, 6, 0
    MsgWinCloseAll
    WorkSetConst 0x8047, 4
    WorkOr 0x8047, 1
    CallTrainerBattle 0x8046, 0, 0x8047
    CallTrainerBattleEnd
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ACD
    PokePartyRecoverAll
    FunfestDispSalesmanMessage 72, 6, 0
    VMJump L_0AE3

L_0ACD:
    FunfestMissionBroadcast 36, 0
    FunfestDispSalesmanMessage 70, 6, 0
    FunfestDispSalesmanMessage 71, 6, 0

L_0AE3:
    MsgWinCloseAll
    VMCall L_00E6
    WorkSetConst 0x8047, 0
    WorkSetConst 0x8046, 0
    WorkSetConst 0x8045, 0
    VMReturn

L_0AFF:
    WorkSetConst 0x8048, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello!\nIt's a present for Pokémon Trainers![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_HelloItsPresentPokemon, 0, 0
    FunfestGetGenericInfo 0, 0x8048
    ItemCheckSpace 0x8048, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B4E
    WordSetItemNameEx 0, 0x8048, 2, 0
    // "Oh, you don't have enough room\nfor [f000]ĉ\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_OhDontHaveEnough, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_0B4E:
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8048
    WorkSet 0x8001, 1
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "When tired, taking a rest is best for\nboth Pokémon and Trainers.[f000]븁\u0000\nPlease visit Pokémon Centers![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_WhenTiredTakingRest, 0, 0
    MsgWinCloseAll
    FunfestMissionBroadcast 37, 0x8048
    VMCall L_00E6
    WorkSetConst 0x8048, 0
    VMReturn

L_0B90:
    WorkSetConst 0x8049, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon Trainers!\nHello![f000]븁\u0000\nIt's a bit sudden, but I have a question.\nDo you know this Pokémon?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_PokemonTrainersHelloIts, 0, 0
    MsgWinCloseAll
    FunfestGetGenericInfo 0, 0x8049
    PVPlay 0x8049, 0
    CallPokemonPreview 0x8049, 0, 0, 0
    PVWait
    // "Please answer with the name of\nthis Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_PleaseAnswerNamePokemon, 0, 0
    MsgWinCloseAll
    CallWordSetPokeNameInput 0x8049, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0C26
    FunfestMissionBroadcast 41, 0
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C10
    // "Hmmm...\nWas it a bit difficult?"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_HmmmBitDifficult, 0, 0
    VMJump L_0C20

L_0C10:
    SEPlay SEQ_SE_FLD_42
    // "Ah, that's too bad!\nIt's not [f000]ā\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_AhThatsTooBad, 0, 0
    SEWait

L_0C20:
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_0C26:
    SEPlay SEQ_SE_FLD_41
    // "Correct!\nThe name is [f000]ā\u0001\u0000![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_CorrectName, 0, 0
    SEWait
    // "Then, see you again somewhere.\nBye![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10785_Text_ThenSeeAgainSomewhere_2, 0, 0
    MsgWinCloseAll
    FunfestMissionBroadcast 38, 0
    VMCall L_00E6
    WorkSetConst 0x8049, 0
    VMReturn
    .balign 4, 0
