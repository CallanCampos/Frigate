#include "asm/field_script.inc"
#include "text/script/underground_ruins.h"

// Script plugin 16, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    VMHalt

Script_2:
    VMCall L_0030
    VMHalt

Script_3:
    VMCall L_0030
    VMHalt

L_0030:
    VMCall L_02C8
    WorkCmpConst EVENT_WORK_0x4000, 0
    VMJumpIf CMP_EQ, L_0049
    VMJump L_004F

L_0049:
    VMJump L_00C7

L_004F:
    WorkCmpConst EVENT_WORK_0x4000, 1
    VMJumpIf CMP_EQ, L_0062
    VMJump L_006C

L_0062:
    Plugin16_Cmd1000 3
    VMJump L_00C7

L_006C:
    WorkCmpConst EVENT_WORK_0x4000, 2
    VMJumpIf CMP_EQ, L_007F
    VMJump L_0089

L_007F:
    Plugin16_Cmd1000 0
    VMJump L_00C7

L_0089:
    WorkCmpConst EVENT_WORK_0x4000, 3
    VMJumpIf CMP_EQ, L_009C
    VMJump L_00A6

L_009C:
    Plugin16_Cmd1000 1
    VMJump L_00C7

L_00A6:
    WorkCmpConst EVENT_WORK_0x4000, 4
    VMJumpIf CMP_EQ, L_00B9
    VMJump L_00C3

L_00B9:
    Plugin16_Cmd1000 2
    VMJump L_00C7

L_00C3:
    Plugin16_Cmd1000 3

L_00C7:
    VMReturn

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PlayerGetGPos 0x8020, 0x8021
    WorkCmpConst 0x8020, 13
    VMJumpIf CMP_EQ, L_00F0
    VMJump L_00FE

L_00F0:
    ActorCmdExec 255, Movement_0298
    VMJump L_0161

L_00FE:
    WorkCmpConst 0x8020, 14
    VMJumpIf CMP_EQ, L_0111
    VMJump L_011F

L_0111:
    ActorCmdExec 255, Movement_02A4
    VMJump L_0161

L_011F:
    WorkCmpConst 0x8020, 16
    VMJumpIf CMP_EQ, L_0132
    VMJump L_0140

L_0132:
    ActorCmdExec 255, Movement_02B0
    VMJump L_0161

L_0140:
    WorkCmpConst 0x8020, 17
    VMJumpIf CMP_EQ, L_0153
    VMJump L_0161

L_0153:
    ActorCmdExec 255, Movement_02BC
    VMJump L_0161

L_0161:
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 5720, 0, 0xed000, 0xf8000, 0, 0x15000, 40
    EvCameraWait
    Plugin16_Cmd1001 0
    Plugin16_Cmd1002 1
    SEPlay SEQ_SE_SW_RELIC_02
    VMSleep 30
    ActorCmdExec 255, Movement_0290
    VMSleep 10
    FadeOutBlack
    ActorCmdWait
    FadeWait
    EvCameraRebind
    EvCameraEnd
    VMCall L_02C8
    WorkCmpConst EVENT_WORK_0x4000, 0
    VMJumpIf CMP_EQ, L_01C0
    VMJump L_01D6

L_01C0:
    RTReserveScript 2
    MapChangeCore ZONE_UNDERGROUND_RUINS_2, 15, 0, 61, 0
    VMJump L_028A

L_01D6:
    WorkCmpConst EVENT_WORK_0x4000, 1
    VMJumpIf CMP_EQ, L_01E9
    VMJump L_01FF

L_01E9:
    RTReserveScript 2
    MapChangeCore ZONE_UNDERGROUND_RUINS_3, 15, 0, 61, 0
    VMJump L_028A

L_01FF:
    WorkCmpConst EVENT_WORK_0x4000, 2
    VMJumpIf CMP_EQ, L_0212
    VMJump L_0228

L_0212:
    RTReserveScript 3
    MapChangeCore ZONE_ROCK_PEAK_CHAMBER, 15, 0, 61, 0
    VMJump L_028A

L_0228:
    WorkCmpConst EVENT_WORK_0x4000, 3
    VMJumpIf CMP_EQ, L_023B
    VMJump L_0251

L_023B:
    RTReserveScript 3
    MapChangeCore ZONE_ICEBERG_CHAMBER, 15, 0, 61, 0
    VMJump L_028A

