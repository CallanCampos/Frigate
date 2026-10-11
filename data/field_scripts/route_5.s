#include "asm/field_script.inc"
#include "text/script/route_5.h"

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
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_22:
    VMCall L_00F6
    VMHalt

Script_17:
    WorkSetConst 0x8025, 0
    Cmd_02B2 0, 0x8025
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C9
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C9
    VMStackPushFlag EVENT_FLAG_0x01b6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C9
    FlagReset EVENT_FLAG_0x03d2

L_00C9:
    WorkSetConst 0x8025, 0
    VMCall L_00D7
    VMHalt

L_00D7:
    Cmd_02B2 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F4
    FlagSet EVENT_FLAG_0x03d2

L_00F4:
    VMReturn

L_00F6:
    Cmd_02B2 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012A
    VMStackPushFlag EVENT_FLAG_0x03d2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0126
    ActorDelete 21

L_0126:
    FlagSet EVENT_FLAG_0x03d2

L_012A:
    VMReturn

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 5\nPerformer Street"
    MsgPlaceSign Route5_Text_Route5PerformerStreet, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Driftveil Drawbridge"
    MsgPlaceSign Route5_Text_DriftveilDrawbridge, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nThere are different Cases\nfor each type of item.[f000]븁\u0000\nItems are placed automatically in\nthe correct Case by their type.[f000]븁\u0000\nThe name of the Case tells you\nwhat type of items will be kept there.[f000]븁\u0000\nAlso, you can place anything in\nFree Space, no matter what it is.[f000]븁\u0000\nSo you can keep items you often use\nin one place."
    MsgPlaceSign Route5_Text_TrainerTipsThereDifferent, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a6c
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_KAIDAN
    ActorNew 377, 438, 2, 251, 249, 0
    SEWait
    // "Bianca: Heeey!"
    // "Bianca: Hey!"
    ActorMsgGendered 1024, Route5_Text_BiancaHeeey, Route5_Text_BiancaHey, 251, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0E58
    ActorCmdWait
    PlayerGetGPos 0x8020, 0x8021
    ActorCmdExec 251, Movement_0418
    VMSleep 8
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E0
    PlayerSetSpecialSequence 1

L_01E0:
    BGMPlay SEQ_BGM_E_BERU
    VMStackPush 0x8021
    VMStackPushConst 438
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0211
    ActorWalkRoute 255, 372, 438, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_0E08
    ActorCmdWait

L_0211:
    VMStackPush 0x8021
    VMStackPushConst 438
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_022E
    ActorCmdExec 255, Movement_0E08
    ActorCmdWait

L_022E:
    // "Nice timing!\nI was wanting to give you this!"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_NiceTimingWantingGive, 251, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 421
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Ta-da![f000]븁\u0000\nIt's a Hidden Machine, Fly![f000]븁\u0000\nWhen you use this move outside of battle,\nyou can go to places you want to go,[f000]븀\u0000\nlike a Pokémon Center.[f000]븁\u0000\nBy the way, [f000]Ā\u0001\u0000,\ndo you know about Hidden Grottoes?"
    // "Ta-da![f000]븁\u0000\nIt's a Hidden Machine, Fly![f000]븁\u0000\nWhen you use this move outside of battle,\nyou can go to places you want to go,[f000]븀\u0000\nlike a Pokémon Center.[f000]븁\u0000\nBy the way, [f000]Ā\u0001\u0000,\ndo you know about Hidden Grottoes?"
    ActorMsgGendered 1024, Route5_Text_TaDaItsHidden, Route5_Text_TaDaItsHidden_2, 251, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0295
    // "Great!\nYou might find one soon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_GreatMightFindOne, 251, 0, 0
    VMJump L_02A1

L_0295:
    // "OK! Then I'll explain![f000]븁\u0000\nSometimes you can find a grotto\namong trees where Pokémon[f000]븀\u0000\nlike to hide.[f000]븁\u0000\nThat place is called a Hidden Grotto.\nMakes sense, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_OkThenIllExplain, 251, 0, 0

