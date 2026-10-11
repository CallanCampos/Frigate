#include "asm/field_script.inc"
#include "text/script/striaton_city_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8023, 0
    SEPlay SEQ_SE_MESSAGE
    // "The blackboard explains Pokémon\nstatus changes in battle.[f000]븁\u0000"
    SystemMsg StriatonCity9_Text_BlackboardExplainsPokemonStatus, 2

L_003E:
    VMStackPush 0x8023
    VMStackPushConst 5
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0139
    // "What do you want to read about?"
    SystemMsg StriatonCity9_Text_WhatWantReadAbout, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 51, 65535, 0
    ListMenuAdd 52, 65535, 1
    ListMenuAdd 53, 65535, 2
    ListMenuAdd 54, 65535, 3
    ListMenuAdd 55, 65535, 4
    ListMenuAdd 56, 65535, 5
    ListMenuShow
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B1
    // "If poisoned, a Pokémon steadily loses HP\nwhen battling.[f000]븁\u0000\nThe poison lingers after the battle.\nTo cure it, use an Antidote.[f000]븁\u0000"
    SystemMsg StriatonCity9_Text_IfPoisonedPokemonSteadily, 2
    VMJump L_0133

L_00B1:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D0
    // "Paralysis reduces the Speed stat\nand may prevent movement.[f000]븁\u0000\nIt remains after battle, so use a\nParlyz Heal.[f000]븁\u0000"
    SystemMsg StriatonCity9_Text_ParalysisReducesSpeedStat, 2
    VMJump L_0133

L_00D0:
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EF
    // "If a Pokémon falls asleep,\nit will be unable to attack.[f000]븁\u0000\nThe Pokémon may wake up on its own,\nbut if a battle ends while it is sleeping,[f000]븀\u0000\nit will stay asleep.[f000]븁\u0000\nWake it up using an Awakening.[f000]븁\u0000"
    SystemMsg StriatonCity9_Text_IfPokemonFallsAsleep, 2
    VMJump L_0133

L_00EF:
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010E
    // "A burn reduces the Attack stat and\nsteadily reduces the victim's HP.[f000]븁\u0000\nA burn lingers after battle.\nCure a burn using a Burn Heal.[f000]븁\u0000"
    SystemMsg StriatonCity9_Text_BurnReducesAttackStat, 2
    VMJump L_0133

L_010E:
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012D
    // "If a Pokémon is frozen, it becomes\ncompletely helpless.[f000]븁\u0000\nThe Pokémon may thaw out on its own,\nbut if a battle ends while it is frozen,[f000]븀\u0000\nit will stay frozen.[f000]븁\u0000\nThaw it out using an Ice Heal.[f000]븁\u0000"
    SystemMsg StriatonCity9_Text_IfPokemonFrozenBecomes, 2
    VMJump L_0133

L_012D:
    WorkSetConst 0x8023, 5

L_0133:
    VMJump L_003E

L_0139:
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 0
    VMStackCmp CMP_GE
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0537
    WorkSetConst 0x8021, 0

L_0172:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_049B
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01D7
    // "I'd like to pick your brain again.\nIs that OK?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_IdLikePickBrain, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D1
    VMJump L_01D7

L_01D1:
    WorkSetConst 0x8021, 1

L_01D7:
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02BF
    // "'Scuse me! Got a minute?[f000]븁\u0000\nYou look like you know\na lot about Pokémon.[f000]븁\u0000\nCould you use your knowledge\nto help me out?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ScuseGotMinuteLook, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B3
    // "Hey, thanks!\nHere's what I'm wondering about.[f000]븁\u0000\nUsually, Ground-type moves can't hit\na Pokémon with the Levitate Ability.[f000]븁\u0000\nBut I'm sure there's some way to do it.\nI just can't for the life of me[f000]븀\u0000\nfigure out how...[f000]븁\u0000\nI have several ideas, but\nI can't decide which one to try.[f000]븁\u0000\nThat's why I want to get\nyour opinion on the matter!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_HeyThanksHeresWhat, 0, 2, 0
    MsgWaitAdvance
    // "First, I'd like some advice about items.[f000]븁\u0000\nI want to use Trick or Switcheroo\nto give an item to a Pokémon that[f000]븀\u0000\nhas the Levitate Ability so I can[f000]븀\u0000\nhit it with Ground-type moves![f000]븁\u0000\nAfter considering which item to give,\nI thought the Iron Ball might work.[f000]븀\u0000\nI'm not sure, though...[f000]븁\u0000\nWhat do you think?\nIs the Iron Ball a good idea?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_FirstIdLikeSome, 0, 2, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32800
    ListMenuAdd 4, 65535, 0
    ListMenuAdd 5, 65535, 1
    ListMenuAdd 6, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02A7
    // "Oh! I knew it![f000]븁\u0000\nHearing you say it makes\nme feel much more confident![f000]븁\u0000\nPassing the Iron Ball to a\nPokémon with Levitate[f000]븀\u0000\nmeans Ground-type moves can hit it![f000]븁\u0000\nThanks!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_OhKnewHearingSay, 0, 2, 0
    MsgWaitAdvance
    WorkSetConst EVENT_WORK_0x40f9, 1
    VMJump L_02AD

