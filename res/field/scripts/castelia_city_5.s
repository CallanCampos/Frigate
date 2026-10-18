#include "asm/field_script.inc"
#include "text/script/castelia_city_5.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0

Script_10:
    VMStackPushFlag EVENT_FLAG_0x0125
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00AA
    FlagReset EVENT_FLAG_0x0302
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AA
    WorkSetConst EVENT_WORK_0x40c8, 1

L_00AA:
    VMHalt

Script_13:
    CasteliaRushInit
    VMHalt

Script_14:
    CasteliaRushInit
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Studio Castelia"
    MsgPlaceSign CasteliaCity5_Text_StudioCastelia, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Castelia's Famous Casteliacone"
    MsgPlaceSign CasteliaCity5_Text_CasteliasFamousCasteliacone, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034C
    VMStackPushFlag EVENT_FLAG_0x0125
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0161
    VMStackPushFlag EVENT_FLAG_0x0124
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014F
    // "Wah ha ha ha![f000]븁\u0000\nThere's a rumor the new Champion\nloves our Casteliacones.[f000]븁\u0000\nAnd suddenly an avalanche of customers\nare screaming for our ice cream![f000]븁\u0000\nI'm screaming for joy! Eeeek!\nIt's the super-popular Casteliacone![f000]븀\u0000\nHow many do you want?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_WahHaHaHa, 0, 0, 0
    FlagSet EVENT_FLAG_0x0124
    VMJump L_015B

L_014F:
    // "The Champion loves them, too!\nA Casteliacone is a perfect[f000]븀\u0000\nsouvenir of Castelia City![f000]븁\u0000\nIt's $100.\nWould you like to buy one?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ChampionLovesThemToo, 0, 0, 0

L_015B:
    VMJump L_0196

L_0161:
    VMStackPushFlag EVENT_FLAG_0x0123
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018A
    // "Castelia City's Casteliacone\nis the perfect souvenir![f000]븁\u0000\nA while ago, our store\nwas really popular.[f000]븁\u0000\nRecently, however, we don't\nget as many customers as we used to.[f000]븁\u0000\nBut, whining won't accomplish anything.\nI just have to work hard to sell them![f000]븁\u0000\nWell then, how about a Casteliacone?[f000]븁\u0000\nIt's $100.\nWould you like to buy one?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_CasteliaCitysCasteliaconePerfect, 0, 0, 0
    FlagSet EVENT_FLAG_0x0123
    VMJump L_0196

L_018A:
    // "A Casteliacone is a perfect\nsouvenir of Castelia City![f000]븁\u0000\nIt's $100.\nWould you like to buy one?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_CasteliaconePerfectSouvenirCastelia, 0, 0, 0

L_0196:
    MoneyWinDisp 31, 1
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32804
    ListMenuAdd 10, 65535, 0
    ListMenuAdd 11, 65535, 1
    ListMenuAdd 12, 65535, 2
    ListMenuShow
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E4
    WorkSetConst 0x8023, 1
    WorkSetConst 0x8025, 100
    VMJump L_0209

L_01E4:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0209
    WorkSetConst 0x8023, 12
    WorkSetConst 0x8025, 1200
    VMJump L_0209

L_0209:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02FB
    ItemCheckSpace ITEM_CASTELIACONE, 0x8023, 0x8027
    MoneyCheck 0x8026, 0x8025
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0263
    MoneyWinClose
    // "Thank you very much![f000]븁\u0000\n...Huh? Oh, dear!\nIt looks like you don't have enough[f000]븀\u0000\nmoney. Please come again sometime.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ThankVeryMuchHuh, 0, 2, 0
    MsgWinCloseAll
    VMJump L_02F5

L_0263:
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028C
    MoneyWinClose
    // "Thank you very much![f000]븁\u0000\n...Huh? Oh, dear!\nIt looks like your Bag is full.[f000]븁\u0000\nPlease come again sometime.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ThankVeryMuchHuh_2, 0, 2, 0
    MsgWinCloseAll
    VMJump L_02F5

L_028C:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x8025
    MoneyWinUpdate
    SEWait
    RecordAdd 21, 1
    RecordAdd 22, 0x8025
    // "Thank you for your business!\nPlease come again![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ThankBusinessPleaseCome, 0, 2, 0
    MsgWinCloseAll
    MoneyWinClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 591
    WorkSet 0x8001, 0x8023
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_DAILY_0x0ac0
    VMStackPushFlag EVENT_FLAG_0x0125
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F5
    FlagSet EVENT_FLAG_0x0125
    VMCall Script_4

