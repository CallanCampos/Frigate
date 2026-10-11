#include "asm/field_script.inc"
#include "text/script/virbank_complex.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Virbank Complex\nWhere Fire Meets Steel"
    MsgPlaceSign VirbankComplex_Text_VirbankComplexWhereFire, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0102
    // "Yo! This is a good deal.[f000]븁\u0000\nWhy don't you trade your Poké Ball\nfor my Great Ball?"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex_Text_YoGoodDealWhy, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EE
    VMCall L_0116
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_007F
    VMJump L_008F

L_007F:
    // "Oh! Seriously? You must be kidding.\nDon't you have a Poké Ball?!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex_Text_OhSeriouslyMustKidding, 0, 0
    VMJump L_00E8

L_008F:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_00A2
    VMJump L_00B2

L_00A2:
    // "Oh! Seriously? You must be kidding.\nYou have way too many Great Balls!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex_Text_OhSeriouslyMustKidding_3, 0, 0
    VMJump L_00E8

L_00B2:
    // "Heh, thanks! Enjoy the Great Ball!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex_Text_HehThanksEnjoyGreat, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WordSetItemName 0, 4
    WordSetItemName 2, 3
    MEPlay SEQ_ME_ITEM
    // "Gave the [f000]ĉ\u0001\u0000 in exchange for\nthe [f000]ĉ\u0001\u0002!"
    SystemMsg VirbankComplex_Text_GaveExchange, 0
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "See? It's a good deal, isn't it?\nWe can trade again tomorrow if you want!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex_Text_SeeItsGoodDeal, 0, 0
    FlagSet EVENT_FLAG_DAILY_0x0acb

L_00E8:
    VMJump L_00F8

L_00EE:
    // "Oh! Seriously? You must be kidding.\nUsually people are happy to trade!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex_Text_OhSeriouslyMustKidding_2, 0, 0

L_00F8:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0110

L_0102:
    // "See? It's a good deal, isn't it?\nWe can trade again tomorrow if you want!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex_Text_SeeItsGoodDeal, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0110:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0116:
    ItemCheckAmount ITEM_POKE_BALL, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0139
    WorkSetConst 0x8020, 0
    VMReturn

L_0139:
    ItemCheckSpace ITEM_GREAT_BALL, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015C
    WorkSetConst 0x8020, 1
    VMReturn

L_015C:
    ItemSub ITEM_POKE_BALL, 1, 0x8010
    ItemAdd ITEM_GREAT_BALL, 1, 0x8010
    WorkSetConst 0x8020, 2
    VMReturn
