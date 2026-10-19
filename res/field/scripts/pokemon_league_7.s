#include "asm/field_script.inc"
#include "text/script/pokemon_league_7.h"

// Script plugin 3, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0037
    WorkSetConst EVENT_WORK_0x400a, 555

L_0037:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0058
    ActorSetGPos 0, 15, 22, 7, 1

L_0058:
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0073
    BMAnmPlayLoop 7, 17, 9

L_0073:
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0096
    Plugin3_Cmd1000 3
    PokemonLeagueCmd_SetCamera 3
    VMJump L_00A2

L_0096:
    ActorSetGPos 0, 15, 22, 7, 1

L_00A2:
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BD
    BMAnmPlayLoop 7, 17, 9

L_00BD:
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025C
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F1
    VMStackPushFlag EVENT_FLAG_0x01e9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0114
    // "Greetings, challenger.\nMy name is Marshal.[f000]븁\u0000\nI am the No. 1 pupil of my mentor, Alder.[f000]븁\u0000\nIn order to master the art of fighting,\nI've kept training.[f000]븁\u0000\nYou're also walking a similar path\nwith your Pokémon.[f000]븁\u0000\nIt is my intention to test you--to take\nyou to the limits of your strength. Kiai![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_GreetingsChallengerNameMarshal, 0, 1, 0
    MsgWinCloseAll
    VMJump L_0122

L_0114:
    // "You look familiar...\nAh, yes. I met you at Twist Mountain.[f000]븁\u0000\nThe strength you are radiating\nis far greater now than before![f000]븁\u0000\nGreetings, challenger.\nMy name is Marshal.[f000]븁\u0000\nI am the No. 1 pupil of my mentor, Alder.[f000]븁\u0000\nIn order to master the art of fighting,\nI've kept training.[f000]븁\u0000\nYou're also walking a similar path\nwith your Pokémon.[f000]븁\u0000\nIt is my intention to test you--to take\nyou to the limits of your strength. Kiai![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_LookFamiliarAhYes, 0, 1, 0
    MsgWinCloseAll

L_0122:
    FlagSet EVENT_FLAG_0x0969
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8020, 0
    GameGetDifficulty 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0157
    CallTrainerBattle TRAINER_ELITE_FOUR_MARSHAL_3, 0, 0
    VMJump L_015F

L_0157:
    CallTrainerBattle TRAINER_ELITE_FOUR_MARSHAL, 0, 0

L_015F:
    WorkSetConst 0x8020, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0184
    CallTrainerBattleEnd
    VMJump L_0186

L_0184:
    CallTrainerLose

L_0186:
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01DB
    // "Now... You have become the strongest\nTrainer in this Pokémon League.[f000]븁\u0000\nThe statue in the central chamber will\ntake you to the Champion's room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_NowHaveBecomeStrongest, 0, 1, 0
    VMJump L_01E7

L_01DB:
    // "Whew! Well done![f000]븁\u0000\nAs your battles continue,\naim for even greater heights!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_WhewWellDoneBattles, 0, 1, 0

L_01E7:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0256

L_01F1:
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0246
    // "Now... You have become the strongest\nTrainer in this Pokémon League.[f000]븁\u0000\nThe statue in the central chamber will\ntake you to the Champion's room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_NowHaveBecomeStrongest, 0, 1, 0
    VMJump L_0252

L_0246:
    // "You are a strong challenger.[f000]븁\u0000\nWalk the path you believe in\nwith the Pokémon you believe in.[f000]븁\u0000\nThe other members of the Elite Four\nare far more powerful than I am.[f000]븁\u0000\nDo not underestimate them!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_StrongChallengerWalkPath, 0, 1, 0

L_0252:
    LastKeyWait
    MsgWinCloseAll

L_0256:
    VMJump L_03B1

L_025C:
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034C
    // "I thank you deeply for the chance for\nanother round of combat against you.[f000]븁\u0000\nIn myself, I seek to develop\nthe strength of a fighter.[f000]븁\u0000\nAnd shatter any weakness in myself![f000]븁\u0000\nPrevailing with the force of\nmy convictions![f000]븁\u0000\nVictory, decisive victory, is my intention!\nChallenger, here I come![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_ThankDeeplyChanceAnother, 0, 1, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0969
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8021, 0
    GameGetDifficulty 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B2
    CallTrainerBattle TRAINER_ELITE_FOUR_MARSHAL_4, 0, 0
    VMJump L_02BA

L_02B2:
    CallTrainerBattle TRAINER_ELITE_FOUR_MARSHAL_2, 0, 0

L_02BA:
    WorkSetConst 0x8021, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DF
    CallTrainerBattleEnd
    VMJump L_02E1

L_02DF:
    CallTrainerLose

L_02E1:
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0336
    // "The strength shown by you and your\nPokémon has deeply impressed me...[f000]븁\u0000\nPlease, continue to the next room\nto face the strongest Trainer[f000]븀\u0000\nof the Unova region!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_StrengthShownByPokemon, 0, 1, 0
    VMJump L_0342

L_0336:
    // "There are still many strong Trainers\nin this Pokémon League.[f000]븁\u0000\nYou should deepen your bonds with\nyour Pokémon by battling with them."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_ThereStillManyStrong, 0, 1, 0

L_0342:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03B1

L_034C:
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0969
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03A1
    // "The strength shown by you and your\nPokémon has deeply impressed me...[f000]븁\u0000\nPlease, continue to the next room\nto face the strongest Trainer[f000]븀\u0000\nof the Unova region!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_StrengthShownByPokemon, 0, 1, 0
    VMJump L_03AD

L_03A1:
    // "During the days when I was young,\nI was wandering all the regions[f000]븀\u0000\nof the world.[f000]븁\u0000\nI was devoted only to training,\nin order to surpass my mentor.[f000]븁\u0000\nAnd when I felt so ashamed\ntwo years ago...[f000]븁\u0000\nMy Pokémon were always there for me.[f000]븁\u0000\nThat thought crossed my mind\neven though I was completely focused[f000]븀\u0000\non our battle...[f000]븁\u0000\nYou're a mysterious Trainer."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague7_Text_DuringDaysWhenYoung, 0, 1, 0

L_03AD:
    LastKeyWait
    MsgWinCloseAll

L_03B1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMSleep 5
    Plugin3_Cmd1013
    SEPlay SEQ_SE_SW_RENBU_01
    VMSleep 60
    Plugin3_Cmd1014 0
    SEPlay SEQ_SE_SW_RENBU_02
    VMSleep 20
    WorkSetConst EVENT_WORK_0x4000, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    Plugin3_Cmd1000 3
    PokemonLeagueCmd_SetCamera 3
    ActorCmdExec 255, Movement_043C
    ActorCmdWait
    Plugin3_Cmd1014 1
    SEPlay SEQ_SE_SW_RENBU_03
    VMSleep 30
    Plugin3_Cmd1016
    VMSleep 30
    ActorCmdExec 0, Movement_0444
    ActorCmdWait
    VMSleep 10
    Plugin3_Cmd1015
    SEPlay SEQ_SE_SW_RENBU_05
    VMSleep 5
    WorkSetConst EVENT_WORK_0x4001, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    MapChangeWarpPad ZONE_POKEMON_LEAGUE_2, 31, 48, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_043C:
    Move 75, 1
    MoveEnd

Movement_0444:
    Move 100, 1
    MoveEnd
