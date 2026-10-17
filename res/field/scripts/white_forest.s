#include "asm/field_script.inc"
#include "text/script/white_forest.h"

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
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0

Script_1:
    VMHalt

Script_2:
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00FE
    VMStackPushFlag EVENT_FLAG_0x03cc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F8
    FlagReset EVENT_FLAG_0x03cc
    ActorAdd 10

L_00F8:
    VMJump L_0147

L_00FE:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0147
    VMStackPushFlag EVENT_FLAG_0x03cc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012C
    FlagReset EVENT_FLAG_0x03cc
    ActorAdd 10

L_012C:
    VMStackPushFlag EVENT_FLAG_0x03cd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0147
    FlagReset EVENT_FLAG_0x03cd
    ActorAdd 6

L_0147:
    VMHalt

Script_3:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 358, 0, 0x109000, 0x1c8000, 0x8501f, 0x158000, 1
    ActorCmdExec 255, Movement_0A54
    ActorCmdWait
    EvCameraWait
    MapChangeWarp ZONE_WHITE_TREEHOLLOW, 7, 14, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "White Forest\nPeople and Nature in Harmony"
    MsgPlaceSign WhiteForest_Text_WhiteForestPeopleNature, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_019D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F2
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_01D6
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_01D6
    VMJump L_01E6

L_01D6:
    ParentActorMsg MSGFILE_SCRIPT, 0x8022, 2, 0
    VMJump L_01F0

L_01E6:
    ParentActorMsg MSGFILE_SCRIPT, 0x8021, 2, 0

L_01F0:
    MoneyWinDisp 31, 1
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32808
    ListMenuAdd 155, 65535, 0
    ListMenuAdd 156, 65535, 1
    ListMenuShow
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DC
    ItemCheckSpace 0x802b, 1, 0x8029
    MoneyCheck 0x802a, 0x802c
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0259
    ParentActorMsg MSGFILE_SCRIPT, 0x8024, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02D6

L_0259:
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0280
    ParentActorMsg MSGFILE_SCRIPT, 0x8025, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02D6

L_0280:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x802c
    MoneyWinUpdate
    SEWait
    ParentActorMsg MSGFILE_SCRIPT, 0x8023, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x802b
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ParentActorMsg MSGFILE_SCRIPT, 0x8026, 2, 0
    LastKeyWait
    MsgWinCloseAll
    RecordAdd 21, 1
    RecordAdd 22, 0x802c
    FlagSet 0x802e

L_02D6:
    VMJump L_02EA

L_02DC:
    ParentActorMsg MSGFILE_SCRIPT, 0x8026, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_02EA:
    MoneyWinClose
    VMJump L_0300

L_02F2:
    ParentActorMsg MSGFILE_SCRIPT, 0x8027, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0300:
    VMReturn

Script_5:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_0328
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_0328
    VMJump L_033A

L_0328:
    WorkSetConst 0x802b, 226
    WorkSetConst 0x802c, 1000
    VMJump L_0346

L_033A:
    WorkSetConst 0x802b, 85
    WorkSetConst 0x802c, 1000

L_0346:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 4
    WorkSetConst 0x8021, 127
    WorkSetConst 0x8022, 128
    WorkSetConst 0x8023, 129
    WorkSetConst 0x8024, 130
    WorkSetConst 0x8025, 131
    WorkSetConst 0x8026, 132
    WorkSetConst 0x8027, 133
    WorkSetConst 0x802e, 2785
    VMCall L_019D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_03B4
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_03B4
    VMJump L_03C6

L_03B4:
    WorkSetConst 0x802b, 227
    WorkSetConst 0x802c, 2000
    VMJump L_03D2

L_03C6:
    WorkSetConst 0x802b, 84
    WorkSetConst 0x802c, 2000

L_03D2:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 5
    WorkSetConst 0x8021, 134
    WorkSetConst 0x8022, 135
    WorkSetConst 0x8023, 136
    WorkSetConst 0x8024, 137
    WorkSetConst 0x8025, 138
    WorkSetConst 0x8026, 139
    WorkSetConst 0x8027, 140
    WorkSetConst 0x802e, 2786
    VMCall L_019D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_0440
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_0440
    VMJump L_0452

L_0440:
    WorkSetConst 0x802b, 235
    WorkSetConst 0x802c, 4000
    VMJump L_045E

L_0452:
    WorkSetConst 0x802b, 107
    WorkSetConst 0x802c, 4000

