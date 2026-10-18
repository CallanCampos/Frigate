#include "asm/field_script.inc"
#include "text/script/global_10310.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x00fc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0043
    // "If you'd like, I could judge the\nintriguing potential of your Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_IfYoudLikeCould, 0, 0
    VMJump L_0051

L_0043:
    // "You can call me the Judge.[f000]븁\u0000\nHeh heh, that's actually not my real\nname, but it's what everyone calls me.[f000]븁\u0000\nIt's because of this weird power I have.\nIt's a gift, really.[f000]븁\u0000\nI can discern--judge, you might say--the\noverall potential of individual Pokémon.[f000]븁\u0000\nIf you'd like, I could judge the\nintriguing potential of your Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_CanCallJudgeHeh, 0, 0
    FlagSet EVENT_FLAG_0x00fc

L_0051:
    ActorMsgClose
    VMCall L_005F
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_005F:
    CallPokeSelect 0, 0x8010, 0x8020, 0
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008C
    // "...Oh?\nYou don't need me to judge. I get it."
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_OhDontNeedJudge, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_008C:
    PokePartyIsEgg 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B5
    // "Judge an Egg?!\nThat's a tall order even for me!"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_JudgeEggThatsTall, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_00B5:
    PokePartyGetIV 0x8020, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 90
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_00E0
    // "I see, I see...[f000]븁\u0000\nThis Pokémon's potential is\ndecent all around.[f000]븁\u0000\nThat's my determination, and it's final.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_SeeSeePokemonsPotential, 0, 0
    VMJump L_0130

L_00E0:
    VMStackPush 0x8010
    VMStackPushConst 120
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0103
    // "I see, I see...[f000]븁\u0000\nThis Pokémon's potential is\nabove average overall.[f000]븁\u0000\nThat's my determination, and it's final.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_SeeSeePokemonsPotential_2, 0, 0
    VMJump L_0130

L_0103:
    VMStackPush 0x8010
    VMStackPushConst 150
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0126
    // "I see, I see...[f000]븁\u0000\nThis Pokémon has\nrelatively superior potential overall.[f000]븁\u0000\nThat's my determination, and it's final.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_SeeSeePokemonHas, 0, 0
    VMJump L_0130

L_0126:
    // "I see, I see...[f000]븁\u0000\nThis Pokémon has\noutstanding potential overall.[f000]븁\u0000\nThat's my determination, and it's final.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_SeeSeePokemonHas_2, 0, 0

L_0130:
    PokePartyGetIV 0x8020, 1, 0x8022
    WorkSetConst 0x8021, 1
    PokePartyGetIV 0x8020, 2, 0x8010
    VMStackPush 0x8010
    VMStackPush 0x8022
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0169
    WorkSetConst 0x8021, 0
    // "Incidentally, I would say the best\npotential lies in its HP.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_IncidentallyWouldSayBest, 0, 0

L_0169:
    PokePartyGetIV 0x8020, 3, 0x8010
    VMStackPush 0x8010
    VMStackPush 0x8022
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B7
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A7
    // "Incidentally, I would say the best\npotential lies in its Attack stat.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_IncidentallyWouldSayBest_2, 0, 0
    VMJump L_01B1

L_01A7:
    // "And its Attack stat\nis also good.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_ItsAttackStatAlso, 0, 0

L_01B1:
    WorkSetConst 0x8021, 0

L_01B7:
    PokePartyGetIV 0x8020, 4, 0x8010
    VMStackPush 0x8010
    VMStackPush 0x8022
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0205
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F5
    // "Incidentally, I would say the best\npotential lies in its Defense stat.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_IncidentallyWouldSayBest_3, 0, 0
    VMJump L_01FF

L_01F5:
    // "I see. Its Defense stat\nis also good.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_SeeItsDefenseStat, 0, 0

L_01FF:
    WorkSetConst 0x8021, 0

L_0205:
    PokePartyGetIV 0x8020, 6, 0x8010
    VMStackPush 0x8010
    VMStackPush 0x8022
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0253
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0243
    // "Incidentally, I would say the best\npotential lies in its Sp. Atk stat.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_IncidentallyWouldSayBest_4, 0, 0
    VMJump L_024D

L_0243:
    // "Its Sp. Atk stat\nis equally good.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_ItsSpAtkStat, 0, 0

L_024D:
    WorkSetConst 0x8021, 0

L_0253:
    PokePartyGetIV 0x8020, 7, 0x8010
    VMStackPush 0x8010
    VMStackPush 0x8022
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A1
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0291
    // "Incidentally, I would say the best\npotential lies in its Sp. Def stat.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_IncidentallyWouldSayBest_5, 0, 0
    VMJump L_029B

L_0291:
    // "Its Sp. Def stat\nis good as well.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_ItsSpDefStat, 0, 0

L_029B:
    WorkSetConst 0x8021, 0

L_02A1:
    PokePartyGetIV 0x8020, 5, 0x8010
    VMStackPush 0x8010
    VMStackPush 0x8022
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EF
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DF
    // "Incidentally, I would say the best\npotential lies in its Speed stat.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_IncidentallyWouldSayBest_6, 0, 0
    VMJump L_02E9

L_02DF:
    // "Well, its Speed stat\nis also good.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_WellItsSpeedStat, 0, 0

L_02E9:
    WorkSetConst 0x8021, 0

L_02EF:
    VMStackPush 0x8022
    VMStackPushConst 15
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0312
    // "It's rather decent in that regard.\nThat's how I judged it."
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_ItsRatherDecentRegard, 0, 0
    VMJump L_0362

L_0312:
    VMStackPush 0x8022
    VMStackPushConst 25
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0335
    // "It's very good in that regard.\nThat's how I judged it."
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_ItsVeryGoodRegard, 0, 0
    VMJump L_0362

L_0335:
    VMStackPush 0x8022
    VMStackPushConst 30
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0358
    // "It's fantastic in that regard.\nThat's how I judged it."
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_ItsFantasticRegardThats, 0, 0
    VMJump L_0362

L_0358:
    // "It can't be better in that regard.\nThat's how I judged it."
    ParentActorMsg MSGFILE_SCRIPT, Global10310_Text_CantBetterRegardThats, 0, 0

L_0362:
    LastKeyWait
    ActorMsgClose
    VMReturn
