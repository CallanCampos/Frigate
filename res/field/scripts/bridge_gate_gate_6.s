#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I searched many other places\nbesides Route 13 and, finally,[f000]븀\u0000\nI met my partner here!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate6_Text_SearchedManyOtherPlaces, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 575, 0
    // "Mwaan! ♪"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate6_Text_Mwaan, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What do people buy from the\nvending machine?[f000]븁\u0000\nI've diligently checked this...\nAnd I learned something.[f000]븁\u0000\nThe chance of getting an extra\nitem is three percent!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate6_Text_WhatPeopleBuyFrom, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
