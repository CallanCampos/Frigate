#include "asm/field_script.inc"
#include "text/script/ns_castle_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_4:
    VMHalt

Script_5:
    VMStackPush EVENT_WORK_0x4113
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0055
    ActorSetGPos 0, 9, 0, 9, 3

L_0055:
    VMHalt

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0078
    VMCall L_021F
    VMJump L_007E

L_0078:
    VMCall L_0084

L_007E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0084:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8021, 0x8022
    ActorGetGPos 0, 0x8023, 0x8024
    WorkAdd 0x8021, 2
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00D1
    ActorWalkRoute 0, 0x8021, 0x8022, 0, 16, 0
    ActorCmdWait

L_00D1:
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00EE
    ActorCmdExec 0, Movement_04EC
    ActorCmdWait

L_00EE:
    // "N: [f000]븉\u0001\u0001You came...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_NCame, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_04F4
    ActorCmdWait
    // "[f000]븉\u0001\u0001This...[f000]븁\u0000\nThis is Team Plasma's castle.[f000]븁\u0000\nThe ruins of Ghetsis's dreams...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_TeamPlasmasCastleRuins, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_04EC
    ActorCmdWait
    // "[f000]븉\u0001\u0001The deepest chamber of this castle...[f000]븁\u0000\nIt's a place that holds a special\nmeaning to me...[f000]븀\u0000\nI have to face you there![f000]븁\u0000\nFollow me![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_DeepestChamberCastleIts, 0, 0, 0
    // "[f000]븉\u0001\u0001Actually...[f000]븁\u0000\nRather than just leading you there,\nI'd prefer to follow. That way, I can[f000]븀\u0000\nsee which path you choose and observe[f000]븀\u0000\nwhat catches your interest.[f000]븁\u0000\nSo, I ask this of you![f000]븁\u0000\nTake me to the deepest chamber\nof this castle![f000]븉\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_ActuallyRatherThanJust, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FF
    // "[f000]븉\u0001\u0001You lead and I'll follow![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_LeadIllFollow, 0, 0, 0
    MsgWinCloseAll
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0176
    PlayerSetSpecialSequence 1

L_0176:
    ActorCmdExec 255, Movement_04AC
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 0
    WorkSet 0x8001, 1
    WorkSet 0x8002, 1
    WorkSet 0x8003, 0
    WorkSet 0x8004, 10537
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst EVENT_WORK_0x400f, 0
    WorkSetConst EVENT_WORK_0x411d, 1
    WorkSetConst EVENT_WORK_0x4114, 1
    FlagSet EVENT_FLAG_0x038f
    VMStackPushFlag EVENT_FLAG_0x019f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F9
    WorkSetConst EVENT_WORK_0x4113, 1

L_01F9:
    VMJump L_021D

L_01FF:
    // "[f000]븉\u0001\u0001Fine...\nI'll be waiting here for you, then.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_FineIllWaitingHere, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_04B4
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x400f, 1

L_021D:
    VMReturn

L_021F:
    WordSetPlayerName 0
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024F
    ActorCmdExec 255, Movement_04DC
    ActorCmdExec 254, Movement_04C4
    VMJump L_02B1

L_024F:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0278
    ActorCmdExec 255, Movement_04EC
    ActorCmdExec 254, Movement_04D4
    VMJump L_02B1

L_0278:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A1
    ActorCmdExec 255, Movement_04F4
    ActorCmdExec 254, Movement_04CC
    VMJump L_02B1

L_02A1:
    ActorCmdExec 255, Movement_04E4
    ActorCmdExec 254, Movement_04BC

L_02B1:
    ActorCmdWait
    // "[f000]븉\u0001\u0001What?[f000]븁\u0000\nYou're leaving at a time like this?[f000]븁\u0000\nMy formula didn't account\nfor this variable...[f000]븉\u0001\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_WhatYoureLeavingTime, 254, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033E
    // "[f000]븉\u0001\u0001Fine...\nI'll be waiting here for you, then.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_FineIllWaitingHere, 254, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 10537
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorWalkRoute 254, 12, 14, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 254, Movement_04EC
    ActorCmdExec 255, Movement_04B4
    ActorCmdWait
    FlagReset EVENT_FLAG_0x038f
    ActorAdd 0
    ActorDelete 254
    WorkSetConst EVENT_WORK_0x4114, 0
    VMJump L_035C

L_033E:
    // "[f000]븉\u0001\u0001Fine...\nThen take me to the deepest chamber.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_FineThenTakeDeepest, 254, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_04AC
    ActorCmdWait
    ActorPairSetMoveEnable 0

L_035C:
    VMReturn

Script_2:
    ActorsPauseAll
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_038D
    ActorCmdExec 255, Movement_04DC
    ActorCmdExec 254, Movement_04C4
    VMJump L_03EF

L_038D:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B6
    ActorCmdExec 255, Movement_04EC
    ActorCmdExec 254, Movement_04D4
    VMJump L_03EF

L_03B6:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DF
    ActorCmdExec 255, Movement_04F4
    ActorCmdExec 254, Movement_04CC
    VMJump L_03EF

L_03DF:
    ActorCmdExec 255, Movement_04E4
    ActorCmdExec 254, Movement_04BC

L_03EF:
    ActorCmdWait
    // "[f000]븉\u0001\u0001There's nothing more of interest there.\nLet's keep moving.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_TheresNothingMoreInterest, 254, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_049C
    ActorCmdWait
    ActorPairSetMoveEnable 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 255, Movement_04EC
    ActorCmdWait
    // "[f000]븉\u0001\u0001You lead and I'll follow![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NsCastle2_Text_LeadIllFollow, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 0
    WorkSet 0x8001, 1
    WorkSet 0x8002, 1
    WorkSet 0x8003, 0
    WorkSet 0x8004, 10537
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorPairSetMoveEnable 1
    ActorCmdExec 255, Movement_049C
    ActorCmdWait
    ActorPairSetMoveEnable 0
    WorkSetConst EVENT_WORK_0x4113, 3
    FlagSet EVENT_FLAG_0x038f
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_049C:
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_04AC:
    Move 15, 1
    MoveEnd

Movement_04B4:
    Move 14, 1
    MoveEnd

Movement_04BC:
    Move 0, 1
    MoveEnd

Movement_04C4:
    Move 1, 1
    MoveEnd

Movement_04CC:
    Move 2, 1
    MoveEnd

Movement_04D4:
    Move 3, 1
    MoveEnd

Movement_04DC:
    Move 32, 1
    MoveEnd

Movement_04E4:
    Move 33, 1
    MoveEnd

Movement_04EC:
    Move 34, 1
    MoveEnd

Movement_04F4:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