L_02F5:
    VMJump L_0346

L_02FB:
    MoneyWinClose
    VMStackPushFlag EVENT_FLAG_0x0125
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0336
    // "Even though you waited in line...\nWell, please come again, OK?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_EvenThoughWaitedLine, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0346

L_0336:
    // "Even though you came to the store...\nWell, please come again, OK?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_EvenThoughCameStore, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0346:
    VMJump L_035C

L_034C:
    // "You bought the last of\nour supply for today...[f000]븁\u0000\nPlease come again, OK?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_BoughtLastOurSupply, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_035C:
    VMStackPush EVENT_WORK_0x40c8
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CC
    ActorCmdExec 255, Movement_0B9C
    ActorCmdWait
    ActorCmdExec 2, Movement_0B5C
    ActorCmdExec 3, Movement_0B6C
    ActorCmdExec 4, Movement_0B6C
    ActorCmdExec 5, Movement_0B6C
    VMSleep 4
    ActorCmdExec 6, Movement_0B74
    ActorCmdWait
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C6
    WorkSetConst EVENT_WORK_0x40c8, 0
    VMJump L_03CC

L_03C6:
    WorkSetConst EVENT_WORK_0x40c8, 1

L_03CC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    FlagReset EVENT_FLAG_0x0303
    ActorAdd 1
    ActorWalkRoute 1, 14, 35, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 1, Movement_0B44
    ActorCmdWait
    ActorCmdExec 255, Movement_0B7C
    ActorCmdWait
    // "Hmm? There's a shop that sells\nice cream here? Is it good?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_HmmTheresShopSells, 1, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 1, 14, 18, 1, 8, 1
    ActorCmdWait
    ActorDelete 1
    FlagSet EVENT_FLAG_0x0303
    VMReturn

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I rode the train and came clear\nfrom Anville Town to get one!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_RodeTrainCameClear, 2, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0B84
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Boy oh boy! If the Champion\nbuys them, too, these have to be cool!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_BoyOhBoyIf, 3, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 3, Movement_0B8C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I can't wait to eat one!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_CantWaitEatOne, 4, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0B8C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My daughter asked me to get\nthem for her, but look at this line!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_DaughterAskedGetThem, 5, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0B8C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06F1
    // "Huh? Oh, here you can buy the dessert\nthat everyone in Castelia City[f000]븀\u0000\nis talking about![f000]븁\u0000\nAre you going to get in line?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_HuhOhHereCan, 6, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06DD
    WorkSetConst EVENT_WORK_0x40c8, 2
    // "The line gets reeeally long!\nBut, it's worth lining up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_LineGetsReeeallyLong, 6, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_NE
    VMStackPush 0x8022
    VMStackPushConst 40
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_055D
    ActorWalkRoute 255, 12, 40, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0B8C
    ActorCmdWait

L_055D:
    ActorCmdExec 6, Movement_0B8C
    ActorCmdWait
    ActorCmdExec 2, Movement_0B50
    VMSleep 16
    ActorCmdExec 3, Movement_0B5C
    ActorCmdExec 4, Movement_0B6C
    ActorCmdExec 5, Movement_0B6C
    ActorCmdExec 6, Movement_0B6C
    ActorCmdExec 255, Movement_0B6C
    ActorCmdWait
    ActorSetGPos 2, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 3, Movement_0B50
    VMSleep 16
    ActorCmdExec 4, Movement_0B5C
    ActorCmdExec 5, Movement_0B6C
    ActorCmdExec 6, Movement_0B6C
    ActorCmdExec 255, Movement_0B6C
    VMSleep 4
    ActorCmdExec 2, Movement_0B74
    ActorCmdWait
    ActorSetGPos 3, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 4, Movement_0B50
    VMSleep 16
    ActorCmdExec 5, Movement_0B5C
    ActorCmdExec 6, Movement_0B6C
    ActorCmdExec 255, Movement_0B6C
    ActorCmdExec 2, Movement_0B6C
    VMSleep 4
    ActorCmdExec 3, Movement_0B74
    ActorCmdWait
    ActorSetGPos 4, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 5, Movement_0B50
    VMSleep 16
    ActorCmdExec 6, Movement_0B5C
    ActorCmdExec 255, Movement_0B6C
    ActorCmdExec 2, Movement_0B6C
    ActorCmdExec 3, Movement_0B6C
    VMSleep 4
    ActorCmdExec 4, Movement_0B74
    ActorCmdWait
    ActorSetGPos 5, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 6, Movement_0B50
    VMSleep 16
    ActorCmdExec 255, Movement_0B5C
    ActorCmdExec 2, Movement_0B6C
    ActorCmdExec 3, Movement_0B6C
    ActorCmdExec 4, Movement_0B6C
    VMSleep 4
    ActorCmdExec 5, Movement_0B74
    ActorCmdWait
    ActorSetGPos 6, 12, 0, 47, 0
    VMCall Script_3
    VMJump L_06EB