L_02A1:
    MsgWinCloseAll
    VMSleep 30
    ActorCmdExec 251, Movement_0E58
    ActorCmdWait
    ActorCmdExec 251, Movement_0DF0
    ActorCmdWait
    // "Wait![f000]븁\u0000\n...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_Wait, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 372, 436, 1, 8, 1
    VMSleep 15
    ActorCmdExec 255, Movement_0DF0
    ActorCmdWait
    ActorCmdExec 251, Movement_0420
    ActorCmdWait
    VMSleep 20
    ActorCmdExec 251, Movement_0DF8
    ActorCmdWait
    // "Over there![f000]븁\u0000\nI heard something from that direction![f000]븁\u0000\nI have good ears.\nHey! Come with me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_OverThereHeardSomething, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_04B8
    VMSleep 5
    ActorWalkRoute 251, 369, 425, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 251, Movement_0438
    ActorCmdWait
    ActorWalkRoute 251, 374, 425, 1, 8, 0
    VMSleep 10
    ActorWalkRoute 255, 373, 425, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 251, Movement_045C
    ActorCmdWait
    // "The sound is coming from\nsomewhere around here.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_SoundComingFromSomewhere, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_047C
    ActorCmdWait
    // "Wow! Here it is![f000]븁\u0000\nThere's a gap, and it looks like\nwe can fit through![f000]븁\u0000\nC'mon! Let's go have a look![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_WowHereTheresGap, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0DF0
    ActorCmdWait
    // "Look!\nYou've found a narrow path![f000]븁\u0000\nWill you follow it?"
    SystemMsg Route5_Text_LookYouveFoundNarrow, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E8
    InfoMsgClose
    ActorCmdExec 251, Movement_0E00
    VMSleep 8
    ActorCmdExec 255, Movement_0E08
    ActorCmdWait
    // "This is too good to pass up! Let's go in![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_TooGoodPassUp, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0DF0
    ActorCmdExec 255, Movement_0DF0
    ActorCmdWait
    VMJump L_03EA

L_03E8:
    InfoMsgClose

L_03EA:
    HiddenHollowSet 1, 0, 0, 0
    RTReserveScript 6
    SEPlay SEQ_SE_KAIDAN
    HiddenHollowCallWarpIn 1
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x4139, 1
    MedalDiscover 57
    MedalDiscover 85
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0418:
    Move 14, 4
    MoveEnd

Movement_0420:
    Move 35, 1
    Move 62, 1
    Move 34, 1
    Move 62, 1
    Move 32, 1
    MoveEnd

Movement_0438:
    Move 34, 1
    Move 62, 1
    Move 35, 1
    Move 62, 1
    Move 32, 1
    Move 62, 1
    Move 35, 1
    Move 75, 1
    MoveEnd

Movement_045C:
    Move 30, 1
    Move 62, 1
    Move 31, 1
    Move 62, 1
    Move 29, 1
    Move 62, 1
    Move 159, 1
    MoveEnd

Movement_047C:
    Move 11, 1
    Move 62, 1
    Move 28, 1
    Move 62, 1
    Move 31, 1
    Move 62, 1
    Move 29, 1
    Move 62, 1
    Move 10, 1
    Move 29, 1
    Move 62, 1
    Move 28, 1
    Move 62, 1
    Move 75, 1
    MoveEnd

Movement_04B8:
    Move 12, 2
    Move 14, 3
    Move 12, 10
    MoveEnd

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_DAILY_0x0abe
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E3
    Random EVENT_WORK_0x4175, 5

L_04E3:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0abf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0508
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0522
    VMJump L_051C

L_0508:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'll keep gathering Berries. Come back\ntomorrow if you want more!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_IllKeepGatheringBerries, 0, 0
    LastKeyWait
    ActorMsgClose

L_051C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0522:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8026, 200
    MoneyWinDisp 31, 1
    WorkCmpConst EVENT_WORK_0x4175, 0
    VMJumpIf CMP_EQ, L_0553
    VMJump L_0561

L_0553:
    WordSetItemNameEx 0, ITEM_POMEG_BERRY, 2, 0
    VMJump L_0606

L_0561:
    WorkCmpConst EVENT_WORK_0x4175, 1
    VMJumpIf CMP_EQ, L_0574
    VMJump L_0582

L_0574:
    WordSetItemNameEx 0, ITEM_KELPSY_BERRY, 2, 0
    VMJump L_0606

L_0582:
    WorkCmpConst EVENT_WORK_0x4175, 2
    VMJumpIf CMP_EQ, L_0595
    VMJump L_05A3

L_0595:
    WordSetItemNameEx 0, ITEM_QUALOT_BERRY, 2, 0
    VMJump L_0606

