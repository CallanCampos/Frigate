#include "asm/field_script.inc"
#include "text/script/global_2280.h"

// Script plugin 16, from the zones that start its scripts

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_1:
    VMCall L_004C
    RTEndGlobal

L_004C:
    // "Drayden: If you wish, I can teach your\nPokémon the strongest[f000]븀\u0000\nDragon-type move.[f000]븁\u0000\nWould you like your Pokémon to learn\nthat move?"
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_DraydenIfWishCan, 0x8011, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0081
    // "Understood.\nCome again if you change your mind."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_UnderstoodComeAgainIf, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_0081:
    MoveTutorCheckParty 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B2
    // "I see...[f000]븁\u0000\nBut there are no Pokémon with\nyou right now that can learn this move."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_SeeButThereNo, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_00D7

L_00B2:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D7
    // "Hmm...[f000]븁\u0000\nThis Pokémon can learn this move, but\nyour bond of friendship is not yet[f000]븀\u0000\nstrong enough.[f000]븁\u0000\nThat means I cannot teach\nit this move."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_HmmPokemonCanLearn, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_00D7:
    // "Which Pokémon should\nlearn this move?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_WhichPokemonShouldLearn, 0x8011, 0, 0
    ActorMsgClose
    MoveTutorCallPokeSelect 0, 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0112
    // "Understood.\nCome again if you change your mind."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_UnderstoodComeAgainIf, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_0112:
    PokePartyIsEgg 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013D
    // "That's an interesting request, but...\nNot even I can teach a move to an Egg."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_ThatsInterestingRequestBut, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_013D:
    MoveTutorCheckPkm 0, 0x8020, 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0158
    VMJump L_0170

L_0158:
    // "Hmm...[f000]븁\u0000\nThis Pokémon can learn this move, but\nyour bond of friendship is not yet[f000]븀\u0000\nstrong enough.[f000]븁\u0000\nThat means I cannot teach\nit this move."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_HmmPokemonCanLearn, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_01CB

L_0170:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0183
    VMJump L_019B

L_0183:
    // "I'm very sorry, but...[f000]븁\u0000\nThis Pokémon cannot learn this move."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_ImVerySorryBut, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_01CB

L_019B:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_01AE
    VMJump L_01CB

L_01AE:
    WordSetMoveName 0, MOVE_DRACO_METEOR
    // "Hmm... It looks like this Pokémon\nalready knows [f000]ć\u0001\u0000."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_HmmLooksLikePokemon, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_01CB

L_01CB:
    WorkSetConst 0x8021, 434
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 1
    VMCall L_098A
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0206
    // "Drayden: Draco Meteor's power is\nexceptional, but using it lowers[f000]븀\u0000\nthe user's Sp. Atk stat."
    ActorMsg MSGFILE_SCRIPT, Global2280_Text_DraydenDracoMeteorsPower, 0x8011, 0, 0
    LastKeyWait
    ActorMsgClose

L_0206:
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_021C
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_021C:
    // "Your quest for power has brought\nyou here...[f000]븁\u0000\nVery well.[f000]븁\u0000\nLet me reward your passion with some\nabsolutely astounding moves.[f000]븁\u0000\nThey're the ultimate moves![f000]븁\u0000\nShall I teach them to\nyour Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_QuestPowerHasBrought, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024D
    // "Hmm... Well, that is fine, as well.\nVisit if you have a change of heart."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_HmmWellFineWell, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_024D:
    MoveTutorCheckParty 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027C
    // "The Pokémon with you can't\nlearn these moves![f000]븁\u0000\nGo talk to my wife! Keh!!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_PokemonCantLearnThese, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_029F