L_02A7:
    WorkSetConst 0x8021, 1

L_02AD:
    VMJump L_02B9

L_02B3:
    WorkSetConst 0x8021, 1

L_02B9:
    VMJump L_0495

L_02BF:
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03A4
    // "I'd like to get your advice on moves.[f000]븁\u0000\nIf I use a certain move on a Pokémon\nwith the Levitate Ability, and then[f000]븀\u0000\nuse a Ground-type move on it,[f000]븀\u0000\nI think that could work.[f000]븁\u0000\nWhich move should I choose:\nSmack Down, Gravity, or Gastro Acid?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_IdLikeGetAdvice, 0, 2, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32800
    ListMenuAdd 10, 65535, 0
    ListMenuAdd 11, 65535, 1
    ListMenuAdd 12, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033E
    // "I see! Smack Down![f000]븁\u0000\nThat move will knock a Flying-type\nPokémon or a Pokémon with Levitate[f000]븀\u0000\nout of the air, so Ground-type moves[f000]븀\u0000\nwill work on it![f000]븁\u0000\nSo if I use Smack Down and then use\na Ground-type move...[f000]븁\u0000\nI get it now!\nThanks!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_SeeSmackDownMove, 0, 2, 0
    MsgWaitAdvance
    WorkSetConst EVENT_WORK_0x40f9, 2
    VMJump L_039E

L_033E:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_036B
    // "I see! Gravity![f000]븁\u0000\nThe move Gravity makes gravity stronger,\nso Ground-type moves will work on[f000]븀\u0000\nPokémon with the Levitate Ability or[f000]븀\u0000\nFlying-type Pokémon.[f000]븁\u0000\nSo if I use Gravity and then use\na Ground-type move...[f000]븁\u0000\nI get it now!\nThanks!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_SeeGravityMoveGravity, 0, 2, 0
    MsgWaitAdvance
    WorkSetConst EVENT_WORK_0x40f9, 2
    VMJump L_039E

L_036B:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0398
    // "I see! Gastro Acid![f000]븁\u0000\nThat's right--Gastro Acid eliminates the\neffects of Abilities.[f000]븁\u0000\nThat means Levitate won't work anymore![f000]븁\u0000\nSo if I use Gastro Acid and then use\na Ground-type move...[f000]븁\u0000\nI get it now!\nThanks!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_SeeGastroAcidThats, 0, 2, 0
    MsgWaitAdvance
    WorkSetConst EVENT_WORK_0x40f9, 2
    VMJump L_039E

L_0398:
    WorkSetConst 0x8021, 1

L_039E:
    VMJump L_0495

L_03A4:
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0495
    // "Finally, I'd like a little advice\nabout Abilities.[f000]븁\u0000\nApparently, some Abilities will let you\nhit Pokémon that have Levitate[f000]븀\u0000\nwith Ground-type moves![f000]븁\u0000\nBut...\nI'm not sure which one to use![f000]븁\u0000\nMold Breaker, Teravolt, and Turboblaze\nall seem like good choices...[f000]븁\u0000\nI'm close, but I can't decide.[f000]븁\u0000\nHey! Please decide for me!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_FinallyIdLikeLittle, 0, 2, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32800
    ListMenuAdd 17, 65535, 0
    ListMenuAdd 18, 65535, 1
    ListMenuAdd 19, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0429
    // "So it's Mold Breaker, then?[f000]븁\u0000\nI guess that's right.\nMold Breaker lets you use moves[f000]븀\u0000\nregardless of the target's Ability![f000]븁\u0000\nLevitate won't matter, so you can land\nhit after hit with Ground-type moves![f000]븁\u0000\nMold Breaker's so cool!\nViva Mold Breaker!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ItsMoldBreakerThen, 0, 2, 0
    MsgWaitAdvance
    WorkSetConst EVENT_WORK_0x40f9, 3
    WorkSetConst 0x8021, 1
    VMJump L_0495

