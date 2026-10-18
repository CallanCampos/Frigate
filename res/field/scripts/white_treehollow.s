#include "asm/field_script.inc"
#include "text/script/white_treehollow.h"

// Script plugin 9, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What? That Trainer named Benga is\nAlder's grandson?[f000]븁\u0000\nI should ask Alder in Floccesy Town\nto train me, too."
    ParentActorMsg MSGFILE_SCRIPT, WhiteTreehollow_Text_WhatTrainerNamedBenga, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0061
    WorkSetConst 0x8021, 0
    VMJump L_0086

L_0061:
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0080
    WorkSetConst 0x8021, 1
    VMJump L_0086

L_0080:
    WorkSetConst 0x8021, 2

L_0086:
    ParentActorMsg MSGFILE_SCRIPT, 0x8021, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_00C5
    WorkSetConst 0x8021, 3
    VMJump L_00EA

L_00C5:
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_00E4
    WorkSetConst 0x8021, 4
    VMJump L_00EA

L_00E4:
    WorkSetConst 0x8021, 5

L_00EA:
    ParentActorMsg MSGFILE_SCRIPT, 0x8021, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
