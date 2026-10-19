#include "asm/field_script.inc"
#include "text/script/castelia_city_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    UnityTowerCmd_02D7 0, 0x8008
    UnityTowerGetVisitorCountry 0x8009
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00FD

L_004F:
    VMStackPush 0x8020
    VMStackPushConst 555
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00F7
    // "Hello, hello!\nWould you like to go to Unity Tower?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity9_Text_HelloHelloWouldLike, 1, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32801
    ListMenuAdd 5, 65535, 0
    ListMenuAdd 6, 65535, 1
    ListMenuAdd 7, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_00A4
    VMJump L_00B6

L_00A4:
    VMCall L_011F
    WorkSetConst 0x8020, 555
    VMJump L_00F1

L_00B6:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_00C9
    VMJump L_00DB

L_00C9:
    // "Unity Tower is a place where Trainers\ngather from all over the world![f000]븁\u0000\nIf you have friends who live far away,\nyou may be able to have a merry reunion![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity9_Text_UnityTowerPlaceWhere, 1, 2, 0
    VMJump L_00F1

L_00DB:
    // "Please come again!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity9_Text_PleaseComeAgain, 1, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8020, 555

L_00F1:
    VMJump L_004F

L_00F7:
    VMJump L_010D

L_00FD:
    // "Hello! This is the ship for Unity Tower."
    ActorMsg MSGFILE_SCRIPT, CasteliaCity9_Text_HelloShipUnityTower, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_010D:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_011F:
    // "One person will be on board!\nPlease get on board![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, CasteliaCity9_Text_OnePersonWillBoard, 1, 2, 0
    MsgWinCloseAll
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0148
    PlayerSetSpecialSequence 1

L_0148:
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016D
    ActorCmdExec 255, Movement_01DC
    VMJump L_01B7

L_016D:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018E
    ActorCmdExec 255, Movement_01E4
    VMJump L_01B7

L_018E:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AF
    ActorCmdExec 255, Movement_01F4
    VMJump L_01B7

L_01AF:
    ActorCmdExec 255, Movement_0200

L_01B7:
    ActorCmdWait
    FadeOutBlackQ
    FadeWait
    FieldClose
    Call3DDemo 4, 0
    FieldOpen
    RTReserveScript 1
    MapChangeCore ZONE_UNITY_TOWER, 407, 0, 762, 2
    VMReturn
    .balign 4, 0

Movement_01DC:
    Move 34, 1
    MoveEnd

Movement_01E4:
    Move 15, 1
    Move 13, 3
    Move 14, 1
    MoveEnd

Movement_01F4:
    Move 13, 1
    Move 14, 1
    MoveEnd

Movement_0200:
    Move 34, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    ActorCmdExec 255, Movement_0220
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0220:
    Move 15, 1
    MoveEnd

Script_1:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 280
    WorkSet 0x8001, 1
    WorkSet 0x8002, 443
    WorkSet 0x8003, 8
    WorkSet 0x8004, 9
    WorkSet 0x8005, 9
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
