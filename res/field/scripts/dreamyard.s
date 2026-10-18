#include "asm/field_script.inc"
#include "text/script/dreamyard.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_3:
    VMStackPushFlag EVENT_FLAG_0x0176
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x410c
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_008E
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0088
    WorkSetConst EVENT_WORK_0x4020, 4510
    VMJump L_008E

L_0088:
    WorkSetConst EVENT_WORK_0x4020, 4509

L_008E:
    VMHalt

Script_1:
    VMHalt

Script_2:
    VMHalt

Script_4:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CA
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_00DB

L_00CA:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_00DB:
    FlagReset EVENT_FLAG_0x0355
    ActorAdd 7
    ActorSetGPos 7, 19, 0, 0x8023, 2
    ActorMoveLinear 7, 13, 0, 0x8023, 24
    VMSleep 16
    ActorMoveLinear 7, 12, 0, 0x8023, 8
    VMSleep 48
    ActorMoveLinear 7, 13, 0, 0x8023, 16
    VMSleep 16
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0149
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_015A

L_0149:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_015A:
    ActorCmdExec 7, Movement_06F4
    ActorCmdWait
    ActorMoveLinear 7, 19, 0, 0x8023, 18
    ActorDelete 7
    FlagSet EVENT_FLAG_0x0355
    WorkSetConst EVENT_WORK_0x410c, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BA
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 1
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_01CB

L_01BA:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 1
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_01CB:
    FlagReset EVENT_FLAG_0x0355
    ActorAdd 7
    ActorSetGPos 7, 44, 2, 22, 2
    ActorMoveLinear 7, 26, 2, 22, 48
    ActorDelete 7
    FlagSet EVENT_FLAG_0x0355
    WorkSetConst EVENT_WORK_0x410c, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0235
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 1
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0246

L_0235:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 1
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_0246:
    FlagReset EVENT_FLAG_0x0355
    ActorAdd 7
    VMStackPush 0x8022
    VMStackPushConst 26
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027F
    ActorSetGPos 7, 17, 4, 12, 3
    ActorMoveLinear 7, 35, 4, 11, 48
    VMJump L_02DB

L_027F:
    VMStackPush 0x8022
    VMStackPushConst 27
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B0
    ActorSetGPos 7, 18, 4, 12, 3
    ActorMoveLinear 7, 36, 4, 11, 48
    VMJump L_02DB

L_02B0:
    VMStackPush 0x8022
    VMStackPushConst 28
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DB
    ActorSetGPos 7, 19, 4, 12, 3
    ActorMoveLinear 7, 37, 4, 11, 48

L_02DB:
    ActorDelete 7
    FlagSet EVENT_FLAG_0x0355
    WorkSetConst EVENT_WORK_0x410c, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0325
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0336

L_0325:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_0336:
    FlagReset EVENT_FLAG_0x0355
    ActorAdd 7
    VMStackPush 0x8022
    VMStackPushConst 35
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_036F
    ActorSetGPos 7, 44, 4, 11, 2
    ActorMoveLinear 7, 26, 4, 11, 48
    VMJump L_039A

L_036F:
    VMStackPush 0x8022
    VMStackPushConst 36
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039A
    ActorSetGPos 7, 45, 4, 11, 2
    ActorMoveLinear 7, 27, 4, 11, 48

L_039A:
    ActorDelete 7
    FlagSet EVENT_FLAG_0x0355
    WorkSetConst EVENT_WORK_0x410c, 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E4
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_03F5

L_03E4:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_03F5:
    FlagReset EVENT_FLAG_0x0355
    ActorAdd 7
    VMStackPush 0x8022
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_042E
    ActorSetGPos 7, 13, 4, 20, 3
    ActorMoveLinear 7, 26, 4, 15, 36
    VMJump L_0459

L_042E:
    VMStackPush 0x8022
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0459
    ActorSetGPos 7, 14, 4, 20, 3
    ActorMoveLinear 7, 27, 4, 15, 36

L_0459:
    ActorCmdExec 7, Movement_06FC
    ActorCmdWait
    VMSleep 8
    ActorCmdExec 255, Movement_06F4
    ActorCmdWait
    VMSleep 16
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_049F
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_04B0

L_049F:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_04B0:
    ActorCmdExec 7, Movement_06F4
    ActorCmdWait
    VMStackPush 0x8022
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04DF
    ActorMoveLinear 7, 31, 4, 15, 12
    VMJump L_04FE

L_04DF:
    VMStackPush 0x8022
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04FE
    ActorMoveLinear 7, 32, 4, 15, 12

L_04FE:
    ActorDelete 7
    FlagSet EVENT_FLAG_0x0355
    WorkSetConst EVENT_WORK_0x410c, 5
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0548
    PVPlay 381, 0
    // "Shuaaan!"
    InfoMsg Dreamyard_Text_Shuaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0559

L_0548:
    PVPlay 380, 0
    // "Huaaaan!"
    InfoMsg Dreamyard_Text_Huaaaan, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_0559:
    FlagReset EVENT_FLAG_0x0355
    ActorAdd 7
    ActorSetGPos 7, 56, 4, 12, 2
    ActorMoveLinear 7, 50, 4, 12, 18
    VMSleep 32
    ActorMoveLinear 7, 48, 4, 12, 6
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05BB
    PVPlay 381, 0
    // "Shuaaaann!"
    ScreamMsg Dreamyard_Text_Shuaaaann, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    CallWildBattle 381, 68, 1
    VMJump L_05D4

L_05BB:
    PVPlay 380, 0
    // "Huaaaaann!"
    ScreamMsg Dreamyard_Text_Huaaaaann, 2
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    CallWildBattle 380, 68, 1

L_05D4:
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_061C
    FlagSet EVENT_FLAG_0x0355
    WorkSetConst EVENT_WORK_0x410c, 6
    VMStackPushFlag EVENT_FLAG_0x01f1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0610
    FlagReset EVENT_FLAG_0x040d
    ActorAdd 11

L_0610:
    ActorDelete 7
    CallWildBattleEnd
    VMJump L_061E

L_061C:
    CallWildLose

L_061E:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0635
    VMJump L_063F

L_0635:
    FlagSet EVENT_FLAG_0x0176
    VMJump L_0696

L_063F:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_065F
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_065F
    VMJump L_0696

L_065F:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0686
    // "Latios flew off into\nthe distant sky..."
    SystemMsg Dreamyard_Text_LatiosFlewOffInto, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0690

L_0686:
    // "Latias flew off into\nthe distant sky..."
    SystemMsg Dreamyard_Text_LatiasFlewOffInto, 2
    LastKeyWait
    InfoMsgClose

L_0690:
    VMJump L_0696

L_0696:
    VMStackPushFlag EVENT_FLAG_0x0186
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06AD
    FlagSet EVENT_FLAG_0x0186

L_06AD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorDelete 11
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 225
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x040d
    FlagSet EVENT_FLAG_0x01f1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 75, 1
    MoveEnd

Movement_06F4:
    Move 35, 1
    MoveEnd

Movement_06FC:
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
