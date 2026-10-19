#include "asm/field_script.inc"
#include "text/script/nimbasa_city_7.h"

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
    VMJumpIf CMP_STACK, L_00A5
    StadiumLoadTrainerTable
    Cmd_0249 EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f, EVENT_WORK_0x4170, EVENT_WORK_0x4171, EVENT_WORK_0x4172
    StadiumFreeTrainerTable

L_00A5:
    FlagSet EVENT_FLAG_0x0289
    FlagSet EVENT_FLAG_0x028a
    FlagSet EVENT_FLAG_0x028b
    FlagSet EVENT_FLAG_0x028c
    FlagSet EVENT_FLAG_0x028d
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D6
    FlagReset EVENT_FLAG_0x0289

L_00D6:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F9
    FlagReset EVENT_FLAG_0x028a
    FlagReset EVENT_FLAG_0x028b
    FlagReset EVENT_FLAG_0x028c
    FlagReset EVENT_FLAG_0x028d

L_00F9:
    StadiumLoadTrainerTable
    StadiumSetupActorSingle 6, 9, 1
    StadiumSetupActorSingle 7, 10, 1
    StadiumSetupActorSingle 8, 11, 1
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014E
    StadiumSetupActorSingle 6, 9, 2
    StadiumSetupActorSingle 7, 10, 2
    StadiumSetupActorSingle 8, 11, 2
    StadiumSetupActorsDouble 19, 12, 22, 2

L_014E:
    TrainerCardHasBadge 0x8020, 6
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0189
    StadiumSetupActorSingle 6, 9, 3
    StadiumSetupActorSingle 7, 10, 3
    StadiumSetupActorSingle 8, 11, 3
    StadiumSetupActorsDouble 19, 12, 22, 3

L_0189:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D6
    StadiumSetupActorSingle 6, 9, 4
    StadiumSetupActorSingle 7, 10, 4
    StadiumSetupActorSingle 8, 11, 4
    StadiumSetupActorsDouble 19, 12, 22, 4
    StadiumSetupActorsDouble 20, 13, 23, 4
    StadiumSetupActorsTriple 14, 15, 16, EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f

L_01D6:
    StadiumFreeTrainerTable
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Everybody makes mistakes.[f000]븁\u0000\nBut goalkeepers cannot afford a mistake,\nbecause they cannot score goals to make[f000]븀\u0000\nup for it.[f000]븁\u0000\nIf one Pokémon on a team makes a\nmistake, however, the other Pokémon[f000]븀\u0000\nand their Trainer can cover for it!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_EverybodyMakesMistakesBut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If a Striker catches a teammate's eye,\nthey understand each other.[f000]븁\u0000\nA Pokémon and its Trainer are the same.\nDon't you agree?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_IfStrikerCatchesTeammates, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Really strong Trainers thoroughly do\nwhatever they can do!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_ReallyStrongTrainersThoroughly, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Soccer is beautiful and fun!\nPokémon are also beautiful and fun!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_SoccerBeautifulFunPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Soccer with Pokémon is Pokémon soccer.\nThe abbreviation is...Poker?[f000]븀\u0000\nNo, wait, that's a fireplace tool.[f000]븀\u0000\nHow about Poccer?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_SoccerPokemonPokemonSoccer, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Throw a sound pass to a team member![f000]븁\u0000\nAfter that, to receive a sound pass,\nyou'll need to move swiftly."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_ThrowSoundPassTeam, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Run! Run!\nJust think about running!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_RunRunJustThink, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Squeeskwaa!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_Squeeskwaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Meep! ♪"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity7_Text_Meep, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
