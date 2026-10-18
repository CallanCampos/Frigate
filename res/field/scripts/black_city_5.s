#include "asm/field_script.inc"
#include "text/script/black_city_5.h"

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
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0

Script_1:
    VMHalt

L_0096:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01EB
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_00CF
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_00CF
    VMJump L_00DF

L_00CF:
    ParentActorMsg MSGFILE_SCRIPT, 0x8022, 2, 0
    VMJump L_00E9

L_00DF:
    ParentActorMsg MSGFILE_SCRIPT, 0x8021, 2, 0

L_00E9:
    MoneyWinDisp 31, 1
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32808
    ListMenuAdd 39, 65535, 0
    ListMenuAdd 40, 65535, 1
    ListMenuShow
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D5
    ItemCheckSpace 0x802b, 1, 0x8029
    MoneyCheck 0x802a, 0x802c
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0152
    ParentActorMsg MSGFILE_SCRIPT, 0x8024, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01CF

L_0152:
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0179
    ParentActorMsg MSGFILE_SCRIPT, 0x8025, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01CF

L_0179:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x802c
    MoneyWinUpdate
    SEWait
    ParentActorMsg MSGFILE_SCRIPT, 0x8023, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x802b
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ParentActorMsg MSGFILE_SCRIPT, 0x8026, 2, 0
    LastKeyWait
    MsgWinCloseAll
    RecordAdd 21, 1
    RecordAdd 22, 0x802c
    FlagSet 0x802e

L_01CF:
    VMJump L_01E3

L_01D5:
    ParentActorMsg MSGFILE_SCRIPT, 0x8026, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_01E3:
    MoneyWinClose
    VMJump L_01F9

L_01EB:
    ParentActorMsg MSGFILE_SCRIPT, 0x8027, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_01F9:
    VMReturn

Script_2:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_0221
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_0221
    VMJump L_0233

L_0221:
    WorkSetConst 0x802b, 321
    WorkSetConst 0x802c, 10000
    VMJump L_023F

L_0233:
    WorkSetConst 0x802b, 83
    WorkSetConst 0x802c, 10000

L_023F:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 5
    WorkSetConst 0x8021, 11
    WorkSetConst 0x8022, 12
    WorkSetConst 0x8023, 13
    WorkSetConst 0x8024, 14
    WorkSetConst 0x8025, 15
    WorkSetConst 0x8026, 16
    WorkSetConst 0x8027, 17
    WorkSetConst 0x802e, 2789
    VMCall L_0096
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_02AD
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_02AD
    VMJump L_02BF

L_02AD:
    WorkSetConst 0x802b, 233
    WorkSetConst 0x802c, 20000
    VMJump L_02CB

L_02BF:
    WorkSetConst 0x802b, 82
    WorkSetConst 0x802c, 20000

L_02CB:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 5
    WorkSetConst 0x8021, 18
    WorkSetConst 0x8022, 19
    WorkSetConst 0x8023, 20
    WorkSetConst 0x8024, 21
    WorkSetConst 0x8025, 22
    WorkSetConst 0x8026, 23
    WorkSetConst 0x8027, 24
    WorkSetConst 0x802e, 2790
    VMCall L_0096
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_0339
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_0339
    VMJump L_034B

L_0339:
    WorkSetConst 0x802b, 252
    WorkSetConst 0x802c, 40000
    VMJump L_0357

L_034B:
    WorkSetConst 0x802b, 108
    WorkSetConst 0x802c, 40000

L_0357:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 5
    WorkSetConst 0x8021, 25
    WorkSetConst 0x8022, 26
    WorkSetConst 0x8023, 27
    WorkSetConst 0x8024, 28
    WorkSetConst 0x8025, 29
    WorkSetConst 0x8026, 30
    WorkSetConst 0x8027, 31
    WorkSetConst 0x802e, 2791
    VMCall L_0096
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    RTCGetWeekDay 0x802d
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_03C5
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_03C5
    VMJump L_03D7

L_03C5:
    WorkSetConst 0x802b, 324
    WorkSetConst 0x802c, 60000
    VMJump L_03E3

L_03D7:
    WorkSetConst 0x802b, 109
    WorkSetConst 0x802c, 60000

L_03E3:
    WordSetItemNameWithArticle 0, 0x802b
    WordSetNumber 1, 0x802c, 5
    WorkSetConst 0x8021, 32
    WorkSetConst 0x8022, 33
    WorkSetConst 0x8023, 34
    WorkSetConst 0x8024, 35
    WorkSetConst 0x8025, 36
    WorkSetConst 0x8026, 37
    WorkSetConst 0x8027, 38
    WorkSetConst 0x802e, 2792
    VMCall L_0096
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    KeysCmd_02D1 0x8020
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_045E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Black City.[f000]븁\u0000\nThis is the city of dreams, greed, and\nmore greed.[f000]븁\u0000\nAnd I am Black City's boss, so I'm\na whirlpool of greed!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_WelcomeBlackCityCity, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04C2

L_045E:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_049B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Your greed is impressive.\nI know.[f000]븁\u0000\nYou climbed right up the Black Tower.\nThat's great![f000]븁\u0000\nI like people who are\nfilled with ambition and greed."
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_GreedImpressiveKnowClimbed, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04C2

L_049B:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04C2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Having amazing Trainers like\nyou here in Black City makes[f000]븀\u0000\nme seem less impressive.[f000]븁\u0000\nBut, whatever!\nMy greed knows no bounds...[f000]븁\u0000\nThat's right! That's why I'm\nthe boss of Black City!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_HavingAmazingTrainersLike, 0, 0
    LastKeyWait
    ActorMsgClose

L_04C2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It has worth because it's expensive.\nIf you think that, you'll get burned![f000]븁\u0000\nYou have to get smarter so you\nwon't get tricked!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_HasWorthBecauseIts, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to become really powerful\nso I can make more money!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_WantBecomeReallyPowerful, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Money can't get you everything.[f000]븁\u0000\nStill, if you have it,\nyou can get almost anything!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_MoneyCantGetEverything, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can't be satisfied by\nbeing the same as everyone else![f000]븁\u0000\nIf you are, you're just not thinking,\nand you'll be tricked by bad people."
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_CantSatisfiedByBeing, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hmmm... Isn't there a better job where\nI can make more money?[f000]븁\u0000\nI mean, come on![f000]븁\u0000\nI want more money\nif I'm going to do the same job!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_HmmmIsntThereBetter, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder how strong this Pokémon\ncould become...[f000]븁\u0000\nStrength is a measure of worth, right?"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_WonderHowStrongPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Everything I want is here!\nIf I only had money! If only![f000]븀\u0000\nThat's right![f000]븀\u0000\nI'm going to work hard to make money!"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_EverythingWantHereIf, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Did civilization develop so that\npeople can get what they want?"
    ParentActorMsg MSGFILE_SCRIPT, BlackCity5_Text_DidCivilizationDevelopPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
