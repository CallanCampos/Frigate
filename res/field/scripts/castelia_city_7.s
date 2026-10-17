#include "asm/field_script.inc"
#include "text/script/castelia_city_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Café Sonata"
    MsgPlaceSign CasteliaCity7_Text_CafeSonata, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PlayerGetGPos 0x8020, 0x8021
    // "Where did I put those sunglasses?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity7_Text_WhereDidPutThose, 0, 3, 1
    ActorMsgClose
    WorkCmpConst 0x8020, 14
    VMJumpIf CMP_EQ, L_0065
    VMJump L_007D

L_0065:
    ActorCmdExec 255, Movement_0200
    ActorCmdExec 0, Movement_020C
    ActorCmdWait
    VMJump L_0129

L_007D:
    WorkCmpConst 0x8020, 15
    VMJumpIf CMP_EQ, L_0090
    VMJump L_00A8

L_0090:
    ActorCmdExec 255, Movement_0200
    ActorCmdExec 0, Movement_0214
    ActorCmdWait
    VMJump L_0129

L_00A8:
    WorkCmpConst 0x8020, 16
    VMJumpIf CMP_EQ, L_00BB
    VMJump L_00D3

L_00BB:
    ActorCmdExec 255, Movement_0200
    ActorCmdExec 0, Movement_0220
    ActorCmdWait
    VMJump L_0129

L_00D3:
    WorkCmpConst 0x8020, 17
    VMJumpIf CMP_EQ, L_00E6
    VMJump L_00FE

L_00E6:
    ActorCmdExec 255, Movement_0200
    ActorCmdExec 0, Movement_022C
    ActorCmdWait
    VMJump L_0129

L_00FE:
    WorkCmpConst 0x8020, 18
    VMJumpIf CMP_EQ, L_0111
    VMJump L_0129

L_0111:
    ActorCmdExec 255, Movement_0200
    ActorCmdExec 0, Movement_0238
    ActorCmdWait
    VMJump L_0129

L_0129:
    // "Ah, I found them.[f000]븁\u0000\nAnd you've really got to have more\nlight to see your way by.[f000]븁\u0000\nTake this--it'll help you see in\ndark places.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity7_Text_AhFoundThemYouve, 0, 3, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 397
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "If you use the move Flash, the accuracy\nof the opponent's moves goes down.[f000]븁\u0000\nWhen you use it twice, the rate to get\nhit by a move will be about half.[f000]븁\u0000\n'Cause it means more light![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity7_Text_IfUseMoveFlash, 0, 3, 0
    MsgWinCloseAll
    WorkCmpConst 0x8020, 15
    VMJumpIf CMP_EQ, L_0178
    VMJump L_0188

L_0178:
    ActorCmdExec 0, Movement_0244
    ActorCmdWait
    VMJump L_01F1

L_0188:
    WorkCmpConst 0x8020, 16
    VMJumpIf CMP_EQ, L_019B
    VMJump L_01AB

L_019B:
    ActorCmdExec 0, Movement_0250
    ActorCmdWait
    VMJump L_01F1

L_01AB:
    WorkCmpConst 0x8020, 17
    VMJumpIf CMP_EQ, L_01BE
    VMJump L_01CE

L_01BE:
    ActorCmdExec 0, Movement_025C
    ActorCmdWait
    VMJump L_01F1

L_01CE:
    WorkCmpConst 0x8020, 18
    VMJumpIf CMP_EQ, L_01E1
    VMJump L_01F1

L_01E1:
    ActorCmdExec 0, Movement_0268
    ActorCmdWait
    VMJump L_01F1

L_01F1:
    WorkSetConst EVENT_WORK_0x4137, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0200:
    Move 75, 1
    Move 34, 1
    MoveEnd

Movement_020C:
    Move 35, 1
    MoveEnd

Movement_0214:
    Move 15, 1
    Move 35, 1
    MoveEnd

Movement_0220:
    Move 15, 2
    Move 35, 1
    MoveEnd

Movement_022C:
    Move 15, 3
    Move 35, 1
    MoveEnd

Movement_0238:
    Move 15, 4
    Move 35, 1
    MoveEnd

Movement_0244:
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_0250:
    Move 14, 2
    Move 35, 1
    MoveEnd

Movement_025C:
    Move 14, 3
    Move 35, 1
    MoveEnd

Movement_0268:
    Move 14, 4
    Move 35, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4137
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D9
    // "Meow!\nHow did you find me?![f000]븁\u0000\nYou are something else!\nSo, I'm going to give you something. This![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity7_Text_MeowHowDidFind, 0, 3, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 397
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "If you use the move Flash, the accuracy\nof the opponent's moves goes down.[f000]븁\u0000\nWhen you use it twice, the rate to get\nhit by a move will be about half.[f000]븁\u0000\n'Cause it means more light!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity7_Text_IfUseMoveFlash_2, 0, 3, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4137, 1
    VMJump L_02E9

L_02D9:
    // "If you use the move Flash, the accuracy\nof the opponent's moves goes down.[f000]븁\u0000\nWhen you use it twice, the rate to get\nhit by a move will be about half.[f000]븁\u0000\n'Cause it means more light!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity7_Text_IfUseMoveFlash_2, 0, 3, 0
    LastKeyWait
    MsgWinCloseAll

L_02E9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMCall L_030B
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMCall L_030B
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_030B:
    SEPlay SEQ_SE_MESSAGE
    MultiMsg 5, 22, 21, 1
    VMSleep 20
    MultiMsg 6, 15, 12, 2
    VMSleep 20
    MsgWinCloseNo 1
    VMSleep 10
    MultiMsg 7, 22, 21, 3
    VMSleep 20
    MsgWinCloseNo 2
    VMSleep 10
    MultiMsg 8, 15, 12, 4
    VMSleep 20
    MsgWinCloseNo 3
    VMSleep 10
    MultiMsg 9, 22, 21, 5
    VMSleep 20
    MsgWinCloseNo 4
    VMSleep 10
    MultiMsg 10, 15, 12, 6
    VMSleep 20
    MsgWinCloseNo 5
    VMSleep 10
    MultiMsg 11, 22, 21, 7
    VMSleep 20
    MsgWinCloseNo 6
    VMSleep 10
    MultiMsg 12, 15, 12, 8
    VMSleep 20
    MsgWinCloseNo 7
    VMSleep 10
    MsgWinCloseNo 8
    VMReturn
    .balign 4, 0