L_027C:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029F
    // "The bond with your Pokémon\nis not strong enough! Keh!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_BondPokemonNotStrong, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_029F:
    // "Well, which Pokémon should I teach?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WellWhichPokemonShould, 0, 0
    ActorMsgClose
    MoveTutorCallPokeSelect 1, 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D6
    // "Hmm... Well, that is fine, as well.\nVisit if you have a change of heart."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_HmmWellFineWell, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_02D6:
    PokePartyIsEgg 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02FF
    // "Teach a move to an Egg?\nImpossible, even for me! Keh!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_TeachMoveEggImpossible, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_02FF:
    MoveTutorCheckPkm 1, 0x8020, 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_031A
    VMJump L_0330

L_031A:
    // "The bond with your Pokémon\nis not strong enough! Keh!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_BondPokemonNotStrong, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_0382

L_0330:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0343
    VMJump L_0359

L_0343:
    // "That Pokémon can't learn a move!\nGo talk to my wife! Keh!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_PokemonCantLearnMove, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_0382

L_0359:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_036C
    VMJump L_0382

L_036C:
    // "That Pokémon already knows\nthat move! Keh!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_PokemonAlreadyKnowsMove, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_0382

L_0382:
    MoveTutorGetMoveID 1, 0x8020, 0x8021
    WordSetPartyPokeName 0, 0x8020
    WordSetMoveName 1, 0x8021
    // "I should teach [f000]ć\u0001\u0001 to\n[f000]ā\u0001\u0000, right? Keh!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ShouldTeachRightKeh, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C5
    // "Hmm... Well, that is fine, as well.\nVisit if you have a change of heart."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_HmmWellFineWell, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_03C5:
    ActorMsgClose
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    VMCall L_098A
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03EC

L_03EC:
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0402
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0402:
    VMStackPushFlag EVENT_FLAG_0x00fd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0429
    // "Heya![f000]븁\u0000\nThe professor gave you a Pokémon to\nstart your journey, right?[f000]븁\u0000\nIf that Pokémon is still with you, and if\nyou two have a strong bond, there's a[f000]븀\u0000\nspecial move it can learn...[f000]븁\u0000\nWant me to teach it a battle-combo move?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_HeyaProfessorGavePokemon, 0, 0
    FlagSet EVENT_FLAG_0x00fd
    VMJump L_0433

L_0429:
    // "A special move...\nShould I teach it a battle-combo move?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_SpecialMoveShouldTeach, 0, 0

L_0433:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_045A
    // "Is that so...\nWell, come on back now, y'hear?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WellComeBackNow, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_045A:
    MoveTutorCheckParty 2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0489
    // "I can't teach these Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_CantTeachThesePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_04AC

L_0489:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04AC
    // "This Pokémon... Well, it doesn't look\nlike it's too comfortable with you yet."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_PokemonWellDoesntLook, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_04AC:
    // "Which Pokémon should learn the move?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WhichPokemonShouldLearn_2, 0, 0
    ActorMsgClose
    MoveTutorCallPokeSelect 2, 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E3
    // "Is that so...\nWell, come on back now, y'hear?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WellComeBackNow, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_04E3:
    PokePartyIsEgg 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050C
    // "Teach a move to an Egg?\nWhat kinda crazy talk is that?!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_TeachMoveEggWhat, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_050C:
    MoveTutorCheckPkm 2, 0x8020, 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0527
    VMJump L_053D

L_0527:
    // "This Pokémon... Well, it doesn't look\nlike it's too comfortable with you yet."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_PokemonWellDoesntLook, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_058F

L_053D:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0550
    VMJump L_0566

L_0550:
    // "Now, that Pokémon can't learn this\nspecial move![f000]븁\u0000\nI told ya, it's only for the Pokémon who\nstarted your journey with you."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_NowPokemonCantLearn, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_058F

L_0566:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0579
    VMJump L_058F

L_0579:
    // "I can't teach it twice. This Pokémon\nalready knows the special move!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_CantTeachTwicePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    VMJump L_058F

L_058F:
    MoveTutorGetMoveID 2, 0x8020, 0x8021
    WordSetPartyPokeName 0, 0x8020
    WordSetMoveName 1, 0x8021
    // "Want me to teach [f000]ć\u0001\u0001\nto [f000]ā\u0001\u0000?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WantTeach, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05D2
    // "Is that so...\nWell, come on back now, y'hear?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WellComeBackNow, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_05D2:
    ActorMsgClose
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    VMCall L_098A
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05F9

L_05F9:
    VMReturn

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 72
    VMStackPushFlag EVENT_FLAG_0x0145
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0638
    WordSetItemNameEx 0, 0x8024, 2, 0
    // "I'm the master Move Tutor![f000]븁\u0000\nAnd I'm also obsessed with\n[f000]ĉ\u0001\u0000![f000]븁\u0000\nGive me [f000]ĉ\u0001\u0000,\nand I'll thank you by teaching[f000]븀\u0000\nyour Pokémon a move![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor, 0, 0
    FlagSet EVENT_FLAG_0x0145
    VMJump L_0642

L_0638:
    // "I'm the master Move Tutor![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor_2, 0, 0

L_0642:
    VMCall L_0747
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 73
    VMStackPushFlag EVENT_FLAG_0x0146
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_068B
    WordSetItemNameEx 0, 0x8024, 2, 0
    // "I'm the master Move Tutor![f000]븁\u0000\nAnd I'm also obsessed with\n[f000]ĉ\u0001\u0000![f000]븁\u0000\nGive me [f000]ĉ\u0001\u0000,\nand I'll thank you by teaching[f000]븀\u0000\nyour Pokémon a move![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor, 0, 0
    FlagSet EVENT_FLAG_0x0146
    VMJump L_0695

L_068B:
    // "I'm the master Move Tutor![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor_2, 0, 0

L_0695:
    VMCall L_0747
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 74
    VMStackPushFlag EVENT_FLAG_0x0147
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06DE
    WordSetItemNameEx 0, 0x8024, 2, 0
    // "I'm the master Move Tutor![f000]븁\u0000\nAnd I'm also obsessed with\n[f000]ĉ\u0001\u0000![f000]븁\u0000\nGive me [f000]ĉ\u0001\u0000,\nand I'll thank you by teaching[f000]븀\u0000\nyour Pokémon a move![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor, 0, 0
    FlagSet EVENT_FLAG_0x0147
    VMJump L_06E8

L_06DE:
    // "I'm the master Move Tutor![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor_2, 0, 0

L_06E8:
    VMCall L_0747
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 75
    VMStackPushFlag EVENT_FLAG_0x0148
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0731
    WordSetItemNameEx 0, 0x8024, 2, 0
    // "I'm the master Move Tutor![f000]븁\u0000\nAnd I'm also obsessed with\n[f000]ĉ\u0001\u0000![f000]븁\u0000\nGive me [f000]ĉ\u0001\u0000,\nand I'll thank you by teaching[f000]븀\u0000\nyour Pokémon a move![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor, 0, 0
    FlagSet EVENT_FLAG_0x0148
    VMJump L_073B

L_0731:
    // "I'm the master Move Tutor![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_ImMasterMoveTutor_2, 0, 0

L_073B:
    VMCall L_0747
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0747:
    // "Want me to teach your\nPokémon a move?"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WantTeachPokemonMove, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0780
    WordSetItemNameEx 0, 0x8024, 2, 0
    // "If you collect some [f000]ĉ\u0001\u0000,\nyou come back now."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_IfCollectSomeCome, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_0780:
    // "Well, that's just fine![f000]븁\u0000\nPick the move you want\nme to teach![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_WellThatsJustFine, 0, 0
    WorkSetConst EVENT_WORK_0x400c, 0
    WorkSetConst EVENT_WORK_0x400d, 0
    WorkSetConst EVENT_WORK_0x400e, 0
    WorkSetConst EVENT_WORK_0x400f, 0
    WorkCmpConst 0x8024, 72
    VMJumpIf CMP_EQ, L_07B5
    VMJump L_07DB

L_07B5:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 246
    WorkSet 0x8001, 0
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0886