L_06DD:
    // "Yeah...\nThis line is way too long.[f000]븁\u0000\nBut, just between you and me,\nI hear the Champion loves them, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_YeahLineWayToo, 6, 0, 0
    MsgWinCloseAll

L_06EB:
    VMJump L_06FF

L_06F1:
    // "Huh? Oh, here you can buy the dessert\nthat everyone in Castelia City[f000]븀\u0000\nis talking about![f000]븁\u0000\nBut it looks like I got the last one.\nThey're sold out for today![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_HuhOhHereCan_2, 6, 0, 0
    MsgWinCloseAll

L_06FF:
    ActorCmdExec 6, Movement_0B8C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    ActorCmdExec 6, Movement_0B94
    VMSleep 3
    ActorCmdExec 255, Movement_0B8C
    ActorCmdWait
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0905
    // "Huh? Oh, here you can buy the dessert\nthat everyone in Castelia City[f000]븀\u0000\nis talking about![f000]븁\u0000\nAre you going to get in line?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_HuhOhHereCan, 6, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08F1
    WorkSetConst EVENT_WORK_0x40c8, 2
    // "The line gets reeeally long!\nBut, it's worth lining up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_LineGetsReeeallyLong, 6, 0, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0B8C
    ActorCmdWait
    ActorCmdExec 2, Movement_0B50
    VMSleep 16
    ActorCmdExec 3, Movement_0B5C
    ActorCmdExec 4, Movement_0B6C
    ActorCmdExec 5, Movement_0B6C
    ActorCmdExec 6, Movement_0B6C
    ActorCmdExec 255, Movement_0B6C
    ActorCmdWait
    ActorSetGPos 2, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 3, Movement_0B50
    VMSleep 16
    ActorCmdExec 4, Movement_0B5C
    ActorCmdExec 5, Movement_0B6C
    ActorCmdExec 6, Movement_0B6C
    ActorCmdExec 255, Movement_0B6C
    VMSleep 4
    ActorCmdExec 2, Movement_0B74
    ActorCmdWait
    ActorSetGPos 3, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 4, Movement_0B50
    VMSleep 16
    ActorCmdExec 5, Movement_0B5C
    ActorCmdExec 6, Movement_0B6C
    ActorCmdExec 255, Movement_0B6C
    ActorCmdExec 2, Movement_0B6C
    VMSleep 4
    ActorCmdExec 3, Movement_0B74
    ActorCmdWait
    ActorSetGPos 4, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 5, Movement_0B50
    VMSleep 16
    ActorCmdExec 6, Movement_0B5C
    ActorCmdExec 255, Movement_0B6C
    ActorCmdExec 2, Movement_0B6C
    ActorCmdExec 3, Movement_0B6C
    VMSleep 4
    ActorCmdExec 4, Movement_0B74
    ActorCmdWait
    ActorSetGPos 5, 12, 0, 47, 0
    VMSleep 8
    ActorCmdExec 6, Movement_0B50
    VMSleep 16
    ActorCmdExec 255, Movement_0B5C
    ActorCmdExec 2, Movement_0B6C
    ActorCmdExec 3, Movement_0B6C
    ActorCmdExec 4, Movement_0B6C
    VMSleep 4
    ActorCmdExec 5, Movement_0B74
    ActorCmdWait
    ActorSetGPos 6, 12, 0, 47, 0
    VMCall Script_12
    VMJump L_08FF

L_08F1:
    // "Yeah...\nThis line is way too long.[f000]븁\u0000\nBut, just between you and me,\nI hear the Champion loves them, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_YeahLineWayToo, 6, 0, 0
    MsgWinCloseAll

L_08FF:
    VMJump L_0913

L_0905:
    // "Huh? Oh, here you can buy the dessert\nthat everyone in Castelia City[f000]븀\u0000\nis talking about![f000]븁\u0000\nBut it looks like I got the last one.\nThey're sold out for today![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_HuhOhHereCan_2, 6, 0, 0
    MsgWinCloseAll

