#include "asm/field_script.inc"
#include "text/script/nimbasa_city_10.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
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
    VMJumpIf CMP_STACK, L_0099
    StadiumLoadTrainerTable
    Cmd_0249 EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f, EVENT_WORK_0x4170, EVENT_WORK_0x4171, EVENT_WORK_0x4172
    StadiumFreeTrainerTable

L_0099:
    FlagSet EVENT_FLAG_0x0289
    FlagSet EVENT_FLAG_0x028a
    FlagSet EVENT_FLAG_0x028b
    FlagSet EVENT_FLAG_0x028c
    FlagSet EVENT_FLAG_0x028d
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CA
    FlagReset EVENT_FLAG_0x0289

L_00CA:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00ED
    FlagReset EVENT_FLAG_0x028a
    FlagReset EVENT_FLAG_0x028b
    FlagReset EVENT_FLAG_0x028c
    FlagReset EVENT_FLAG_0x028d

L_00ED:
    StadiumLoadTrainerTable
    StadiumSetupActorSingle 0, 0, 1
    StadiumSetupActorSingle 1, 1, 1
    StadiumSetupActorSingle 2, 2, 1
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0142
    StadiumSetupActorSingle 0, 0, 2
    StadiumSetupActorSingle 1, 1, 2
    StadiumSetupActorSingle 2, 2, 2
    StadiumSetupActorsDouble 21, 3, 12, 2

L_0142:
    TrainerCardHasBadge 0x8020, 6
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_017D
    StadiumSetupActorSingle 0, 0, 3
    StadiumSetupActorSingle 1, 1, 3
    StadiumSetupActorSingle 2, 2, 3
    StadiumSetupActorsDouble 21, 3, 12, 3

L_017D:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CA
    StadiumSetupActorSingle 0, 0, 4
    StadiumSetupActorSingle 1, 1, 4
    StadiumSetupActorSingle 2, 2, 4
    StadiumSetupActorsDouble 21, 3, 12, 4
    StadiumSetupActorsDouble 22, 4, 13, 4
    StadiumSetupActorsTriple 5, 6, 7, EVENT_WORK_0x4170, EVENT_WORK_0x4171, EVENT_WORK_0x4172

L_01CA:
    StadiumFreeTrainerTable
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon tennis is wonderful![f000]븁\u0000\nPlayers are never alone.\nTheir Pokémon are always with them!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity10_Text_PokemonTennisWonderfulPlayers, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Tennis is all about rackets and balls.\nThere's nothing profound about it."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity10_Text_TennisAllAboutRackets, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am a ball boy who collects\ntennis balls.[f000]븁\u0000\nYou are a Pokémon Trainer who\ncollects Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity10_Text_AmBallBoyWho, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "To become ball boys,\nwe gotta practice."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity10_Text_BecomeBallBoysWe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 506, 0
    // "Woowoof!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity10_Text_Woowoof, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 506, 0
    // "Yap!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity10_Text_Yap, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
