#include "asm/field_script.inc"
#include "text/script/pokemon_league_8.h"

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
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0037
    WorkSetConst EVENT_WORK_0x400a, 555

L_0037:
    VMHalt

Script_2:
    PokemonLeagueCmd_PlayCaitlinAmbience
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005A
    ActorSetGPos 0, 15, 15, 7, 1

L_005A:
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0075
    BMAnmPlayLoop 7, 18, 5

L_0075:
    VMHalt

Script_3:
    PokemonLeagueCmd_PlayCaitlinAmbience
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009A
    Plugin3_Cmd1000 4
    PokemonLeagueCmd_SetCamera 4
    VMJump L_00A6

L_009A:
    ActorSetGPos 0, 15, 15, 7, 1

L_00A6:
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C1
    BMAnmPlayLoop 7, 18, 5

L_00C1:
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0239
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CE
    // "It's me who appeared\nwhen the flower opened up.[f000]븁\u0000\nYou, standing over there...[f000]븁\u0000\nYou look like a Pokémon Trainer\nwith strength and kindness.[f000]븁\u0000\nWhat I look for in my opponent is\nsuperb strength...[f000]븁\u0000\nI'm counting on you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_ItsWhoAppearedWhen, 0, 1, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x096a
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8020, 0
    GameGetDifficulty 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0134
    CallTrainerBattle TRAINER_ELITE_FOUR_CAITLIN_3, 0, 0
    VMJump L_013C

L_0134:
    CallTrainerBattle TRAINER_ELITE_FOUR_CAITLIN, 0, 0

L_013C:
    WorkSetConst 0x8020, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0161
    CallTrainerBattleEnd
    VMJump L_0163

L_0161:
    CallTrainerLose

L_0163:
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
    VMJumpIf CMP_STACK, L_01B8
    // "Somehow, you managed to defeat the\nentire Elite Four of the Pokémon League.[f000]븁\u0000\nCheck the statue in the center of the\nplaza for the way to the Champion's room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_SomehowManagedDefeatEntire, 0, 1, 0
    VMJump L_01C4

L_01B8:
    // "You haven't faced all of the members\nof the Elite Four yet, have you?[f000]븁\u0000\nDon't concern yourself about me.\nGo on ahead."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_HaventFacedAllMembers, 0, 1, 0

L_01C4:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0233

L_01CE:
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
    VMJumpIf CMP_STACK, L_0223
    // "Somehow, you managed to defeat the\nentire Elite Four of the Pokémon League.[f000]븁\u0000\nCheck the statue in the center of the\nplaza for the way to the Champion's room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_SomehowManagedDefeatEntire, 0, 1, 0
    VMJump L_022F

L_0223:
    // "Winning is important,\nbut what's more important is[f000]븀\u0000\nwhether I've done better this time.[f000]븁\u0000\nBecause if I can't surpass myself,\nI can't get close to my ideals.[f000]븁\u0000\nI want to improve and win more elegantly,\nso I invite you to be my opponent[f000]븀\u0000\nagain in the future, if you wish."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_WinningImportantButWhats, 0, 1, 0

L_022F:
    LastKeyWait
    MsgWinCloseAll

L_0233:
    VMJump L_038E

L_0239:
    VMStackPushFlag EVENT_FLAG_0x096a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0329
    // "It's me who appeared\nwhen the flower opened up.[f000]븁\u0000\nYou who have been waiting...[f000]븁\u0000\nYou look like a Pokémon Trainer\nwith refined strength and[f000]븀\u0000\ndeepened kindness.[f000]븁\u0000\nWhat I look for in my opponent is\nsuperb strength...[f000]븁\u0000\nPlease unleash your power\nto the fullest![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_ItsWhoAppearedWhen_2, 0, 1, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x096a
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8021, 0
    GameGetDifficulty 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028F
    CallTrainerBattle TRAINER_ELITE_FOUR_CAITLIN_4, 0, 0
    VMJump L_0297

L_028F:
    CallTrainerBattle TRAINER_ELITE_FOUR_CAITLIN_2, 0, 0

L_0297:
    WorkSetConst 0x8021, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BC
    CallTrainerBattleEnd
    VMJump L_02BE

L_02BC:
    CallTrainerLose

L_02BE:
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
    VMJumpIf CMP_STACK, L_0313
    // "You have defeated the\nPokémon League's Elite Four.[f000]븁\u0000\nYou have earned the right to\nproceed to the Champion's room.[f000]븁\u0000\nMaking an entrance is not the point.[f000]븁\u0000\nOnce you're there, you'll need to\nunleash your power to the fullest!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_HaveDefeatedPokemonLeagues, 0, 1, 0
    VMJump L_031F

L_0313:
    // "Alas! Even with the knowledge and skill\npassed down in my family of Trainers,[f000]븀\u0000\nI still can't win.[f000]븁\u0000\nThe reason I came here in the first place\nwas to encounter Trainers like you..."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_AlasEvenKnowledgeSkill, 0, 1, 0

L_031F:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_038E

L_0329:
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
    VMJumpIf CMP_STACK, L_037E
    // "You have defeated the\nPokémon League's Elite Four.[f000]븁\u0000\nYou have earned the right to\nproceed to the Champion's room.[f000]븁\u0000\nMaking an entrance is not the point.[f000]븁\u0000\nOnce you're there, you'll need to\nunleash your power to the fullest!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_HaveDefeatedPokemonLeagues, 0, 1, 0
    VMJump L_038A

L_037E:
    // "When I battle you,\nI can't help but smile...[f000]븁\u0000\nBecause I was able to improve myself,\nand because you're an excellent Trainer.[f000]븁\u0000\nI want to improve and win more elegantly,\nso I invite you to be my opponent[f000]븀\u0000\nagain in the future, if you wish."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague8_Text_WhenBattleCantHelp, 0, 1, 0

L_038A:
    LastKeyWait
    MsgWinCloseAll

L_038E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMSleep 10
    Plugin3_Cmd1005 0
    SEPlay SEQ_SE_SW_CATTLEYA_01
    VMSleep 40
    WorkSetConst EVENT_WORK_0x4000, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    Plugin3_Cmd1000 4
    PokemonLeagueCmd_SetCamera 4
    VMSleep 5
    Plugin3_Cmd1005 1
    VMSleep 40
    Plugin3_Cmd1006
    SEPlay SEQ_SE_SW_CATTLEYA_02
    VMSleep 30
    Plugin3_Cmd1007
    VMSleep 30
    Plugin3_Cmd1008 0, 15
    VMSleep 60
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
