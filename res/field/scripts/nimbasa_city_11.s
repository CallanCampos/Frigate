#include "asm/field_script.inc"
#include "text/script/nimbasa_city_11.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    WorkSetConst 0x8020, 0
    VMStackPush EVENT_WORK_0x416d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x416e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x416f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4170
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4171
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4172
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_008D
    StadiumLoadTrainerTable
    Cmd_0249 EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f, EVENT_WORK_0x4170, EVENT_WORK_0x4171, EVENT_WORK_0x4172
    StadiumFreeTrainerTable

L_008D:
    FlagSet EVENT_FLAG_0x0289
    FlagSet EVENT_FLAG_0x028a
    FlagSet EVENT_FLAG_0x028b
    FlagSet EVENT_FLAG_0x028c
    FlagSet EVENT_FLAG_0x028d
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BE
    FlagReset EVENT_FLAG_0x0289

L_00BE:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E1
    FlagReset EVENT_FLAG_0x028a
    FlagReset EVENT_FLAG_0x028b
    FlagReset EVENT_FLAG_0x028c
    FlagReset EVENT_FLAG_0x028d

L_00E1:
    StadiumLoadTrainerTable
    StadiumSetupActorSingle 12, 0, 1
    StadiumSetupActorSingle 13, 1, 1
    StadiumSetupActorSingle 14, 2, 1
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0136
    StadiumSetupActorSingle 12, 0, 2
    StadiumSetupActorSingle 13, 1, 2
    StadiumSetupActorSingle 14, 2, 2
    StadiumSetupActorsDouble 23, 3, 10, 2

L_0136:
    TrainerCardHasBadge 0x8020, 6
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0171
    StadiumSetupActorSingle 12, 0, 3
    StadiumSetupActorSingle 13, 1, 3
    StadiumSetupActorSingle 14, 2, 3
    StadiumSetupActorsDouble 23, 3, 10, 3

L_0171:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BE
    StadiumSetupActorSingle 12, 0, 4
    StadiumSetupActorSingle 13, 1, 4
    StadiumSetupActorSingle 14, 2, 4
    StadiumSetupActorsDouble 23, 3, 10, 4
    StadiumSetupActorsDouble 24, 4, 11, 4
    StadiumSetupActorsTriple 5, 6, 7, EVENT_WORK_0x4170, EVENT_WORK_0x4171, EVENT_WORK_0x4172

L_01BE:
    StadiumFreeTrainerTable
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A fight against your opponent can end in\nthe blink of an eye![f000]븁\u0000\nThe important thing is how much you\nprepare before the fight."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity11_Text_FightAgainstOpponentCan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't play basketball just because\nI'm tall.[f000]븁\u0000\nMaybe I grew tall because I wanted to\nplay basketball so badly?!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity11_Text_DontPlayBasketballJust, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 559, 0
    // "Aaagy!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity11_Text_Aaagy, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
