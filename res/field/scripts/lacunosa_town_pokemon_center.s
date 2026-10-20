#include "asm/field_script.inc"
#include "text/script/lacunosa_town_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    RTCGetDayPart 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0053
    FlagReset EVENT_FLAG_0x0312
    VMJump L_0057

L_0053:
    FlagSet EVENT_FLAG_0x0312

L_0057:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 255
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 11
    WorkSet 0x8001, 2
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "True, nobody goes outside at night\nand it's very peaceful...[f000]븁\u0000\nBut it's all because of\na terrifying Pokémon.[f000]븁\u0000\nI'm not sure how I feel about that."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_TrueNobodyGoesOutside, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00F8

L_00E4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't have anything to do ever since\nI took a post here.[f000]븁\u0000\nSince nobody goes outside at night,\nit's very peaceful."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_DontHaveAnythingEver, 0, 0
    LastKeyWait
    ActorMsgClose

L_00F8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Every Gym Badge tells the story\nof a hard-won victory against[f000]븀\u0000\na worthy opponent.[f000]븁\u0000\nI can look at a Gym Badge and\ntell you that story.[f000]븁\u0000\nCan I see one of your Gym Badges?"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_EveryGymBadgeTells, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013F
    // "OK. You don't have to show me anything.\nI'm sure your memories are all you need!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_OkDontHaveShow, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0164

L_013F:
    WorkSetConst 0x8021, 1

L_0145:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0164
    VMCall L_016A
    VMJump L_0145

L_0164:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_016A:
    WorkSetConst 0x8022, 0
    // "Which Badge's story\nwould you like to hear?"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_WhichBadgesStoryWould, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    TrainerCardHasBadge 0x8010, 0
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A6
    ListMenuAdd 18, 65535, 0

L_01A6:
    TrainerCardHasBadge 0x8010, 1
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C7
    ListMenuAdd 19, 65535, 1

L_01C7:
    TrainerCardHasBadge 0x8010, 2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E8
    ListMenuAdd 20, 65535, 2

L_01E8:
    TrainerCardHasBadge 0x8010, 3
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0209
    ListMenuAdd 21, 65535, 3

L_0209:
    TrainerCardHasBadge 0x8010, 4
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_022A
    ListMenuAdd 22, 65535, 4

L_022A:
    TrainerCardHasBadge 0x8010, 5
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024B
    ListMenuAdd 23, 65535, 5

L_024B:
    TrainerCardHasBadge 0x8010, 6
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026C
    ListMenuAdd 24, 65535, 6

L_026C:
    TrainerCardHasBadge 0x8010, 7
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028D
    ListMenuAdd 25, 65535, 7

L_028D:
    ListMenuAdd 26, 65535, 255
    ListMenuShow
    VMStackPush 0x8022
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02D2
    // "OK. You don't have to show me anything.\nI'm sure your memories are all you need!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_OkDontHaveShow, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMReturn

L_02D2:
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_02E5
    VMJump L_02F7

L_02E5:
    // "You got the Basic Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotBasicBadge, 0x8011, 2, 0
    VMJump L_03FA

L_02F7:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_030A
    VMJump L_031C

L_030A:
    // "You got the Toxic Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotToxicBadge, 0x8011, 2, 0
    VMJump L_03FA

L_031C:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_032F
    VMJump L_0341

L_032F:
    // "You got the Insect Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotInsectBadge, 0x8011, 2, 0
    VMJump L_03FA

L_0341:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_0354
    VMJump L_0366

L_0354:
    // "You got the Bolt Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotBoltBadge, 0x8011, 2, 0
    VMJump L_03FA

L_0366:
    WorkCmpConst 0x8022, 4
    VMJumpIf CMP_EQ, L_0379
    VMJump L_038B

L_0379:
    // "You got the Quake Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotQuakeBadge, 0x8011, 2, 0
    VMJump L_03FA

L_038B:
    WorkCmpConst 0x8022, 5
    VMJumpIf CMP_EQ, L_039E
    VMJump L_03B0

L_039E:
    // "You got the Jet Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotJetBadge, 0x8011, 2, 0
    VMJump L_03FA

L_03B0:
    WorkCmpConst 0x8022, 6
    VMJumpIf CMP_EQ, L_03C3
    VMJump L_03D5

L_03C3:
    // "You got the Legend Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotLegendBadge, 0x8011, 2, 0
    VMJump L_03FA

L_03D5:
    WorkCmpConst 0x8022, 7
    VMJumpIf CMP_EQ, L_03E8
    VMJump L_03FA

L_03E8:
    // "You got the Wave Badge with...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_GotWaveBadge, 0x8011, 2, 0
    VMJump L_03FA

L_03FA:
    WordSetGymVictoryParty 0x8022, 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0413
    VMJump L_0425

L_0413:
    // "[f000]ā\u0001\u0000.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_Empty, 0x8011, 2, 0
    VMJump L_04DE

L_0425:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0438
    VMJump L_044A

L_0438:
    // "[f000]ā\u0001\u0000 and [f000]ā\u0001\u0001.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_And, 0x8011, 2, 0
    VMJump L_04DE

L_044A:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_045D
    VMJump L_046F

L_045D:
    // "[f000]ā\u0001\u0000, [f000]ā\u0001\u0001, and\n[f000]ā\u0001\u0002.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_And_2, 0x8011, 2, 0
    VMJump L_04DE

L_046F:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_0482
    VMJump L_0494

L_0482:
    // "[f000]ā\u0001\u0000, [f000]ā\u0001\u0001,\n[f000]ā\u0001\u0002, and [f000]ā\u0001\u0003.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_And_3, 0x8011, 2, 0
    VMJump L_04DE

L_0494:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_04A7
    VMJump L_04B9

L_04A7:
    // "[f000]ā\u0001\u0000, [f000]ā\u0001\u0001,\n[f000]ā\u0001\u0002, [f000]ā\u0001\u0003, and[f000]븀\u0000\n[f000]ā\u0001\u0004.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_And_4, 0x8011, 2, 0
    VMJump L_04DE

L_04B9:
    WorkCmpConst 0x8010, 6
    VMJumpIf CMP_EQ, L_04CC
    VMJump L_04DE

L_04CC:
    // "[f000]ā\u0001\u0000, [f000]ā\u0001\u0001,\n[f000]ā\u0001\u0002, [f000]ā\u0001\u0003,[f000]븀\u0000\n[f000]ā\u0001\u0004, and [f000]ā\u0001\u0005.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_And_5, 0x8011, 2, 0
    VMJump L_04DE

L_04DE:
    // "Want to show me another Gym Badge?"
    ActorMsg MSGFILE_SCRIPT, LacunosaTownPokemonCenter_Text_WantShowAnotherGym, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050B
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMReturn

L_050B:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 1
    VMReturn
    .balign 4, 0
