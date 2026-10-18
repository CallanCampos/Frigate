#include "asm/field_script.inc"
#include "text/script/accumula_town_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Listen up! You Trainers should always\nkeep a smile on your face![f000]븁\u0000\nIf you're not smiling, your Pokémon might\nfeel like something's wrong, you know?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_ListenUpTrainersShould, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Which Pokémon should hold what item...[f000]븁\u0000\nThere's no right answer,\nso it's hard to decide.[f000]븁\u0000\nStill, I like spending time thinking\nit over."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_WhichPokemonShouldHold, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wow! A Pokémon!\nHow cool! I'm so jealous![f000]븁\u0000\nHa ha! Just kidding!\nI'm a Pokémon Trainer now![f000]븁\u0000\nJust like the Trainer from\nNuvema I met two years ago!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_WowPokemonHowCool, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 515, 0
    // "Paaan!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_Paaan, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01b1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01b2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01B7
    VMStackPushFlag EVENT_FLAG_0x01af
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00FA
    VMCall L_02E2
    VMJump L_01B1

L_00FA:
    VMStackPushFlag EVENT_FLAG_0x01af
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0131
    // "My Ambipom knows Nasty Plot![f000]븁\u0000\nI'll make your Excadrill really strong,\nso let's battle sometime, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_AmbipomKnowsNastyPlot, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01B1

L_0131:
    // "The Excadrill I got from you\nhas become really strong![f000]븀\u0000\nI'm sure you'll be bowled over![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_ExcadrillGotFromHas, 0, 0
    // "Hey! If it's all right with you,\nwould you have a Pokémon battle with me?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_HeyIfItsAll_2, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A3
    // "OK! Here we go![f000]븁\u0000\nWe'll have an actual match, so you can\nreally see how I've raised Excadrill![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_OkHereWeGo, 0, 0
    MsgWinCloseAll
    CallTradedPokemonBattle TRAINER_LASS_DIANA, 0, 0, 2
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0191
    CallTrainerBattleEnd
    VMJump L_0193

L_0191:
    CallTrainerLose

L_0193:
    FlagSet EVENT_FLAG_0x01b1
    VMCall L_03AB
    VMJump L_01B1

L_01A3:
    // "I get it...[f000]븁\u0000\nYou don't want to have battles\nall the time."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetDontWantHave, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01B1:
    VMJump L_02DC

L_01B7:
    VMStackPushFlag EVENT_FLAG_0x01b1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01b2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02CE
    VMStackPushFlag EVENT_FLAG_0x01b0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0209
    VMCall L_03AB
    VMJump L_02C8

L_0209:
    VMStackPushFlag EVENT_FLAG_0x01b0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0240
    // "My Alakazam knows Psycho Cut![f000]븁\u0000\nI'll make your Hippowdon really\ntough, so let's battle sometime, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_AlakazamKnowsPsychoCut, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02C8

L_0240:
    // "The Hippowdon I got from you\nhas become really strong![f000]븀\u0000\nI'm sure you'll be bowled over![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_HippowdonGotFromHas, 0, 0
    // "Hey! If it's all right with you,\nwould you have a Pokémon battle with me?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_HeyIfItsAll_3, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BA
    // "OK! Here we go![f000]븁\u0000\nI'll show you how well I've raised\nHippowdon by having a battle with you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_OkHereWeGo_2, 0, 0
    MsgWinCloseAll
    CallTradedPokemonBattle TRAINER_LASS_DIANA_2, 0, 0, 3
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A0
    CallTrainerBattleEnd
    VMJump L_02A2

L_02A0:
    CallTrainerLose

L_02A2:
    FlagSet EVENT_FLAG_0x01b2
    // "Hey! I'm so glad we were able\nto trade Pokémon and have a battle!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_HeyImGladWe, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02C8

L_02BA:
    // "I get it...[f000]븁\u0000\nYou don't want to have battles\nall the time."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetDontWantHave, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02C8:
    VMJump L_02DC

L_02CE:
    // "Hey! I'm so glad we were able\nto trade Pokémon and have a battle!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_HeyImGladWe, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02DC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_02E2:
    // "Hey! If it's all right with you,\nlet's trade![f000]븁\u0000\nI'll trade you my Ambipom\nfor your Excadrill!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_HeyIfItsAll, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039B
    MsgWinCloseAll
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0387
    FieldTradeCheck 0x8022, TRADE_AMBIPOM, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0373
    // "Great!\nThen, let's start the trade![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GreatThenLetsStart, 0, 0
    MsgWinCloseAll
    FieldTradeSavePokemon 0x8020, 2
    FieldTradeStart 29, 0x8020
    // "My Ambipom knows Nasty Plot![f000]븁\u0000\nI'll make your Excadrill really strong,\nso let's battle sometime, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_AmbipomKnowsNastyPlot, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01af
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_0381

L_0373:
    // "I get it...\nThat's your trusty partner.[f000]븁\u0000\nIf you change your mind,\nlet's trade Pokémon, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetThatsTrustyPartner, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0381:
    VMJump L_0395

L_0387:
    // "I get it...\nThat's your trusty partner.[f000]븁\u0000\nIf you change your mind,\nlet's trade Pokémon, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetThatsTrustyPartner, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0395:
    VMJump L_03A9

L_039B:
    // "I get it...\nThat's your trusty partner.[f000]븁\u0000\nIf you change your mind,\nlet's trade Pokémon, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetThatsTrustyPartner, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03A9:
    VMReturn

L_03AB:
    // "Whew!\nThat was so fun![f000]븁\u0000\nHey! If it's all right with you,\ncould you give me a Hippowdon?[f000]븁\u0000\nI'll trade you my Alakazam!\nC'mon, let's trade Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_WhewFunHeyIf, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0464
    MsgWinCloseAll
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0450
    FieldTradeCheck 0x8022, TRADE_ALAKAZAM, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_043C
    // "Great!\nThen, let's start the trade![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GreatThenLetsStart_2, 0, 0
    MsgWinCloseAll
    FieldTradeSavePokemon 0x8020, 3
    FieldTradeStart 30, 0x8020
    // "My Alakazam knows Psycho Cut![f000]븁\u0000\nI'll make your Hippowdon really\ntough, so let's battle sometime, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_AlakazamKnowsPsychoCut, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01b0
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_044A

L_043C:
    // "I get it...\nThat's your trusty partner.[f000]븁\u0000\nIf you change your mind,\nlet's trade Pokémon, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetThatsTrustyPartner_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_044A:
    VMJump L_045E

L_0450:
    // "I get it...\nThat's your trusty partner.[f000]븁\u0000\nIf you change your mind,\nlet's trade Pokémon, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetThatsTrustyPartner_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_045E:
    VMJump L_0472

L_0464:
    // "I get it...\nThat's your trusty partner.[f000]븁\u0000\nIf you change your mind,\nlet's trade Pokémon, OK?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown2_Text_GetThatsTrustyPartner_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0472:
    VMReturn
