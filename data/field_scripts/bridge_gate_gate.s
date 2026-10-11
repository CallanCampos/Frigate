#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello! If you cross the Skyarrow Bridge\nfrom this side, you will reach[f000]븀\u0000\nPinwheel Forest and Nacrene City."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_HelloIfCrossSkyarrow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0087
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I walked so far! My legs are sore!\n...Um, you don't have to look."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_WalkedFarLegsSore, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_009B

L_0087:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Skyarrow Bridge has\nbeen around for a long time."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_SkyarrowBridgeHasBeen, 0, 0
    LastKeyWait
    ActorMsgClose

L_009B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We're inspecting the Skyarrow Bridge\nto make sure it's strong enough.[f000]븁\u0000\nThis is the first inspection in a few\nyears, so it may take a while.[f000]븀\u0000\nPlease wait. Thank you."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_WereInspectingSkyarrowBridge, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorCmdExec 1, Movement_0374
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FE
    ActorCmdExec 1, Movement_0354
    VMSleep 8
    ActorCmdExec 255, Movement_034C
    ActorCmdWait
    VMJump L_0127

L_00FE:
    VMStackPush 0x8021
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0127
    ActorCmdExec 1, Movement_034C
    VMSleep 8
    ActorCmdExec 255, Movement_0354
    ActorCmdWait

L_0127:
    // "Please wait to cross.[f000]븁\u0000\nWe're inspecting the Skyarrow Bridge\nto make sure it's strong enough.[f000]븁\u0000\nThis is the first inspection in a few\nyears, so it may take a while.[f000]븀\u0000\nThank you for your patience.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_PleaseWaitCrossWere, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_036C
    VMSleep 8
    ActorCmdExec 1, Movement_035C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 217
    WorkSet 0x8001, 1
    WorkSet 0x8002, 442
    WorkSet 0x8003, 5
    WorkSet 0x8004, 6
    WorkSet 0x8005, 6
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_032C
    // "Free-for-all! It's the Castelia\nHarlequin Hunt! You haven't visited...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_FreeAllItsCastelia, 0, 0
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023B
    // "You still need to visit\nthis many places: Wow! Zero![f000]븁\u0000\nThat means you've completed\nthe Castelia Harlequin Hunt![f000]븁\u0000\nCongratulations!\nThis is a small commemorative gift![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_StillNeedVisitMany_2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 50
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "The Castelia Harlequin Hunt is a way\nto make more people love Castelia City![f000]븁\u0000\nThat's why we generously gave you a\nBicycle at the beginning. It's the best[f000]븀\u0000\nway to get around Castelia City![f000]븁\u0000\nKeep loving Castelia City![f000]븁\u0000\nCastelia City, Castelia City,\nCastelia City! ♪ Here we go! ♪"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_CasteliaHarlequinHuntWay, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40e2, 6
    VMJump L_0326

L_023B:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

L_0247:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0311
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0138
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0295
    // "The Medal Office![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_MedalOffice, 4, 0, 0
    WorkAdd 0x8023, 1
    VMJump L_0305

L_0295:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0139
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02D0
    // "Passerby Analytics HQ![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_PasserbyAnalyticsHq, 4, 0, 0
    WorkAdd 0x8023, 1
    VMJump L_0305

L_02D0:
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x013a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0305
    // "The Battle Company![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_BattleCompany, 4, 0, 0
    WorkAdd 0x8023, 1

L_0305:
    WorkAdd 0x8024, 1
    VMJump L_0247

L_0311:
    WordSetNumber 0, 0x8023, 1
    // "You still need to visit\nthis many places: [f000]Ȁ\u0001\u0000![f000]븁\u0000\nSo explore Castelia City, and enjoy\nthe Castelia Harlequin Hunt!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_StillNeedVisitMany, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0326:
    VMJump L_033A

L_032C:
    // "The Castelia Harlequin Hunt is a way\nto make more people love Castelia City![f000]븁\u0000\nThat's why we generously gave you a\nBicycle at the beginning. It's the best[f000]븀\u0000\nway to get around Castelia City![f000]븁\u0000\nKeep loving Castelia City![f000]븁\u0000\nCastelia City, Castelia City,\nCastelia City! ♪ Here we go! ♪"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate_Text_CasteliaHarlequinHuntWay, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_033A:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_034C:
    Move 35, 1
    MoveEnd

Movement_0354:
    Move 34, 1
    MoveEnd

Movement_035C:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_036C:
    Move 12, 1
    MoveEnd

Movement_0374:
    Move 75, 1
    MoveEnd