L_05A3:
    WorkCmpConst EVENT_WORK_0x4175, 3
    VMJumpIf CMP_EQ, L_05B6
    VMJump L_05C4

L_05B6:
    WordSetItemNameEx 0, ITEM_HONDEW_BERRY, 2, 0
    VMJump L_0606

L_05C4:
    WorkCmpConst EVENT_WORK_0x4175, 4
    VMJumpIf CMP_EQ, L_05D7
    VMJump L_05E5

L_05D7:
    WordSetItemNameEx 0, ITEM_GREPA_BERRY, 2, 0
    VMJump L_0606

L_05E5:
    WorkCmpConst EVENT_WORK_0x4175, 5
    VMJumpIf CMP_EQ, L_05F8
    VMJump L_0606

L_05F8:
    WordSetItemNameEx 0, ITEM_TAMATO_BERRY, 2, 0
    VMJump L_0606

L_0606:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0abe
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_062B
    // "Hi, Trainer! Will you buy the Berries\nI gathered?[f000]븁\u0000\nFive [f000]ĉ\u0001\u0000 for just $200!\nYou want them. You'll buy them, right?"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_HiTrainerWillBuy, 0, 2, 0
    VMJump L_0637

L_062B:
    // "I'll sell you five [f000]ĉ\u0001\u0000\nfor just $200.[f000]븀\u0000\nYou want them. You'll buy them, right?"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_IllSellFiveJust, 0, 2, 0

L_0637:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08FE
    WorkCmpConst EVENT_WORK_0x4175, 0
    VMJumpIf CMP_EQ, L_0661
    VMJump L_066F

L_0661:
    ItemCheckSpace ITEM_POMEG_BERRY, 5, 0x8028
    VMJump L_0714

L_066F:
    WorkCmpConst EVENT_WORK_0x4175, 1
    VMJumpIf CMP_EQ, L_0682
    VMJump L_0690

L_0682:
    ItemCheckSpace ITEM_KELPSY_BERRY, 5, 0x8028
    VMJump L_0714

L_0690:
    WorkCmpConst EVENT_WORK_0x4175, 2
    VMJumpIf CMP_EQ, L_06A3
    VMJump L_06B1

L_06A3:
    ItemCheckSpace ITEM_QUALOT_BERRY, 5, 0x8028
    VMJump L_0714

L_06B1:
    WorkCmpConst EVENT_WORK_0x4175, 3
    VMJumpIf CMP_EQ, L_06C4
    VMJump L_06D2

L_06C4:
    ItemCheckSpace ITEM_HONDEW_BERRY, 5, 0x8028
    VMJump L_0714

L_06D2:
    WorkCmpConst EVENT_WORK_0x4175, 4
    VMJumpIf CMP_EQ, L_06E5
    VMJump L_06F3

L_06E5:
    ItemCheckSpace ITEM_GREPA_BERRY, 5, 0x8028
    VMJump L_0714

L_06F3:
    WorkCmpConst EVENT_WORK_0x4175, 5
    VMJumpIf CMP_EQ, L_0706
    VMJump L_0714

L_0706:
    ItemCheckSpace ITEM_TAMATO_BERRY, 5, 0x8028
    VMJump L_0714

L_0714:
    MoneyCheck 0x8027, 0x8026
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0745
    MoneyWinClose
    // "Oh! But your Bag is full!"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_OhButBagFull, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08F8

L_0745:
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0770
    MoneyWinClose
    // "Oh! But you don't have enough money."
    ActorMsg MSGFILE_SCRIPT, Route5_Text_OhButDontHave, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08F8

L_0770:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x8026
    MoneyWinUpdate
    SEWait
    // "Five [f000]ĉ\u0001\u0000 for $200!\nYou're a smart shopper![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_Five200YoureSmart, 0, 2, 0
    MsgWinCloseAll
    MoneyWinClose
    WorkCmpConst EVENT_WORK_0x4175, 0
    VMJumpIf CMP_EQ, L_079F
    VMJump L_07C5

L_079F:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 169
    WorkSet 0x8001, 5
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_08E2

L_07C5:
    WorkCmpConst EVENT_WORK_0x4175, 1
    VMJumpIf CMP_EQ, L_07D8
    VMJump L_07FE

L_07D8:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 170
    WorkSet 0x8001, 5
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_08E2

