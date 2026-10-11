#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0047
    ActorSetGPos 2, 3, 0, 8, 3

L_0047:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you've crossed\nall of the bridges in Unova,[f000]븀\u0000\nsomething really cool will appear![f000]븁\u0000\nIf I spread this rumor, I wonder\nif it'll become an urban legend..."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate3_Text_WhenYouveCrossedAll, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Some Trainers take a Pokémon\ncalled Rotom into the storeroom[f000]븀\u0000\nof Shopping Mall Nine.[f000]븀\u0000\nI wonder what they're doing..."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate3_Text_SomeTrainersTakePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Halt![f000]븁\u0000\nThe Tubeline Bridge is currently\nundergoing a test to see how[f000]븀\u0000\nmany people it can hold![f000]븁\u0000\nThat's right! I can't let any\nmore people in right now!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate3_Text_HaltTubelineBridgeCurrently, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00C4

L_00B0:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The results of the test showed that\n4,934 people can be on the[f000]븀\u0000\nTubeline Bridge at one time."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate3_Text_ResultsTestShowed4, 0, 0
    LastKeyWait
    ActorMsgClose

L_00C4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0101
    ActorCmdExec 2, Movement_0154
    VMSleep 40
    ActorCmdExec 255, Movement_0184
    ActorCmdWait
    VMJump L_012A

L_0101:
    VMStackPush 0x8022
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012A
    ActorCmdExec 2, Movement_0160
    VMSleep 40
    ActorCmdExec 255, Movement_017C
    ActorCmdWait

L_012A:
    // "Halt![f000]븁\u0000\nThe Tubeline Bridge is currently\nundergoing a test to see how[f000]븀\u0000\nmany people it can hold![f000]븁\u0000\nThat's right! I can't let any\nmore people in right now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BridgeGateGate3_Text_HaltTubelineBridgeCurrently_2, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_018C
    VMSleep 8
    ActorCmdExec 2, Movement_016C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0154:
    Move 75, 1
    Move 32, 1
    MoveEnd

Movement_0160:
    Move 75, 1
    Move 33, 1
    MoveEnd

Movement_016C:
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_017C:
    Move 32, 1
    MoveEnd

Movement_0184:
    Move 33, 1
    MoveEnd

Movement_018C:
    Move 15, 1
    MoveEnd
