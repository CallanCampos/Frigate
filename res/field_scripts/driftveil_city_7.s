#include "asm/field_script.inc"
#include "text/script/driftveil_city_7.h"

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
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are medicinal herbs that make\nPokémon healthy.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_ThereMedicinalHerbsMake, 0, 0, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 15
    WorkSet 0x8001, 0
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    MoneyWinDisp 31, 1
    // "Welcome! This is straight from the farm![f000]븁\u0000\nMoomoo Milk--one bottle for $500.\nWould you like some for your trip?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_WelcomeStraightFromFarm, 1, 4, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32801
    ListMenuAdd 8, 65535, 0
    ListMenuAdd 9, 65535, 1
    ListMenuAdd 10, 65535, 2
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E0
    WorkSetConst 0x8020, 1
    WorkSetConst 0x8022, 500
    VMJump L_0105

L_00E0:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0105
    WorkSetConst 0x8020, 12
    WorkSetConst 0x8022, 6000
    VMJump L_0105

L_0105:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0201
    ItemCheckSpace ITEM_MOOMOO_MILK, 0x8020, 0x8024
    MoneyCheck 0x8023, 0x8022
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0161
    MoneyWinClose
    // "Oh, my!\nYour Bag is full!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_OhBagFull, 1, 4, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01FB

L_0161:
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018C
    MoneyWinClose
    // "Oh, my!\nYou don't have enough money!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_OhDontHaveEnough, 1, 4, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01FB

L_018C:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x8022
    MoneyWinUpdate
    SEWait
    RecordAdd 21, 1
    RecordAdd 22, 0x8022
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CB
    // "Here it is! Your Moomoo Milk![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_HereMoomooMilk, 1, 4, 0
    MsgWinCloseAll
    VMJump L_01D9

L_01CB:
    // "A dozen! That's 12 bottles.\nHere it is, your Moomoo Milk![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_DozenThats12Bottles, 1, 4, 0
    MsgWinCloseAll

L_01D9:
    MoneyWinClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 33
    WorkSet 0x8001, 0x8020
    RTCallGlobal 2802
    VMStackPop 0x8001
    VMStackPop 0x8000

L_01FB:
    VMJump L_0213

L_0201:
    MoneyWinClose
    // "Please buy it next time![f000]븁\u0000\nJust so you know, our Moomoo Milk\nis straight from the farm.[f000]븁\u0000\nIt's super fresh!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_PleaseBuyNextTime, 1, 4, 0
    LastKeyWait
    MsgWinCloseAll

L_0213:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I used to be part of Team Plasma.\nI have various kinds of incense.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_UsedPartTeamPlasma, 3, 0, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 16
    WorkSet 0x8001, 0
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The medicinal herbs imported from\nanother region work very well![f000]븁\u0000\nBut Pokémon don't seem to like them,\nbecause they taste a little bitter."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_MedicinalHerbsImportedFrom, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, I'm sorry. I'm all sold out.[f000]븁\u0000\nThat Charles guy bought\neverything I had."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_OhImSorryIm, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There must be a convenient town where\neverything is imported and everything[f000]븀\u0000\nis available."
    // "Somewhere, there's a laid-back town\nwhere people value the year's seasons.[f000]븁\u0000\nI heard we've imported a lot of goods\nfrom that town!"
    ActorMsgVersioned 1024, DriftveilCity7_Text_ThereMustConvenientTown, DriftveilCity7_Text_SomewhereTheresLaidBack, 7, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Looking at seasonal vegetables is fun.[f000]븁\u0000\nBecause when the seasons change,\nthe vegetables available change!"
    // "Whenever we come to the market,\nvegetables of all seasons are available.[f000]븁\u0000\nIsn't that a marvel?"
    ActorMsgVersioned 1024, DriftveilCity7_Text_LookingSeasonalVegetablesFun, DriftveilCity7_Text_WheneverWeComeMarket, 9, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Goods carried away from Driftveil arrive\nin a town somewhere else...[f000]븁\u0000\nYeah. The world is connected."
    // "Goods carried away from a town\nsomewhere else arrive in Driftveil...[f000]븁\u0000\nYeah. The world is connected."
    ActorMsgVersioned 1024, DriftveilCity7_Text_GoodsCarriedAwayFrom, DriftveilCity7_Text_GoodsCarriedAwayFrom_2, 8, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0144
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_040E
    // "You! Glad you came!\nYou want something good?"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_GladCameWantSomething, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FA
    // "Then, show me a Pokémon Lv. 30 or more![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_ThenShowPokemonLv, 0, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    PokePartyGetCount 0x8025, 0

L_034F:
    VMStackPush 0x8025
    VMStackPush 0x8026
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_038F
    PokePartyGetParam 0x8027, 0x8026, 158
    VMStackPush 0x8027
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0383
    WorkAdd 0x8028, 1

L_0383:
    WorkAdd 0x8026, 1
    VMJump L_034F

L_038F:
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_03E6
    // "Oh! Strong![f000]븁\u0000\nYou, great!\nTake this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_OhStrongGreatTake, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 268
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "You! Glad you came!\nGet good at Expert Belts![f000]븁\u0000\nIf a Pokémon has an Expert Belt, the\npower of its moves is slightly boosted[f000]븀\u0000\nwhen they are super effective!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_GladCameGetGood, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0144
    VMJump L_03F4

L_03E6:
    // "Oh! You--not so good yet![f000]븁\u0000\nAfter training Pokémon,\ncome back here again!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_OhNotGoodYet, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03F4:
    VMJump L_0408

L_03FA:
    // "You, modest!\nYou should just want something good!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_ModestShouldJustWant, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0408:
    VMJump L_041C

L_040E:
    // "You! Glad you came!\nGet good at Expert Belts![f000]븁\u0000\nIf a Pokémon has an Expert Belt, the\npower of its moves is slightly boosted[f000]븀\u0000\nwhen they are super effective!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity7_Text_GladCameGetGood, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_041C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
