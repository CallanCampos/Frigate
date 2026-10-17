#include "asm/field_script.inc"
#include "text/script/global_10635.h"

// Script plugin 13, from the zones that start its scripts

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
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

Script_1:
    VMCall L_0075
    VMHalt

Script_2:
    FieldGetContinueFlag 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0073
    VMCall L_0075

L_0073:
    VMHalt

L_0075:
    PokemonCenterCmd_Medal 1000
    VMCall L_00E2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BF
    FlagReset EVENT_FLAG_0x02ea
    MedalGetGuruActor 0x8010, 0x8024
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B3
    ActorAdd 0x8024

L_00B3:
    VMCall L_0151
    VMJump L_00E0

L_00BF:
    MedalGetGuruActor 0x8010, 0x8024
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DC
    ActorDelete 0x8024

L_00DC:
    FlagSet EVENT_FLAG_0x02ea

L_00E0:
    VMReturn

L_00E2:
    ItemCheckAmount ITEM_MEDAL_BOX, 1, 0x8010
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0109
    WorkSetConst 0x8010, 0
    VMReturn

L_0109:
    MedalGetCount 2, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0129
    WorkSetConst 0x8010, 1
    VMReturn

L_0129:
    MedalGetCount 0, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0149
    WorkSetConst 0x8010, 1
    VMReturn

L_0149:
    VMCall L_06BB
    VMReturn

L_0151:
    ActorGetGPos 0x8024, 0x8025, 0x8026
    PlayerGetGPos 0x8027, 0x8028
    VMStackPush 0x8027
    VMStackPush 0x8025
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPush 0x8026
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0194
    WorkAdd 0x8025, 1
    ActorSetGPos 0x8024, 0x8025, 0, 0x8026, 1

L_0194:
    VMReturn

Script_3:
    ActorsPauseAll
    ActorCmdExec 255, Movement_07A0
    ActorCmdWait
    ActorCmdExec 1, Movement_0348
    ActorCmdWait
    ActorWalkRoute 1, 107, 664, 1, 8, 1
    ActorCmdWait
    VMCall L_0299
    ActorCmdExec 1, Movement_0354
    ActorCmdWait
    ActorDelete 1
    FlagSet EVENT_FLAG_0x02da
    FlagReset EVENT_FLAG_0x02db
    ActorAdd 0
    BMCreateHandleByGPos 0x8029, 1, 107, 661
    BMHndAudioVisualAnmPlay 0x8029, 0
    BMHndAnmWait 0x8029
    SEPlay SEQ_SE_KAIDAN
    ActorSetGPos 0, 107, 2, 661, 1
    SEWait
    ActorWalkRoute 0, 107, 662, 1, 8, 0
    ActorCmdExec 255, Movement_07E0
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8029, 1
    BMHndAnmWait 0x8029
    BMReleaseHandle 0x8029
    WordSetPlayerName 0
    // "Alder: [f000]Ā\u0001\u0000![f000]븁\u0000\nAs for the newly opened Pokémon Gym\nin Aspertia City, I heard a new[f000]븀\u0000\nGym Leader has arrived there.[f000]븁\u0000\nYou should go and test how\nstrong you've become![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10635_Text_AlderNewlyOpenedPokemon, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_07E0
    ActorCmdWait
    BMCreateHandleByGPos 0x8029, 1, 107, 661
    BMHndAudioVisualAnmPlay 0x8029, 0
    BMHndAnmWait 0x8029
    ActorCmdExec 0, Movement_07A8
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    BMHndAudioVisualAnmPlay 0x8029, 1
    BMHndAnmWait 0x8029
    BMReleaseHandle 0x8029
    FlagSet EVENT_FLAG_0x02db
    WorkSetConst EVENT_WORK_0x40a5, 6
    FlagSet EVENT_FLAG_0x010b
    WorkSetConst EVENT_WORK_0x40a8, 1
    WorkSetConst EVENT_WORK_0x409e, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0299:
    // "I know this is sudden,\nbut nice to meet you![f000]븁\u0000\nNow, don't say anything.\nJust take this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10635_Text_KnowSuddenButNice, 1, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 627
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "That is a Medal Box![f000]븁\u0000\nAnd, people call me\nMr. Medal![f000]븁\u0000\n...By the way, do you know a competition\ncalled the Medal Rally?"
    ActorMsg MSGFILE_SCRIPT, Global10635_Text_MedalBoxPeopleCall, 1, 0, 0
    YesNoWin 0x8010
    // "Whether you know it or not,\nI'll explain it to you![f000]븁\u0000\nThe Medal Rally is an event\nthat evaluates various activities[f000]븀\u0000\nof Trainers.[f000]븁\u0000\nSo...in commemoration of your\nparticipation, please take this Medal![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10635_Text_WhetherKnowNotIll, 1, 0, 0
    ActorMsgClose
    WorkSetConst 0x8020, 0
    VMCall L_0771
    // "And here's some help for the Medal Rally!\nI'll give you Hint Medals, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10635_Text_HeresSomeHelpMedal, 1, 0, 0
    ActorMsgClose
    MedalDiscoverInitial
    MedalGetCount 1, 0x8022
    WordSetPlayerName 0
    WordSetNumber 1, 0x8022, 3
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_032F
    // "[f000]Ā\u0001\u0000 received\n[f000]Ȃ\u0001\u0001 [f000][ff00]\u0001\u0002Hint Medals[f000][ff00]\u0001\u0000![f000]븁\u0000"
    SystemMsg Global10635_Text_ReceivedHintMedals, 0
    VMJump L_0335