L_07FE:
    WorkCmpConst EVENT_WORK_0x4175, 2
    VMJumpIf CMP_EQ, L_0811
    VMJump L_0837

L_0811:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 171
    WorkSet 0x8001, 5
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_08E2

L_0837:
    WorkCmpConst EVENT_WORK_0x4175, 3
    VMJumpIf CMP_EQ, L_084A
    VMJump L_0870

L_084A:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 172
    WorkSet 0x8001, 5
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_08E2

L_0870:
    WorkCmpConst EVENT_WORK_0x4175, 4
    VMJumpIf CMP_EQ, L_0883
    VMJump L_08A9

L_0883:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 173
    WorkSet 0x8001, 5
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_08E2

L_08A9:
    WorkCmpConst EVENT_WORK_0x4175, 5
    VMJumpIf CMP_EQ, L_08BC
    VMJump L_08E2

L_08BC:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 174
    WorkSet 0x8001, 5
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_08E2

L_08E2:
    // "I'll keep gathering Berries. Come back\ntomorrow if you want more!"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_IllKeepGatheringBerries, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    MoneyWinClose
    FlagSet EVENT_FLAG_DAILY_0x0abf

L_08F8:
    VMJump L_0910

L_08FE:
    MoneyWinClose
    // "Boo!\nAnd I went to all that trouble[f000]븀\u0000\nto gather Berries."
    ActorMsg MSGFILE_SCRIPT, Route5_Text_BooWentAllTrouble, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0910:
    FlagSet EVENT_FLAG_DAILY_0x0abe
    VMReturn

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8024, 3
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0959
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nNow, I'm having a Triple Battle\nwith the opponent in front of me!"
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nNow, I'm having a Rotation Battle\nwith the opponent in front of me!"
    ActorMsgVersioned 1024, Route5_Text_ImHeartbreakerNameCharles, Route5_Text_ImHeartbreakerNameCharles_6, 8, 0, 0
    LastKeyWait
    ActorMsgClose
    ActorCmdExec 8, Movement_0E48
    ActorCmdWait
    VMJump L_09D3

L_0959:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0119
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09CD
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI wanted to get the attention of a girl\nI like, so I mastered a new style of[f000]븀\u0000\nPokémon battling called Triple Battle.[f000]븁\u0000\nWant to learn about it?"
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI wanted to get the attention of a girl\nI like, so I mastered a new style of[f000]븀\u0000\nPokémon battling called Rotation Battle.[f000]븁\u0000\nWant to learn about it?"
    ActorMsgVersioned 1024, Route5_Text_ImHeartbreakerNameCharles_2, Route5_Text_ImHeartbreakerNameCharles_7, 8, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09B5
    FlagSet EVENT_FLAG_0x0119
    // "In Triple Battles, you send out three\nPokémon at a time and battle![f000]븁\u0000\nThe rules are simple: just make all of\nyour opponent's Pokémon faint.[f000]븁\u0000\nAnd that's a rough explanation\nof Triple Battles.[f000]븁\u0000"
    // "In Rotation Battles, you send out three\nPokémon at a time and battle![f000]븁\u0000\nOne Pokémon takes the lead position,\nand the other two stand on each side.[f000]븁\u0000\nThe trick is, each turn you can change\ntheir positions...[f000]븁\u0000\nAnd that's a rough explanation\nof Rotation Battles.[f000]븁\u0000"
    ActorMsgVersioned 1024, Route5_Text_TripleBattlesSendOut, Route5_Text_RotationBattlesSendOut, 8, 0, 0
    VMCall L_09D9
    VMJump L_09C7

L_09B5:
    // "Oh, man! Getting someone's attention is\nreally hard."
    // "Oh, man! Getting someone's attention is\nreally hard."
    ActorMsgVersioned 1024, Route5_Text_OhManGettingSomeones, Route5_Text_OhManGettingSomeones_2, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_09C7:
    VMJump L_09D3

L_09CD:
    VMCall L_09D9

