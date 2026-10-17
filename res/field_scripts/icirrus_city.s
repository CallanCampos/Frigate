#include "asm/field_script.inc"
#include "text/script/icirrus_city.h"

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

Script_14:
    WorkSetConst 0x8020, 0
    RTCGetSeason 0x8020
    WorkCmpConst EVENT_WORK_0x418c, 1
    VMJumpIf CMP_EQ, L_0057
    VMJump L_0076

L_0057:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0070
    WorkSetConst EVENT_WORK_0x418c, 0

L_0070:
    VMJump L_010C

L_0076:
    WorkCmpConst EVENT_WORK_0x418c, 2
    VMJumpIf CMP_EQ, L_0089
    VMJump L_00A8

L_0089:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00A2
    WorkSetConst EVENT_WORK_0x418c, 0

L_00A2:
    VMJump L_010C

L_00A8:
    WorkCmpConst EVENT_WORK_0x418c, 3
    VMJumpIf CMP_EQ, L_00BB
    VMJump L_00DA

L_00BB:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00D4
    WorkSetConst EVENT_WORK_0x418c, 0

L_00D4:
    VMJump L_010C

L_00DA:
    WorkCmpConst EVENT_WORK_0x418c, 4
    VMJumpIf CMP_EQ, L_00ED
    VMJump L_010C

L_00ED:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0106
    WorkSetConst EVENT_WORK_0x418c, 0

L_0106:
    VMJump L_010C

L_010C:
    FlagReset EVENT_FLAG_0x09ff
    WorkSetConst 0x8020, 0
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Icirrus City\nSky Glittering with Flowers of Snow"
    MsgPlaceSign IcirrusCity_Text_IcirrusCitySkyGlittering, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Fans of Everything Pokémon\nThe Pokémon Fan Club"
    MsgPlaceSign IcirrusCity_Text_FansEverythingPokemonPokemon, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Former Icirrus City\nPokémon Gym"
    MsgPlaceSign IcirrusCity_Text_FormerIcirrusCityPokemon, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The legendary Pokémon Reshiram\nshared its wisdom with the hero[f000]븀\u0000\nand defied foes with columns of fire.[f000]븁\u0000\nThe hero and that Pokémon were as\nclose as a parent and child.[f000]븁\u0000\nIt was indeed awe inspiring\nto see them!"
    // "The legendary Pokémon Zekrom\nshared its wisdom with the hero[f000]븀\u0000\nand defied foes with fierce lightning.[f000]븁\u0000\nThe hero and that Pokémon were as\nclose as a parent and child.[f000]븁\u0000\nIt was indeed awe inspiring\nto see them!"
    ActorMsgVersioned 1024, IcirrusCity_Text_LegendaryPokemonReshiramShared, IcirrusCity_Text_LegendaryPokemonZekromShared, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Always, the tower looms,\ndisdainful of the wind and snow.[f000]븁\u0000\nPerhaps its presence is telling us\nnot to forget dreams and ideals..."
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_AlwaysTowerLoomsDisdainful, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The legendary Pokémon gave help\nto the hero who sought truth.[f000]븀\u0000\nBecause of that, a region was created.[f000]븁\u0000\nThen, when its physical form was lost,\nthe Pokémon became the Light Stone,[f000]븀\u0000\nwaiting patiently for a new hero.[f000]븁\u0000\nThe story I heard when I was little\nturned out to be true."
    // "The legendary Pokémon gave help\nto the hero who sought ideals.[f000]븀\u0000\nBecause of that, a region was created.[f000]븁\u0000\nThen, when its physical form was lost,\nthe Pokémon became the Dark Stone,[f000]븀\u0000\nwaiting patiently for a new hero.[f000]븁\u0000\nThe story I heard when I was little\nturned out to be true."
    ActorMsgVersioned 1024, IcirrusCity_Text_LegendaryPokemonGaveHelp, IcirrusCity_Text_LegendaryPokemonGaveHelp_2, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Brycen has been surrounded by\nIce-type Pokémon and has trained in the[f000]븀\u0000\nmartial arts ever since he was a child.[f000]븁\u0000\nHe's become an action star\nusing that experience!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_BrycenHasBeenSurrounded, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Truth! Ideals!\nTwo dragons!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_TruthIdealsTwoDragons, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When one is divided, ♪\nthe world will broaden. ♪"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_WhenOneDividedWorld, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Fused by splicers.\nDivided by splicers."
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_FusedBySplicersDivided, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Truth and ideals.\nMove forward hand in hand. ♪"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_TruthIdealsMoveForward, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    RTCGetSeason 0x8021
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0287
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When the wetlands freeze, it's slippery.\nI wonder how wetland Pokémon manage?"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_WhenWetlandsFreezeIts, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_029B

