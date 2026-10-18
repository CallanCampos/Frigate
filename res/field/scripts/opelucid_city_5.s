#include "asm/field_script.inc"
#include "text/script/opelucid_city_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
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
    WorkSetConst 0x802f, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FE
    // "Mimicking somebody is fun, isn't it?[f000]븁\u0000\nDo you want to find the one who\nmimics me among my friends?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_MimickingSomebodyFunIsnt, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E0
    // "OK! Here goes!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_OkHereGoes, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0718
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x4000, 1
    VMCall L_011C
    VMJump L_00F8

L_00E0:
    // "I see..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_See, 0, 0
    LastKeyWait
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0718
    ActorCmdWait

L_00F8:
    VMJump L_0116

L_00FE:
    // "Who mimicked me?\nSpeak to that person."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_WhoMimickedSpeakPerson, 0, 0
    LastKeyWait
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0718
    ActorCmdWait

L_0116:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_011C:
    PlayerGetGPos 0x8025, 0x8026
    WorkSetConst 0x8028, 13
    WorkSetConst 0x802a, 14
    WorkSetConst 0x802c, 6
    WorkSetConst 0x802e, 19
    WorkSetConst 0x8029, 6
    WorkSetConst 0x802b, 15
    WorkSetConst 0x802d, 10
    WorkSetConst 0x802f, 10
    EvCameraInit
    EvCameraUnbind
    VMStackPush 0x8026
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0187
    EvCameraMoveTo 9688, 0, 0xed000, 0x9b000, 0, 0xa3000, 40
    VMJump L_019F

L_0187:
    EvCameraMoveTo 9688, 0, 0xed000, 0x9b000, 0, 0xa3000, 16

L_019F:
    EvCameraWait
    Random EVENT_WORK_0x4001, 4
    WorkSetConst 0x8020, 3
    WorkAdd 0x8020, EVENT_WORK_0x4001
    MultiMsg 0x8020, 0x8028, 0x8029, 1
    VMSleep 32
    MsgWinCloseNo 1
    VMSleep 16
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01EE
    WorkSetConst 0x8021, 12
    WorkSetConst 0x8022, 13
    VMJump L_0257

L_01EE:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0213
    WorkSetConst 0x8021, 14
    WorkSetConst 0x8022, 15
    VMJump L_0257

L_0213:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0238
    WorkSetConst 0x8021, 16
    WorkSetConst 0x8022, 17
    VMJump L_0257

L_0238:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0257
    WorkSetConst 0x8021, 18
    WorkSetConst 0x8022, 19

L_0257:
    Random EVENT_WORK_0x4002, 3
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029A
    MultiMsg 0x8020, 0x802c, 0x802d, 2
    MultiMsg 0x8021, 0x802a, 0x802b, 3
    MultiMsg 0x8022, 0x802e, 0x802f, 4
    WorkSetConst EVENT_WORK_0x4003, 0
    VMJump L_030E

L_029A:
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D7
    MultiMsg 0x8021, 0x802c, 0x802d, 2
    MultiMsg 0x8020, 0x802a, 0x802b, 3
    MultiMsg 0x8022, 0x802e, 0x802f, 4
    WorkSetConst EVENT_WORK_0x4003, 1
    VMJump L_030E

L_02D7:
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030E
    MultiMsg 0x8021, 0x802c, 0x802d, 2
    MultiMsg 0x8022, 0x802a, 0x802b, 3
    MultiMsg 0x8020, 0x802e, 0x802f, 4
    WorkSetConst EVENT_WORK_0x4003, 2

L_030E:
    VMSleep 32
    MsgWinCloseNo 2
    MsgWinCloseNo 3
    MsgWinCloseNo 4
    VMSleep 16
    VMStackPush 0x8026
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_033F
    EvCameraMoveToDefault 40
    VMJump L_0343

L_033F:
    EvCameraMoveToDefault 16

L_0343:
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn

