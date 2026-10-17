#include "asm/field_script.inc"
#include "text/script/dragonspiral_tower_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Cedric Juniper: Two years ago...\nOn the top floor of this tower...[f000]븁\u0000\nOne lone man faced\na legendary Pokémon.[f000]븁\u0000\nHis name was N.[f000]븁\u0000\nHe sought Reshiram in order to\nunderstand the meaning of truth.[f000]븁\u0000\nI wonder if he succeeded\nin finding his own truth."
    // "Cedric Juniper: Two years ago...\nOn the top floor of this tower...[f000]븁\u0000\nOne lone man faced\na legendary Pokémon.[f000]븁\u0000\nHis name was N.[f000]븁\u0000\nHe sought Zekrom in order to\nunderstand his ideals.[f000]븁\u0000\nI wonder if he succeeded in\ndiscovering his ideals."
    ActorMsgVersioned 1024, DragonspiralTower2_Text_CedricJuniperTwoYears_2, DragonspiralTower2_Text_CedricJuniperTwoYears, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    // "We still don't know anything about it.[f000]븁\u0000\nCould the Dragonspiral Tower somehow\nsymbolize ideals?[f000]븀\u0000\nCould it somehow represent truth?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower2_Text_WeStillDontKnow, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0248
    ActorCmdWait
    ActorCmdExec 1, Movement_0260
    ActorCmdWait
    PlayerGetGPos 0x8020, 0x8021
    WorkSub 0x8021, 1
    ActorWalkRoute 1, 0x8020, 0x8021, 1, 8, 0
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 22
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0089

L_0089:
    // "Cedric Juniper: So you came\nhere as well![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower2_Text_CedricJuniperCameHere, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0240
    ActorCmdWait
    VMSleep 20
    // "It was two years ago...[f000]븁\u0000\nIn this tower, a certain man and\na certain Pokémon came face-to-face.[f000]븁\u0000\nThis man sought the truth\nso he could change the world.[f000]븁\u0000"
    // "It was two years ago...[f000]븁\u0000\nIn this tower, a certain man and\na certain Pokémon came face-to-face.[f000]븁\u0000\nThis man pursued his ideals\nso he could change the world.[f000]븁\u0000"
    ActorMsgVersioned 1024, DragonspiralTower2_Text_TwoYearsAgoTower_2, DragonspiralTower2_Text_TwoYearsAgoTower, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0248
    ActorCmdWait
    // "That's right. This building\nrising serenely into the sky[f000]븀\u0000\nis the Dragonspiral Tower.[f000]븁\u0000\nIt has towered over this land\nsince before Unova was founded.[f000]븁\u0000\nOn the highest floor, the legendary\nDragon-type Pokémon was waiting for[f000]븀\u0000\nthe appearance of a person seeking[f000]븀\u0000\nthe truth...[f000]븁\u0000\nIt was exactly how the legends said\nit would be...[f000]븁\u0000"
    // "That's right. This building\nrising serenely into the sky[f000]븀\u0000\nis the Dragonspiral Tower.[f000]븁\u0000\nIt has towered over this land\nsince before Unova was founded.[f000]븁\u0000\nOn the highest floor, the legendary\nDragon-type Pokémon was waiting for[f000]븀\u0000\nthe appearance of a person seeking[f000]븀\u0000\nhis or her ideals...[f000]븁\u0000\nIt was exactly how the legends said\nit would be...[f000]븁\u0000"
    ActorMsgVersioned 1024, DragonspiralTower2_Text_ThatsRightBuildingRising_2, DragonspiralTower2_Text_ThatsRightBuildingRising, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0260
    ActorCmdWait
    // "Ah!\nThe Light Stone![f000]븁\u0000\nIf that's the case, you must\nbe headed to the top floor.[f000]븁\u0000\nChanging the world...[f000]븁\u0000\nThat's an outrageous idea,\nbut it is possible to change yourself.[f000]븀\u0000\n...As long as you seek the truth.[f000]븁\u0000"
    // "Ah!\nThe Dark Stone![f000]븁\u0000\nIf that's the case, you must\nbe headed to the top floor.[f000]븁\u0000\nChanging the world...[f000]븁\u0000\nThat's an outrageous idea,\nbut it is possible to change yourself.[f000]븀\u0000\n...As long as you pursue your ideals.[f000]븁\u0000"
    ActorMsgVersioned 1024, DragonspiralTower2_Text_AhLightStoneIf, DragonspiralTower2_Text_AhDarkStoneIf, 1, 0, 0
    // "My! That conversation\nsure took a serious turn![f000]븁\u0000\nMaybe something light and sweet\nwould help balance things out![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower2_Text_ConversationSureTookSerious, 1, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 54
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Be seeing you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DragonspiralTower2_Text_Seeing, 1, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8021, 51
    VMStackPush 0x8020
    VMStackPushConst 21
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0150
    ActorWalkRoute 1, 21, 0x8021, 2, 8, 0
    VMJump L_0168

L_0150:
    ActorCmdExec 1, Movement_0210
    ActorCmdWait
    ActorWalkRoute 1, 22, 0x8021, 2, 8, 1

L_0168:
    WorkCmpConst 0x8020, 20
    VMJumpIf CMP_EQ, L_017B
    VMJump L_0185

L_017B:
    VMSleep 24
    VMJump L_01DC

L_0185:
    WorkCmpConst 0x8020, 21
    VMJumpIf CMP_EQ, L_0198
    VMJump L_01A2

L_0198:
    VMSleep 18
    VMJump L_01DC

L_01A2:
    WorkCmpConst 0x8020, 22
    VMJumpIf CMP_EQ, L_01B5
    VMJump L_01BF

L_01B5:
    VMSleep 24
    VMJump L_01DC

L_01BF:
    WorkCmpConst 0x8020, 23
    VMJumpIf CMP_EQ, L_01D2
    VMJump L_01DC

L_01D2:
    VMSleep 30
    VMJump L_01DC

L_01DC:
    ActorCmdExec 255, Movement_0248
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 1
    SEWait
    FlagSet EVENT_FLAG_0x03f4
    WorkSetConst EVENT_WORK_0x4148, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0210:
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

Movement_0240:
    Move 32, 1
    MoveEnd

Movement_0248:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0260:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd
