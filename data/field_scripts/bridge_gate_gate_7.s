#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0031
    ActorSetGPos 2, 20, 0, 4, 0

L_0031:
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0062
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That elevator wasn't broken.\nI heard a Pokémon called Rotom[f000]븀\u0000\nwas playing a prank![f000]븁\u0000\nThat's right! Rotom is a Pokémon\nthat can go inside electrical appliances!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate7_Text_ElevatorWasntBrokenHeard, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0076

L_0062:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What? Oh no!\nIs the elevator broken?"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate7_Text_WhatOhNoElevator, 0, 0
    LastKeyWait
    ActorMsgClose

L_0076:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "No matter what time or place,\nI have my umbrella at the ready![f000]븁\u0000\nAn ounce of prevention\nis worth a pound of cure!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate7_Text_NoMatterWhatTime, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0181
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0126
    // "Oh, awesome! That just made my day![f000]븁\u0000\nThis vending machine gave me\nan extra drink! For free![f000]븁\u0000\nI'll share my spoils with you.\nOtherwise, I'll burst with joy![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate7_Text_OhAwesomeJustMade, 0, 0
    ItemCheckSpace ITEM_ULTRA_BALL, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0112
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "If I keep getting extra drinks,\nI'll be rich!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate7_Text_IfKeepGettingExtra, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0181
    VMJump L_0120

L_0112:
    // "So awesome![f000]븁\u0000\nI was so moved![f000]븁\u0000\nWhat?! Your bag's full!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate7_Text_AwesomeMovedWhatBags, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0120:
    VMJump L_0134

L_0126:
    // "If I keep getting extra drinks,\nI'll be rich!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate7_Text_IfKeepGettingExtra, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0134:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