L_0251:
    WorkCmpConst EVENT_WORK_0x4000, 4
    VMJumpIf CMP_EQ, L_0264
    VMJump L_027A

L_0264:
    RTReserveScript 3
    MapChangeCore ZONE_IRON_CHAMBER, 15, 0, 61, 0
    VMJump L_028A

L_027A:
    RTReserveScript 2
    MapChangeCore ZONE_UNDERGROUND_RUINS_2, 15, 0, 61, 0

L_028A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0290:
    Move 8, 2
    MoveEnd

Movement_0298:
    Move 15, 2
    Move 0, 1
    MoveEnd

Movement_02A4:
    Move 15, 1
    Move 0, 1
    MoveEnd

Movement_02B0:
    Move 14, 1
    Move 0, 1
    MoveEnd

Movement_02BC:
    Move 14, 2
    Move 0, 1
    MoveEnd

L_02C8:
    WorkSetConst 0x8022, 0
    KeysCmd_02B1 0x8022
    RTCGetTime 0x8008, 0x8009
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F7
    WorkSetConst EVENT_WORK_0x4000, 2
    VMJump L_036A

L_02F7:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0316
    WorkSetConst EVENT_WORK_0x4000, 4
    VMJump L_036A

L_0316:
    VMStackPush 0x8022
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0335
    WorkSetConst EVENT_WORK_0x4000, 3
    VMJump L_036A

L_0335:
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_GE
    VMStackPush 0x8008
    VMStackPushConst 12
    VMStackCmp CMP_LT
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0364
    WorkSetConst EVENT_WORK_0x4000, 0
    VMJump L_036A

L_0364:
    WorkSetConst EVENT_WORK_0x4000, 1

L_036A:
    DebugPrint EVENT_WORK_0x4000
    WorkSetConst 0x8022, 0
    VMReturn

Script_5:
    ActorsPauseAll
    Plugin16_Cmd1001 1
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 5720, 0, 0xed000, 0xf8000, 0, 0x15000, 1
    EvCameraWait
    FadeInBlack
    ActorCmdExec 255, Movement_03CC
    VMSleep 20
    Plugin16_Cmd1002 0
    SEPlay SEQ_SE_SW_RELIC_03
    ActorCmdWait
    FadeWait
    VMSleep 15
    SEStop
    EvCameraMoveToDefault 10
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03CC:
    Move 9, 3
    MoveEnd

Script_6:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047E
    SEPlay SEQ_SE_MESSAGE
    // "An old switch is at your feet!\nStep on it?"
    InfoMsg UndergroundRuins_Text_OldSwitchFeetStep, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047C
    WorkSetConst 0x8023, 0
    KeysCmd_02B1 0x8023
    SEPlay SEQ_SE_FLD_44
    SEWait
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0475
    InfoMsgClose_0039
    SEPlay SEQ_SE_SW_RELIC_01
    EvCameraShake 6, 0, 3, 10, 1, 0, 1, 3
    FadeEx 3, 0, 16, 4
    FadeExWait
    Plugin16_Cmd1000 0
    VMSleep 60
    FadeEx 3, 16, 0, 4
    FadeExWait
    // "A loud, heavy sound echoed\non the other side of the door..."
    InfoMsg UndergroundRuins_Text_LoudHeavySoundEchoed, 2
    WorkSetConst EVENT_WORK_0x4001, 1
    VMJump L_047A

L_0475:
    // "Nothing seems to happen..."
    InfoMsg UndergroundRuins_Text_NothingSeemsHappen, 2

L_047A:
    LastKeyWait

L_047C:
    InfoMsgClose_0039

L_047E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    RTCGetTime 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_GE
    VMStackPush 0x8008
    VMStackPushConst 12
    VMStackCmp CMP_LT
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04C9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What is going on\nwith this door?[f000]븁\u0000\nIt leads to a different place\ndepending on whether the[f000]븀\u0000\nsun is up or not!"
    ParentActorMsg MSGFILE_SCRIPT, UndergroundRuins_Text_WhatGoingDoorLeads, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04DD

L_04C9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What is going on\nwith this door?[f000]븁\u0000\nIt leads to a different place\ndepending on whether the[f000]븀\u0000\nmoon is out or not!"
    ParentActorMsg MSGFILE_SCRIPT, UndergroundRuins_Text_WhatGoingDoorLeads_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_04DD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