L_09D3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_09D9:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nHey! If you're a Trainer, how about a\nTriple Battle?"
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nHey! If you're a Trainer, how about a\nRotation Battle?"
    ActorMsgVersioned 1024, Route5_Text_ImHeartbreakerNameCharles_3, Route5_Text_ImHeartbreakerNameCharles_8, 8, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C28
    WorkSetConst 0x8029, 0
    PokePartyGetCount 0x8029, 2
    VMStackPush 0x8029
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0A35
    // "I hate to burst your bubble when you're\nall fired up, but...[f000]븁\u0000\nIn Triple Battles, you need three or\nmore Pokémon to battle."
    // "I hate to burst your bubble when you're\nall fired up, but...[f000]븁\u0000\nIn Rotation Battles, you need three or\nmore Pokémon to battle."
    ActorMsgVersioned 1024, Route5_Text_HateBurstBubbleWhen, Route5_Text_HateBurstBubbleWhen_2, 8, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0C22

L_0A35:
    // "You've got a good attitude, don't you![f000]븁\u0000\nI'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI'm always at full throttle.[f000]븁\u0000"
    // "You've got a good attitude, don't you![f000]븁\u0000\nI'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI'm always at full throttle.[f000]븁\u0000"
    ActorMsgVersioned 1024, Route5_Text_YouveGotGoodAttitude, Route5_Text_YouveGotGoodAttitude_2, 8, 0, 0
    ActorMsgClose
    VMCall L_0C42
    GameGetVersion 0x8023
    WorkCmpConst 0x8023, 22
    VMJumpIf CMP_EQ, L_0A62
    VMJump L_0A70

L_0A62:
    CallTrainerBattle TRAINER_MOTORCYCLIST_CHARLES_2, 0, 0
    VMJump L_0A91

L_0A70:
    WorkCmpConst 0x8023, 23
    VMJumpIf CMP_EQ, L_0A83
    VMJump L_0A91

L_0A83:
    CallTrainerBattle TRAINER_MOTORCYCLIST_CHARLES, 0, 0
    VMJump L_0A91

L_0A91:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ACE
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1558000, 0, 0x1b18000, 1
    EvCameraWait
    CallTrainerBattleEnd
    VMJump L_0AD0

L_0ACE:
    CallTrainerLose

L_0AD0:
    // "Sheesh. That's embarrassing. Getting\nschooled when I was planning to teach.[f000]븁\u0000\nStill, you have potential![f000]븁\u0000\nYou have to understand your Pokémon\nto win in a Triple Battle.[f000]븁\u0000"
    // "Sheesh. That's embarrassing. Getting\nschooled when I was planning to teach.[f000]븁\u0000\nStill, you have potential![f000]븁\u0000\nYou have to understand your Pokémon\nto win in a Rotation Battle.[f000]븁\u0000"
    ActorMsgVersioned 1024, Route5_Text_SheeshThatsEmbarrassingGetting, Route5_Text_SheeshThatsEmbarrassingGetting_2, 8, 0, 0
    ActorMsgClose
    VMCall L_0D4C
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nRiding a bike and becoming the wind fits\na bad boy like me.[f000]븁\u0000"
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nRiding a bike and becoming the wind fits a\nbad boy like me.[f000]븁\u0000"
    ActorMsgVersioned 1024, Route5_Text_ImHeartbreakerNameCharles_5, Route5_Text_ImHeartbreakerNameCharles_10, 8, 0, 0
    ActorMsgClose
    VMCall L_0DB6
    VMSleep 30
    EvCameraReturn 20
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 9, Movement_0E20
    VMSleep 8
    ActorCmdExec 255, Movement_0DF8
    ActorCmdExec 13, Movement_0E08
    ActorCmdExec 14, Movement_0E08
    ActorCmdExec 15, Movement_0E08
    ActorCmdExec 16, Movement_0E08
    ActorCmdExec 17, Movement_0E08
    ActorCmdWait
    // "You were great![f000]븁\u0000\nCharles, too.\nHe was great to some extent, I guess![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route5_Text_WereGreatCharlesToo, 9, 0, 0
    MsgWinCloseAll
    MultiMsg 68, 3, 3, 1
    VMSleep 10
    MultiMsg 69, 20, 13, 2
    VMSleep 10
    MultiMsg 70, 5, 20, 3
    VMSleep 30
    MsgWinCloseNo 1
    VMSleep 5
    MsgWinCloseNo 2
    VMSleep 5
    MsgWinCloseNo 3
    ActorCmdExec 9, Movement_0EAC
    ActorCmdExec 10, Movement_0EBC
    ActorCmdExec 11, Movement_0ECC
    ActorCmdExec 12, Movement_0ED8
    ActorCmdExec 13, Movement_0EE4
    ActorCmdExec 14, Movement_0EEC
    ActorCmdExec 15, Movement_0EF4
    ActorCmdExec 16, Movement_0EFC
    VMSleep 30
    FadeEx 3, 0, 16, 4
    ActorCmdWait
    FadeExWait
    ActorDelete 8
    ActorDelete 9
    ActorDelete 10
    ActorDelete 11
    ActorDelete 12
    ActorDelete 13
    ActorDelete 14
    ActorDelete 15
    ActorDelete 16
    ActorDelete 17
    FlagSet EVENT_FLAG_0x02f8
    VMSleep 30
    FadeEx 3, 16, 0, 4
    FadeExWait