Script_2:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm not a mimicker.\nBut mimicking is fun."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_ImNotMimickerBut, 0, 0
    LastKeyWait
    MsgWinCloseAll
    PlayerGetDir 0x8024
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0395
    ActorCmdExec 2, Movement_0710
    ActorCmdWait

L_0395:
    VMJump L_046B

L_039B:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046B
    SEPlay SEQ_SE_MESSAGE
    // "Did that friend mimic me?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_DidFriendMimic, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_045B
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FA
    // "Correct![f000]븁\u0000\nTrainer, you're great!\nDo you want to try again?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_CorrectTrainerYoureGreat, 0, 0, 0
    VMJump L_0406

L_03FA:
    // "Hmmm... Too bad.\nDo you want to try again?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_HmmmTooBadWant, 0, 0, 0

L_0406:
    WorkSetConst EVENT_WORK_0x4000, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0445
    // "OK! Here goes!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_OkHereGoes, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4000, 1
    VMCall L_011C
    VMJump L_0455

L_0445:
    // "I see..."
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_See, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0455:
    VMJump L_046B

L_045B:
    // "Which of my friends\nmimicked me?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_WhichFriendsMimicked, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_046B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to cherish my originality."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_WantCherishOriginality, 0, 0
    LastKeyWait
    MsgWinCloseAll
    PlayerGetDir 0x8024
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_04BB
    ActorCmdExec 3, Movement_0700
    ActorCmdWait

L_04BB:
    VMJump L_0591

L_04C1:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0591
    SEPlay SEQ_SE_MESSAGE
    // "Did that friend mimic me?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_DidFriendMimic, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0581
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0520
    // "Correct![f000]븁\u0000\nTrainer, you're great!\nDo you want to try again?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_CorrectTrainerYoureGreat, 0, 0, 0
    VMJump L_052C

L_0520:
    // "Hmmm... Too bad.\nDo you want to try again?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_HmmmTooBadWant, 0, 0, 0

L_052C:
    WorkSetConst EVENT_WORK_0x4000, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_056B
    // "OK! Here goes!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_OkHereGoes, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4000, 1
    VMCall L_011C
    VMJump L_057B

L_056B:
    // "I see..."
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_See, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_057B:
    VMJump L_0591

L_0581:
    // "Which of my friends\nmimicked me?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_WhichFriendsMimicked, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0591:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05E7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's nicer to be mimicked\nthan to mimic somebody!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_ItsNicerMimickedThan, 0, 0
    LastKeyWait
    MsgWinCloseAll
    PlayerGetDir 0x8024
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_05E1
    ActorCmdExec 1, Movement_0708
    ActorCmdWait

L_05E1:
    VMJump L_06B7

L_05E7:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06B7
    SEPlay SEQ_SE_MESSAGE
    // "Did that friend mimic me?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_DidFriendMimic, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06A7
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0646
    // "Correct![f000]븁\u0000\nTrainer, you're great!\nDo you want to try again?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_CorrectTrainerYoureGreat, 0, 0, 0
    VMJump L_0652

L_0646:
    // "Hmmm... Too bad.\nDo you want to try again?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_HmmmTooBadWant, 0, 0, 0

L_0652:
    WorkSetConst EVENT_WORK_0x4000, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0691
    // "OK! Here goes!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_OkHereGoes, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4000, 1
    VMCall L_011C
    VMJump L_06A1

L_0691:
    // "I see..."
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_See, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_06A1:
    VMJump L_06B7

L_06A7:
    // "Which of my friends\nmimicked me?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_WhichFriendsMimicked, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_06B7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can use the target's\nlast move during a battle.[f000]븁\u0000\nThat is Mimic![f000]븁\u0000\nMy Galvantula is charming,\neven though it won't learn Mimic!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_CanUseTargetsLast, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 596, 0
    // "Bzzz... Zzz..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity5_Text_BzzzZzz, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0700:
    Move 35, 1
    MoveEnd

Movement_0708:
    Move 34, 1
    MoveEnd

Movement_0710:
    Move 32, 1
    MoveEnd

Movement_0718:
    Move 33, 1
    MoveEnd
