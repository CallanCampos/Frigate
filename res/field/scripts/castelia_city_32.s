#include "asm/field_script.inc"
#include "text/script/castelia_city_32.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_4:
    ActorsPauseAll
    ActorCmdExec 2, Movement_041C
    VMSleep 4
    ActorCmdExec 4, Movement_0410
    ActorCmdExec 3, Movement_0410
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 1
    ActorWalkRoute 4, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 4, Movement_03F8
    ActorCmdWait
    // "Hey! This is our secret spot![f000]븁\u0000\nYou can't come waltzin' in\nhere like you own the place![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_HeyOurSecretSpot, 4, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_ROUGHNECK_RICKY, 0, 0
    VMCall L_0382
    // "You aren't one of us...\nBut I don't mind tough Trainers."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_ArentOneUsBut, 4, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4147, 1
    TrainerFlagSet TRAINER_ROUGHNECK_RICKY
    ActorCmdExec 4, Movement_03AC
    VMSleep 8
    ActorCmdExec 3, Movement_03BC
    VMSleep 4
    ActorCmdExec 255, Movement_0400
    ActorCmdWait
    VMCall L_020B
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4147
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FD
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I recognize toughness when I see it.\nSo you're one of us now!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_RecognizeToughnessWhenSee, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0111

L_00FD:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you beat all three of us, maybe\nwe will let you be in our group."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_IfBeatAllThree, 0, 0
    LastKeyWait
    ActorMsgClose

L_0111:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    TrainerFlagGet TRAINER_DANCER_JEAN_PAUL, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x4147
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You're our leader now, and Tina's\nthe second in command![f000]븀\u0000\nAll right! Let's dance!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_YoureOurLeaderNow, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_018B

L_014C:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0179
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We've always stuck together!\nI'm not going to trust some outsider!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_WeveAlwaysStuckTogether, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_018B

L_0179:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_020B
    VMCall L_0265

L_018B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    TrainerFlagGet TRAINER_GUITARIST_TINA, EVENT_WORK_0x400f
    TrainerFlagGet TRAINER_DANCER_JEAN_PAUL, EVENT_WORK_0x400e
    VMStackPush EVENT_WORK_0x4147
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "New buildings create old places.[f000]븁\u0000\nGot it?\nEven Castelia City has an underbelly."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_NewBuildingsCreateOld, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0205

L_01CC:
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you want to battle me, you'll\nhave to defeat Jean-Paul first!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_IfWantBattleYoull, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0205

L_01F9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0265

L_0205:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_020B:
    // "This worn-out lot is our paradise![f000]븁\u0000\nWe're not gonna let some stranger\ncome in and trash the place![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_WornOutLotOur, 3, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_DANCER_JEAN_PAUL, 0, 0
    VMCall L_0382
    // "What's with you anyway?!\nYour fighting has a tight rhythm.[f000]븀\u0000\nI was groovin' with it before I knew it!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_WhatsAnywayFightingHas, 3, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 3, Movement_03CC
    VMSleep 8
    ActorCmdExec 2, Movement_03DC
    VMSleep 4
    ActorCmdExec 255, Movement_03F0
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x4147, 2
    TrainerFlagSet TRAINER_DANCER_JEAN_PAUL
    VMReturn

L_0265:
    // "You made my crew cry![f000]븁\u0000\nI'm gonna pay you back in spades![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_MadeCrewCryIm, 2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_GUITARIST_TINA, 0, 0
    VMCall L_0382
    // "People like you deserve a Medal...[f000]븁\u0000\nOK. I've decided.\nYou're the new boss of this area!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity32_Text_PeopleLikeDeserveMedal, 2, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 2, Movement_03E4
    ActorCmdWait
    MedalGive 95
    WorkSetConst EVENT_WORK_0x4147, 3
    TrainerFlagSet TRAINER_GUITARIST_TINA
    VMReturn

L_02AB:
    VMStackPush EVENT_WORK_0x4147
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F4
    ActorSetGPos 255, 17, 0, 6, 3
    ActorSetGPos 2, 17, 0, 4, 1
    ActorSetGPos 4, 18, 0, 6, 2
    ActorSetGPos 3, 15, 0, 5, 1
    VMJump L_0380

L_02F4:
    VMStackPush EVENT_WORK_0x4147
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033D
    ActorSetGPos 255, 17, 0, 6, 2
    ActorSetGPos 2, 17, 0, 4, 1
    ActorSetGPos 4, 19, 0, 5, 1
    ActorSetGPos 3, 16, 0, 6, 3
    VMJump L_0380

L_033D:
    VMStackPush EVENT_WORK_0x4147
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0380
    ActorSetGPos 255, 17, 0, 6, 0
    ActorSetGPos 2, 17, 0, 5, 1
    ActorSetGPos 4, 19, 0, 5, 2
    ActorSetGPos 3, 15, 0, 5, 3

L_0380:
    VMReturn

L_0382:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03A7
    VMCall L_02AB
    CallTrainerBattleEnd
    VMJump L_03A9

L_03A7:
    CallTrainerLose

L_03A9:
    VMReturn
    .balign 4, 0

Movement_03AC:
    Move 15, 1
    Move 12, 1
    Move 33, 1
    MoveEnd

Movement_03BC:
    Move 15, 1
    Move 13, 1
    Move 35, 1
    MoveEnd

Movement_03CC:
    Move 14, 1
    Move 12, 1
    Move 33, 1
    MoveEnd

Movement_03DC:
    Move 13, 1
    MoveEnd

Movement_03E4:
    Move 12, 1
    Move 33, 1
    MoveEnd

Movement_03F0:
    Move 32, 1
    MoveEnd

Movement_03F8:
    Move 33, 1
    MoveEnd

Movement_0400:
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0410:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_041C:
    Move 75, 1
    MoveEnd
