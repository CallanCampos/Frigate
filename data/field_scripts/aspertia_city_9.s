#include "asm/field_script.inc"
#include "text/script/aspertia_city_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_4:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 58
    WorkSet 0x8001, 1
    WorkSet 0x8002, 269
    WorkSet 0x8003, 1
    WorkSet 0x8004, 2
    WorkSet 0x8005, 2
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
    // "I asked Alder from Floccesy Town\nto teach here.[f000]븁\u0000\nHe declined, saying it was the\nera of young people now."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity9_Text_AskedAlderFromFloccesy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There's a technique that enables you\nto cancel evolution![f000]븁\u0000\nHere, I'll read the textbook to you.[f000]븁\u0000\n“You can surprise a Pokémon and stop\nits evolution by pressing the B Button[f000]븀\u0000\nwhen a Pokémon is evolving.\""
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity9_Text_TheresTechniqueEnablesCancel, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    TrainerCardHasBadge 0x8008, 0
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0108
    // "Aspertia Pokémon Gym[f000]븁\u0000\nGym Leader: Cheren\nCertified Trainers:"
    InfoMsg AspertiaCity9_Text_AspertiaPokemonGymGym, 2
    VMJump L_012B

L_0108:
    VMStackPush EVENT_WORK_0x40ab
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0126
    // "Aspertia Pokémon Gym[f000]븁\u0000\nGym Leader: Cheren\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg AspertiaCity9_Text_AspertiaPokemonGymGym_3, 2
    VMJump L_012B

L_0126:
    // "Aspertia Pokémon Gym[f000]븁\u0000\nGym Leader: Cheren\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000"
    InfoMsg AspertiaCity9_Text_AspertiaPokemonGymGym_2, 2

L_012B:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You know how you can have\nyour Pokémon hold items?[f000]븁\u0000\nWell, it seems like they don't know\nhow to use items made by people,[f000]븀\u0000\nlike Potions."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity9_Text_KnowHowCanHave, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Normal-type Pokémon are weak\nto Fighting-type Pokémon.[f000]븁\u0000\nBut the only Pokémon around here like\nthat are the Riolu in Floccesy Ranch...[f000]븁\u0000\nIf you're going to battle with a Fire-,\nWater-, or Grass-type Pokémon,[f000]븀\u0000\nit'll be a simple test of strength!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity9_Text_NormalTypePokemonWeak, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Gym Leader is in the middle\nof a heated battle right now!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity9_Text_GymLeaderMiddleHeated, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8023, 0
    SEPlay SEQ_SE_MESSAGE
    // "The blackboard explains Pokémon\nstatus changes in battle.[f000]븁\u0000"
    SystemMsg AspertiaCity9_Text_BlackboardExplainsPokemonStatus, 2

L_019B:
    VMStackPush 0x8023
    VMStackPushConst 5
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0296
    // "What do you want to read about?"
    SystemMsg AspertiaCity9_Text_WhatWantReadAbout, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 13, 65535, 0
    ListMenuAdd 14, 65535, 1
    ListMenuAdd 15, 65535, 2
    ListMenuAdd 16, 65535, 3
    ListMenuAdd 17, 65535, 4
    ListMenuAdd 18, 65535, 5
    ListMenuShow
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020E
    // "If poisoned, a Pokémon steadily loses HP\nwhen battling.[f000]븁\u0000\nThe poison lingers after the battle.\nTo cure it, use an Antidote.[f000]븁\u0000"
    SystemMsg AspertiaCity9_Text_IfPoisonedPokemonSteadily, 2
    VMJump L_0290

L_020E:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_022D
    // "Paralysis reduces the Speed stat\nand may prevent movement.[f000]븁\u0000\nIt remains after battle, so use a\nParlyz Heal.[f000]븁\u0000"
    SystemMsg AspertiaCity9_Text_ParalysisReducesSpeedStat, 2
    VMJump L_0290

L_022D:
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024C
    // "If a Pokémon falls asleep, it will be\nunable to attack.[f000]븁\u0000\nThe Pokémon may wake up on its own,\nbut if a battle ends while it is[f000]븀\u0000\nsleeping, it will stay asleep.[f000]븁\u0000\nWake it up using an Awakening.[f000]븁\u0000"
    SystemMsg AspertiaCity9_Text_IfPokemonFallsAsleep, 2
    VMJump L_0290

L_024C:
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026B
    // "A burn reduces the Attack stat and\nsteadily reduces the victim's HP.[f000]븁\u0000\nA burn lingers after battle.\nCure a burn using a Burn Heal.[f000]븁\u0000"
    SystemMsg AspertiaCity9_Text_BurnReducesAttackStat, 2
    VMJump L_0290

L_026B:
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028A
    // "If a Pokémon is frozen, it becomes\ncompletely helpless.[f000]븁\u0000\nThe Pokémon may thaw out on its own,\nbut if a battle ends while it is[f000]븀\u0000\nfrozen, it will stay frozen.[f000]븁\u0000\nThaw it out using an Ice Heal.[f000]븁\u0000"
    SystemMsg AspertiaCity9_Text_IfPokemonFrozenBecomes, 2
    VMJump L_0290

L_028A:
    WorkSetConst 0x8023, 5

L_0290:
    VMJump L_019B

L_0296:
    InfoMsgClose
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
