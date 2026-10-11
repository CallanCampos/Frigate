#include "asm/field_script.inc"
#include "text/script/pokemon_league_5.h"

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
    VMStackPushFlag EVENT_FLAG_0x0967
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
    ActorSetGPos 0, 15, 12, 7, 0

L_0058:
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0073
    BMAnmPlayLoop 7, 18, 8

L_0073:
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0096
    Plugin3_Cmd1000 1
    PokemonLeagueCmd_SetCamera 1
    VMJump L_00A2

L_0096:
    ActorSetGPos 0, 15, 12, 7, 0

L_00A2:
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BD
    BMAnmPlayLoop 7, 18, 8

L_00BD:
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0237
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CC
    // "“There is one man\n  who wanders the world[f000]븀\u0000\n  with a white dragon Pokémon[f000]븀\u0000\n  to search for truth...\"[f000]븁\u0000\nThat's part of a novel I'm writing.\nI want to write down the event[f000]븀\u0000\nthat happened on that day...[f000]븁\u0000\nSorry, it has nothing to do with you...\nYou're a challenger, right?[f000]븁\u0000\nI'm the Elite Four's Ghost-type\nPokémon user, Shauntal, and I[f000]븀\u0000\nshall be your opponent.[f000]븁\u0000"
    // "“There is one man\n  who wanders the world[f000]븀\u0000\n  with a black dragon Pokémon[f000]븀\u0000\n  to pursue ideals...\"[f000]븁\u0000\nThat's part of a novel I'm writing.\nI want to write down the event[f000]븀\u0000\nthat happened on that day...[f000]븁\u0000\nSorry, it has nothing to do with you...\nYou're a challenger, right?[f000]븁\u0000\nI'm the Elite Four's Ghost-type\nPokémon user, Shauntal, and I[f000]븀\u0000\nshall be your opponent.[f000]븁\u0000"
    ActorMsgVersioned 1024, PokemonLeague5_Text_ThereOneManWho_2, PokemonLeague5_Text_ThereOneManWho, 0, 1, 0
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0967
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8020, 0
    GameGetDifficulty 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0132
    CallTrainerBattle TRAINER_ELITE_FOUR_SHAUNTAL_3, 0, 0
    VMJump L_013A

L_0132:
    CallTrainerBattle TRAINER_ELITE_FOUR_SHAUNTAL, 0, 0

L_013A:
    WorkSetConst 0x8020, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015F
    CallTrainerBattleEnd
    VMJump L_0161

L_015F:
    CallTrainerLose

L_0161:
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
    VMJumpIf CMP_STACK, L_01B6
    // "Challenger, if you defeat the entire\nElite Four of the Pokémon League,[f000]븀\u0000\nyou can go on to challenge the Champion.[f000]븁\u0000\nAnd you have earned that right.[f000]븁\u0000\nReturn to the plaza in the center\nand check the statue."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_ChallengerIfDefeatEntire, 0, 1, 0
    VMJump L_01C2

L_01B6:
    // "My Pokémon and the challenger's Pokémon.[f000]븁\u0000\nEveryone battled even though\nthey were hurt... Thank you."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_PokemonChallengersPokemonEveryone, 0, 1, 0

L_01C2:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0231

L_01CC:
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
    VMJumpIf CMP_STACK, L_0221
    // "Challenger, if you defeat the entire\nElite Four of the Pokémon League,[f000]븀\u0000\nyou can go on to challenge the Champion.[f000]븁\u0000\nAnd you have earned that right.[f000]븁\u0000\nReturn to the plaza in the center\nand check the statue."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_ChallengerIfDefeatEntire, 0, 1, 0
    VMJump L_022D

L_0221:
    // "Fortitude is needed if you're going\nto battle, don't you think?[f000]븁\u0000\nBecause both you and your opponent\nget hurt.[f000]븁\u0000\nBut if you don't understand the pain,\nyou'll focus on the result and forget[f000]븀\u0000\nabout the bonds with your Pokémon.[f000]븁\u0000\nThat's why I want to write passages\nfull of heart and soul in my novels."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_FortitudeNeededIfYoure, 0, 1, 0

L_022D:
    LastKeyWait
    MsgWinCloseAll

L_0231:
    VMJump L_0426

L_0237:
    VMStackPushFlag EVENT_FLAG_0x0967
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C1
    Random 0x8010, 5
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0263
    VMJump L_0275

L_0263:
    // "“‘Go, Volcarona!\n  Use Heat Wave here!'[f000]븀\u0000\n  The Trainer solemnly ordered[f000]븀\u0000\n  the Pokémon who resembled[f000]븀\u0000\n  his first partner...\"[f000]븁\u0000\nThat's part of a novel I wrote.[f000]븁\u0000\nI absolutely love writing about the close\nbonds between the Trainers and[f000]븀\u0000\nthe Pokémon that I've competed against.[f000]븁\u0000\nCould I use you and your Pokémon as\na subject?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_GoVolcaronaUseHeat, 0, 1, 0
    VMJump L_02F0

L_0275:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0288
    VMJump L_029A

