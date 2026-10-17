#include "asm/field_script.inc"
#include "text/script/accumula_town_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0151
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008A
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004C
    VMCall L_009E
    VMJump L_0084

L_004C:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006B
    VMCall L_0172
    VMJump L_0084

L_006B:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0084
    VMCall L_0246

L_0084:
    VMJump L_0098

L_008A:
    // "Looking at the Pokédex is fun![f000]븁\u0000\nPokémon can be a lot bigger\nor smaller than you imagine!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_LookingPokedexFunPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0098:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_009E:
    VMStackPushFlag EVENT_FLAG_0x0150
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C6
    WordSetPokeSpecies 0, 495
    // "Which Pokémon did you pick\nto be your partner at the beginning?[f000]븁\u0000\n...\n...[f000]븁\u0000\nOh, really? It was [f000]ā\u0001\u0000?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_WhichPokemonDidPick, 0, 0
    MsgWaitAdvance
    FlagSet EVENT_FLAG_0x0150

L_00C6:
    // "All righty, I'll quiz you about Snivy!\nIs Snivy's height 2'04\"?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_AllRightyIllQuiz, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FB
    // "Too bad! Well, I guess you don't\nknow as much as I thought!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_TooBadWellGuess, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0170

L_00FB:
    // "Correct! OK, next question!\nIs Snivy's weight 18 lbs.?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_CorrectOkNextQuestion, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0130
    // "Too bad! Well, I guess you don't\nknow as much as I thought!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_TooBadWellGuess, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0170

L_0130:
    // "Correct! I knew you'd get it!\nI'm so happy you got it right![f000]븀\u0000\nHere, this is for you!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_CorrectKnewYoudGet, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 551
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Looking at the Pokédex is fun![f000]븁\u0000\nPokémon can be a lot bigger\nor smaller than you imagine!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_LookingPokedexFunPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0151

L_0170:
    VMReturn

L_0172:
    VMStackPushFlag EVENT_FLAG_0x0150
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019A
    WordSetPokeSpecies 0, 498
    // "Which Pokémon did you pick\nto be your partner at the beginning?[f000]븁\u0000\n...\n...[f000]븁\u0000\nOh, really? It was [f000]ā\u0001\u0000?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_WhichPokemonDidPick, 0, 0
    MsgWaitAdvance
    FlagSet EVENT_FLAG_0x0150

L_019A:
    // "Well, then I'll quiz you about Tepig!\nIs Tepig's height 1'08\"?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_WellThenIllQuiz, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0236
    // "Correct! OK, next question!\nIs Tepig's weight 21.8 lbs?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_CorrectOkNextQuestion_2, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0222
    // "Correct! I knew you'd get it!\nI'm so happy you got it right![f000]븀\u0000\nHere, this is a gift for you!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_CorrectKnewYoudGet_2, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 548
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Looking at the Pokédex is fun![f000]븁\u0000\nPokémon can be a lot bigger\nor smaller than you imagine!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_LookingPokedexFunPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0151
    VMJump L_0230

L_0222:
    // "Too bad! Well, I guess you don't notice\nthings as much as I would've thought."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_TooBadWellGuess_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0230:
    VMJump L_0244

L_0236:
    // "Too bad! Well, I guess you don't notice\nthings as much as I would've thought."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_TooBadWellGuess_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0244:
    VMReturn

L_0246:
    VMStackPushFlag EVENT_FLAG_0x0150
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026E
    WordSetPokeSpecies 0, 501
    // "Which Pokémon did you pick\nto be your partner at the beginning?[f000]븁\u0000\n...\n...[f000]븁\u0000\nOh, really? It was [f000]ā\u0001\u0000?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_WhichPokemonDidPick, 0, 0
    MsgWaitAdvance
    FlagSet EVENT_FLAG_0x0150

L_026E:
    // "OK! I'll quiz you about Oshawott!\nIs Oshawott's height 2'00\"?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_OkIllQuizAbout, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A3
    // "Too bad! Well, I guess you overlook\nthings more than I would've thought."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_TooBadWellGuess_3, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0318

L_02A3:
    // "Correct! OK, next question!\nIs Oshawott's weight 13.0 lbs.?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_CorrectOkNextQuestion_3, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030A
    // "Correct! I knew you'd get it!\nI'm so happy you got it right![f000]븀\u0000\nHere, take this!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_CorrectKnewYoudGet_3, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 549
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Looking at the Pokédex is fun![f000]븁\u0000\nPokémon can be a lot bigger\nor smaller than you imagine!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_LookingPokedexFunPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0151
    VMJump L_0318

L_030A:
    // "Too bad! Well, I guess you overlook\nthings more than I would've thought."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_TooBadWellGuess_3, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0318:
    VMReturn

Script_2:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 4
    WorkSet 0x8001, 10
    WorkSet 0x8002, 103
    WorkSet 0x8003, 14
    WorkSet 0x8004, 15
    WorkSet 0x8005, 15
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Skree skree!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_SkreeSkree, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi, hi!\nLet's play Pokémon rock-paper-scissors!"
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_HiHiLetsPlay, 2, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0512
    // "Here goes!\nPokémon rock-paper-scissors..."
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_HereGoesPokemonRock, 2, 2, 0
    WorkSetConst 0x8020, 0
    Random 0x8020, 100
    WorkSetConst 0x8021, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32801
    ListMenuAdd 26, 65535, 0
    ListMenuAdd 27, 65535, 1
    ListMenuAdd 28, 65535, 2
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0468
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0452
    // "Your Pokémon is Fire type,\nand mine is Grass type...[f000]븁\u0000\nGrass type is weak against Fire type...\nso I lose."
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_PokemonFireTypeMine, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0462

L_0452:
    // "Your Pokémon is Fire type,\nand mine is Water type...[f000]븁\u0000\nFire type is weak against Water type...\nso I win!"
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_PokemonFireTypeMine_2, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0462:
    VMJump L_050C

L_0468:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04BA
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04A4
    // "Your Pokémon is Grass type,\nand mine is Water type...[f000]븁\u0000\nWater type is weak against Grass type...\nso I lose."
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_PokemonGrassTypeMine, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04B4

L_04A4:
    // "Your Pokémon is Grass type,\nand mine is Fire type...[f000]븁\u0000\nGrass type is weak against Fire type...\nso I win!"
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_PokemonGrassTypeMine_2, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_04B4:
    VMJump L_050C

L_04BA:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050C
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04F6
    // "Your Pokémon is Water type,\nand mine is Fire type...[f000]븁\u0000\nFire type is weak against Water type...\nso I lose."
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_PokemonWaterTypeMine, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0506

L_04F6:
    // "Your Pokémon is Water type,\nand mine is Grass type...[f000]븁\u0000\nWater type is weak against Grass type...\nso I win!"
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_PokemonWaterTypeMine_2, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0506:
    VMJump L_050C

L_050C:
    VMJump L_0522

L_0512:
    // "Oh, that's no fun!"
    ActorMsg MSGFILE_SCRIPT, AccumulaTown7_Text_OhThatsNoFun, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0522:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