L_07DB:
    WorkCmpConst 0x8024, 73
    VMJumpIf CMP_EQ, L_07EE
    VMJump L_0814

L_07EE:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 245
    WorkSet 0x8001, 0
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0886

L_0814:
    WorkCmpConst 0x8024, 74
    VMJumpIf CMP_EQ, L_0827
    VMJump L_084D

L_0827:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 244
    WorkSet 0x8001, 0
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0886

L_084D:
    WorkCmpConst 0x8024, 75
    VMJumpIf CMP_EQ, L_0860
    VMJump L_0886

L_0860:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 243
    WorkSet 0x8001, 0
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0886

L_0886:
    DebugPrint EVENT_WORK_0x400c
    DebugPrint EVENT_WORK_0x400d
    DebugPrint EVENT_WORK_0x400e
    DebugPrint EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08C1
    WordSetItemNameEx 0, 0x8024, 2, 0
    // "If you collect some [f000]ĉ\u0001\u0000,\nyou come back now."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_IfCollectSomeCome, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_08C1:
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08E4
    // "This Pokémon can't learn\nthat move!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_PokemonCantLearnMove_2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_08E4:
    PokePartyHasMove 0x8010, EVENT_WORK_0x400d, EVENT_WORK_0x400e
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_090F
    // "This Pokémon already knows that move!"
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_PokemonAlreadyKnowsMove_2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_090F:
    WorkGet 0x8021, EVENT_WORK_0x400d
    WorkGet 0x8020, EVENT_WORK_0x400e
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 1
    VMCall L_098A
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0956
    WordSetItemNameEx 0, 0x8024, 2, 0
    // "If you collect some [f000]ĉ\u0001\u0000,\nyou come back now."
    ParentActorMsg MSGFILE_SCRIPT, Global2280_Text_IfCollectSomeCome, 0, 0
    LastKeyWait
    ActorMsgClose

L_0956:
    VMReturn

Script_8:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "The Pokémon I've taken a shine to are\nthe ones you get from the professor![f000]븁\u0000\nTo teach all of these Pokémon a\nspecial move, a battle-combo move...[f000]븀\u0000\nThat's my pledge!"
    MsgPlaceSign Global2280_Text_PokemonIveTakenShine, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

L_098A:
    WorkSetConst 0x8028, 0
    PokePartyGetMoveCount 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0A39
    WordSetPartyPokeName 0, 0x8020
    WordSetMoveName 1, 0x8021
    MEPlay SEQ_ME_LVUP
    // "[f000]Ă\u0001\u0000 learned\n[f000]ć\u0001\u0001!"
    SystemMsg Global2280_Text_Learned, 0
    MEWait
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09DA
    LastKeyWait
    VMJump L_09DC

L_09DA:
    MsgWaitAdvance

L_09DC:
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0A2D
    WorkSetConst 0x8029, 0
    ItemSub 0x8024, EVENT_WORK_0x400f, 0x8029
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8024, EVENT_WORK_0x400f, 0
    WordSetNumber 2, EVENT_WORK_0x400f, 2
    // "[f000]Ā\u0001\u0000 handed over\n[f000]ȁ\u0001\u0002 [f000]ĉ\u0001\u0001 in exchange."
    SystemMsg Global2280_Text_HandedOverExchange, 0
    LastKeyWait
    WorkSetConst 0x8029, 0

L_0A2D:
    InfoMsgClose
    PokePartyLearnMove 0x8020, 0x8010, 0x8021
    VMReturn

L_0A39:
    WorkSetConst 0x8027, 1

L_0A3F:
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AA8
    VMCall L_0B0B
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A7D
    WorkSetConst 0x8010, 1
    WorkSetConst 0x8027, 0
    VMJump L_0AA2

L_0A7D:
    VMCall L_0AAA
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AA2
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8027, 0

L_0AA2:
    VMJump L_0A3F

L_0AA8:
    VMReturn