L_0C22:
    VMJump L_0C3A

L_0C28:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI have some advice for you.\nChallenge is the essence of life!"
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI have some advice for you.[f000]븁\u0000\nChallenge is the essence of life!"
    ActorMsgVersioned 1024, Route5_Text_ImHeartbreakerNameCharles_4, Route5_Text_ImHeartbreakerNameCharles_9, 8, 0, 0
    LastKeyWait
    ActorMsgClose

L_0C3A:
    WorkSetConst 0x8029, 0
    VMReturn

L_0C42:
    ActorCmdExec 9, Movement_0EA4
    ActorCmdExec 10, Movement_0E18
    ActorCmdExec 11, Movement_0E40
    ActorCmdExec 12, Movement_0E40
    ActorCmdExec 13, Movement_0E48
    ActorCmdExec 14, Movement_0E48
    ActorCmdExec 15, Movement_0E48
    ActorCmdExec 16, Movement_0E48
    ActorCmdWait
    ActorCmdExec 9, Movement_0DF0
    ActorCmdExec 10, Movement_0DF0
    ActorCmdExec 17, Movement_0E48
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1558000, 0, 0x1b18000, 20
    PlayerGetDir 0x8022
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_0CEB
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_0CEB
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_0CEB
    VMJump L_0CFF

L_0CEB:
    ActorWalkRoute 255, 342, 433, 0, 8, 0
    VMJump L_0D30

L_0CFF:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_0D12
    VMJump L_0D30

L_0D12:
    ActorCmdExec 255, Movement_0E18
    ActorCmdWait
    ActorWalkRoute 255, 342, 433, 0, 8, 0
    VMJump L_0D30

L_0D30:
    VMSleep 8
    ActorCmdExec 8, Movement_0E48
    ActorCmdWait
    ActorCmdExec 255, Movement_0E00
    ActorCmdWait
    EvCameraWait
    VMReturn

L_0D4C:
    ActorWalkRoute 8, 338, 433, 0, 6, 0
    VMSleep 8
    ActorCmdExec 13, Movement_0E38
    ActorCmdExec 14, Movement_0E20
    ActorCmdExec 15, Movement_0E18
    ActorCmdExec 16, Movement_0E40
    ActorCmdWait
    ActorCmdExec 14, Movement_0E38
    ActorCmdExec 15, Movement_0E40
    ActorCmdExec 17, Movement_0E40
    ActorCmdWait
    EvCameraMoveTo 9688, 0, 0xed000, 0x1528000, 0, 0x1b18000, 40
    EvCameraWait
    VMReturn

L_0DB6:
    ActorCmdExec 8, Movement_0E78
    VMSleep 18
    ActorCmdExec 13, Movement_0E50
    ActorCmdExec 14, Movement_0E50
    ActorCmdExec 15, Movement_0E50
    ActorCmdExec 16, Movement_0E50
    ActorCmdExec 17, Movement_0E50
    ActorCmdWait
    VMReturn
    .balign 4, 0

Movement_0DF0:
    Move 32, 1
    MoveEnd

Movement_0DF8:
    Move 33, 1
    MoveEnd

Movement_0E00:
    Move 34, 1
    MoveEnd

Movement_0E08:
    Move 35, 1
    MoveEnd

Movement_0E10:
    Move 38, 3
    MoveEnd

Movement_0E18:
    Move 13, 1
    MoveEnd

Movement_0E20:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_0E38:
    Move 1, 1
    MoveEnd

Movement_0E40:
    Move 0, 1
    MoveEnd

Movement_0E48:
    Move 3, 1
    MoveEnd

Movement_0E50:
    Move 2, 1
    MoveEnd

