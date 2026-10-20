#include "asm/field_script.inc"
#include "text/script/pokemon_league_6.h"

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
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0037
    WorkSetConst EVENT_WORK_0x400a, 555

L_0037:
    VMHalt

Script_2:
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0054
    BMAnmPlayLoop 7, 19, 7

L_0054:
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0071
    Plugin3_Cmd1000 2
    PokemonLeagueCmd_SetCamera 2

L_0071:
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008C
    BMAnmPlayLoop 7, 19, 7

L_008C:
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0204
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0199
    // "What will be determined here is\nwhich of us can absorb the opponent's[f000]븀\u0000\nlight and shine...[f000]븁\u0000\nBut who will decide that?[f000]븁\u0000\nIt shall be I, Grimsley of the Elite Four,\nand I will fulfill my duty to be[f000]븀\u0000\nyour opponent.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_WhatWillDeterminedHere, 0, 1, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0968
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8020, 0
    GameGetDifficulty 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FF
    CallTrainerBattle TRAINER_ELITE_FOUR_GRIMSLEY_3, 0, 0
    VMJump L_0107

L_00FF:
    CallTrainerBattle TRAINER_ELITE_FOUR_GRIMSLEY, 0, 0

L_0107:
    WorkSetConst 0x8020, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012C
    CallTrainerBattleEnd
    VMJump L_012E

L_012C:
    CallTrainerLose

L_012E:
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
    VMJumpIf CMP_STACK, L_0183
    // "Astonishing![f000]븁\u0000\nYou have defeated every member of\nthe Pokémon League's Elite Four.[f000]븁\u0000\nBut it isn't over yet.[f000]븁\u0000\nThere is one more opponent against whom\nyou must prove your strength.[f000]븁\u0000\nCheck the statue in the central plaza,\nand continue to the final room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_AstonishingHaveDefeatedEvery, 0, 1, 0
    VMJump L_018F

L_0183:
    // "Whether or not you get to fight at full\nstrength, whether or not luck smiles[f000]븀\u0000\non you--none of that matters.[f000]븁\u0000\nOnly results matter. And a loss is a loss.[f000]븁\u0000\nSee, victory shines like a bright light.[f000]븁\u0000\nAnd right now, you and your Pokémon\nare shining brilliantly."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_WhetherNotGetFight, 0, 1, 0

L_018F:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01FE

L_0199:
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
    VMJumpIf CMP_STACK, L_01EE
    // "Astonishing![f000]븁\u0000\nYou have defeated every member of\nthe Pokémon League's Elite Four.[f000]븁\u0000\nBut it isn't over yet.[f000]븁\u0000\nThere is one more opponent against whom\nyou must prove your strength.[f000]븁\u0000\nCheck the statue in the central plaza,\nand continue to the final room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_AstonishingHaveDefeatedEvery, 0, 1, 0
    VMJump L_01FA

L_01EE:
    // "Now, I'm nothing more than\nthe one who lost his light...[f000]븁\u0000\nBut this loss will make me shine\neven brighter next time...[f000]븁\u0000\nIf I think that way, it's not too bad.[f000]븁\u0000\nSigh...[f000]븁\u0000\nYou should take that strength and test\nit against the rest of the Elite Four."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_NowImNothingMore, 0, 1, 0

L_01FA:
    LastKeyWait
    MsgWinCloseAll

L_01FE:
    VMJump L_0359

L_0204:
    VMStackPushFlag EVENT_FLAG_0x0968
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F4
    // "Life is a serious battle, and you have\nto use the tools you're given.[f000]븁\u0000\nIt's more important to master the cards\nyou're holding than to complain about[f000]븀\u0000\nthe ones your opponents were dealt.[f000]븁\u0000\nLet us begin.\nAnd may the best Trainer win![f000]븁\u0000\nContests like this are proof\nthat you are really living...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_LifeSeriousBattleHave, 0, 1, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0968
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8021, 0
    GameGetDifficulty 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025A
    CallTrainerBattle TRAINER_ELITE_FOUR_GRIMSLEY_4, 0, 0
    VMJump L_0262

L_025A:
    CallTrainerBattle TRAINER_ELITE_FOUR_GRIMSLEY_2, 0, 0

L_0262:
    WorkSetConst 0x8021, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0287
    CallTrainerBattleEnd
    VMJump L_0289

L_0287:
    CallTrainerLose

L_0289:
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
    VMJumpIf CMP_STACK, L_02DE
    // "Astonishing![f000]븁\u0000\nYou have defeated every member of\nthe Pokémon League's Elite Four.[f000]븁\u0000\nBut it isn't over yet.[f000]븁\u0000\nThere is one more opponent against whom\nyou must prove your strength.[f000]븁\u0000\nCheck the statue in the central plaza,\nand continue to the final room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_AstonishingHaveDefeatedEvery_2, 0, 1, 0
    VMJump L_02EA

L_02DE:
    // "There are bad ways to win--\nand good ways to lose.[f000]븁\u0000\nWhat's interesting and troubling is that\nit's not always clear which is which.[f000]븁\u0000\nA flipped coin doesn't always land\nheads or tails.[f000]븁\u0000\nSometimes it may never land at all..."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_ThereBadWaysWin, 0, 1, 0

L_02EA:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0359

L_02F4:
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
    VMJumpIf CMP_STACK, L_0349
    // "Astonishing![f000]븁\u0000\nYou have defeated every member of\nthe Pokémon League's Elite Four.[f000]븁\u0000\nBut it isn't over yet.[f000]븁\u0000\nThere is one more opponent against whom\nyou must prove your strength.[f000]븁\u0000\nCheck the statue in the central plaza,\nand continue to the final room."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_AstonishingHaveDefeatedEvery_2, 0, 1, 0
    VMJump L_0355

L_0349:
    // "There's nothing left\nfor the loser.[f000]븁\u0000\nI guess that's not true...\nEverything has a meaning.[f000]븁\u0000\nI just have to use the disappointment\nas a motivation to get strong.[f000]븁\u0000\nThat said...\nYou should take that strength and test[f000]븀\u0000\nit against the rest of the Elite Four!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague6_Text_TheresNothingLeftLoser, 0, 1, 0

L_0355:
    LastKeyWait
    MsgWinCloseAll

L_0359:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    Plugin3_Cmd1002 0
    SEPlay SEQ_SE_SW_GEEMA_01
    VMSleep 20
    Plugin3_Cmd1003 0
    SEPlay SEQ_SE_SW_GEEMA_02
    VMSleep 25
    WorkSetConst EVENT_WORK_0x4000, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    Plugin3_Cmd1000 2
    PokemonLeagueCmd_SetCamera 2
    VMSleep 10
    Plugin3_Cmd1003 1
    SEPlay SEQ_SE_SW_GEEMA_03
    VMSleep 50
    Plugin3_Cmd1002 1
    SEPlay SEQ_SE_SW_GEEMA_01
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
