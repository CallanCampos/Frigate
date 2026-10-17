#include "asm/field_script.inc"
#include "text/script/cave_of_being.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    ActorCmdExec 0, Movement_0384
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0061
    WorkSub 0x8022, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    VMJump L_0081

L_0061:
    WorkSub 0x8022, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 0, Movement_0350
    ActorCmdWait

L_0081:
    WordSetPlayerName 0
    // "Professor Juniper: Why, if it isn't\n[f000]Ā\u0001\u0000![f000]븁\u0000\nThat's someone with a Pokédex for you,\nnoticing a place like this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CaveOfBeing_Text_ProfessorJuniperWhyIf, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0368
    ActorCmdWait
    VMSleep 8
    ActorCmdExec 0, Movement_0350
    ActorCmdWait
    // "Still...this space...[f000]븁\u0000\nIt's hard to put into words, but\nit feels full of something..."
    ActorMsg MSGFILE_SCRIPT, CaveOfBeing_Text_StillSpaceItsHard, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4115, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    // "A mysterious presence can be felt!"
    SystemMsg CaveOfBeing_Text_MysteriousPresenceCanFelt, 2
    LastKeyWait
    InfoMsgClose
    FlagReset EVENT_FLAG_0x0394
    ActorCmdExec 0, Movement_0348
    ActorCmdExec 255, Movement_0358
    ActorCmdWait
    ActorWalkRoute 255, 18, 17, 1, 8, 0
    ActorCmdWait
    ActorWalkRoute 0, 17, 19, 1, 8, 0
    ActorCmdWait
    VMSleep 16
    ActorAdd 2
    ActorMoveLinear 2, 18, 0, 15, 32
    ActorSetGPos 2, 18, 0, 15, 1
    PVPlay 480, 0
    // "Kyouuuun!"
    InfoMsg CaveOfBeing_Text_Kyouuuun, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "Uxie![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CaveOfBeing_Text_Uxie, 0, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorAdd 3
    ActorMoveLinear 3, 16, 0, 17, 32
    ActorSetGPos 3, 16, 0, 17, 1
    PVPlay 481, 0
    // "Kyauun!"
    InfoMsg CaveOfBeing_Text_Kyauun, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0340
    VMSleep 3
    ActorCmdExec 0, Movement_0340
    ActorCmdWait
    // "Mesprit...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CaveOfBeing_Text_Mesprit, 0, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorAdd 1
    ActorMoveLinear 1, 20, 0, 17, 32
    ActorSetGPos 1, 20, 0, 17, 1
    PVPlay 482, 0
    // "Kyuuun!"
    InfoMsg CaveOfBeing_Text_Kyuuun, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0338
    VMSleep 3
    ActorCmdExec 0, Movement_0338
    ActorCmdWait
    // "And Azelf![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CaveOfBeing_Text_Azelf, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0348
    VMSleep 3
    ActorCmdExec 0, Movement_0348
    ActorCmdWait
    VMSleep 32
    ActorCmdExec 2, Movement_03A8
    ActorCmdWait
    ActorMoveLinear 2, 19, 0, 20, 16
    ActorCmdExec 2, Movement_0398
    ActorCmdWait
    ActorMoveLinear 2, 18, 0, 24, 10
    ActorDelete 2
    // "Uxie went flying off somewhere...[f000]븁\u0000"
    SystemMsg CaveOfBeing_Text_UxieWentFlyingOff, 2
    InfoMsgClose
    VMSleep 8
    ActorCmdExec 3, Movement_03A0
    ActorCmdWait
    ActorMoveLinear 3, 15, 0, 20, 12
    ActorCmdExec 3, Movement_0398
    ActorCmdWait
    ActorMoveLinear 3, 18, 0, 24, 12
    ActorDelete 3
    // "Mesprit went flying off somewhere...[f000]븁\u0000"
    SystemMsg CaveOfBeing_Text_MespritWentFlyingOff, 2
    InfoMsgClose
    VMSleep 8
    ActorCmdExec 1, Movement_03A8
    ActorCmdWait
    ActorMoveLinear 1, 21, 0, 20, 10
    ActorCmdExec 1, Movement_0398
    ActorCmdWait
    ActorMoveLinear 1, 18, 0, 24, 12
    ActorDelete 1
    // "Azelf went flying off somewhere...[f000]븁\u0000"
    SystemMsg CaveOfBeing_Text_AzelfWentFlyingOff, 2
    InfoMsgClose
    VMSleep 16
    ActorCmdExec 255, Movement_0350
    ActorCmdWait
    WordSetPlayerName 0
    // "Professor Juniper: Those three are\nUxie, Mesprit, and Azelf.[f000]븁\u0000\nThey're thought to be the Pokémon\nthat gave us knowledge, emotion,[f000]븀\u0000\nand willpower.[f000]븁\u0000\nStill, I wonder where those three went...[f000]븁\u0000\nHey, [f000]Ā\u0001\u0000![f000]븁\u0000\nLook all over the Unova region,\nand try to find them!"
    ActorMsg MSGFILE_SCRIPT, CaveOfBeing_Text_ProfessorJuniperThoseThree, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 18, 27, 1, 8, 1
    ActorCmdWait
    ActorDelete 0
    WorkSetConst EVENT_WORK_0x4115, 4
    WorkSetConst EVENT_WORK_0x4116, 1
    WorkSetConst EVENT_WORK_0x4117, 1
    WorkSetConst EVENT_WORK_0x4118, 1
    FlagSet EVENT_FLAG_0x0394
    FlagSet EVENT_FLAG_0x0395
    HollowRivalCmd_0262 0, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Still...this space...[f000]븁\u0000\nIt's hard to put into words, but\nit feels full of something..."
    ParentActorMsg MSGFILE_SCRIPT, CaveOfBeing_Text_StillSpaceItsHard, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0338:
    Move 35, 1
    MoveEnd

Movement_0340:
    Move 34, 1
    MoveEnd

Movement_0348:
    Move 32, 1
    MoveEnd

Movement_0350:
    Move 33, 1
    MoveEnd

Movement_0358:
    Move 12, 1
    MoveEnd
    Move 13, 1
    MoveEnd

Movement_0368:
    Move 61, 1
    Move 35, 1
    Move 34, 1
    Move 61, 1
    MoveEnd
    Move 75, 1
    MoveEnd

Movement_0384:
    Move 33, 1
    Move 75, 1
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_0398:
    Move 1, 1
    MoveEnd

Movement_03A0:
    Move 2, 1
    MoveEnd

Movement_03A8:
    Move 3, 1
    MoveEnd