Movement_0E58:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd

Movement_0E78:
    Move 10, 1
    Move 14, 1
    Move 22, 8
    MoveEnd
    VMStackAdd
    VMNop2
    VMStackMul
    VMReturn
    .byte 0xfe
    .balign 4, 0
    Move 14, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0EA4:
    Move 13, 2
    MoveEnd

Movement_0EAC:
    Move 15, 1
    Move 33, 1
    Move 15, 4
    MoveEnd

Movement_0EBC:
    Move 63, 1
    Move 32, 1
    Move 15, 5
    MoveEnd

Movement_0ECC:
    Move 13, 1
    Move 15, 5
    MoveEnd

Movement_0ED8:
    Move 34, 1
    Move 15, 5
    MoveEnd

Movement_0EE4:
    Move 15, 8
    MoveEnd

Movement_0EEC:
    Move 14, 6
    MoveEnd

Movement_0EF4:
    Move 14, 6
    MoveEnd

Movement_0EFC:
    Move 14, 6
    MoveEnd

Script_6:
    ActorsPauseAll
    TrainerCardHasBadge 0x8024, 3
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F41
    SEPlay SEQ_SE_MESSAGE
    ActorCmdExec 9, Movement_0E10
    ActorCmdWait
    // "Am I going to lose to the heartbreaker?\nI have a girlfriend!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_AmGoingLoseHeartbreaker, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0F5F

L_0F41:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 9, Movement_0E58
    ActorCmdWait
    // "You have four Gym Badges...\nYou might be able to defeat Charles![f000]븁\u0000\nPlease beat him for me!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_HaveFourGymBadges, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0F5F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Because it's a battle, one person has\nto win and one person has to lose.[f000]븁\u0000\nI wish my boyfriend could just enjoy\nthe battle and not worry about losing.[f000]븀\u0000\nSometimes he gets too serious."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_BecauseItsBattleOne, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There seems to be a lot to think about,\nbut it looks really fun!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_ThereSeemsLotThink, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Battling with three Pokémon.\nThat itself makes me very excited!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_BattlingThreePokemonItself, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Triple Battle and\nRotation Battle...[f000]븁\u0000\nThere are various styles of\nPokémon battling."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_TripleBattleRotationBattle, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Trainer called Charles\nis quite tough to beat.[f000]븁\u0000\nIf you have four Gym Badges,\nyou might be a match for him."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_TrainerCalledCharlesQuite, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are so many different Pokémon,\nit's difficult to decide which ones[f000]븀\u0000\nto battle with."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_ThereManyDifferentPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are a lot of Pokémon.\nSo I like this!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_ThereLotPokemonLike, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Looking at this kind of battle\nmakes me curious about the[f000]븀\u0000\nPokémon World Tournament.[f000]븀\u0000\nI hear it will be held in Driftveil City."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_LookingKindBattleMakes, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The drawbridge goes up\nwhen a ship needs to pass.[f000]븁\u0000\nThen, the Pokémon that are resting\non the bridge fly away all at once!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_DrawbridgeGoesUpWhen, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01b3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1090
    // "Cheren: I heard that you met\na Pokémon Trainer called N.[f000]븁\u0000\nThat's not really why I'm here, though.\nWill you battle me?"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_CherenHeardMetPokemon, 0, 0
    FlagSet EVENT_FLAG_0x01b3
    VMJump L_109A

L_1090:
    // "Cheren: There's something I want\nto check by having a battle with you.[f000]븁\u0000\nHow about it?"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_CherenTheresSomethingWant, 0, 0

L_109A:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11DA
    Cmd_02B5 0, 1
    // "You remind me of [f000]Ā\u0001\u0001.\nThat makes me excited about[f000]븀\u0000\nthis Pokémon battle![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_RemindMakesExcitedAbout, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    Cmd_02B3 0, 0x802a
    WorkCmpConst 0x802a, 0
    VMJumpIf CMP_EQ, L_10E8
    VMJump L_10F4

L_10E8:
    WorkSetConst 0x802b, 709
    VMJump L_1132

L_10F4:
    WorkCmpConst 0x802a, 1
    VMJumpIf CMP_EQ, L_1107
    VMJump L_1113

L_1107:
    WorkSetConst 0x802b, 710
    VMJump L_1132

