#include "asm/field_script.inc"
#include "text/script/victory_road_16.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8021, 0x8022
    SEPlay SEQ_SE_KAIDAN
    ActorNew 21, 0x8022, 0, 251, 291, 0
    SEWait
    BGMPlay SEQ_BGM_E_HUE
    // "Wait up![f000]븁\u0000"
    InfoMsg VictoryRoad16_Text_WaitUp, 2
    InfoMsgClose_0039
    ActorCmdExec 255, Movement_018C
    ActorCmdWait
    WorkAdd 0x8021, 2
    ActorWalkRoute 251, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: I'll battle with you\nbefore you take on the Pokémon League.[f000]븁\u0000\nThe more Pokémon battles you have,\nthe stronger you get, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad16_Text_IllBattleBeforeTake, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_016C
    ActorCmdWait
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009E
    CallTrainerBattle TRAINER_RIVAL_16, 0, 0
    VMJump L_00C7

L_009E:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BF
    CallTrainerBattle TRAINER_RIVAL_17, 0, 0
    VMJump L_00C7

L_00BF:
    CallTrainerBattle TRAINER_RIVAL_18, 0, 0

L_00C7:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E6
    CallTrainerBattleEnd
    VMJump L_00E8

L_00E6:
    CallTrainerLose

L_00E8:
    VMSleep 8
    ActorCmdExec 251, Movement_017C
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000![f000]븁\u0000\nThanks to you, I accomplished what\nI set out to do during my journey![f000]븁\u0000\nI wish I could've shown you\nmy little sister's huge smile![f000]븁\u0000\nThis is my thanks![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad16_Text_ThanksAccomplishedWhatSet, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_01AC
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 351
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "I think you're really amazing![f000]븁\u0000\nSo become the Champion![f000]븁\u0000\nGet the proof that you're a Trainer\nyour Pokémon can be proud of![f000]븁\u0000\nSee you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VictoryRoad16_Text_ThinkYoureReallyAmazing, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 21, 0x8022, 1, 8, 0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x4124, 1
    HollowRivalCmd_0262 1, 38
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_016C:
    Move 100, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_017C:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_018C:
    Move 35, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_01AC:
    Move 14, 1
    MoveEnd
