#include "asm/field_script.inc"
#include "text/script/nimbasa_city_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
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
    VMJumpIf CMP_STACK, L_00A1
    StadiumLoadTrainerTable
    Cmd_0249 EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f, EVENT_WORK_0x4170, EVENT_WORK_0x4171, EVENT_WORK_0x4172
    StadiumFreeTrainerTable

L_00A1:
    FlagSet EVENT_FLAG_0x0289
    FlagSet EVENT_FLAG_0x028a
    FlagSet EVENT_FLAG_0x028b
    FlagSet EVENT_FLAG_0x028c
    FlagSet EVENT_FLAG_0x028d
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D2
    FlagReset EVENT_FLAG_0x0289

L_00D2:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F5
    FlagReset EVENT_FLAG_0x028a
    FlagReset EVENT_FLAG_0x028b
    FlagReset EVENT_FLAG_0x028c
    FlagReset EVENT_FLAG_0x028d

L_00F5:
    StadiumLoadTrainerTable
    StadiumSetupActorSingle 3, 15, 1
    StadiumSetupActorSingle 4, 0, 1
    StadiumSetupActorSingle 5, 1, 1
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014A
    StadiumSetupActorSingle 3, 15, 2
    StadiumSetupActorSingle 4, 0, 2
    StadiumSetupActorSingle 5, 1, 2
    StadiumSetupActorsDouble 17, 2, 11, 2

L_014A:
    TrainerCardHasBadge 0x8020, 6
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0185
    StadiumSetupActorSingle 3, 15, 3
    StadiumSetupActorSingle 4, 0, 3
    StadiumSetupActorSingle 5, 1, 3
    StadiumSetupActorsDouble 17, 2, 11, 3

L_0185:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D2
    StadiumSetupActorSingle 3, 15, 4
    StadiumSetupActorSingle 4, 0, 4
    StadiumSetupActorSingle 5, 1, 4
    StadiumSetupActorsDouble 17, 2, 11, 4
    StadiumSetupActorsDouble 18, 3, 14, 4
    StadiumSetupActorsTriple 4, 5, 6, EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f

L_01D2:
    StadiumFreeTrainerTable
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "To be good at something, you have to do\nit repeatedly, day after day after day."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_GoodSomethingHaveRepeatedly, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When I was a kid, I played catch with\nmy Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_WhenKidPlayedCatch, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's not bad to grow up.[f000]븁\u0000\nBut it's bad to forget what it felt like\nto be a child."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_ItsNotBadGrow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I have baseball, and I have Pokémon...\nHow happy I am!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_HaveBaseballHavePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you don't swing a bat, you can't hit\na ball.[f000]븁\u0000\nIf you don't do things, you won't fail...\nbut you won't succeed, either."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_IfDontSwingBat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I give 100 percent of my energy all the\ntime! That is the only thing I can do."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_Give100PercentEnergy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What I brag about is my bat!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_WhatBragAboutBat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Bwoof!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity6_Text_Bwoof, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