L_0287:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you walk in the wetlands, you might\nsurprise wild Pokémon into popping out!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_IfWalkWetlandsMight, 0, 0
    LastKeyWait
    ActorMsgClose

L_029B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x039d
    FlagReset EVENT_FLAG_0x039e
    FlagReset EVENT_FLAG_0x039f
    ActorAdd 10
    ActorAdd 11
    ActorAdd 12
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetGPos 0x8022, 0x8023
    WorkCmpConst 0x8023, 195
    VMJumpIf CMP_EQ, L_02E0
    VMJump L_030A

L_02E0:
    ActorSetGPos 10, 219, 3, 195, 2
    ActorSetGPos 11, 221, 3, 196, 2
    ActorSetGPos 12, 221, 3, 195, 2
    VMJump L_0347

L_030A:
    WorkCmpConst 0x8023, 196
    VMJumpIf CMP_EQ, L_031D
    VMJump L_0347

L_031D:
    ActorSetGPos 10, 219, 3, 196, 2
    ActorSetGPos 11, 221, 3, 195, 2
    ActorSetGPos 12, 221, 3, 196, 2
    VMJump L_0347

L_0347:
    ActorCmdExec 10, Movement_0828
    ActorCmdExec 11, Movement_0828
    ActorCmdExec 12, Movement_0828
    ActorCmdWait
    ActorCmdExec 255, Movement_0808
    ActorCmdWait
    BGMPlay SEQ_BGM_DARK_TRINITY
    VMStackPushFlag EVENT_FLAG_0x0196
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0398
    // "You...\nCongratulations.[f000]븁\u0000\nGhetsis won't do anything anymore...[f000]븁\u0000\nNo, to be more precise,\nhe can't do anything anymore...[f000]븁\u0000\nThat's why we won't forgive you.\nBattle us!"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_CongratulationsGhetsisWontAnything, 10, 0, 0
    FlagSet EVENT_FLAG_0x0196
    VMJump L_03A4

L_0398:
    // "Battle us!"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_BattleUs, 10, 0, 0

L_03A4:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06C4
    VMCall L_070A
    // "I'll win! That is the only way\nto regain his lost heart![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_IllWinOnlyWay, 10, 0, 0
    MsgWinCloseAll
    ActorCmdExec 10, Movement_07C0
    ActorCmdWait
    CallTrainerBattle TRAINER_TEAM_PLASMA_SHADOW_5, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0400
    CallTrainerBattleEnd
    VMJump L_0408

L_0400:
    VMCall L_0792
    CallTrainerLose

L_0408:
    ActorCmdExec 10, Movement_0844
    ActorCmdWait
    ActorDelete 10
    ActorCmdExec 11, Movement_0844
    ActorCmdWait
    WorkCmpConst 0x8023, 195
    VMJumpIf CMP_EQ, L_0433
    VMJump L_0445

L_0433:
    ActorSetGPos 11, 219, 3, 195, 2
    VMJump L_046A

L_0445:
    WorkCmpConst 0x8023, 196
    VMJumpIf CMP_EQ, L_0458
    VMJump L_046A

L_0458:
    ActorSetGPos 11, 219, 3, 196, 2
    VMJump L_046A

L_046A:
    ActorCmdExec 11, Movement_0838
    ActorCmdWait
    FlagSet EVENT_FLAG_0x039d
    WorkSetConst 0x8024, 0
    PokePartyGetCount 0x8024, 2
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_068E
    // "What do you want?\nDo you want to keep battling?"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_WhatWantWantKeep, 11, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0658
    // "Battle for Ghetsis![f000]븁\u0000\nThat is the only proof\nthat I'm living now.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_BattleGhetsisOnlyProof, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_07C0
    ActorCmdWait
    CallTrainerBattle TRAINER_TEAM_PLASMA_SHADOW_6, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F9
    CallTrainerBattleEnd
    VMJump L_0501

L_04F9:
    VMCall L_0792
    CallTrainerLose

L_0501:
    ActorCmdExec 11, Movement_0844
    ActorCmdWait
    ActorDelete 11
    FlagSet EVENT_FLAG_0x039e
    ActorCmdExec 12, Movement_0844
    ActorCmdWait
    WorkCmpConst 0x8023, 195
    VMJumpIf CMP_EQ, L_0530
    VMJump L_0542

L_0530:
    ActorSetGPos 12, 219, 3, 195, 2
    VMJump L_0567

L_0542:
    WorkCmpConst 0x8023, 196
    VMJumpIf CMP_EQ, L_0555
    VMJump L_0567

L_0555:
    ActorSetGPos 12, 219, 3, 196, 2
    VMJump L_0567