L_032F:
    // "[f000]Ā\u0001\u0000 received\na [f000][ff00]\u0001\u0002Hint Medal[f000][ff00]\u0001\u0000![f000]븁\u0000"
    SystemMsg Global10635_Text_ReceivedHintMedal, 0

L_0335:
    InfoMsgClose
    // "For your information,\nyou can get Medals if you meet[f000]븀\u0000\ntheir conditions.[f000]븁\u0000\nGo to a Pokémon Center,\nand you can get Medals from me[f000]븀\u0000\none after another![f000]븁\u0000\nFirst, please use your Medal Box and\ncheck the inside![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10635_Text_InformationCanGetMedals, 1, 0, 0
    ActorMsgClose
    VMReturn
    .balign 4, 0

Movement_0348:
    Move 75, 1
    Move 32, 1
    MoveEnd

Movement_0354:
    Move 13, 4
    Move 15, 6
    Move 13, 5
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x409e, 0
    VMJumpIf CMP_EQ, L_037F
    VMJump L_0391

L_037F:
    VMCall L_0299
    WorkSetConst EVENT_WORK_0x409e, 1
    VMJump L_0486

L_0391:
    WorkCmpConst EVENT_WORK_0x409e, 2
    VMJumpIf CMP_EQ, L_03A4
    VMJump L_03BD

L_03A4:
    WordSetPlaceName 0, 28
    // "We'll have an award ceremony\nat the Medal Rally Office[f000]븀\u0000\nin [f000]ą\u0001\u0000.[f000]븁\u0000\nSo, please come to the office!"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_WellHaveAwardCeremony, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0486

L_03BD:
    WorkCmpConst EVENT_WORK_0x409e, 4
    VMJumpIf CMP_EQ, L_03D0
    VMJump L_03E7

L_03D0:
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000. Finally,\nyou've come this far.[f000]븁\u0000\nWell, to honor the best and ultimate\nachievement, we'll have an extravagant[f000]븀\u0000\nevent at the Medal Rally Office![f000]븁\u0000\nYo! You're the best!"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_FinallyYouveComeFar, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0486

L_03E7:
    WorkCmpConst EVENT_WORK_0x409e, 1
    VMJumpIf CMP_EQ, L_03FA
    VMJump L_0451