L_045E:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 5
    WorkSetConst 0x8021, 141
    WorkSetConst 0x8022, 142
    WorkSetConst 0x8023, 143
    WorkSetConst 0x8024, 144
    WorkSetConst 0x8025, 145
    WorkSetConst 0x8026, 146
    WorkSetConst 0x8027, 147
    WorkSetConst 0x802e, 2787
    VMCall L_019D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_04CC
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_04CC
    VMJump L_04DE

L_04CC:
    WorkSetConst 0x802b, 221
    WorkSetConst 0x802c, 6000
    VMJump L_04EA

L_04DE:
    WorkSetConst 0x802b, 110
    WorkSetConst 0x802c, 6000

L_04EA:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 5
    WorkSetConst 0x8021, 148
    WorkSetConst 0x8022, 149
    WorkSetConst 0x8023, 150
    WorkSetConst 0x8024, 151
    WorkSetConst 0x8025, 152
    WorkSetConst 0x8026, 153
    WorkSetConst 0x8027, 154
    WorkSetConst 0x802e, 2788
    VMCall L_019D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0565
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Silvia: Ommm...[f000]븁\u0000\nOh! I'm sorry! I was sunbathing, and my\nmind went blank, like I was meditating...[f000]븀\u0000\nOmmm...[f000]븁\u0000\nIsn't the weather great?\nWould you like to “om\" with me?"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_SilviaOmmmOhIm, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_05C9

L_0565:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05A2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Silvia: W-wow! Did you see that?[f000]븁\u0000\nThat lazybones store owner put\nnew items in the shop![f000]븀\u0000\nHe's had the same old stuff[f000]븀\u0000\non display forever![f000]븁\u0000\nHmmm...[f000]븁\u0000\nIt's fine as long as it doesn't\nrain, I guess."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_SilviaWWowDid, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_05C9

L_05A2:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_05C9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Silvia: While I was just standing around\nand watching Pokémon play,[f000]븀\u0000\nI started to feel so happy and peaceful![f000]븁\u0000\nI hope tomorrow is a pleasant\nday just like today!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_SilviaWhileJustStanding, 0, 0
    LastKeyWait
    ActorMsgClose

L_05C9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0602
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Robbie: My big brother is working\nin the shop, but he thinks putting[f000]븀\u0000\nnew goods out is too much trouble.[f000]븁\u0000\nSo go explore a hollow or something,\nand check back later."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RobbieBigBrotherWorking, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0669

L_0602:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_063F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Robbie: Amazing! My brother\nfinally put some new items[f000]븀\u0000\nin the shop he's working at.[f000]븁\u0000\nHe's the type who can do\nanything if he wants to!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RobbieAmazingBrotherFinally, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0669

L_063F:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0669
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Robbie: My brother is just\nraring to go lately![f000]븁\u0000\nHe keeps putting all sorts of\nnew products in the store![f000]븁\u0000\nHe said he was inspired by a\nTrainer named [f000]Ā\u0001\u0000[f000]븀\u0000\nwho became the best Trainer[f000]븀\u0000\nin the White Treehollow!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RobbieBrotherJustRaring, 0, 0
    LastKeyWait
    ActorMsgClose

L_0669:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WordSetPlayerName 0
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_06A5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ryder: Recently in Unova's Challenge--\nthe White Treehollow--[f000]븀\u0000\ntalented Trainers are falling[f000]븀\u0000\none after another.[f000]븁\u0000\nApparently, a Trainer named\n[f000]Ā\u0001\u0000 is on a rampage.[f000]븁\u0000\nThat's not you, is it? I want to have\na match with that Trainer sometime."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RyderRecentlyUnovasChallenge, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_06CC

L_06A5:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_06CC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ryder: You're [f000]Ā\u0001\u0000?!\nYou're the best in the White Treehollow![f000]븁\u0000\nThat's incredible. I'm still\nhaving problems with the first area...[f000]븁\u0000\nGive me some tips later, all right?"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RyderYoureYoureBest, 0, 0
    LastKeyWait
    ActorMsgClose

L_06CC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0705
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Carlos: In White Forest, everyone\nshares the gathered Berries.[f000]븁\u0000\nI picked some extra ones\nfor the older folks."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_CarlosWhiteForestEveryone, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_076C

L_0705:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0742
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Carlos: Living here every day,\neating Berries, makes me miss[f000]븀\u0000\nthe exciting and exotic food[f000]븀\u0000\nyou can get in the city sometimes.[f000]븁\u0000\nMaybe it's time again to go on\na quest for delicious food.[f000]븀\u0000\nIt's been a while."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_CarlosLivingHereEvery, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_076C