L_1113:
    WorkCmpConst 0x802a, 2
    VMJumpIf CMP_EQ, L_1126
    VMJump L_1132

L_1126:
    WorkSetConst 0x802b, 711
    VMJump L_1132

L_1132:
    CallTrainerBattle 0x802b, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1159
    CallTrainerBattleEnd
    VMJump L_115B

L_1159:
    CallTrainerLose

L_115B:
    Cmd_02B5 0, 1
    ActorCmdExec 21, Movement_0DF8
    ActorCmdWait
    // "I talked with [f000]Ā\u0001\u0001 here before.[f000]븁\u0000\nWe all have our own brand of strength,\npeople and Pokémon both.[f000]븁\u0000\nThe strength to make our dreams\na reality, the strength to protect[f000]븀\u0000\nwhat we hold most dear...[f000]븀\u0000\nThat's what I said.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_TalkedHereBeforeWe, 0, 0
    // "I was thinking about how I hope N can see\nthe same thing...but during our battle,[f000]븀\u0000\nI came to understand something.[f000]븁\u0000\nDuring his journey, N also saw\nwhat we saw.[f000]븁\u0000\nIf I tell this story, [f000]Ā\u0001\u0001\nwill probably be happy.[f000]븁\u0000\nThank you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_ThinkingAboutHowHope, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11AC
    ActorWalkRoute 21, 370, 438, 0, 8, 1
    VMJump L_11BA

L_11AC:
    ActorWalkRoute 21, 370, 439, 0, 8, 0

L_11BA:
    VMSleep 20
    ActorCmdExec 255, Movement_0E08
    ActorCmdWait
    ActorDelete 21
    FlagSet EVENT_FLAG_0x03d2
    FlagSet EVENT_FLAG_0x01b6
    VMJump L_11E8

L_11DA:
    // "Not interested?[f000]븁\u0000\nWell, I'll be here, so when you change\nyour mind, please come battle me."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_NotInterestedWellIll, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_11E8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    ItemCheckAmount ITEM_PROP_CASE, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12D8
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerFlagGet TRAINER_MUSICIAN_PRESTON, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12C4
    TrainerBGMPlayPush TRAINER_MUSICIAN_PRESTON
    // "Hum fiercely! My battle song!\nBattle fiercely! My Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_HumFiercelyBattleSong_2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_MUSICIAN_PRESTON, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1265
    TrainerFlagSet TRAINER_MUSICIAN_PRESTON
    CallTrainerBattleEnd
    VMJump L_1267

L_1265:
    CallTrainerLose

L_1267:
    MusicalIsPropOwned 84, EVENT_WORK_0x4001
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12B0
    // "In battling you, I came to understand...[f000]븁\u0000\nYou're the best!\nHere! This is for you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_BattlingCameUnderstandYoure, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8008, 84
    WorkSetConst 0x8009, 1
    RTCallGlobal 10466
    // "If you want your Pokémon to\nhold this Electric Guitar,[f000]븀\u0000\ngo to the Musical Theater![f000]븀\u0000\nCooler than cool!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_IfWantPokemonHold, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_12BE

L_12B0:
    // "In battling you, I came to understand...[f000]븁\u0000\nYou're the best![f000]븁\u0000\nAfter all, you have the\nElectric Guitar Prop!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_BattlingCameUnderstandYoure_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_12BE:
    VMJump L_12D2

L_12C4:
    // "If you want your Pokémon to\nhold this Electric Guitar,[f000]븀\u0000\ngo to the Musical Theater![f000]븀\u0000\nCooler than cool!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_IfWantPokemonHold, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_12D2:
    VMJump L_12EC

L_12D8:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hum fiercely! My battle song!\nBattle fiercely! My Pokémon![f000]븁\u0000\nHuh?\nYou don't have a Prop Case, do you?[f000]븀\u0000\nThen I won't battle you!"
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_HumFiercelyBattleSong, 0, 0
    LastKeyWait
    ActorMsgClose

L_12EC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In hot summer, I want to cool down\nwith Water-type Pokémon.[f000]븁\u0000\nOn the other hand, in cold winter,\nI want to warm up with Fire-type Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_HotSummerWantCool, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A Pokémon you are proud of\nwins with your favorite move![f000]븁\u0000\nThat's when a Trainer definitely smiles."
    ParentActorMsg MSGFILE_SCRIPT, Route5_Text_PokemonProudWinsFavorite, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
