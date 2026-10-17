#include "asm/field_script.inc"
#include "text/script/castelia_city_10.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_7:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    RTCGetDate 0x8020, 0x8021
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 5
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 7
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 11
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 13
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 17
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 19
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 23
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 29
    VMJumpIf CMP_EQ, L_00C9
    WorkCmpConst 0x8021, 31
    VMJumpIf CMP_EQ, L_00C9
    VMJump L_00D5

L_00C9:
    WorkSetConst EVENT_WORK_0x4001, 1
    VMJump L_00DB

L_00D5:
    WorkSetConst EVENT_WORK_0x4001, 0

L_00DB:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    VMHalt

Script_6:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0106
    BMSetVisible 8, 0, 10, 0

L_0106:
    VMHalt

Script_8:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0125
    BMSetVisible 8, 0, 10, 0

L_0125:
    VMHalt

Script_2:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    VMSleep 70
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "Everyone, we've arrived at\nCastelia City![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_EveryoneWeveArrivedCastelia, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02BC
    ActorCmdExec 255, Movement_02BC
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: So this is Castelia City...\nIt's much bigger than I'd heard![f000]븁\u0000\nBut, it doesn't matter![f000]븁\u0000\nI'm going to find Team Plasma\nno matter where they run![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_CasteliaCityItsMuch, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02C4
    ActorCmdWait
    // "Oh yeah. [f000]Ā\u0001\u0000![f000]븁\u0000\nHere, let's register each other's\nXtransceiver number.[f000]븁\u0000\nWe didn't even need to in Aspertia.\nWe could see each other anytime![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_OhYeahHereLets, 1, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_SW_LC_NO
    SEWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9467, 0, 0x17b000, 0x136000, 0, 0x98000, 56
    ActorWalkRoute 1, 16, 9, 1, 8, 0
    ActorCmdExec 255, Movement_02D4
    ActorCmdWait
    EvCameraWait
    // "That's a strange ship.\nA sailing ship in this day and age?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_ThatsStrangeShipSailing, 1, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 56
    ActorCmdExec 1, Movement_0208
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 1
    SEWait
    HollowRivalCmd_0263 1
    WorkSetConst EVENT_WORK_0x40ae, 2
    WorkSetConst EVENT_WORK_0x40e2, 1
    FlagSet EVENT_FLAG_0x02ec
    HollowRivalCmd_0262 1, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0208:
    Move 12, 10
    MoveEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What can I do for you?\nWould you like to sail to Virbank City?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_WhatCanWouldLike, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0263
    // "Of course!\nPlease, step this way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_CoursePleaseStepWay, 0, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    RTReserveScript 5
    MapChangeCore ZONE_VIRBANK_CITY_5, 6, 0, 5, 1
    VMJump L_0273

L_0263:
    // "OK then! Please come talk to me\nwhenever you'd like to board!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_OkThenPleaseCome, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0273:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_02BC:
    Move 32, 1
    MoveEnd

Movement_02C4:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_02D4:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    VMStackPush EVENT_WORK_0x40ae
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_030B
    CallPlaceNameDisp
    DebugPrint 22

L_030B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you just can't stand it anymore,\nscream at the ocean!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_WhenJustCantStand, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That black sailing ship...\nWhat could it be?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_BlackSailingShipWhat, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0370

L_035C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That black sailing ship...\nWhat could it have been?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity10_Text_BlackSailingShipWhat_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_0370:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