L_0429:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_045C
    // "So it's Teravolt, then?![f000]븁\u0000\nI guess that's right.\nTeravolt lets you use moves[f000]븀\u0000\nregardless of the target's Ability![f000]븁\u0000\nLevitate won't matter, so you can land\nhit after hit with Ground-type moves![f000]븁\u0000\nTeravolt's so cool!\nViva Teravolt!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ItsTeravoltThenGuess, 0, 2, 0
    MsgWaitAdvance
    WorkSetConst EVENT_WORK_0x40f9, 3
    WorkSetConst 0x8021, 1
    VMJump L_0495

L_045C:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_048F
    // "So it's Turboblaze, then?![f000]븁\u0000\nI guess that's right.\nTurboblaze lets you use moves[f000]븀\u0000\nregardless of the target's Ability![f000]븁\u0000\nLevitate won't matter, so you can land\nhit after hit with Ground-type moves![f000]븁\u0000\nTurboblaze is so cool!\nViva Turboblaze!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ItsTurboblazeThenGuess, 0, 2, 0
    MsgWaitAdvance
    WorkSetConst EVENT_WORK_0x40f9, 3
    WorkSetConst 0x8021, 1
    VMJump L_0495

L_048F:
    WorkSetConst 0x8021, 1

L_0495:
    VMJump L_0172

L_049B:
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0521
    // "Thanks! Because of you,\nI'm prepared for Levitate![f000]븁\u0000\nAnd I'm even more prepared\nfor anti-Levitate tactics![f000]븁\u0000\nHeh heh heh...[f000]븁\u0000\nNow, no matter which Pokémon I face,\nvictory is assured for my[f000]븀\u0000\nlevitating Pokémon!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ThanksBecauseImPrepared, 0, 2, 0
    MsgWaitAdvance
    // "Well! Do you dare battle my\ninvincible Pokémon with Levitate?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_WellDareBattleInvincible, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050B
    MsgWinCloseAll
    CallTrainerBattle TRAINER_ACE_TRAINER_DUSTY, 0, 0
    VMCall L_05D8
    WorkSetConst EVENT_WORK_0x40f9, 4
    // "Hmm...[f000]븁\u0000\nI thought I was completely\nprepared for anti-Levitate tactics...[f000]븁\u0000\nIn real battles, things never\ngo as well as you would think.[f000]븁\u0000\nOr maybe you're just that good!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_HmmThoughtCompletelyPrepared, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_051B

L_050B:
    // "If you want to battle, come talk\nto me again! I'll take you on anytime!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_IfWantBattleCome, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_051B:
    VMJump L_0531

L_0521:
    // "Oh... Well, can't be helped.[f000]븁\u0000\nIf you change your mind,\nI'd still like to pick your brain..."
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_OhWellCantHelped, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0531:
    VMJump L_05D2

L_0537:
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05AF
    // "Well! Do you dare battle my\ninvincible Pokémon with Levitate?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_WellDareBattleInvincible, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0599
    MsgWinCloseAll
    CallTrainerBattle TRAINER_ACE_TRAINER_DUSTY, 0, 0
    VMCall L_05D8
    WorkSetConst EVENT_WORK_0x40f9, 4
    // "Hmm...[f000]븁\u0000\nI thought I was completely\nprepared for anti-Levitate tactics...[f000]븁\u0000\nIn real battles, things never\ngo as well as you would think.[f000]븁\u0000\nOr maybe you're just that good!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_HmmThoughtCompletelyPrepared, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_05A9

L_0599:
    // "If you want to battle, come talk\nto me again! I'll take you on anytime!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_IfWantBattleCome, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_05A9:
    VMJump L_05D2

L_05AF:
    VMStackPush EVENT_WORK_0x40f9
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05D2
    // "Hmm...[f000]븁\u0000\nI thought I was completely\nprepared for anti-Levitate tactics...[f000]븁\u0000\nIn real battles, things never\ngo as well as you would think.[f000]븁\u0000\nOr maybe you're just that good!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_HmmThoughtCompletelyPrepared, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_05D2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_05D8:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05F7
    CallTrainerBattleEnd
    VMJump L_05F9

L_05F7:
    CallTrainerLose

