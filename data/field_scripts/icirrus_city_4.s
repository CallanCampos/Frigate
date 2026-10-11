#include "asm/field_script.inc"
#include "text/script/icirrus_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_4:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0039
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_003F

L_0039:
    WorkSetConst EVENT_WORK_0x4000, 0

L_003F:
    FlagSet EVENT_FLAG_DAILY_0x0ac5
    VMHalt

Script_6:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AC
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    ActorGetGPos 0, 0x8020, 0x8021
    ActorSetGPos 0, 0x8020, 0, 0x8021, 1
    ActorGetGPos 2, 0x8020, 0x8021
    ActorSetGPos 2, 0x8020, 0, 0x8021, 1
    ActorGetGPos 1, 0x8020, 0x8021
    ActorSetGPos 1, 0x8020, 0, 0x8021, 1
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0

L_00AC:
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst 0x8028, 0
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FF
    WorkSetConst 0x8028, 0
    VMJump L_0105

L_00FF:
    WorkSetConst 0x8028, 1

L_0105:
    PepQuizGenerate 0x8028, 0x8025, 0x8026, 0x8027
    WorkSetConst 0x8028, 0
    WordSetPlayerName 0
    ActorCmdExec 0, Movement_03F8
    ActorCmdWait
    BGMPlay SEQ_BGM_E_TSURETEKE2
    ActorCmdExec 0, Movement_0400
    ActorCmdWait
    // "Wye: Hi!\nThis way, pleeeeease![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeHiWayPleeeeease, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_040C
    ActorCmdExec 255, Movement_0418
    ActorCmdWait
    ActorCmdExec 1, Movement_043C
    ActorCmdExec 2, Movement_0444
    ActorCmdWait
    ActorCmdExec 0, Movement_0424
    ActorCmdWait
    // "Wye: Exciting! Thrilling! Zippy! Chilling!\nIt's “Pep Quiz\"![f000]븁\u0000\nToday's challenger is--this person![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeExcitingThrillingZippy, 0, 0, 0
    ActorMsgClose
    // "Aha: Hiya, welcome![f000]븁\u0000\n“Pep Quiz\" starts NOW![f000]븁\u0000\nAnswer lots of quizzy questions,\nand watch your brain get brainier![f000]븁\u0000\nLet's start...with...a question![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaHiyaWelcomePep, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 2, Movement_044C
    ActorCmdWait
    // "Ditoh: Good luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_DitohGoodLuck, 2, 0, 0
    ActorMsgClose
    // "Aha: A question![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaQuestion, 1, 0, 0
    MEPlay SEQ_ME_QUIZ
    MEWait
    ActorMsg MSGFILE_SCRIPT, 0x8025, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_0454
    ActorCmdWait
    // "Wye: Oh, my! It's tremendously difficult!\nCan the challenger answer this?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeOhItsTremendously, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 2, Movement_0488
    ActorCmdWait
    // "Ditoh: H-i-n-t! H-i-n-t![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_DitohHNT, 2, 0, 0
    ActorMsgClose
    // "Aha: Oh-oh. The audience\nis asking for a hint![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaOhOhAudience, 1, 0, 0
    ActorMsgClose
    // "Wye: OK.\nI'll give you a hint![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeOkIllGive, 0, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8026, 0, 0, 0
    ActorMsgClose
    // "Aha: Ha ha, this is a good hint!\nChallenger, please answer![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaHaHaGood, 1, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8025, 1, 0, 0
    ActorMsgClose
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPhraseSelect 4, 0x8022, 0x8023, 0x8010
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0280
    VMStackPush 0x8022
    VMStackPush 0x8027
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027A
    WorkSetConst 0x8010, 1
    VMJump L_0280

L_027A:
    WorkSetConst 0x8010, 0

L_0280:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F1
    SEPlay SEQ_SE_FLD_41
    SEWait
    // "Aha: Woo-hoo!\nThat is c-o-r-r-e-c-t![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaWooHooC, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_0498
    ActorCmdWait
    // "Wye: You go! Yeah, you do![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeGoYeah, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 2, Movement_0488
    ActorCmdWait
    // "Ditoh: Yeah! Yeah! Good hustle![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_DitohYeahYeahGood, 2, 0, 0
    ActorMsgClose
    // "Aha: Congratulations![f000]븁\u0000\nNow--THIS is a prize.\nIt's an Antidote![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaCongratulationsNowPrize, 1, 0, 0
    ActorMsgClose
    WorkSetConst 0x8024, 18
    VMJump L_0349

L_02F1:
    SEPlay SEQ_SE_FLD_42
    SEWait
    // "Aha: Oh, no. Too bad!\nThat's not right, 'cause you are wrong![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaOhNoToo, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_04A0
    ActorCmdWait
    // "Wye: Aww... Sadness...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeAwwSadness, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 2, Movement_0488
    ActorCmdWait
    // "Ditoh: Good hustle!\nGustle! Gustle![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_DitohGoodHustleGustle, 2, 0, 0
    ActorMsgClose
    // "Aha: Yeah, you gustle![f000]븁\u0000\nHere ya go... Take this memento.\nIt's a Parlyz Heal![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaYeahGustleHere, 1, 0, 0
    ActorMsgClose
    WorkSetConst 0x8024, 22

L_0349:
    SEPlay SEQ_SE_FLD_10
    ActorNew 4, 5, 1, 251, 110, 0
    SEWait
    ActorCmdExec 255, Movement_0430
    ActorCmdWait
    ActorDelete 251
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8024
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Wye: Exciting! Thrilling! Zippy! Chilling!\nThat's “Pep Quiz\"![f000]븁\u0000\nSee ya tomorrow!"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeExcitingThrillingZippy_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    BGMChangeMap
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wye: I want to be on TV soon!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_WyeWantTvSoon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Aha: Do you like quiz shows?"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_AhaLikeQuizShows, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ditoh: Gussssssssstle!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity4_Text_DitohGussssssssstle, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03F8:
    Move 75, 1
    MoveEnd

Movement_0400:
    Move 37, 3
    Move 17, 3
    MoveEnd

Movement_040C:
    Move 12, 4
    Move 1, 1
    MoveEnd

Movement_0418:
    Move 12, 4
    Move 2, 1
    MoveEnd

Movement_0424:
    Move 12, 1
    Move 1, 1
    MoveEnd

Movement_0430:
    Move 63, 1
    Move 14, 1
    MoveEnd

Movement_043C:
    Move 35, 1
    MoveEnd

Movement_0444:
    Move 34, 1
    MoveEnd

Movement_044C:
    Move 38, 4
    MoveEnd

Movement_0454:
    Move 2, 1
    Move 62, 1
    Move 3, 1
    Move 62, 1
    Move 2, 1
    Move 62, 1
    Move 3, 1
    Move 62, 1
    Move 18, 1
    Move 19, 2
    Move 18, 1
    Move 33, 1
    MoveEnd

Movement_0488:
    Move 50, 1
    Move 61, 1
    Move 50, 1
    MoveEnd

Movement_0498:
    Move 49, 1
    MoveEnd

Movement_04A0:
    Move 2, 1
    Move 62, 1
    Move 3, 1
    Move 62, 1
    Move 2, 1
    Move 62, 1
    Move 3, 1
    Move 62, 1
    Move 33, 1
    MoveEnd
