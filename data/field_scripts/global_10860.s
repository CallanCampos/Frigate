#include "asm/field_script.inc"
#include "text/script/global_10860.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    MEPlay SEQ_ME_CALL
    // "The Xtransceiver is ringing!"
    SystemMsg Global10860_Text_XtransceiverRinging, 2
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 picked up\nthe Xtransceiver.[f000]븁\u0000"
    SystemMsg Global10860_Text_PickedUpXtransceiver, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    HollowRivalCmd_0265 0
    FadeInBlackQ
    FadeWait
    RTGetZoneID 0x8021
    WorkCmpConst 0x8021, 62
    VMJumpIf CMP_EQ, L_004E
    VMJump L_005A

L_004E:
    WorkSetConst EVENT_WORK_0x4126, 2
    VMJump L_0210

L_005A:
    WorkCmpConst 0x8021, 96
    VMJumpIf CMP_EQ, L_006D
    VMJump L_0079

L_006D:
    WorkSetConst EVENT_WORK_0x4127, 2
    VMJump L_0210

L_0079:
    WorkCmpConst 0x8021, 107
    VMJumpIf CMP_EQ, L_008C
    VMJump L_0098

L_008C:
    WorkSetConst EVENT_WORK_0x4128, 2
    VMJump L_0210

L_0098:
    WorkCmpConst 0x8021, 406
    VMJumpIf CMP_EQ, L_00AB
    VMJump L_00B7

L_00AB:
    WorkSetConst EVENT_WORK_0x4129, 2
    VMJump L_0210

L_00B7:
    WorkCmpConst 0x8021, 412
    VMJumpIf CMP_EQ, L_00CA
    VMJump L_00D6

L_00CA:
    WorkSetConst EVENT_WORK_0x412a, 2
    VMJump L_0210

L_00D6:
    WorkCmpConst 0x8021, 458
    VMJumpIf CMP_EQ, L_00E9
    VMJump L_00F5

L_00E9:
    WorkSetConst EVENT_WORK_0x412b, 2
    VMJump L_0210

L_00F5:
    WorkCmpConst 0x8021, 329
    VMJumpIf CMP_EQ, L_0108
    VMJump L_0114

L_0108:
    WorkSetConst EVENT_WORK_0x412c, 2
    VMJump L_0210

L_0114:
    WorkCmpConst 0x8021, 331
    VMJumpIf CMP_EQ, L_0127
    VMJump L_0133

L_0127:
    WorkSetConst EVENT_WORK_0x412d, 2
    VMJump L_0210

L_0133:
    WorkCmpConst 0x8021, 337
    VMJumpIf CMP_EQ, L_0146
    VMJump L_0152

L_0146:
    WorkSetConst EVENT_WORK_0x412e, 2
    VMJump L_0210

L_0152:
    WorkCmpConst 0x8021, 348
    VMJumpIf CMP_EQ, L_0165
    VMJump L_0171

L_0165:
    WorkSetConst EVENT_WORK_0x412f, 2
    VMJump L_0210

L_0171:
    WorkCmpConst 0x8021, 365
    VMJumpIf CMP_EQ, L_0184
    VMJump L_0190

L_0184:
    WorkSetConst EVENT_WORK_0x4130, 2
    VMJump L_0210

L_0190:
    WorkCmpConst 0x8021, 368
    VMJumpIf CMP_EQ, L_01A3
    VMJump L_01AF

L_01A3:
    WorkSetConst EVENT_WORK_0x4131, 2
    VMJump L_0210

L_01AF:
    WorkCmpConst 0x8021, 370
    VMJumpIf CMP_EQ, L_01C2
    VMJump L_01CE

L_01C2:
    WorkSetConst EVENT_WORK_0x4132, 2
    VMJump L_0210

L_01CE:
    WorkCmpConst 0x8021, 374
    VMJumpIf CMP_EQ, L_01E1
    VMJump L_01ED

L_01E1:
    WorkSetConst EVENT_WORK_0x4133, 2
    VMJump L_0210

L_01ED:
    WorkCmpConst 0x8021, 383
    VMJumpIf CMP_EQ, L_0200
    VMJump L_020C

L_0200:
    WorkSetConst EVENT_WORK_0x4134, 2
    VMJump L_0210

L_020C:
    DebugPrint 0x8021

L_0210:
    HollowRivalCmd_02D3 5, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0283
    WorkSetConst EVENT_WORK_0x4126, 2
    WorkSetConst EVENT_WORK_0x4127, 2
    WorkSetConst EVENT_WORK_0x4128, 2
    WorkSetConst EVENT_WORK_0x4129, 2
    WorkSetConst EVENT_WORK_0x412a, 2
    WorkSetConst EVENT_WORK_0x412b, 2
    WorkSetConst EVENT_WORK_0x412c, 2
    WorkSetConst EVENT_WORK_0x412d, 2
    WorkSetConst EVENT_WORK_0x412e, 2
    WorkSetConst EVENT_WORK_0x412f, 2
    WorkSetConst EVENT_WORK_0x4130, 2
    WorkSetConst EVENT_WORK_0x4131, 2
    WorkSetConst EVENT_WORK_0x4132, 2
    WorkSetConst EVENT_WORK_0x4133, 2
    WorkSetConst EVENT_WORK_0x4134, 2

L_0283:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