L_0AAA:
    WordSetMoveName 0, 0x8021
    // "Give up on learning the\nmove [f000]ć\u0001\u0000?"
    SystemMsg Global2280_Text_GiveUpLearningMove, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B03
    WordSetPartyPokeName 0, 0x8020
    WordSetMoveName 1, 0x8021
    // "[f000]Ă\u0001\u0000 did not learn [f000]ć\u0001\u0001!"
    SystemMsg Global2280_Text_DidNotLearn, 0
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AF7
    LastKeyWait
    VMJump L_0AF9

L_0AF7:
    MsgWaitAdvance

L_0AF9:
    InfoMsgClose
    WorkSetConst 0x8028, 1
    VMReturn

L_0B03:
    WorkSetConst 0x8028, 0
    VMReturn

L_0B0B:
    WorkSetConst 0x8028, 0
    WordSetPartyPokeName 0, 0x8020
    WordSetMoveName 1, 0x8021
    // "[f000]Ă\u0001\u0000 is trying to\nlearn [f000]ć\u0001\u0001![f000]븁\u0000\nBut [f000]Ă\u0001\u0000 can't learn more than\nfour moves.[f000]븁\u0000\nDelete a move to make room\nfor [f000]ć\u0001\u0001?"
    SystemMsg Global2280_Text_TryingLearnButCant, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B3A
    VMReturn

L_0B3A:
    InfoMsgClose
    CallPokeMoveReplace 0x8010, 0x8025, 0x8020, 0x8021
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B5B
    VMReturn

L_0B5B:
    PokePartyGetMove 0x8026, 0x8020, 0x8025
    WordSetMoveName 0, 0x8026
    // "Is it OK to forget the\nmove [f000]ć\u0001\u0000?"
    SystemMsg Global2280_Text_OkForgetMove, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B87
    VMReturn

L_0B87:
    PokePartyGetMove 0x8026, 0x8020, 0x8025
    WordSetPartyPokeName 0, 0x8020
    WordSetMoveName 1, 0x8026
    WordSetMoveName 2, 0x8021
    // "1, [f000]븂\u0001\u00142, and[f000]븂\u0001\u0014... [f000]븂\u0001\u0014... [f000]븂\u0001\u0014... Ta-da![f000]븅\u0001\u0003[f000]븅\u0001\u0006[f000]븁\u0000\n[f000]Ă\u0001\u0000 forgot how to\nuse [f000]ć\u0001\u0001.[f000]븁\u0000\nAnd...[f000]븁\u0000"
    SystemMsg Global2280_Text_12TaDa, 0
    // "[f000]Ă\u0001\u0000 learned [f000]ć\u0001\u0002!"
    SystemMsg Global2280_Text_Learned_2, 0
    MEPlay SEQ_ME_LVUP
    MEWait
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BCB
    LastKeyWait
    VMJump L_0BCD

L_0BCB:
    MsgWaitAdvance

L_0BCD:
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0C1E
    WorkSetConst 0x802a, 0
    ItemSub 0x8024, EVENT_WORK_0x400f, 0x802a
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8024, EVENT_WORK_0x400f, 0
    WordSetNumber 2, EVENT_WORK_0x400f, 2
    // "[f000]Ā\u0001\u0000 handed over\n[f000]ȁ\u0001\u0002 [f000]ĉ\u0001\u0001 in exchange."
    SystemMsg Global2280_Text_HandedOverExchange, 0
    LastKeyWait
    WorkSetConst 0x802a, 0

L_0C1E:
    InfoMsgClose
    PokePartyLearnMove 0x8020, 0x8025, 0x8021
    WorkSetConst 0x8028, 1
    VMReturn
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0

Script_9:
    WorkGet 0x8020, 0x8000
    WorkGet 0x8021, 0x8001
    WorkSetConst 0x8022, 1
    WorkSetConst 0x8023, 1
    VMCall L_098A
    RTEndGlobal
    VMHalt
    .balign 4, 0
