#include "asm/field_script.inc"
#include "text/script/unity_tower.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    ActorCmdExec 255, Movement_0024
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0024:
    Move 14, 3
    MoveEnd

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Return to Castelia City?"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower_Text_ReturnCasteliaCity, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0100
    // "Well then, please board the ship![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, UnityTower_Text_WellThenPleaseBoard, 0, 0
    MsgWinCloseAll
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007C
    PlayerSetSpecialSequence 1

L_007C:
    PlayerGetDir 0x8008
    WorkCmpConst 0x8008, 0
    VMJumpIf CMP_EQ, L_0093
    VMJump L_00A9

L_0093:
    ActorCmdExec 255, Movement_0114
    ActorCmdExec 0, Movement_0134
    VMJump L_00DE

L_00A9:
    WorkCmpConst 0x8008, 2
    VMJumpIf CMP_EQ, L_00BC
    VMJump L_00CA

L_00BC:
    ActorCmdExec 255, Movement_011C
    VMJump L_00DE

L_00CA:
    ActorCmdExec 255, Movement_0128
    VMSleep 16
    ActorCmdExec 0, Movement_0134

L_00DE:
    ActorCmdWait
    RTReserveScript 3
    FadeOutBlackQ
    FadeWait
    SEPlay SEQ_SE_VDEMO_02
    MapChangeCore ZONE_CASTELIA_CITY_9, 12, 0, 11, 3
    SEWait
    VMJump L_010E

L_0100:
    // "Please board the ship\nat your convenience."
    ParentActorMsg MSGFILE_SCRIPT, UnityTower_Text_PleaseBoardShipConvenience, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_010E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0114:
    Move 15, 2
    MoveEnd

Movement_011C:
    Move 13, 1
    Move 15, 1
    MoveEnd

Movement_0128:
    Move 13, 1
    Move 15, 3
    MoveEnd

Movement_0134:
    Move 35, 1
    MoveEnd
