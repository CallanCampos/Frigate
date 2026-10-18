#include "asm/field_script.inc"
#include "text/script/castelia_city_28.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_2:
    VMStackPush EVENT_WORK_0x411a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004B
    ActorSetGPos 0, 9, 0, 2, 1
    VMJump L_0057

L_004B:
    ActorSetGPos 0, 7, 0, 2, 1

L_0057:
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x411a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CD
    // "Do you want to go up?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_WantGoUp, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0194
    TrainerCardGetSex 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0172
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D3
    ActorCmdExec 0, Movement_03DC
    VMJump L_00FC

L_00D3:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F4
    ActorCmdExec 0, Movement_0408
    VMJump L_00FC

L_00F4:
    ActorCmdExec 0, Movement_0434

L_00FC:
    // "I need to check you.[f000]븁\u0000\nFrisk, frisk...\nFrisk, frisk... And one more frisk...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_NeedCheckFriskFrisk, 0, 0, 0
    ActorMsgClose
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012D
    ActorCmdExec 0, Movement_03F4
    VMJump L_0156

L_012D:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014E
    ActorCmdExec 0, Movement_0420
    VMJump L_0156

L_014E:
    ActorCmdExec 0, Movement_044C

L_0156:
    ActorCmdWait
    // "You don't seem to have\nanything suspicious.[f000]븁\u0000\nOK! You can go.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_DontSeemHaveAnything, 0, 0, 0
    ActorMsgClose
    VMCall L_036F
    VMJump L_0188

L_0172:
    // "OK! You can go."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_OkCanGo, 0, 0, 0
    MsgWaitAdvance
    ActorMsgClose
    VMCall L_036F

L_0188:
    WorkSetConst EVENT_WORK_0x411a, 1
    VMJump L_01C7

L_0194:
    MsgWinCloseAll
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01B7
    ActorCmdExec 0, Movement_046C
    ActorCmdWait

L_01B7:
    // "...OK. That's fine, then."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_OkThatsFineThen, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_01C7:
    VMJump L_01DD

L_01CD:
    // "OK! You can go."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_OkCanGo, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_01DD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    PlayerGetDir 0x8021
    ActorCmdExec 0, Movement_03AC
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_020C
    ActorCmdExec 255, Movement_0460

L_020C:
    ActorCmdWait
    // "Do you want to go up?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_WantGoUp, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_032E
    TrainerCardGetSex 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030C
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026D
    ActorCmdExec 0, Movement_03DC
    VMJump L_0296

L_026D:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028E
    ActorCmdExec 0, Movement_0408
    VMJump L_0296

L_028E:
    ActorCmdExec 0, Movement_0434

L_0296:
    // "I need to check you.[f000]븁\u0000\nFrisk, frisk...\nFrisk, frisk... And one more frisk...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_NeedCheckFriskFrisk, 0, 0, 0
    ActorMsgClose
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C7
    ActorCmdExec 0, Movement_03F4
    VMJump L_02F0

L_02C7:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E8
    ActorCmdExec 0, Movement_0420
    VMJump L_02F0

L_02E8:
    ActorCmdExec 0, Movement_044C

L_02F0:
    ActorCmdWait
    // "You don't seem to have\nanything suspicious.[f000]븁\u0000\nOK! You can go.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_DontSeemHaveAnything, 0, 0, 0
    ActorMsgClose
    VMCall L_036F
    VMJump L_0322

L_030C:
    // "OK! You can go."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_OkCanGo, 0, 0, 0
    MsgWaitAdvance
    ActorMsgClose
    VMCall L_036F

L_0322:
    WorkSetConst EVENT_WORK_0x411a, 1
    VMJump L_0369

L_032E:
    MsgWinCloseAll
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0351
    ActorCmdExec 0, Movement_046C
    ActorCmdWait

L_0351:
    // "...OK. That's fine, then.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_OkThatsFineThen_2, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 255, Movement_0474
    ActorCmdWait

L_0369:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_036F:
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0396
    ActorCmdExec 0, Movement_03BC
    ActorCmdWait
    VMJump L_03A0

L_0396:
    ActorCmdExec 0, Movement_03D0
    ActorCmdWait

L_03A0:
    VMReturn
    .balign 4, 0
    Move 35, 1
    MoveEnd

Movement_03AC:
    Move 34, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_03BC:
    Move 13, 1
    Move 15, 2
    Move 12, 1
    Move 33, 1
    MoveEnd

Movement_03D0:
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_03DC:
    Move 35, 1
    Move 13, 1
    Move 15, 2
    Move 12, 1
    Move 34, 4
    MoveEnd

Movement_03F4:
    Move 13, 1
    Move 14, 2
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_0408:
    Move 34, 1
    Move 13, 1
    Move 14, 2
    Move 12, 1
    Move 35, 4
    MoveEnd

Movement_0420:
    Move 13, 1
    Move 15, 2
    Move 12, 1
    Move 34, 1
    MoveEnd

Movement_0434:
    Move 33, 1
    Move 15, 1
    Move 13, 2
    Move 14, 1
    Move 32, 4
    MoveEnd

Movement_044C:
    Move 14, 1
    Move 12, 2
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_0460:
    Move 63, 1
    Move 35, 1
    MoveEnd

Movement_046C:
    Move 33, 1
    MoveEnd

Movement_0474:
    Move 13, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm invited to a party, but the person\nin front of the elevator wants[f000]븀\u0000\nto pat me down.[f000]븁\u0000\nOr is it just my imagination?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_ImInvitedPartyBut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I talked with a lot of people upstairs.\nIt was fun!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_TalkedLotPeopleUpstairs, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Where are you from?[f000]븁\u0000\nReally? You're from Aspertia City?\nIt's a great place![f000]븀\u0000\nThat outlook is fantastic!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity28_Text_WhereFromReallyYoure, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