L_0742:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_076C
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Carlos: When I went to the shop\nto prepare for my trip,[f000]븀\u0000\nI noticed the selection[f000]븀\u0000\nof items has gotten much better.[f000]븁\u0000\nBut the guy in the shop was saying\nhe can't lose to [f000]Ā\u0001\u0000.[f000]븀\u0000\nWhat could he have been talking about?"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_CarlosWhenWentShop, 0, 0
    LastKeyWait
    ActorMsgClose

L_076C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_07A5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Gene: A hollow has suddenly appeared in\nthe white tree! That tree is really[f000]븀\u0000\nimportant to us here in White Forest![f000]븁\u0000\nInvestigators and Trainers\nhave come out of the woodwork,[f000]븀\u0000\nbut we locals will be the ones[f000]븀\u0000\nwho make it to the lowest floor!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_GeneHollowHasSuddenly, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_080F

L_07A5:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07E5
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Gene: Hey, [f000]Ā\u0001\u0000![f000]븁\u0000\nI heard the news!\nYou made it really deep[f000]븀\u0000\ninto the hollow, didn't you?[f000]븁\u0000\nHurry and get to the lowest level\nfor me--I'm about to give up!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_GeneHeyHeardNews, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_080F

L_07E5:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_080F
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Gene: On behalf of everyone in the\nforest, I want to congratulate you[f000]븀\u0000\non becoming the top Trainer of[f000]븀\u0000\nthe White Treehollow![f000]븁\u0000\nCongratulations!\nThat was a major accomplishment!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_GeneBehalfEveryoneForest, 0, 0
    LastKeyWait
    ActorMsgClose

L_080F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0848
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Miho: Waah! I'm so bored![f000]븁\u0000\nI just got here, but I can't\nhandle living like this[f000]븀\u0000\nwhere there's nothing to do![f000]븁\u0000\nI miss the neon so much!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_MihoWaahImBored, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_08AC

L_0848:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0885
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Miho: Since I've moved here,\nmy skin has become so smooth![f000]븁\u0000\nI wonder if it's because I'm\neating fresh-picked Berries every day.[f000]븁\u0000\nNature is so amazing!\nHooray for nature!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_MihoSinceIveMoved, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_08AC

L_0885:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_08AC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Miho: Lying on my back in a meadow\nand idly watching the sun sink[f000]븀\u0000\nbehind the trees...[f000]븁\u0000\nIt's these simple, ordinary things\nthat are the most fun in this place![f000]븀\u0000\nThe people here taught me that!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_MihoLyingBackMeadow, 0, 0
    LastKeyWait
    ActorMsgClose

L_08AC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_08E5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rosaline: Oh no! This is no good at all...\nEvery day, I just nap away,[f000]븀\u0000\nand now I'm really rusty![f000]븁\u0000\nOK! OK! I'm going to get back\ninto fighting shape by training[f000]븀\u0000\nin the White Treehollow!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RosalineOhNoNo, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_094C

L_08E5:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0922
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rosaline: I'm also taking on the\nWhite Treehollow![f000]븁\u0000\nThere are nothing but strange\nTrainers inside![f000]븁\u0000\nBut everyone in there\nis really tough!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RosalineImAlsoTaking, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_094C

L_0922:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_094C
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rosaline: I went back to basics and\nwas training at the White Treehollow.[f000]븁\u0000\nIt made me remember the simple\njoys of Pokémon battling, the way[f000]븀\u0000\nI felt when I'd just started my journey.[f000]븁\u0000\n[f000]Ā\u0001\u0000, did you find anything\nimportant when you were battling there?"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_RosalineWentBackBasics, 0, 0
    LastKeyWait
    ActorMsgClose

L_094C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Grace: So you're [f000]Ā\u0001\u0000, then?[f000]븁\u0000\nMy grandson was all excited about\nthis amazing Trainer, so I finally[f000]븀\u0000\ncame to see for myself![f000]븁\u0000\nI thought you'd look scary, but...\nActually, you're quite a cutie!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_GraceYoureThenGrandson, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 518, 0
    // "Muwaaaan!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_Muwaaaan, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Squee?"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_Squee, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 548, 0
    // "Lill lill..."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_LillLill, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 531, 0
    // "Pololo."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_Pololo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 619, 0
    // "Fooo!"
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_Fooo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Lola rolaa."
    ParentActorMsg MSGFILE_SCRIPT, WhiteForest_Text_LolaRolaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_0A54:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