L_03FA:
    VMCall L_0490
    VMCall L_0511
    MedalGetCount 3, 0x8022
    MedalGetCount 4, 0x8023
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0441
    WorkSetConst EVENT_WORK_0x409e, 2
    WordSetPlayerName 0
    WordSetPlaceName 1, 28
    // "Oh! [f000]Ā\u0001\u0000!\nGreat! Really great![f000]븁\u0000\nYou know what? Just now\nyou passed the checkpoint[f000]븀\u0000\nthe Medal Rally Office set[f000]븀\u0000\nand reached the goal![f000]븁\u0000\nSo, we'll have an award ceremony\nat the Medal Rally Office[f000]븀\u0000\nin [f000]ą\u0001\u0001.[f000]븁\u0000\nSo, please come to the office!"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_OhGreatReallyGreat, 0, 0
    VMJump L_0447

L_0441:
    VMCall L_0708

L_0447:
    LastKeyWait
    ActorMsgClose
    VMJump L_0486

L_0451:
    WorkCmpConst EVENT_WORK_0x409e, 3
    VMJumpIf CMP_EQ, L_0464
    VMJump L_0486

L_0464:
    VMCall L_0490
    VMCall L_0511
    VMCall L_059F
    VMCall L_0708
    LastKeyWait
    ActorMsgClose
    VMJump L_0486

L_0486:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    VMReturn
    VMReturn

L_0490:
    MedalGetCount 2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04AA
    VMReturn

L_04AA:
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000. I've been waiting for you!\nYou're doing terrific![f000]븀\u0000\nHere's a new Medal![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_IveBeenWaitingYoure, 0, 0
    ActorMsgClose
    PokemonCenterCmd_FindEarnedMedal 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04D8
    VMCall L_0771

L_04D8:
    PokemonCenterCmd_FindEarnedMedal 0x8010, 0x8020

L_04DE:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_050F
    // "And! I have another Medal\nI want to give! Please! Please![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_HaveAnotherMedalWant, 0, 0
    ActorMsgClose
    VMCall L_0771
    PokemonCenterCmd_FindEarnedMedal 0x8010, 0x8020
    VMJump L_04DE

L_050F:
    VMReturn

L_0511:
    MedalGetCount 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_052B
    VMReturn

L_052B:
    WordSetPlayerName 0
    WordSetNumber 1, 0x8010, 3
    // "Hmm... [f000]Ā\u0001\u0000![f000]븁\u0000\nI'd like you to collect more Medals.\nSo, I'll give you Hint Medals![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_HmmIdLikeCollect, 0, 0
    ActorMsgClose
    MedalGetCount 0, 0x8022
    WordSetPlayerName 0
    WordSetNumber 1, 0x8022, 3
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_056F
    // "[f000]Ā\u0001\u0000 received\n[f000]Ȃ\u0001\u0001 [f000][ff00]\u0001\u0002Hint Medals[f000][ff00]\u0001\u0000![f000]븁\u0000"
    SystemMsg Global10635_Text_ReceivedHintMedals, 0
    VMJump L_0575

L_056F:
    // "[f000]Ā\u0001\u0000 received\na [f000][ff00]\u0001\u0002Hint Medal[f000][ff00]\u0001\u0000![f000]븁\u0000"
    SystemMsg Global10635_Text_ReceivedHintMedal, 0

L_0575:
    InfoMsgClose
    PokemonCenterCmd_Medal 1002
    MedalGetCount 5, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_059D
    // "Well... I don't have any more Hint Medals\nI can give to you.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_WellDontHaveAny, 0, 0

L_059D:
    VMReturn

L_059F:
    VMCall L_06BB
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05BA
    VMReturn

L_05BA:
    WordSetPlayerName 0
    // "Wow! [f000]Ā\u0001\u0000.\nYou collected a lot of Medals![f000]븁\u0000\nI'll upgrade your Medal Box.[f000]븁\u0000\nAiya! Yah![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_WowCollectedLotMedals, 0, 0
    ActorMsgClose