L_05F9:
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We increased the number of Trainers'\nSchools to meet the demands of[f000]븀\u0000\nTrainers who want to know[f000]븀\u0000\nmore about Pokémon because they[f000]븀\u0000\nlove them!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_WeIncreasedNumberTrainers, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon have mysterious powers\ncalled Abilities.[f000]븁\u0000\nSome work even when the Pokémon\nis not in battle.[f000]븁\u0000\nFor example, when a Pokémon with the\nSuction Cups or Sticky Hold Ability is at[f000]븀\u0000\nthe front of your party, you're more[f000]븀\u0000\nlikely to get a bite when fishing!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_PokemonHaveMysteriousPowers, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Wide Lens and Zoom Lens are very\nsimilar. Here's the difference.[f000]븁\u0000\nThe Wide Lens boosts accuracy by 10%![f000]븁\u0000\nThe Zoom Lens boosts accuracy by 20%,\nbut only if the holder moves after[f000]븀\u0000\nthe target.[f000]븁\u0000\nThat means the Zoom Lens is best for\nslower Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_WideLensZoomLens, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0163
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0864
    // "Would you like to take a simple quiz?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_WouldLikeTakeSimple, 4, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_084E
    // "So here is the question:[f000]븁\u0000\nWhich of the following Berries\ncures confusion?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_HereQuestionWhichFollowing, 4, 2, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32800
    ListMenuAdd 32, 65535, 0
    ListMenuAdd 33, 65535, 1
    ListMenuAdd 34, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06E7
    // "Ding ding ding![f000]븁\u0000\nRight answer!\nNow for the next question.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_DingDingDingRight, 4, 2, 0
    WorkSetConst 0x8022, 1
    VMJump L_0749

L_06E7:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0710
    // "Buzzzzz![f000]븁\u0000\nToo bad... That's incorrect.\nPlease try again sometime!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_BuzzzzzTooBadThats, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0749

L_0710:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0739
    // "Buzzzzz![f000]븁\u0000\nToo bad... That's incorrect.\nPlease try again sometime!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_BuzzzzzTooBadThats, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0749

L_0739:
    // "Come talk to me if you want\nto take a quiz."
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ComeTalkIfWant, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0749:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0848
    // "OK! The move Swagger raises the target's\nAttack, but it also causes which[f000]븀\u0000\nstatus condition?"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_OkMoveSwaggerRaises, 4, 2, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32800
    ListMenuAdd 37, 65535, 0
    ListMenuAdd 38, 65535, 1
    ListMenuAdd 39, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07B4
    // "Buzzzzz![f000]븁\u0000\nToo bad... That's incorrect.\nPlease try again sometime!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_BuzzzzzTooBadThats, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0848

L_07B4:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07DD
    // "Buzzzzz![f000]븁\u0000\nToo bad... That's incorrect.\nPlease try again sometime!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_BuzzzzzTooBadThats, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0848

L_07DD:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0838
    // "Ding ding ding! We have a winner!\nFantastic answer![f000]븁\u0000\nHere's a prize for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_DingDingDingWe, 4, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 156
    WorkSet 0x8001, 3
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0163
    // "Here's a combo you can use in a\nDouble Battle![f000]븁\u0000\nHave one of your Pokémon hold a\nPersim Berry.[f000]븁\u0000\nThen, have the other one use Swagger\non that Pokémon.[f000]븁\u0000\nIts Attack will go up, and the Berry will\nheal the confusion immediately.[f000]븀\u0000\nThen it can hit the opponent harder![f000]븁\u0000\nThere are many possible combinations\nin Double Battle!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_HeresComboCanUse, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0848

L_0838:
    // "Come talk to me if you want\nto take a quiz."
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ComeTalkIfWant, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0848:
    VMJump L_085E

L_084E:
    // "Come talk to me if you want\nto take a quiz."
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_ComeTalkIfWant, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_085E:
    VMJump L_0874

L_0864:
    // "Here's a combo you can use in a\nDouble Battle![f000]븁\u0000\nHave one of your Pokémon hold a\nPersim Berry.[f000]븁\u0000\nThen, have the other one use Swagger\non that Pokémon.[f000]븁\u0000\nIts Attack will go up, and the Berry will\nheal the confusion immediately.[f000]븀\u0000\nThen it can hit the opponent harder![f000]븁\u0000\nThere are many possible combinations\nin Double Battle!"
    ActorMsg MSGFILE_SCRIPT, StriatonCity9_Text_HeresComboCanUse, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0874:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