L_0913:
    ActorCmdExec 6, Movement_0B8C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ABF
    VMStackPushFlag EVENT_FLAG_0x0124
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_095F
    // "Wah ha ha ha![f000]븁\u0000\nThere's a rumor the new Champion\nloves our Casteliacones.[f000]븁\u0000\nAnd suddenly an avalanche of customers\nare screaming for our ice cream![f000]븁\u0000\nI'm screaming for joy! Eeeek!\nIt's the super-popular Casteliacone![f000]븀\u0000\nHow many do you want?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_WahHaHaHa, 0, 0, 0
    FlagSet EVENT_FLAG_0x0124
    VMJump L_096B

L_095F:
    // "The Champion loves them, too!\nA Casteliacone is a perfect[f000]븀\u0000\nsouvenir of Castelia City![f000]븁\u0000\nIt's $100.\nWould you like to buy one?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ChampionLovesThemToo, 0, 0, 0

L_096B:
    MoneyWinDisp 31, 1
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32804
    ListMenuAdd 10, 65535, 0
    ListMenuAdd 11, 65535, 1
    ListMenuAdd 12, 65535, 2
    ListMenuShow
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09B9
    WorkSetConst 0x8023, 1
    WorkSetConst 0x8025, 100
    VMJump L_09DE

L_09B9:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09DE
    WorkSetConst 0x8023, 12
    WorkSetConst 0x8025, 1200
    VMJump L_09DE

L_09DE:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0AA7
    ItemCheckSpace ITEM_CASTELIACONE, 0x8023, 0x8027
    MoneyCheck 0x8026, 0x8025
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A38
    MoneyWinClose
    // "Thank you very much![f000]븁\u0000\n...Huh? Oh, dear!\nIt looks like you don't have enough[f000]븀\u0000\nmoney. Please come again sometime.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ThankVeryMuchHuh, 0, 2, 0
    MsgWinCloseAll
    VMJump L_0AA1

L_0A38:
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A61
    MoneyWinClose
    // "Thank you very much![f000]븁\u0000\n...Huh? Oh, dear!\nIt looks like your Bag is full.[f000]븁\u0000\nPlease come again sometime.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ThankVeryMuchHuh_2, 0, 2, 0
    MsgWinCloseAll
    VMJump L_0AA1

L_0A61:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x8025
    MoneyWinUpdate
    SEWait
    // "Thank you for your business!\nPlease come again![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_ThankBusinessPleaseCome, 0, 2, 0
    MsgWinCloseAll
    MoneyWinClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 591
    WorkSet 0x8001, 0x8023
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_DAILY_0x0ac0

L_0AA1:
    VMJump L_0AB9

L_0AA7:
    MoneyWinClose
    // "Even though you waited in line...\nWell, please come again, OK?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_EvenThoughWaitedLine, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0AB9:
    VMJump L_0ACF

L_0ABF:
    // "You bought the last of\nour supply for today...[f000]븁\u0000\nPlease come again, OK?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity5_Text_BoughtLastOurSupply, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0ACF:
    VMStackPush EVENT_WORK_0x40c8
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B3F
    ActorCmdExec 255, Movement_0B9C
    ActorCmdWait
    ActorCmdExec 2, Movement_0B5C
    ActorCmdExec 3, Movement_0B6C
    ActorCmdExec 4, Movement_0B6C
    ActorCmdExec 5, Movement_0B6C
    VMSleep 4
    ActorCmdExec 6, Movement_0B74
    ActorCmdWait
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B39
    WorkSetConst EVENT_WORK_0x40c8, 0
    VMJump L_0B3F

L_0B39:
    WorkSetConst EVENT_WORK_0x40c8, 1

L_0B3F:
    VMReturn
    .balign 4, 0

Movement_0B44:
    Move 34, 1
    Move 75, 1
    MoveEnd

Movement_0B50:
    Move 15, 2
    Move 13, 12
    MoveEnd

Movement_0B5C:
    Move 12, 1
    Move 34, 1
    Move 63, 2
    MoveEnd

Movement_0B6C:
    Move 12, 1
    MoveEnd

Movement_0B74:
    Move 12, 8
    MoveEnd

Movement_0B7C:
    Move 35, 1
    MoveEnd

Movement_0B84:
    Move 34, 1
    MoveEnd

Movement_0B8C:
    Move 32, 1
    MoveEnd

Movement_0B94:
    Move 33, 1
    MoveEnd

Movement_0B9C:
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 75, 1
    MoveEnd