L_0288:
    // "“‘Do you know Thunderbolt?' was\n  his first greeting to me.[f000]븁\u0000\n  It wasn't until after we battled that\n  I learned his name was Volkner.\"[f000]븁\u0000\nThat's part of a novel I wrote.[f000]븁\u0000\nI absolutely love writing about the close\nbonds between the Trainers and[f000]븀\u0000\nthe Pokémon that I've competed against.[f000]븁\u0000\nCould I use you and your Pokémon as\na subject?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_KnowThunderboltHisFirst, 0, 1, 0
    VMJump L_02F0

L_029A:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_02AD
    VMJump L_02BF

L_02AD:
    // "“‘Hey, you! Use Overheat!'[f000]븁\u0000\n  A Fire-type Pokémon user\n  with a hairstyle that would be great[f000]븀\u0000\n  with Head Charge just gave the order[f000]븀\u0000\n  and left...\"[f000]븁\u0000\nThat's part of a novel I wrote.[f000]븁\u0000\nI absolutely love writing about the close\nbonds between the Trainers and[f000]븀\u0000\nthe Pokémon that I've competed against.[f000]븁\u0000\nCould I use you and your Pokémon as\na subject?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_HeyUseOverheatFire, 0, 1, 0
    VMJump L_02F0

L_02BF:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_02D2
    VMJump L_02E4

L_02D2:
    // "“The woman who uses Ghost types,\n  and the woman who uses Ground types.[f000]븁\u0000\n  I couldn't ask the reason\n  why their names and appearances[f000]븀\u0000\n  are so similar.\"[f000]븁\u0000\nThat's part of a novel I wrote.[f000]븁\u0000\nI absolutely love writing about the close\nbonds between the Trainers and[f000]븀\u0000\nthe Pokémon that I've competed against.[f000]븁\u0000\nCould I use you and your Pokémon as\na subject?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_WomanWhoUsesGhost, 0, 1, 0
    VMJump L_02F0

L_02E4:
    // "“‘Yes. My code name is Looker.'[f000]븁\u0000\n  He sounded a bit aloof, but his partner\n  Croagunk's strategy was tricky.\"[f000]븁\u0000\nThat's part of a novel I wrote.[f000]븁\u0000\nI absolutely love writing about the close\nbonds between the Trainers and[f000]븀\u0000\nthe Pokémon that I've competed against.[f000]븁\u0000\nCould I use you and your Pokémon as\na subject?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_YesCodeNameLooker, 0, 1, 0

L_02F0:
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0967
    WorkSetConst EVENT_WORK_0x400a, 555
    WorkSetConst 0x8021, 0
    GameGetDifficulty 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0327
    CallTrainerBattle TRAINER_ELITE_FOUR_SHAUNTAL_4, 0, 0
    VMJump L_032F

L_0327:
    CallTrainerBattle TRAINER_ELITE_FOUR_SHAUNTAL_2, 0, 0

L_032F:
    WorkSetConst 0x8021, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0354
    CallTrainerBattleEnd
    VMJump L_0356

L_0354:
    CallTrainerLose

L_0356:
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
    VMJumpIf CMP_STACK, L_03AB
    // "Challenger, if you defeat the entire\nElite Four of the Pokémon League, you[f000]븀\u0000\ncan go on to challenge the Champion.[f000]븁\u0000\nAnd you have earned that right.[f000]븁\u0000\nYour story is yours and yours alone.\nPlease weave a wonderful tale!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_ChallengerIfDefeatEntire_2, 0, 1, 0
    VMJump L_03B7

L_03AB:
    // "All the Pokémon on both sides\nbattled so bravely.[f000]븀\u0000\nEven though they got hurt...[f000]븁\u0000\nThank you."
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_AllPokemonBothSides, 0, 1, 0

L_03B7:
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0426

L_03C1:
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
    VMJumpIf CMP_STACK, L_0416
    // "Challenger, if you defeat the entire\nElite Four of the Pokémon League, you[f000]븀\u0000\ncan go on to challenge the Champion.[f000]븁\u0000\nAnd you have earned that right.[f000]븁\u0000\nYour story is yours and yours alone.\nPlease weave a wonderful tale!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_ChallengerIfDefeatEntire_2, 0, 1, 0
    VMJump L_0422

L_0416:
    // "It's important to be tough and resilient\nwhen you battle, don't you think?[f000]븁\u0000\nYou have to accept that both sides will\nget hurt. You have to understand that[f000]븀\u0000\nthe pain of losing is a natural outcome.[f000]븁\u0000\nIf you just focus on winning, you can\nforget that the bond between you and[f000]븀\u0000\nyour Pokémon is the most important thing.[f000]븁\u0000\nI try to focus on that bond in my novels.\nI want them to be full of heart and soul!"
    ActorMsg MSGFILE_SCRIPT, PokemonLeague5_Text_ItsImportantToughResilient, 0, 1, 0

L_0422:
    LastKeyWait
    MsgWinCloseAll

L_0426:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    Plugin3_Cmd1009
    VMSleep 31
    Plugin3_Cmd1010 0
    VMSleep 35
    Plugin3_Cmd1010 1
    VMSleep 7
    Plugin3_Cmd1010 2
    VMSleep 7
    Plugin3_Cmd1010 3
    VMSleep 7
    Plugin3_Cmd1010 4
    WorkSetConst EVENT_WORK_0x4000, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    Plugin3_Cmd1000 1
    PokemonLeagueCmd_SetCamera 1
    Plugin3_Cmd1011 37
    Plugin3_Cmd1012
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
