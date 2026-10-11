#include "asm/field_script.inc"
#include "text/script/route_13.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 13"
    MsgPlaceSign Route13_Text_Route13, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 13"
    MsgPlaceSign Route13_Text_Route13_2, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nChange your type on the Trainer Card\nto change how other players see you.[f000]븁\u0000\nYou'll look different to others in the\nUnion Room and the Tag Log![f000]븁\u0000\nMatch it with your introduction or\ncharacter to show your individuality!"
    MsgPlaceSign Route13_Text_TrainerTipsChangeType, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a72
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorCmdExec 255, Movement_03B4
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x2ac8000, 0, 0xbe8000, 32
    EvCameraWait
    VMStackPush 0x8022
    VMStackPushConst 187
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CB
    ActorCmdExec 21, Movement_03CC
    ActorCmdWait

L_00CB:
    PVPlay 638, 0
    // "Kawbraa!"
    ScreamMsg Route13_Text_Kawbraa, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    EvCameraMoveToDefault 32
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x40db, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 638, 0
    // "Kawbraa!"
    ScreamMsg Route13_Text_Kawbraa, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMStackPushFlag EVENT_FLAG_0x012e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013E
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8023, 1
    CallWildBattle 638, 45, 0x8023
    WorkSetConst 0x8023, 0
    VMJump L_0158

L_013E:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 1
    CallWildBattle 638, 65, 0x8024
    WorkSetConst 0x8024, 0

L_0158:
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0183
    FlagSet EVENT_FLAG_0x0322
    FlagSet EVENT_FLAG_0x012e
    ActorDelete 21
    CallWildBattleEnd
    VMJump L_0185

L_0183:
    CallWildLose

L_0185:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_019C
    VMJump L_01A6

L_019C:
    FlagSet EVENT_FLAG_0x012f
    VMJump L_01CC

L_01A6:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_01C6
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_01C6
    VMJump L_01CC

L_01C6:
    VMJump L_01CC

L_01CC:
    VMStackPushFlag EVENT_FLAG_0x012f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E9
    // "Cobalion ran away\nbeyond the road..."
    SystemMsg Route13_Text_CobalionRanAwayBeyond, 2
    LastKeyWait
    InfoMsgClose

L_01E9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x00da
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0238
    // "What am I doing?\nI'm hunting for TREASURE![f000]븁\u0000\nTreasure hunting is fun. Sometimes things\nare buried in the sand dunes.[f000]븁\u0000\nActually, I just found something![f000]븁\u0000\nBut it's the same one as I found before,\nso I will give this to you.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route13_Text_WhatAmDoingIm, 0, 0
    MsgWinCloseAll
    VMCall L_0289
    // "I'll be hunting for treasure here\ntomorrow, too.[f000]븁\u0000\nI may find something, so if you have\ntime, stop by.[f000]븁\u0000\nI don't find as many things\nas I used to, though..."
    ParentActorMsg MSGFILE_SCRIPT, Route13_Text_IllHuntingTreasureHere, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ab2
    FlagSet EVENT_FLAG_0x00da
    VMJump L_0283

L_0238:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0275
    // "I just found a treasure.[f000]븁\u0000\nBut it's the same one as I found before,\nso I'll give you this one.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route13_Text_JustFoundTreasureBut, 0, 0
    MsgWinCloseAll
    VMCall L_0289
    // "I'll be hunting for treasure here\ntomorrow, too.[f000]븁\u0000\nI may find something, so if you have\ntime, stop by.[f000]븁\u0000\nI don't find as many things\nas I used to, though..."
    ParentActorMsg MSGFILE_SCRIPT, Route13_Text_IllHuntingTreasureHere, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ab2
    VMJump L_0283

L_0275:
    // "I'll be hunting for treasure here\ntomorrow, too.[f000]븁\u0000\nI may find something, so if you have\ntime, stop by.[f000]븁\u0000\nI don't find as many things\nas I used to, though..."
    ParentActorMsg MSGFILE_SCRIPT, Route13_Text_IllHuntingTreasureHere, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0283:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0289:
    WorkSetConst 0x8025, 0
    Random 0x8025, 5
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_02A8
    VMJump L_02CE

L_02A8:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 65
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_03B2

L_02CE:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_02E1
    VMJump L_0307

L_02E1:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 66
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_03B2

L_0307:
    WorkCmpConst 0x8025, 2
    VMJumpIf CMP_EQ, L_031A
    VMJump L_0340

L_031A:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 67
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_03B2

L_0340:
    WorkCmpConst 0x8025, 3
    VMJumpIf CMP_EQ, L_0353
    VMJump L_0379

L_0353:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 68
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_03B2

L_0379:
    WorkCmpConst 0x8025, 4
    VMJumpIf CMP_EQ, L_038C
    VMJump L_03B2

L_038C:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 69
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_03B2

L_03B2:
    VMReturn

Movement_03B4:
    Move 75, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_03CC:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    Move 75, 1
    MoveEnd
