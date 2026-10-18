#include "asm/field_script.inc"
#include "text/script/global_10400.h"

    ScriptEntry Script_1
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
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8023, 1
    MoneyWinDisp 31, 1
    // "It's a vending machine.\nWhich drink would you like?"
    SystemMsg Global10400_Text_ItsVendingMachineWhich, 2
    WorkSetConst 0x8024, 0

L_004E:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028C
    ListMenu_AnchorTopRight 31, 5, 0x8024, 1, 32800
    ListMenuAdd 6, 65535, 1
    ListMenuAdd 7, 65535, 2
    ListMenuAdd 8, 65535, 3
    ListMenuAdd 9, 65535, 0
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B1
    // "Decided not to buy a drink."
    SystemMsg Global10400_Text_DecidedNotBuyDrink, 2
    WorkSetConst 0x8023, 0
    VMJump L_0157

L_00B1:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D6
    // "Decided not to buy a drink."
    SystemMsg Global10400_Text_DecidedNotBuyDrink, 2
    WorkSetConst 0x8023, 0
    VMJump L_0157

L_00D6:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_00E9
    VMJump L_0101

L_00E9:
    WorkSetConst 0x8021, 30
    WorkSetConst 0x8022, 200
    WorkSetConst 0x8024, 0
    VMJump L_0157

L_0101:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0114
    VMJump L_012C

L_0114:
    WorkSetConst 0x8021, 31
    WorkSetConst 0x8022, 300
    WorkSetConst 0x8024, 1
    VMJump L_0157

L_012C:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_013F
    VMJump L_0157

L_013F:
    WorkSetConst 0x8021, 32
    WorkSetConst 0x8022, 350
    WorkSetConst 0x8024, 2
    VMJump L_0157

L_0157:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0286
    ItemCheckSpace 0x8021, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A1
    WorkGet 0x8008, 0x8021
    RTCallGlobal 2804
    WorkSetConst 0x8025, 1
    WorkSetConst 0x8023, 0
    VMJump L_0286

L_01A1:
    MoneyCheck 0x8010, 0x8022
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027A
    VMSleep 10
    SEPlay SEQ_SE_FLD_06
    SEWait
    MoneySub 0x8022
    MoneyWinUpdate
    WordSetItemName 0, 0x8021
    // "A [f000]ĉ\u0001\u0000 dropped down![f000]븁\u0000"
    SystemMsg Global10400_Text_DroppedDown, 2
    InfoMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8021
    WorkSet 0x8001, 1
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    RecordAdd 125, 1
    ItemCheckSpace 0x8021, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026E
    WorkSetConst 0x8026, 0
    Random 0x8026, 32
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026E
    SEPlay SEQ_SE_FLD_06
    SEWait
    WordSetItemName 0, 0x8021
    // "Bonus! Another [f000]ĉ\u0001\u0000\ndropped down.[f000]븁\u0000"
    SystemMsg Global10400_Text_BonusAnotherDroppedDown, 2
    InfoMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8021
    WorkSet 0x8001, 1
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    MedalGive 52

L_026E:
    // "Would you like to buy another one?"
    SystemMsg Global10400_Text_WouldLikeBuyAnother, 2
    VMJump L_0286

L_027A:
    // "Not enough money..."
    SystemMsg Global10400_Text_NotEnoughMoney, 2
    WorkSetConst 0x8023, 0

L_0286:
    VMJump L_004E

L_028C:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A3
    LastKeyWait
    InfoMsgClose

L_02A3:
    MoneyWinClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