L_0567:
    ActorCmdExec 12, Movement_0838
    ActorCmdWait
    PokePartyGetCount 0x8024, 2
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0632
    // "What do you want?\nDo you want to keep battling?"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_WhatWantWantKeep_2, 12, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_060C
    // "We, the Shadow Triad,\nwere, are, and will be the[f000]븀\u0000\nroyal servants of Ghetsis![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_WeShadowTriadWere, 12, 0, 0
    MsgWinCloseAll
    ActorCmdExec 12, Movement_07C0
    ActorCmdWait
    CallTrainerBattle TRAINER_TEAM_PLASMA_SHADOW_7, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05EC
    CallTrainerBattleEnd
    VMJump L_05F4

L_05EC:
    VMCall L_0792
    CallTrainerLose

L_05F4:
    ActorCmdExec 12, Movement_0844
    ActorCmdWait
    ActorDelete 12
    FlagSet EVENT_FLAG_0x039f
    VMJump L_062C

L_060C:
    // "I see...[f000]븁\u0000\nWhen the season changes,\nI'll battle you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_SeeWhenSeasonChanges_2, 12, 0, 0
    MsgWinCloseAll
    ActorCmdExec 12, Movement_0844
    ActorCmdWait
    ActorDelete 12
    FlagSet EVENT_FLAG_0x039f

L_062C:
    VMJump L_0652

L_0632:
    // "The next is me...[f000]븁\u0000\nBut you don't have enough Pokémon\nfor a Rotation Battle.[f000]븁\u0000\nI'll battle you when the season changes.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_NextButDontHave_2, 12, 0, 0
    MsgWinCloseAll
    ActorCmdExec 12, Movement_0844
    ActorCmdWait
    ActorDelete 12
    FlagSet EVENT_FLAG_0x039f

L_0652:
    VMJump L_0688

L_0658:
    // "I see...[f000]븁\u0000\nWhen the season changes,\nI'll battle you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_SeeWhenSeasonChanges, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_0844
    ActorCmdExec 12, Movement_0844
    ActorCmdWait
    ActorDelete 11
    ActorDelete 12
    FlagSet EVENT_FLAG_0x039e
    FlagSet EVENT_FLAG_0x039f

L_0688:
    VMJump L_06BE

L_068E:
    // "The next is me...[f000]븁\u0000\nBut you don't have enough Pokémon\nfor a Triple Battle.[f000]븁\u0000\nI'll battle you when the season changes.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity_Text_NextButDontHave, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_0844
    ActorCmdExec 12, Movement_0844
    ActorCmdWait
    ActorDelete 11
    ActorDelete 12
    FlagSet EVENT_FLAG_0x039e
    FlagSet EVENT_FLAG_0x039f

L_06BE:
    VMJump L_0702

L_06C4:
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x039d
    FlagSet EVENT_FLAG_0x039e
    FlagSet EVENT_FLAG_0x039f
    ActorCmdExec 10, Movement_0844
    ActorCmdExec 11, Movement_0844
    ActorCmdExec 12, Movement_0844
    ActorCmdWait
    ActorDelete 10
    ActorDelete 11
    ActorDelete 12
    ActorCmdExec 255, Movement_07B8
    ActorCmdWait

L_0702:
    BGMChangeMap
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_070A:
    WorkSetConst 0x8025, 0
    RTCGetSeason 0x8025
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_0727
    VMJump L_0733

L_0727:
    WorkSetConst EVENT_WORK_0x418c, 1
    VMJump L_0790

L_0733:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_0746
    VMJump L_0752

L_0746:
    WorkSetConst EVENT_WORK_0x418c, 2
    VMJump L_0790

L_0752:
    WorkCmpConst 0x8025, 2
    VMJumpIf CMP_EQ, L_0765
    VMJump L_0771

L_0765:
    WorkSetConst EVENT_WORK_0x418c, 3
    VMJump L_0790

L_0771:
    WorkCmpConst 0x8025, 3
    VMJumpIf CMP_EQ, L_0784
    VMJump L_0790

L_0784:
    WorkSetConst EVENT_WORK_0x418c, 4
    VMJump L_0790

L_0790:
    VMReturn

L_0792:
    FlagSet EVENT_FLAG_0x039d
    FlagSet EVENT_FLAG_0x039e
    FlagSet EVENT_FLAG_0x039f
    VMReturn
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_07B8:
    Move 14, 1
    MoveEnd

Movement_07C0:
    Move 18, 1
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
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0808:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_0828:
    Move 184, 1
    MoveEnd
    Move 185, 1
    MoveEnd

Movement_0838:
    Move 70, 1
    Move 184, 1
    MoveEnd

Movement_0844:
    Move 185, 1
    Move 69, 1
    MoveEnd
