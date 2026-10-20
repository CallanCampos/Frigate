#include "asm/field_script.inc"
#include "text/script/nacrene_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Lenora's research materials are stored\nin an orderly fashion."
    InfoMsg NacreneCity4_Text_LenorasResearchMaterialsStored, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The bones Lenora is using for research\nare on display."
    InfoMsg NacreneCity4_Text_BonesLenoraUsingResearch, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01bd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0073
    // "Some problems you can't solve even if you\nthink about them your whole life.[f000]븁\u0000\nSome problems have different answers\ndepending on the person.[f000]븁\u0000\nStill, the reason I can't keep my\ncuriosity down is this:[f000]븀\u0000\nI want to figure out the truth, but I[f000]븀\u0000\nalso have a desire for adventure!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_SomeProblemsCantSolve, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_020C

L_0073:
    // "So, what would you like to do?[f000]븁\u0000\nWhich do you like better,\nthe Cover Fossil or the Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_WhatWouldLikeWhich, 0, 2, 0

L_007F:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_020C
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 2, 65535, 0
    ListMenuAdd 3, 65535, 1
    ListMenuAdd 4, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_00C8
    VMJump L_013B

L_00C8:
    // "Do you really want to choose\nthe Cover Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_ReallyWantChooseCover, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0129
    // "The Cover Fossil!\nNow, that's a nice choice![f000]븁\u0000\nIf you want to restore it,\ngo to our reception counter![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_CoverFossilNowThats, 0, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 572
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01bd
    WorkSetConst 0x8021, 1
    VMJump L_0135

L_0129:
    // "Which do you like better,\nthe Cover Fossil or the Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_WhichLikeBetterCover, 0, 2, 0

L_0135:
    VMJump L_0206

L_013B:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_014E
    VMJump L_01C1

L_014E:
    // "Do you really want to choose\nthe Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_ReallyWantChoosePlume, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AF
    // "The Plume Fossil!\nNow, that's a nice choice![f000]븁\u0000\nIf you want to restore it,\ngo to our reception counter![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_PlumeFossilNowThats, 0, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 573
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01bd
    WorkSetConst 0x8021, 1
    VMJump L_01BB

L_01AF:
    // "Which do you like better,\nthe Cover Fossil or the Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_WhichLikeBetterCover, 0, 2, 0

L_01BB:
    VMJump L_0206

L_01C1:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_01D4
    VMJump L_01F0

L_01D4:
    // "Oh, come now![f000]븁\u0000\nDon't be shy!\nYou're too young to be bashful!"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_OhComeNowDont, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8021, 1
    VMJump L_0206

L_01F0:
    // "Oh, come now![f000]븁\u0000\nDon't be shy!\nYou're too young to be bashful!"
    ActorMsg MSGFILE_SCRIPT, NacreneCity4_Text_OhComeNowDont, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8021, 1

L_0206:
    VMJump L_007F

L_020C:
    VMStackPushFlag EVENT_FLAG_0x01bf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0223
    FlagSet EVENT_FLAG_0x01bf

L_0223:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
