#include "asm/field_script.inc"
#include "text/script/virbank_city_gym.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_1:
    VMStackPushFlag EVENT_FLAG_0x02fb
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004B
    WorkSetConst EVENT_WORK_0x4000, 1
    WorkSetConst EVENT_WORK_0x4001, 1
    WorkSetConst EVENT_WORK_0x4002, 1
    WorkSetConst EVENT_WORK_0x4003, 1
    VMJump L_0063

L_004B:
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst EVENT_WORK_0x4001, 1
    WorkSetConst EVENT_WORK_0x4002, 1
    WorkSetConst EVENT_WORK_0x4003, 1

L_0063:
    VMHalt

Script_2:
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40aa
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E2
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PlayerGetGPos 0x8020, 0x8021
    WorkCmpConst 0x8020, 8
    VMJumpIf CMP_EQ, L_00A1
    VMJump L_00B1

L_00A1:
    ActorCmdExec 0, Movement_01F4
    ActorCmdWait
    VMJump L_0101

L_00B1:
    WorkCmpConst 0x8020, 9
    VMJumpIf CMP_EQ, L_00C4
    VMJump L_00D4

L_00C4:
    ActorCmdExec 0, Movement_0200
    ActorCmdWait
    VMJump L_0101

L_00D4:
    WorkCmpConst 0x8020, 10
    VMJumpIf CMP_EQ, L_00E7
    VMJump L_00F7

L_00E7:
    ActorCmdExec 0, Movement_020C
    ActorCmdWait
    VMJump L_0101

L_00F7:
    ActorCmdExec 0, Movement_01F4
    ActorCmdWait

L_0101:
    ActorCmdExec 255, Movement_0238
    ActorCmdWait
    VMStackPushFlag EVENT_FLAG_0x0074
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0150
    // "This is a Pokémon Gym, and\nit's also a rock club![f000]븁\u0000\nThe Gym Leader and the others\nare practicing inside, but please[f000]븀\u0000\nfeel free to challenge all of them![f000]븁\u0000\nOh! You'll need to stay hydrated.\nHere you go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCityGym_Text_PokemonGymItsAlso, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0074

L_0150:
    WorkCmpConst 0x8020, 8
    VMJumpIf CMP_EQ, L_0163
    VMJump L_0173

L_0163:
    ActorCmdExec 0, Movement_0218
    ActorCmdWait
    VMJump L_01DC

L_0173:
    WorkCmpConst 0x8020, 9
    VMJumpIf CMP_EQ, L_0186
    VMJump L_0196

L_0186:
    ActorCmdExec 0, Movement_0220
    ActorCmdWait
    VMJump L_01DC

L_0196:
    WorkCmpConst 0x8020, 10
    VMJumpIf CMP_EQ, L_01A9
    VMJump L_01B9

L_01A9:
    ActorCmdExec 0, Movement_022C
    ActorCmdWait
    VMJump L_01DC

L_01B9:
    WorkCmpConst 0x8020, 8
    VMJumpIf CMP_EQ, L_01CC
    VMJump L_01DC

L_01CC:
    ActorCmdExec 0, Movement_0218
    ActorCmdWait
    VMJump L_01DC

L_01DC:
    WorkSetConst EVENT_WORK_0x40aa, 1

L_01E2:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_01F4:
    Move 75, 1
    Move 3, 1
    MoveEnd

Movement_0200:
    Move 75, 1
    Move 15, 1
    MoveEnd

Movement_020C:
    Move 75, 1
    Move 15, 2
    MoveEnd

Movement_0218:
    Move 1, 1
    MoveEnd

Movement_0220:
    Move 14, 1
    Move 1, 1
    MoveEnd

Movement_022C:
    Move 14, 2
    Move 1, 1
    MoveEnd

Movement_0238:
    Move 34, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 1
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0275
    // "The Trainers in this Pokémon Gym\nall use Poison-type Pokémon![f000]븁\u0000\nGrass- and Bug-type attacks\ndon't work well against Poison types,[f000]븀\u0000\nso be careful![f000]븁\u0000\nAlso, if your Pokémon are poisoned,\ntheir HP will keep decreasing. Watch out![f000]븁\u0000\nAnd you should know that Poison-type\nPokémon can't be poisoned!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCityGym_Text_TrainersPokemonGymAll, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0283

L_0275:
    // "The shining Gym Badge is important\nbecause you and your Pokémon[f000]븀\u0000\nwon it together![f000]븀\u0000\nPlease remember that always!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCityGym_Text_ShiningGymBadgeImportant, 0, 0
    LastKeyWait
    ActorMsgClose

L_0283:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    TrainerCardHasBadge 0x8008, 1
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    // "Virbank City Pokémon Gym[f000]븁\u0000\nLeader: Roxie\nCertified Trainers:"
    InfoMsg VirbankCityGym_Text_VirbankCityPokemonGym, 2
    VMJump L_02DC

L_02B9:
    VMStackPush EVENT_WORK_0x40ac
    VMStackPushConst 4
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_02D7
    // "Virbank City Pokémon Gym[f000]븁\u0000\nLeader: Roxie\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg VirbankCityGym_Text_VirbankCityPokemonGym_3, 2
    VMJump L_02DC

L_02D7:
    // "Virbank City Pokémon Gym[f000]븁\u0000\nLeader: Roxie\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000"
    InfoMsg VirbankCityGym_Text_VirbankCityPokemonGym_2, 2

L_02DC:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "I can sing!\nWanted: The rest of a band!"
    InfoMsg VirbankCityGym_Text_CanSingWantedRest, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
