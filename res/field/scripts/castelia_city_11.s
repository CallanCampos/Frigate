#include "asm/field_script.inc"
#include "text/script/castelia_city_11.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FadeInBlack
    FadeWait
    ActorCmdExec 0, Movement_0204
    ActorCmdWait
    ActorCmdExec 255, Movement_022C
    ActorCmdWait
    ActorCmdExec 0, Movement_0220
    ActorCmdWait
    VMStackPush EVENT_WORK_0x417b
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E8
    // "Congratulations![f000]븁\u0000\nYou've defeated all the Trainers\non the ship![f000]븁\u0000\nPlease accept this prize.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_CongratulationsYouveDefeatedAll, 0, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    RTCGetWeekDay 0x8020
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_006C
    VMJump L_0092

L_006C:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 42
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01E8

L_0092:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_00A5
    VMJump L_00CB

L_00A5:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 43
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01E8

L_00CB:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_00DE
    VMJump L_0104

L_00DE:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 42
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01E8

L_0104:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_0117
    VMJump L_013D

L_0117:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 54
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01E8

L_013D:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_0150
    VMJump L_0176

L_0150:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 42
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01E8

L_0176:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_0189
    VMJump L_01AF

L_0189:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 504
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01E8

L_01AF:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_01C2
    VMJump L_01E8

L_01C2:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 50
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01E8

L_01E8:
    // "Thank you for sailing with us on the\nRoyal Unova.[f000]븁\u0000\nHave a nice day!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_ThankSailingUsRoyal, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x417b, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0204:
    Move 35, 1
    Move 1, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 33, 1
    MoveEnd

Movement_0220:
    Move 13, 1
    Move 34, 1
    MoveEnd

Movement_022C:
    Move 14, 2
    Move 3, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0414
    RTCGetDayPart 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FE
    MoneyWinDisp 31, 1
    // "Welcome to the Royal Unova![f000]븁\u0000\nThe scenery is exhilarating.\nThe battles are exciting.[f000]븁\u0000\nThe ticket is $1,000.\nWould you like to get on board?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_WelcomeRoyalUnovaScenery, 0, 2, 0
    WorkSetConst 0x8024, 0

L_029A:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_03F8
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32803
    ListMenuAdd 13, 65535, 0
    ListMenuAdd 14, 65535, 1
    ListMenuAdd 15, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_02E3
    VMJump L_038A

L_02E3:
    WorkSetConst 0x8024, 1
    MoneyCheck 0x8022, 1000
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0372
    WorkSetConst 0x8025, 0
    Cmd_024A 0x8025
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0366
    // "The power of the C-Gear will be turned\noff. Is that OK?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_PowerCGearWill, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034E
    VMCall L_0450
    VMJump L_0360

L_034E:
    MoneyWinClose
    // "Certainly.\nPlease come back again."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_CertainlyPleaseComeBack, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0360:
    VMJump L_036C

L_0366:
    VMCall L_0450

L_036C:
    VMJump L_0384

L_0372:
    MoneyWinClose
    // "I'm sorry, you don't have enough money.\nPlease come back again."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_ImSorryDontHave, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0384:
    VMJump L_03F2

L_038A:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_039D
    VMJump L_03A9

L_039D:
    VMCall L_04BD
    VMJump L_03F2

L_03A9:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_03BC
    VMJump L_03DA

L_03BC:
    MoneyWinClose
    WorkSetConst 0x8024, 1
    // "Certainly.\nPlease come back again."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_CertainlyPleaseComeBack, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03F2

L_03DA:
    MoneyWinClose
    WorkSetConst 0x8024, 1
    // "Certainly.\nPlease come back again."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_CertainlyPleaseComeBack, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_03F2:
    VMJump L_029A

L_03F8:
    VMJump L_040E

L_03FE:
    // "The Royal Unova operates only in\nthe evening.[f000]븀\u0000\nPlease come back again."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_RoyalUnovaOperatesOnly, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_040E:
    VMJump L_0424

L_0414:
    // "We are closed for the day.\nPlease come again tomorrow."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_WeClosedDayPlease, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0424:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_042C:
    Move 1, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 33, 1
    MoveEnd

Movement_0444:
    Move 65, 1
    Move 15, 3
    MoveEnd

L_0450:
    MoneySub 1000
    MoneyWinUpdate
    SEPlay SEQ_SE_SYS_22
    SEWait
    FieldSubscreenDisable
    FunfestBGMReturn
    // "We are leaving the port shortly.\nBon voyage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_WeLeavingPortShortly, 0, 2, 0
    MsgWinCloseAll
    MoneyWinClose
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_048B
    PlayerSetSpecialSequence 1

L_048B:
    ActorCmdExec 0, Movement_042C
    ActorCmdExec 255, Movement_0444
    ActorCmdWait
    RecordAdd 126, 1
    WorkSetConst EVENT_WORK_0x417b, 0
    FlagSet EVENT_FLAG_DAILY_0x0ab8
    RTReserveScript 29
    MapChangeWarp ZONE_ROYAL_UNOVA, 29, 29, 0
    VMReturn

L_04BD:
    // "The Royal Unova leaves once a day.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_RoyalUnovaLeavesOnce, 0, 2, 0
    // "The ship goes under a big bridge called\nthe Skyarrow Bridge in the Unova region,[f000]븀\u0000\nthen turns around at Marvelous Bridge[f000]븀\u0000\nand returns to Castelia City.[f000]븁\u0000\nWhen the whistle blows five times,\nit means we've arrived at the dock.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_ShipGoesUnderBig, 0, 2, 0
    // "From the deck of the world-famous Royal\nUnova, you can enjoy a spectacular view.[f000]븁\u0000\nAnd the passenger cabins are full of\nTrainers who love to battle.[f000]븁\u0000\nPlease enjoy both the view and\nthe battles![f000]븁\u0000\nNote that as C-Gears may affect radio\nsignals or instruments in the ship,[f000]븀\u0000\nwe ask that you refrain from using them.[f000]븀\u0000\nThank you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_FromDeckWorldFamous, 0, 2, 0
    // "The ticket is $1,000.\nWould you like to get on board?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity11_Text_Ticket1000Would, 0, 2, 0
    VMReturn
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