L_05C9:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06B9
    PokemonCenterCmd_Medal 1003
    MedalGetCount 7, 0x8021
    WordSetMedalRank 1, 0x8021
    // "[f000]Ā\u0001\u0000's Medal Box\nhas been upgraded to[f000]븀\u0000\n[f000][ff00]\u0001\u0002[f000]Ķ\u0001\u0001[f000][ff00]\u0001\u0000 Rank!"
    SystemMsg Global10635_Text_SMedalBoxHas, 0
    MEPlay SEQ_ME_MD_FAN01
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    // "In commemoration of the upgrade,\nI'll give you a Medal, as well![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_CommemorationUpgradeIllGive, 0, 0
    ActorMsgClose
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_0619
    VMJump L_0625

L_0619:
    WorkSetConst 0x8020, 2
    VMJump L_0688

L_0625:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_0638
    VMJump L_0644

L_0638:
    WorkSetConst 0x8020, 3
    VMJump L_0688

L_0644:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_0657
    VMJump L_0663

L_0657:
    WorkSetConst 0x8020, 4
    VMJump L_0688

L_0663:
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_0676
    VMJump L_0682

L_0676:
    WorkSetConst 0x8020, 5
    VMJump L_0688

L_0682:
    WorkSetConst 0x8020, 1

L_0688:
    VMCall L_0771
    VMCall L_06BB
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06B3
    // "A further upgrade for you!\nHaaah! Yaaah![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_FurtherUpgradeHaaahYaaah, 0, 0
    ActorMsgClose

L_06B3:
    VMJump L_05C9

L_06B9:
    VMReturn

L_06BB:
    MedalGetCount 7, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06DB
    WorkSetConst 0x8010, 0
    VMReturn

L_06DB:
    MedalGetCount 3, 0x8022
    MedalGetCount 4, 0x8023
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0700
    WorkSetConst 0x8010, 1
    VMReturn

L_0700:
    WorkSetConst 0x8010, 0
    VMReturn

L_0708:
    MedalGetCount 6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0735
    WorkSetConst EVENT_WORK_0x409e, 4
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000. Finally,\nyou've come this far.[f000]븁\u0000\nWell, to honor the best and ultimate\nachievement, we'll have an extravagant[f000]븀\u0000\nevent at the Medal Rally Office![f000]븁\u0000\nYo! You're the best!"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_FinallyYouveComeFar, 0, 0
    VMReturn

L_0735:
    MedalGetCount 7, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0759
    // "Well, good luck with getting a new Medal!"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_WellGoodLuckGetting, 0, 0
    VMReturn

L_0759:
    MedalGetCount 4, 0x8023
    WordSetNumber 0, 0x8023, 3
    // "Well, aim to receive [f000]Ȃ\u0001\u0000 Medals!\nGood luck!"
    ParentActorMsg MSGFILE_SCRIPT, Global10635_Text_WellAimReceiveMedals, 0, 0
    VMReturn

L_0771:
    WorkSetConst 0x802a, 0
    MedalGetFieldEffectID 0x8020, 0x802a
    DebugPrint 0x802a
    PlayFieldEffect 0x802a
    MedalAcknowledge 0x8020, 1
    WordSetPlayerName 0
    WordSetMedalName 1, 0x8020
    // "[f000]Ā\u0001\u0000 obtained\nthe [f000][ff00]\u0001\u0002[f000]ĵ\u0001\u0001[f000][ff00]\u0001\u0000 Medal.[f000]븁\u0000"
    SystemMsg Global10635_Text_ObtainedMedal, 0
    InfoMsgClose
    VMReturn
    .balign 4, 0

Movement_07A0:
    Move 13, 1
    MoveEnd

Movement_07A8:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
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

Movement_07E0:
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
