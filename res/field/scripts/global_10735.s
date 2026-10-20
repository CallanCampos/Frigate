#include "asm/field_script.inc"
#include "text/script/global_10735.h"

// Script plugin 9, from the zones that start its scripts

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
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntry Script_20
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
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0

Script_19:
    ActorsPauseAll
    Plugin9_Cmd1001 4, 8, 7
    VMCall L_2454
    RTReserveScript 10742
    SEStop
    WorkSetConst EVENT_WORK_0x4191, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00E0:
    Plugin9_Cmd1001 4, 7, 7
    VMCall L_2454
    RTReserveScript 10742
    SEStop
    VMReturn

Script_20:
    ActorsPauseAll
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin9_Cmd1001 4, 8, 7
    VMCall L_2454
    RTReserveScript 10742
    SEStop

Script_1:
    Plugin9_Cmd1000
    VMHalt

Script_2:
    ActorsPauseAll
    ActorCmdExec 255, Movement_2BA0
    ActorCmdWait
    VMCall L_242D
    VMCall L_2611
    Plugin9_Cmd1001 0, 8, 7
    VMCall L_247B
    FadeEx 3, 16, 0, 2
    FadeExWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 255, Movement_2BAC
    ActorCmdWait
    VMCall L_242D
    Plugin9_Cmd1001 1, 8, 7
    VMCall L_247B
    FadeEx 3, 16, 0, 2
    FadeExWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    ActorCmdExec 255, Movement_2BC4
    ActorCmdWait
    VMCall L_242D
    Plugin9_Cmd1001 2, 8, 7
    VMCall L_247B
    FadeEx 3, 16, 0, 2
    FadeExWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    ActorCmdExec 255, Movement_2BB8
    ActorCmdWait
    VMCall L_242D
    Plugin9_Cmd1001 3, 8, 7
    VMCall L_247B
    FadeEx 3, 16, 0, 2
    FadeExWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    Plugin9_Cmd1023 0x8020
    EvCameraInit
    EvCameraUnbind
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0243
    EvCameraMoveTo 3672, 0, 0xed000, 0x68000, 0x20000, 0xfffe0000, 20
    VMJump L_025B

L_0243:
    EvCameraMoveTo 4056, 0, 0xed000, 0x68000, 0x10000, 0xfffed000, 20

L_025B:
    Plugin9_Cmd1016 0, 1
    ActorCmdExec 255, Movement_2B90
    ActorCmdWait
    Plugin9_Cmd1016 0, 0
    EvCameraWait
    // "Where would you like to go to?"
    SystemMsg Global10735_Text_WhereWouldLikeGo, 2
    Plugin9_Cmd1004 0x8035
    Plugin9_Cmd1006 0x8021
    Plugin9_Cmd1024 0, 0x8021, 0x8033
    Plugin9_Cmd1024 1, 0x8021, 0x8034
    WorkAdd 0x8034, 0x8033
    WorkSetConst 0x8032, 0
    DebugPrint 0x8033
    DebugPrint 0x8034
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32815

L_02AE:
    VMStackPush 0x8033
    VMStackPush 0x8034
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_032E
    VMStackPush 0x8035
    VMStackPush 0x8032
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0316
    WorkGet 0x802d, 0x8033
    WorkAdd 0x802d, 1
    WordSetNumber 0, 0x802d, 2
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0308
    ListMenuAdd 134, 65535, 0x8033
    VMJump L_0310

L_0308:
    ListMenuAdd 135, 65535, 0x8033

L_0310:
    VMJump L_031C

L_0316:
    WorkGet 0x8036, 0x8033

L_031C:
    WorkAdd 0x8033, 1
    WorkAdd 0x8032, 1
    VMJump L_02AE

L_032E:
    ListMenuAdd 136, 65535, 240
    ListMenuAdd 137, 65535, 255
    ListMenuShow
    InfoMsgClose
    VMStackPush 0x802f
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMStackPush 0x802f
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0509
    VMStackPush 0x802f
    VMStackPushConst 240
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0457
    VMStackPush 0x802f
    VMStackPush 0x8036
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_03E6
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C2
    EvCameraMoveTo 3672, 0, 0xed000, 0x68000, 0x9a120, 0xfffe0000, 40
    WorkSetConst EVENT_WORK_0x4190, 1
    VMJump L_03E0

L_03C2:
    EvCameraMoveTo 4056, 0, 0xed000, 0x68000, 0xfff95ee0, 0xfffed000, 40
    WorkSetConst EVENT_WORK_0x4190, 0

L_03E0:
    VMJump L_043B

L_03E6:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_041D
    EvCameraMoveTo 3672, 0, 0xed000, 0x68000, 0xfffa5ee0, 0xfffe0000, 40
    WorkSetConst EVENT_WORK_0x4190, 0
    VMJump L_043B

L_041D:
    EvCameraMoveTo 4056, 0, 0xed000, 0x68000, 0x8a120, 0xfffed000, 40
    WorkSetConst EVENT_WORK_0x4190, 1

L_043B:
    VMCall L_2454
    RTReserveScript 10743
    Plugin9_Cmd1005 0x802f
    SEStop
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMJump L_0503

L_0457:
    // "If you return to the lobby, you will\nquit your challenge of this area.[f000]븁\u0000\nWould you like to return to the lobby?[f000]븁\u0000"
    SystemMsg Global10735_Text_IfReturnLobbyWill, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E3
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A7
    EvCameraMoveTo 3672, 0, 0xed000, 0x68000, 0xfffa5ee0, 0xfffe0000, 40
    VMJump L_04BF

L_04A7:
    EvCameraMoveTo 4056, 0, 0xed000, 0x68000, 0x8a120, 0xfffed000, 40

L_04BF:
    VMCall L_2454
    Plugin9_Cmd1001 4, 8, 7
    RTReserveScript 10742
    SEStop
    WorkSetConst EVENT_WORK_0x4191, 1
    EvCameraRebind
    EvCameraEnd
    VMJump L_0503

L_04E3:
    Plugin9_Cmd1016 0, 1
    ActorCmdExec 255, Movement_2C00
    ActorCmdWait
    EvCameraMoveToDefault 20
    Plugin9_Cmd1016 0, 0
    EvCameraWait
    EvCameraRebind
    EvCameraEnd

L_0503:
    VMJump L_0529

L_0509:
    Plugin9_Cmd1016 0, 1
    ActorCmdExec 255, Movement_2C00
    ActorCmdWait
    EvCameraMoveToDefault 20
    Plugin9_Cmd1016 0, 0
    EvCameraWait
    EvCameraRebind
    EvCameraEnd

L_0529:
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8032, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8010, 1

L_055B:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0580
    WorkSetConst 0x8010, 0
    VMCall L_0586
    VMJump L_055B

L_0580:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0586:
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8010, 0
    Plugin9_Cmd1023 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05BB
    // "Welcome![f000]븁\u0000\nThis is Unova's Challenge--\nthe Black Tower![f000]븁\u0000\nPlease, let me know what you would\nlike to do."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_WelcomeUnovasChallengeBlack, 0x8011, 4, 0
    VMJump L_05C7

L_05BB:
    // "Hey! Welcome! Welcome![f000]븁\u0000\nThis is Unova's Challenge--\nthe White Treehollow![f000]븁\u0000\nWhat would you like to do today?"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyWelcomeWelcomeUnovas, 0x8011, 4, 0

L_05C7:
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32823
    ListMenuAdd 131, 65535, 1
    ListMenuAdd 130, 65535, 0
    ListMenuAdd 132, 65535, 2
    ListMenuShow
    VMStackPush 0x8037
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0718
    WorkCmpConst 0x8037, 0
    VMJumpIf CMP_EQ, L_0610
    VMJump L_0668

L_0610:
    WordSetPlayerName 0
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0644
    // "This facility was made to provide a\nplace where Trainers could come to[f000]븀\u0000\nfocus on training.[f000]븁\u0000\nInside, you will find many other\nTrainers who also came to this facility[f000]븀\u0000\nto challenge themselves.[f000]븁\u0000\nWhen you run into another Trainer,\nthey will surely challenge you to[f000]븀\u0000\na battle.[f000]븁\u0000\nDefeat all of the Trainers that block\nyour way to complete an area.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_FacilityMadeProvidePlace, 0x8011, 4, 0
    // "In order to complete an area, you must\nfind the Boss Trainer of the area and[f000]븀\u0000\ndefeat him or her in battle.[f000]븁\u0000\nBy defeating the Boss Trainer, you can\nadvance on to the next area.[f000]븁\u0000\nIf you'd like, you can simply battle with\nother Trainers to improve your skills.[f000]븁\u0000\nYou can also focus on finding the Boss\nTrainer to complete the area.[f000]븁\u0000\nSo how about it? Would you like to\nchallenge yourself and find out just[f000]븀\u0000\nhow strong you really are?[f000]븁\u0000\nA word of warning: once you enter the\nfacility, usage of items inside your[f000]븀\u0000\nBag is prohibited.[f000]븁\u0000\nWe thank you for your cooperation.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_OrderCompleteAreaMust, 0x8011, 4, 0
    VMJump L_065C

L_0644:
    // "This facility was made so that you\nTrainers can come and train to your[f000]븀\u0000\nheart's content.[f000]븁\u0000\nInside, you'll find lots of other\nTrainers who also came here to[f000]븀\u0000\nchallenge themselves.[f000]븁\u0000\nWhen you run into another Trainer,\nthey'll surely challenge you to a battle.[f000]븁\u0000\nDefeat all of the Trainers that are in\nyour way to complete an area.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_FacilityMadeTrainersCan, 0x8011, 4, 0
    // "If you want to complete an area, you've\ngot to find and defeat the Boss Trainer[f000]븀\u0000\nof that area.[f000]븁\u0000\nBy defeating the Boss Trainer, you can\nadvance on to the next area.[f000]븁\u0000\nIf you want, you can simply battle with\nother Trainers to improve your skills.[f000]븁\u0000\nYou can also focus on finding the Boss\nTrainer to complete the area.[f000]븁\u0000\nWell, what are you waiting for? Get in\nthere and test your strength![f000]븁\u0000\nThere's a ton of powerful\nTrainers inside![f000]븁\u0000\nOh yeah! Once you're inside, you can't\nuse any of the items in your Bag.[f000]븀\u0000\nDon't you forget it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_IfWantCompleteArea, 0x8011, 4, 0

L_065C:
    WorkSetConst 0x8010, 1
    VMJump L_0712

L_0668:
    WorkCmpConst 0x8037, 1
    VMJumpIf CMP_EQ, L_067B
    VMJump L_06BE

L_067B:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06A0
    // "Which area would you like to challenge?"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_WhichAreaWouldLike, 0x8011, 4, 0
    VMJump L_06AC

L_06A0:
    // "Which area do you want to challenge?"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_WhichAreaWantChallenge, 0x8011, 4, 0

L_06AC:
    VMCall L_075B
    WorkSetConst 0x8010, 0
    VMJump L_0712

L_06BE:
    WorkCmpConst 0x8037, 2
    VMJumpIf CMP_EQ, L_06D1
    VMJump L_0712

L_06D1:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06F6
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8011, 4, 0
    VMJump L_0702

L_06F6:
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8011, 4, 0

L_0702:
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8010, 0
    VMJump L_0712

L_0712:
    VMJump L_0753

L_0718:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_073D
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8011, 4, 0
    VMJump L_0749

L_073D:
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8011, 4, 0

L_0749:
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8010, 0

L_0753:
    WorkSetConst 0x8037, 0
    VMReturn

L_075B:
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8039, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x803b, 0
    Plugin9_Cmd1023 0x8020
    KeysCmd_02D1 0x803b
    VMStackPush 0x803b
    VMStackPushConst 9
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0794
    WorkSetConst 0x803b, 9

L_0794:
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    WorkSetConst 0x803a, 0

L_07A3:
    VMStackPush 0x803a
    VMStackPush 0x803b
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_07DD
    WorkGet 0x8039, 0x803a
    WorkAdd 0x8039, 1
    WordSetNumber 0, 0x8039, 2
    ListMenuAdd 139, 65535, 0x803a
    WorkAdd 0x803a, 1
    VMJump L_07A3

L_07DD:
    ListMenuAdd 137, 65535, 255
    ListMenuShow
    VMStackPush 0x8022
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMStackPush 0x8022
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0B76
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_082F
    // "Would you like to challenge this area?"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_WouldLikeChallengeArea, 0x8011, 4, 0
    VMJump L_083B

L_082F:
    // "Do you want to challenge this area?"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_WantChallengeArea, 0x8011, 4, 0

L_083B:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B37
    Cmd_01DD 12, 0x8022, 0
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0877
    Cmd_02C5 22
    VMJump L_087B

L_0877:
    Cmd_02C5 32

L_087B:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08A0
    // "We will recover your Pokémon before\nyou begin your challenge."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_WeWillRecoverPokemon, 0x8011, 4, 0
    VMJump L_08AC

L_08A0:
    // "We'll recover your Pokémon before\nyou start your challenge."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_WellRecoverPokemonBefore, 0x8011, 4, 0

L_08AC:
    PokePartyRecoverAll
    MEPlay SEQ_SE_RECOVERY
    MEWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08D9
    // "Are you all ready to go?\nGood luck to you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_AllReadyGoGood, 0x8011, 4, 0
    VMJump L_08E5

L_08D9:
    // "You look all ready to go!\nBest of luck in there![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_LookAllReadyGo, 0x8011, 4, 0

L_08E5:
    ActorMsgClose
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0908
    ActorCmdExec 0x8011, Movement_2B44
    VMJump L_0910

L_0908:
    ActorCmdExec 0x8011, Movement_2B30

L_0910:
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0978
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_095A
    EvCameraMoveTo 2776, 0, 0xed000, 0x78000, 0x22000, 0x17000, 40
    VMJump L_0972

L_095A:
    EvCameraMoveTo 2776, 0, 0xed000, 0xb8000, 0x22000, 0x17000, 40

L_0972:
    VMJump L_09C1

L_0978:
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_09A9
    EvCameraMoveTo 3544, 0, 0xed000, 0x78000, 0x19000, 0x15000, 40
    VMJump L_09C1

L_09A9:
    EvCameraMoveTo 3544, 0, 0xed000, 0xb8000, 0x19000, 0x15000, 40

L_09C1:
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_09E2
    ActorCmdExec 255, Movement_2B70
    VMJump L_09EA

L_09E2:
    ActorCmdExec 255, Movement_2B80

L_09EA:
    ActorCmdWait
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0A0B
    Plugin9_Cmd1016 0, 1
    VMJump L_0A11

L_0A0B:
    Plugin9_Cmd1016 1, 1

L_0A11:
    ActorCmdExec 255, Movement_2B90
    ActorCmdWait
    EvCameraWait
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0A3C
    Plugin9_Cmd1016 0, 0
    VMJump L_0A42

L_0A3C:
    Plugin9_Cmd1016 1, 0

L_0A42:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AB0
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0A8C
    EvCameraMoveTo 2776, 0, 0xed000, 0x78000, 0x9c120, 0x17000, 40
    WorkSetConst EVENT_WORK_0x4190, 1
    VMJump L_0AAA

L_0A8C:
    EvCameraMoveTo 2776, 0, 0xed000, 0xb8000, 0x9c120, 0x17000, 40
    WorkSetConst EVENT_WORK_0x4190, 1

L_0AAA:
    VMJump L_0B05

L_0AB0:
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0AE7
    EvCameraMoveTo 3544, 0, 0xed000, 0x78000, 0xfff9eee0, 0x15000, 40
    WorkSetConst EVENT_WORK_0x4190, 0
    VMJump L_0B05

L_0AE7:
    EvCameraMoveTo 3544, 0, 0xed000, 0xb8000, 0xfff9eee0, 0x15000, 40
    WorkSetConst EVENT_WORK_0x4190, 0

L_0B05:
    VMCall L_2454
    Plugin9_Cmd1003
    Plugin9_Cmd1024 0, 0x8022, 0x8038
    Plugin9_Cmd1005 0x8038
    SEStop
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    Plugin9_Cmd1010 EVENT_WORK_0x4020
    RTReserveScript 10743
    WorkSetConst EVENT_WORK_0x4191, 0
    PedometerStart
    VMJump L_0B70

L_0B37:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B60
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0B70

L_0B60:
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose

L_0B70:
    VMJump L_0BAF

L_0B76:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B9F
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0BAF

L_0B9F:
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose

L_0BAF:
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x8039, 0
    WorkSetConst 0x8038, 0
    VMReturn

Script_8:
    ActorsPauseAll
    WorkSetConst 0x803c, 0
    WorkSetConst 0x803d, 0
    Plugin9_Cmd1006 0x8021
    Plugin9_Cmd1023 0x8020
    GameGetVersion 0x802c
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C02
    WorkSetConst 0x8023, 0
    VMJump L_0C08

L_0C02:
    WorkSetConst 0x8023, 0

L_0C08:
    VMCall L_2454
    EvCameraInit
    EvCameraUnbind
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C74
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0C56
    EvCameraMoveTo 2776, 0, 0xed000, 0x78000, 0x5f090, 0x17000, 1
    VMJump L_0C6E

L_0C56:
    EvCameraMoveTo 2776, 0, 0xed000, 0xb8000, 0x5f090, 0x17000, 1

L_0C6E:
    VMJump L_0CBD

L_0C74:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0CA5
    EvCameraMoveTo 3544, 0, 0xed000, 0x78000, 0xfffdbf70, 0x15000, 1
    VMJump L_0CBD

L_0CA5:
    EvCameraMoveTo 3544, 0, 0xed000, 0xb8000, 0xfffdbf70, 0x15000, 1

L_0CBD:
    EvCameraWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D21
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0D03
    EvCameraMoveTo 2776, 0, 0xed000, 0x78000, 0x22000, 0x17000, 25
    VMJump L_0D1B

L_0D03:
    EvCameraMoveTo 2776, 0, 0xed000, 0xb8000, 0x22000, 0x17000, 25

L_0D1B:
    VMJump L_0D6A

L_0D21:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0D52
    EvCameraMoveTo 3544, 0, 0xed000, 0x78000, 0x19000, 0x15000, 25
    VMJump L_0D6A

L_0D52:
    EvCameraMoveTo 3544, 0, 0xed000, 0xb8000, 0x19000, 0x15000, 25

L_0D6A:
    FadeEx 3, 16, 0, 2
    EvCameraWait
    SEStop
    VMCall L_26B6
    SEWait
    EvCameraWait
    FadeExWait
    EvCameraMoveToDefault 20
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0DA7
    Plugin9_Cmd1016 0, 1
    VMJump L_0DAD

L_0DA7:
    Plugin9_Cmd1016 1, 1

L_0DAD:
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 255, Movement_2C00
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0DF8
    Plugin9_Cmd1016 0, 0
    ActorCmdExec 0x8023, Movement_2B44
    ActorCmdExec 255, Movement_2BD0
    ActorCmdWait
    ActorCmdExec 0x8023, Movement_2B58
    ActorCmdWait
    VMJump L_0E1A

L_0DF8:
    Plugin9_Cmd1016 1, 0
    ActorCmdExec 0x8023, Movement_2B30
    ActorCmdExec 255, Movement_2BE8
    ActorCmdWait
    ActorCmdExec 0x8023, Movement_2B64
    ActorCmdWait

L_0E1A:
    Plugin9_Cmd1020 0x803d
    KeysCmd_02D1 0x8024
    VMStackPush 0x803d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1148
    WorkGet 0x8025, 0x8021
    Cmd_01DD 13, 0x8025, 0
    VMCall L_2A32
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush 0x8025
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0EC5
    PedometerGet 0x8010
    VMStackPush 0x8010
    VMStackPushConst 100
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0E87
    MedalGive 158

L_0E87:
    Plugin9_Cmd1024 3, 0, 0x8008
    Plugin9_Cmd1024 4, 0, 0x8009
    VMStackPush 0x8009
    VMStackPushConst 3
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0EAE
    MedalGive 156

L_0EAE:
    VMStackPush 0x8008
    VMStackPush 0x8009
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EC5
    MedalGive 154

L_0EC5:
    VMStackPush 0x8025
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EE4
    MedalDiscover 154
    MedalDiscover 156
    MedalDiscover 158

L_0EE4:
    VMStackPush 0x8025
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F03
    MedalDiscover 155
    MedalDiscover 157
    MedalDiscover 159

L_0F03:
    VMStackPush 0x8025
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMStackPush 0x8025
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0F7F
    PedometerGet 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1000
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0F41
    MedalGive 159

L_0F41:
    Plugin9_Cmd1024 3, 0, 0x8008
    Plugin9_Cmd1024 4, 0, 0x8009
    VMStackPush 0x8009
    VMStackPushConst 5
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0F68
    MedalGive 157

L_0F68:
    VMStackPush 0x8008
    VMStackPush 0x8009
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F7F
    MedalGive 155

L_0F7F:
    VMStackPush 0x8021
    VMStackPush 0x8024
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1103
    WorkAdd 0x8024, 1
    Plugin9_Cmd1019 0x8024
    EvCameraInit
    EvCameraUnbind
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10B1
    VMCall L_2214
    WordSetPlayerName 0
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_101D
    // "[f000]Ā\u0001\u0000! Excuse me![f000]븁\u0000\nWe keep this a secret from the general\npublic, but this facility contains some[f000]븀\u0000\nvery special areas.[f000]븁\u0000\nAreas 6 and beyond are these\nspecial areas![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ExcuseWeKeepSecret, 0x8023, 2, 0
    EvCameraMoveTo 9688, 0, 0xed000, 0xb5000, 0, 0x39000, 30
    // "Did you notice this elevator?[f000]븁\u0000\nRide this elevator if you want to\nchallenge Areas 6 and beyond.[f000]븁\u0000\nBut, be careful! Areas 6 and beyond\nare outside of our administration.[f000]븁\u0000\nWe don't actually know what kind of\nTrainers are in there...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_DidNoticeElevatorRide, 0x8023, 2, 0
    // "Furthermore, it's rumored that Areas 6\nand beyond contain more floors per[f000]븀\u0000\narea than the earlier areas.[f000]븁\u0000\nIf you can't find the Trainer you're\nlooking for, try a different floor.[f000]븁\u0000\nJust use the elevator to move between\nthe floors of an area.[f000]븁\u0000\nAnyway, if you're thinking about\nchallenging Areas 6 and beyond, make[f000]븀\u0000\nsure you've trained your Pokémon[f000]븀\u0000\nsufficiently and are fully prepared![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_FurthermoreItsRumoredAreas, 0x8023, 2, 0
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8023, 4, 0
    VMJump L_1065

L_101D:
    // "Hey! [f000]Ā\u0001\u0000![f000]븁\u0000\nI'm letting you in on a secret here, but\nthis facility actually contains some very[f000]븀\u0000\nspecial areas.[f000]븁\u0000\nI'm talking about Areas 6 and beyond![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyImLettingSecret, 0x8023, 2, 0
    EvCameraMoveTo 9688, 0, 0xed000, 0xb5000, 0, 0x39000, 30
    // "Did you ever wonder about this elevator?[f000]븁\u0000\nTake a ride on it if you want to\nchallenge Areas 6 and beyond.[f000]븁\u0000\nBut, be careful! Areas 6 and beyond\nare outside of our administration.[f000]븁\u0000\nWe don't actually know what kind of\nTrainers are in there...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_DidEverWonderAbout, 0x8023, 2, 0
    // "Furthermore, it's rumored that Areas 6\nand beyond contain more floors per[f000]븀\u0000\narea than the earlier areas.[f000]븁\u0000\nIf you can't find the Trainer you're\nlooking for, try a different floor.[f000]븁\u0000\nJust use the elevator to move between\nthe floors of an area.[f000]븁\u0000\nAnyway, if you're thinking about\nchallenging Areas 6 and beyond, make[f000]븀\u0000\nsure you've trained your Pokémon[f000]븀\u0000\nsufficiently and are fully prepared![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_FurthermoreItsRumoredAreas_2, 0x8023, 2, 0
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8023, 4, 0

L_1065:
    MsgWaitAdvance
    ActorMsgClose
    EvCameraWait
    Plugin9_Cmd1025
    EvCameraMoveToDefault 20
    EvCameraWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1090
    MedalGive 146
    VMJump L_1094

L_1090:
    MedalGive 148

L_1094:
    VMStackPush 0x802c
    VMStackPush 0x8020
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10AB
    CallUnovaLinkKeyUnlock 2

L_10AB:
    VMJump L_10FD

L_10B1:
    VMStackPush 0x8021
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10F1
    VMCall L_22F5
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10E7
    MedalGive 147
    VMJump L_10EB

L_10E7:
    MedalGive 149

L_10EB:
    VMJump L_10FD

L_10F1:
    VMCall L_2214
    VMCall L_23C9

L_10FD:
    VMJump L_113E

L_1103:
    VMCall L_2214
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_112E
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8023, 2, 0
    VMJump L_113A

L_112E:
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8023, 2, 0

L_113A:
    LastKeyWait
    ActorMsgClose

L_113E:
    EvCameraRebind
    EvCameraEnd
    VMJump L_121C

L_1148:
    WordSetPlayerName 0
    VMStackPush EVENT_WORK_0x4191
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11E7
    Plugin9_Cmd1029 0x803c
    VMStackPush 0x803c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11B0
    Plugin9_Cmd1031 1
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_119E
    // "Excuse me, [f000]Ā\u0001\u0000.\nYou've decided to quit, then?[f000]븁\u0000\nI am quite sorry about this, but I'm\nafraid I have to take back the prize[f000]븀\u0000\nmoney you won during your challenge.[f000]븁\u0000\nThe total amount comes to $[f000]Ȇ\u0001\u0001.[f000]븁\u0000\nI wish you better luck next time!"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ExcuseYouveDecidedQuit, 0x8023, 4, 0
    VMJump L_11AA

L_119E:
    // "Hey! [f000]Ā\u0001\u0000!\nYou quit, eh?[f000]븁\u0000\nI'm sorry it's got to be like this, but\nI have to take back the prize money[f000]븀\u0000\nyou won during your challenge.[f000]븁\u0000\nThe total comes out to $[f000]Ȇ\u0001\u0001.[f000]븁\u0000\nBetter luck next time, huh?"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyQuitEhIm, 0x8023, 4, 0

L_11AA:
    VMJump L_11E1

L_11B0:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11D5
    // "So, you've decided to quit, [f000]Ā\u0001\u0000?[f000]븁\u0000\nToo bad, but better luck next time!"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_YouveDecidedQuitToo, 0x8023, 4, 0
    VMJump L_11E1

L_11D5:
    // "[f000]Ā\u0001\u0000! You quit?[f000]븁\u0000\nToo bad, but better luck next time!"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_QuitTooBadBut, 0x8023, 4, 0

L_11E1:
    VMJump L_1218

L_11E7:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_120C
    // "[f000]Ā\u0001\u0000! Excuse me!\nAre you OK?[f000]븁\u0000\nIt's too bad things didn't work out,\nbut better luck next time!"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ExcuseOkItsToo, 0x8023, 4, 0
    VMJump L_1218

L_120C:
    // "Hey! [f000]Ā\u0001\u0000!\nYou OK?[f000]븁\u0000\nIt's too bad things didn't work out,\nbut better luck next time!"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyOkItsToo, 0x8023, 4, 0

L_1218:
    LastKeyWait
    ActorMsgClose

L_121C:
    Plugin9_Cmd1021 0
    PedometerEnd
    PokePartyRecoverAll
    WorkSetConst 0x803d, 0
    WorkSetConst 0x803c, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x803e, 0
    WorkSetConst 0x803f, 0
    Plugin9_Cmd1023 0x8020
    VMCall L_1E4C
    Plugin9_Cmd1006 0x8021
    Plugin9_Cmd1024 0, 0x8021, 0x803f
    Plugin9_Cmd1024 6, 0x8021, 0x803e
    EvCameraInit
    EvCameraUnbind
    VMStackPush EVENT_WORK_0x4190
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12C8
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12AA
    EvCameraMoveTo 3672, 0, 0xed000, 0x68000, 0xfffe2f70, 0xfffe0000, 1
    VMJump L_12C2

L_12AA:
    EvCameraMoveTo 4056, 0, 0xed000, 0x68000, 0xfffd2f70, 0xfffed000, 1

L_12C2:
    VMJump L_1311

L_12C8:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12F9
    EvCameraMoveTo 3672, 0, 0xed000, 0x68000, 0x5d090, 0xfffe0000, 1
    VMJump L_1311

L_12F9:
    EvCameraMoveTo 4056, 0, 0xed000, 0x68000, 0x4d090, 0xfffed000, 1

L_1311:
    EvCameraWait
    FadeEx 3, 16, 0, 2
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_134E
    EvCameraMoveTo 3672, 0, 0xed000, 0x68000, 0x20000, 0xfffe0000, 20
    VMJump L_1366

L_134E:
    EvCameraMoveTo 4056, 0, 0xed000, 0x68000, 0x10000, 0xfffed000, 20

L_1366:
    EvCameraWait
    FadeExWait
    WorkGet 0x802d, 0x803f
    WorkAdd 0x802d, 0x803e
    WorkAdd 0x802d, 1
    WorkGet 0x802e, 0x8021
    WorkAdd 0x802e, 1
    WordSetNumber 0, 0x802e, 2
    WordSetNumber 1, 0x802d, 2
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_13B5
    // "[f000]봂\u0000Black Tower\n[f000]봂\u0000Area [f000]ȁ\u0001\u0000: [f000]ȁ\u0001\u0001F"
    SystemMsg Global10735_Text_BlackTowerAreaF, 2
    VMJump L_13BB

L_13B5:
    // "[f000]봂\u0000White Treehollow\n[f000]봂\u0000Area [f000]ȁ\u0001\u0000: B[f000]ȁ\u0001\u0001"
    SystemMsg Global10735_Text_WhiteTreehollowAreaB, 2

L_13BB:
    VMCall L_26B6
    SEWait
    InfoMsgClose
    EvCameraMoveToDefault 20
    Plugin9_Cmd1016 0, 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 255, Movement_2C00
    ActorCmdWait
    Plugin9_Cmd1016 0, 0
    WorkSetConst 0x803f, 0
    WorkSetConst 0x803e, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkGet 0x8028, 0x8011
    Plugin9_Cmd1022 0x8028, 0x8026
    Plugin9_Cmd1018 0x8026, 0x802b
    Plugin9_Cmd1011 0x8028, 0x8029
    Plugin9_Cmd1026 0x8028, 0x8030
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14BF
    TrainerClassBGMPlayPush 0x802b
    VMCall L_14EE
    Plugin9_Cmd1007
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14B1
    CallTrainerBattleEnd
    Plugin9_Cmd1009 0x8028, 1
    RecordAdd 49, 1
    VMStackPush 0x8030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1482
    VMCall L_158F
    ActorMsgClose
    VMCall L_2A2E

L_1482:
    Plugin9_Cmd1013 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14A5
    VMCall L_26DD
    VMJump L_14AB

L_14A5:
    VMCall L_2113

L_14AB:
    VMJump L_14B9

L_14B1:
    FieldOpenRestoreLCD
    VMCall L_00E0

L_14B9:
    VMJump L_14E8

L_14BF:
    Plugin9_Cmd1013 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14E2
    VMCall L_26DD
    VMJump L_14E8

L_14E2:
    VMCall L_2113

L_14E8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_14EE:
    WorkSetConst 0x8040, 0
    KeysCmd_02D1 0x8024
    Plugin9_Cmd1006 0x8021
    Plugin9_Cmd1023 0x8020
    VMStackPush 0x8030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_157D
    Plugin9_Cmd1024 11, 0, 0x8040
    VMStackPush 0x8040
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1565
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1553
    // "Hey, newcomer. You finally made it.\nI've been waiting for you.[f000]븁\u0000\nYou know what you have to do to\nopen that gate, right?[f000]븁\u0000\nLet's get this started, then!\nI'll determine whether you have the[f000]븀\u0000\nstrength to face the Boss Trainer[f000]븀\u0000\nor not![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyNewcomerFinallyMade, 0x8011, 0, 0
    VMJump L_155F

L_1553:
    // "Hey, newcomer. You finally made it.\nI've been waiting for you.[f000]븁\u0000\nYou know what you have to do to\nopen that gate, right?[f000]븁\u0000\nLet's get this started, then!\nI'll determine whether you have the[f000]븀\u0000\nstrength to face the Boss Trainer[f000]븀\u0000\nor not![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyNewcomerFinallyMade_2, 0x8011, 0, 0

L_155F:
    VMJump L_1577

L_1565:
    Plugin9_Cmd1027 1, 0x8031
    ActorMsg MSGFILE_SCRIPT, 0x8031, 0x8011, 0, 0

L_1577:
    VMJump L_1585

L_157D:
    TrainerSayMessage 0x8026, TRAINER_NONE, 0x8011

L_1585:
    ActorMsgClose
    WorkSetConst 0x8040, 0
    VMReturn

L_158F:
    WorkSetConst 0x8041, 0
    KeysCmd_02D1 0x8024
    Plugin9_Cmd1006 0x8021
    Plugin9_Cmd1023 0x8020
    Plugin9_Cmd1024 11, 0, 0x8041
    VMStackPush 0x8041
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_15F3
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_15E1
    // "I lost! You're pretty good![f000]븁\u0000\nWith that kind of strength, you\nshould be OK![f000]븁\u0000\nI'll open the gate for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_LostYourePrettyGood, 0x8011, 0, 0
    VMJump L_15ED

L_15E1:
    // "I lost...\nYou're pretty good, you know?[f000]븁\u0000\nWith that kind of strength, you\nshould be OK![f000]븁\u0000\nI'll open the gate for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_LostYourePrettyGood_2, 0x8011, 0, 0

L_15ED:
    VMJump L_1605

L_15F3:
    Plugin9_Cmd1027 0, 0x8031
    ActorMsg MSGFILE_SCRIPT, 0x8031, 0x8011, 0, 0

L_1605:
    WorkSetConst 0x8041, 0
    VMReturn

Script_10:
    ActorsPauseAll
    TrainerEyeGetTrainerID 0, 0x8028
    Plugin9_Cmd1022 0x8028, 0x8026
    Plugin9_Cmd1018 0x8026, 0x802b
    Plugin9_Cmd1011 0x8028, 0x8029
    Plugin9_Cmd1026 0x8028, 0x8030
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_16D5
    TrainerClassBGMPlayPush 0x802b
    TrainerEyeEventInit 0
    TrainerEyeEventStart
    TrainerGetActorID 0, 0x8027
    VMCall L_14EE
    Plugin9_Cmd1007
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_16CD
    CallTrainerBattleEnd
    Plugin9_Cmd1009 0x8028, 1
    RecordAdd 49, 1
    VMStackPush 0x8030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_169E
    VMCall L_158F
    ActorMsgClose
    VMCall L_2A2E

L_169E:
    Plugin9_Cmd1013 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_16C1
    VMCall L_26DD
    VMJump L_16C7

L_16C1:
    VMCall L_2113

L_16C7:
    VMJump L_16D5

L_16CD:
    FieldOpenRestoreLCD
    VMCall L_00E0

L_16D5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8043, 0
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8045, 0
    WorkSetConst 0x8046, 0
    WorkSetConst 0x8047, 0
    WorkSetConst 0x8048, 0
    GameGetVersion 0x802c
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    KeysCmd_02D1 0x8024
    Plugin9_Cmd1006 0x8021
    Plugin9_Cmd1023 0x8020
    Plugin9_Cmd1008 0x8046
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_176B
    VMStackPush 0x8021
    VMStackPush 0x8024
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1759
    WorkSetConst 0x8044, 33
    WorkSetConst 0x8045, 66
    VMJump L_1765

L_1759:
    WorkSetConst 0x8044, 11
    WorkSetConst 0x8045, 55

L_1765:
    VMJump L_179C

L_176B:
    VMStackPush 0x8021
    VMStackPush 0x8024
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1790
    WorkSetConst 0x8044, 22
    WorkSetConst 0x8045, 77
    VMJump L_179C

L_1790:
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8045, 44

L_179C:
    WorkAdd 0x8044, 0x8021
    WorkAdd 0x8045, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_17DA
    VMStackPush 0x8020
    VMStackPush 0x802c
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_17DA
    WorkAdd 0x8044, 1
    WorkAdd 0x8045, 1

L_17DA:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_17FF
    ActorMsg MSGFILE_SCRIPT, 0x8044, 0, 2, 0
    VMJump L_180B

L_17FF:
    ActorMsg MSGFILE_SCRIPT, 0x8044, 1, 2, 0

L_180B:
    ActorMsgClose
    CallTrainerBattle 0x8046, 0, 2
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1943
    CallTrainerBattleEnd
    RecordAdd 49, 1
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1859
    ActorMsg MSGFILE_SCRIPT, 0x8045, 0, 2, 0
    VMJump L_1865

L_1859:
    ActorMsg MSGFILE_SCRIPT, 0x8045, 1, 2, 0

L_1865:
    ActorMsgClose
    Plugin9_Cmd1021 1
    PlayerGetGPos 0x8047, 0x8048
    VMStackPush 0x8048
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_18BF
    ActorCmdExec 255, Movement_2CAC
    VMSleep 8
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_18B1
    ActorCmdExec 0, Movement_2CEC
    VMJump L_18B9

L_18B1:
    ActorCmdExec 1, Movement_2CEC

L_18B9:
    VMJump L_190D

L_18BF:
    VMStackPush 0x8048
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_190D
    ActorWalkRoute 255, 5, 8, 0, 8, 0
    VMSleep 8
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1905
    ActorCmdExec 0, Movement_2CD8
    VMJump L_190D

L_1905:
    ActorCmdExec 1, Movement_2CD8

L_190D:
    ActorCmdWait
    ActorWalkRoute 255, 6, 14, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_2BAC
    ActorCmdWait
    VMCall L_242D
    Plugin9_Cmd1001 4, 8, 7
    RTReserveScript 10742
    SEStop
    VMJump L_194B

L_1943:
    FieldOpenRestoreLCD
    VMCall L_00E0

L_194B:
    WorkSetConst 0x8048, 0
    WorkSetConst 0x8047, 0
    WorkSetConst 0x8046, 0
    WorkSetConst 0x8045, 0
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8043, 0
    WorkSetConst 0x8042, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8049, 0
    Plugin9_Cmd1023 0x8020
    Plugin9_Cmd1006 0x8021
    ActorCmdExec 255, Movement_2CA4
    ActorCmdWait
    ActorCmdExec 255, Movement_2C10
    ActorCmdWait
    ActorCmdExec 251, Movement_2C08
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_19DA
    WorkSetConst 0x8049, 140
    WorkAdd 0x8049, 0x8021
    ActorMsg MSGFILE_SCRIPT, 0x8049, 251, 2, 0
    VMJump L_19F2

L_19DA:
    WorkSetConst 0x8049, 146
    WorkAdd 0x8049, 0x8021
    ActorMsg MSGFILE_SCRIPT, 0x8049, 251, 2, 0

L_19F2:
    ActorMsgClose
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1A1B
    ActorWalkRoute 251, 6, 14, 4, 8, 1
    VMJump L_1A29

L_1A1B:
    ActorWalkRoute 251, 14, 7, 4, 8, 1

L_1A29:
    ActorCmdWait
    ActorDelete 251
    VMCall L_242D
    SEWait
    WorkSetConst 0x8049, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst 0x804a, 0
    Plugin9_Cmd1023 0x8020
    Plugin9_Cmd1006 0x8021
    ActorCmdExec 255, Movement_2CA4
    ActorCmdWait
    ActorCmdExec 255, Movement_2C10
    ActorCmdWait
    ActorCmdExec 251, Movement_2C08
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1AA2
    WorkSetConst 0x804a, 140
    WorkAdd 0x804a, 0x8021
    ActorMsg MSGFILE_SCRIPT, 0x804a, 251, 2, 0
    VMJump L_1ABA

L_1AA2:
    WorkSetConst 0x804a, 146
    WorkAdd 0x804a, 0x8021
    ActorMsg MSGFILE_SCRIPT, 0x804a, 251, 2, 0

L_1ABA:
    ActorMsgClose
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1ADD
    ActorCmdExec 255, Movement_2C18
    VMJump L_1AE5

L_1ADD:
    ActorCmdExec 255, Movement_2C30

L_1AE5:
    ActorCmdWait
    ActorWalkRoute 251, 6, 2, 4, 8, 1
    ActorCmdWait
    Plugin9_Cmd1016 0, 1
    ActorCmdExec 251, Movement_2C4C
    ActorCmdWait
    Plugin9_Cmd1016 0, 0
    ActorDelete 251
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1B32
    ActorCmdExec 255, Movement_2C5C
    VMJump L_1B3A

L_1B32:
    ActorCmdExec 255, Movement_2C6C

L_1B3A:
    ActorCmdWait
    WorkSetConst 0x804a, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    Plugin9_Cmd1023 0x8020
    ActorCmdExec 255, Movement_2CB4
    ActorWalkRoute 251, 6, 1, 4, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_2C10
    ActorCmdWait
    ActorWalkRoute 251, 3, 7, 4, 8, 1
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1B9F
    WorkSetConst 0x8031, 153
    VMJump L_1BA5

L_1B9F:
    WorkSetConst 0x8031, 156

L_1BA5:
    ActorMsg MSGFILE_SCRIPT, 0x8031, 251, 2, 0
    ActorMsgClose
    ActorWalkRoute 251, 6, 0, 4, 8, 0
    ActorCmdWait
    ActorCmdExec 251, Movement_2C8C
    ActorCmdWait
    ActorDelete 251
    VMCall L_242D
    SEWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    Plugin9_Cmd1023 0x8020
    ActorCmdExec 255, Movement_2CB4
    ActorCmdWait
    ActorCmdExec 255, Movement_2C10
    ActorCmdWait
    ActorWalkRoute 251, 3, 7, 4, 8, 1
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1C28
    WorkSetConst 0x8031, 152
    VMJump L_1C2E

L_1C28:
    WorkSetConst 0x8031, 155

L_1C2E:
    ActorMsg MSGFILE_SCRIPT, 0x8031, 251, 2, 0
    ActorMsgClose
    ActorWalkRoute 251, 6, 13, 4, 8, 0
    ActorCmdWait
    ActorCmdExec 251, Movement_2C94
    ActorCmdWait
    ActorDelete 251
    VMCall L_242D
    SEWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    Plugin9_Cmd1023 0x8020
    ActorWalkRoute 251, 1, 7, 0, 8, 0
    ActorCmdExec 255, Movement_2CA4
    ActorCmdWait
    ActorCmdExec 255, Movement_2C10
    ActorCmdWait
    ActorWalkRoute 251, 6, 4, 0, 8, 0
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1CBF
    WorkSetConst 0x8031, 154
    VMJump L_1CC5

L_1CBF:
    WorkSetConst 0x8031, 157

L_1CC5:
    ActorMsg MSGFILE_SCRIPT, 0x8031, 251, 2, 0
    ActorMsgClose
    ActorWalkRoute 251, 0, 7, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 251, Movement_2CBC
    ActorCmdWait
    ActorDelete 251
    VMCall L_242D
    SEWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    Plugin9_Cmd1023 0x8020
    ActorCmdExec 255, Movement_2C10
    ActorCmdWait
    ActorCmdExec 255, Movement_2C84
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1D40
    ActorWalkRoute 1, 6, 10, 4, 8, 1
    VMJump L_1D4E

L_1D40:
    ActorWalkRoute 0, 6, 10, 4, 8, 1

L_1D4E:
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1D6F
    WorkSetConst 0x8031, 158
    VMJump L_1D75

L_1D6F:
    WorkSetConst 0x8031, 159

L_1D75:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1D9A
    ActorMsg MSGFILE_SCRIPT, 0x8031, 1, 2, 0
    VMJump L_1DA6

L_1D9A:
    ActorMsg MSGFILE_SCRIPT, 0x8031, 0, 2, 0

L_1DA6:
    ActorCmdWait
    ActorMsgClose
    ActorWalkRoute 255, 7, 11, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_2BC4
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1DE5
    ActorCmdExec 1, Movement_2C9C
    VMJump L_1DED

L_1DE5:
    ActorCmdExec 0, Movement_2C9C

L_1DED:
    VMSleep 5
    ActorCmdExec 255, Movement_2CC4
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1E18
    ActorDelete 1
    VMJump L_1E1C

L_1E18:
    ActorDelete 0

L_1E1C:
    VMCall L_242D
    SEWait
    ActorWalkRoute 255, 6, 11, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_2BA0
    ActorCmdWait
    FlagSet EVENT_FLAG_0x0352
    FlagSet EVENT_FLAG_0x0353
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_1E4C:
    Plugin9_Cmd1023 0x8020
    Plugin9_Cmd1006 0x8021
    GameGetVersion 0x802c
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1EC7
    FlagGet EVENT_FLAG_0x0110, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1EC1
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1EAB
    ActorNew 6, 8, 0, 251, 48, 0
    VMJump L_1EB9

L_1EAB:
    ActorNew 6, 8, 0, 251, 47, 0

L_1EB9:
    RTReserveScript 10748
    FlagSet EVENT_FLAG_0x0110

L_1EC1:
    VMJump L_2111

L_1EC7:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1F36
    FlagGet EVENT_FLAG_0x0111, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1F30
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1F1A
    ActorNew 6, 8, 0, 251, 74, 0
    VMJump L_1F28

L_1F1A:
    ActorNew 6, 8, 0, 251, 64, 0

L_1F28:
    RTReserveScript 10748
    FlagSet EVENT_FLAG_0x0111

L_1F30:
    VMJump L_2111

L_1F36:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1FB8
    VMStackPush 0x802c
    VMStackPush 0x8020
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1FB2
    FlagGet EVENT_FLAG_0x0112, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1FB2
    RTReserveScript 10747
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1FA0
    ActorNew 6, 8, 0, 251, 73, 0
    VMJump L_1FAE

L_1FA0:
    ActorNew 6, 8, 0, 251, 46, 0

L_1FAE:
    FlagSet EVENT_FLAG_0x0112

L_1FB2:
    VMJump L_2111

L_1FB8:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_203A
    VMStackPush 0x802c
    VMStackPush 0x8020
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2034
    FlagGet EVENT_FLAG_0x0113, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2034
    RTReserveScript 10747
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2022
    ActorNew 6, 8, 0, 251, 73, 0
    VMJump L_2030

L_2022:
    ActorNew 6, 8, 0, 251, 46, 0

L_2030:
    FlagSet EVENT_FLAG_0x0113

L_2034:
    VMJump L_2111

L_203A:
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_20BC
    VMStackPush 0x802c
    VMStackPush 0x8020
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_20B6
    FlagGet EVENT_FLAG_0x0114, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_20B6
    RTReserveScript 10747
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_20A4
    ActorNew 6, 8, 0, 251, 73, 0
    VMJump L_20B2

L_20A4:
    ActorNew 6, 8, 0, 251, 46, 0

L_20B2:
    FlagSet EVENT_FLAG_0x0114

L_20B6:
    VMJump L_2111

L_20BC:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2111
    VMStackPush 0x802c
    VMStackPush 0x8020
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2111
    FlagGet EVENT_FLAG_0x01cc, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2111
    ActorNew 6, 8, 0, 251, 304, 0
    RTReserveScript 10747
    FlagSet EVENT_FLAG_0x01cc

L_2111:
    VMReturn

L_2113:
    WorkSetConst 0x804b, 0
    WorkSetConst 0x804c, 0
    WorkSetConst 0x804d, 0
    WorkSetConst 0x804e, 0
    WorkGet 0x8028, 0x8011
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_215C
    WorkSetConst 0x804b, 88
    WorkSetConst 0x804c, 89
    WorkSetConst 0x804d, 90
    VMJump L_216E

L_215C:
    WorkSetConst 0x804b, 91
    WorkSetConst 0x804c, 92
    WorkSetConst 0x804d, 93

L_216E:
    Plugin9_Cmd1022 0x8028, 0x8026
    Plugin9_Cmd1014 0x8026, 0x804e
    VMStackPush 0x804e
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_21EA
    ActorMsg MSGFILE_SCRIPT, 0x804b, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_21D4
    ActorMsgClose
    VMCall L_2A0C
    ActorMsg MSGFILE_SCRIPT, 0x804d, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    Plugin9_Cmd1015 0x8026, 1
    VMJump L_21E4

L_21D4:
    ActorMsg MSGFILE_SCRIPT, 0x804c, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_21E4:
    VMJump L_21FA

L_21EA:
    ActorMsg MSGFILE_SCRIPT, 0x804d, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_21FA:
    WorkSetConst 0x804e, 0
    WorkSetConst 0x804d, 0
    WorkSetConst 0x804c, 0
    WorkSetConst 0x804b, 0
    VMReturn

L_2214:
    WorkSetConst 0x804f, 0
    WorkSetConst 0x8050, 0
    Plugin9_Cmd1024 9, 0, 0x804f
    Plugin9_Cmd1024 10, 0, 0x8050
    Plugin9_Cmd1023 0x8020
    WorkGet 0x8021, 0x8025
    WorkAdd 0x8021, 1
    WordSetPlayerName 0
    WordSetNumber 1, 0x8021, 2
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_226F
    // "[f000]Ā\u0001\u0000! Excuse me![f000]븁\u0000\nCongratulations on completing Area [f000]ȁ\u0001\u0001![f000]븁\u0000\nPlease accept this prize in honor of\nyour accomplishment![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ExcuseCongratulationsCompletingArea, 0x8023, 2, 0
    VMJump L_227B

L_226F:
    // "Hey! [f000]Ā\u0001\u0000![f000]븁\u0000\nLooks like you completed Area [f000]ȁ\u0001\u0001![f000]븁\u0000\nHere's a prize to commemorate\nyour success![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyLooksLikeCompleted, 0x8023, 2, 0

L_227B:
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x804f
    WorkSet 0x8001, 0x8050
    RTCallGlobal 2817
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst 0x8050, 0
    WorkSetConst 0x804f, 0
    VMReturn

L_22AB:
    WorkSetConst 0x8051, 0
    WorkSetConst 0x8052, 0
    Plugin9_Cmd1024 9, 0, 0x8051
    Plugin9_Cmd1024 10, 0, 0x8052
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8051
    WorkSet 0x8001, 0x8052
    RTCallGlobal 2817
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst 0x8052, 0
    WorkSetConst 0x8051, 0
    VMReturn

L_22F5:
    VMStackPush 0x802c
    VMStackPush 0x8020
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_238C
    WorkSetConst EVENT_WORK_0x4144, 1
    FlagReset EVENT_FLAG_0x03e4
    WordSetPlayerName 0
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_235A
    // "[f000]Ā\u0001\u0000! Excuse me![f000]븁\u0000\n...All of the areas? Can that be true?[f000]븁\u0000\nIncredible!\nCongratulations![f000]븁\u0000\nTo commemorate your conquering the\nareas, please accept this prize![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ExcuseAllAreasCan, 0x8023, 2, 0
    ActorMsgClose
    VMCall L_22AB
    // "Please continue aiming to become\nthe top Trainer in the land! Good luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_PleaseContinueAimingBecome, 0x8023, 2, 0
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8023, 4, 0
    VMJump L_2386

L_235A:
    // "Hey! [f000]Ā\u0001\u0000![f000]븁\u0000\n...All of the areas? Can that be true?[f000]븁\u0000\nYou sure know how to impress!\nCongratulations![f000]븁\u0000\nTo commemorate your conquering the\nareas, please accept this prize![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_HeyAllAreasCan, 0x8023, 2, 0
    ActorMsgClose
    VMCall L_22AB
    // "Don't get complacent, though![f000]븁\u0000\nKeep on aiming to become the top Trainer\nin the land![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_DontGetComplacentThough, 0x8023, 2, 0
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8023, 4, 0

L_2386:
    VMJump L_23C3

L_238C:
    VMCall L_2214
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_23B7
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8023, 2, 0
    VMJump L_23C3

L_23B7:
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8023, 2, 0

L_23C3:
    LastKeyWait
    ActorMsgClose
    VMReturn

L_23C9:
    Plugin9_Cmd1023 0x8020
    KeysCmd_02D1 0x8024
    WorkAdd 0x8024, 1
    WordSetNumber 0, 0x8024, 2
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_240F
    // "In addition, you will now be able to\nchallenge Area [f000]ȁ\u0001\u0000![f000]븁\u0000\nWe look forward to your continued\nsuccess in future challenges.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_AdditionWillNowAble, 0x8023, 2, 0
    // "Thank you. Please come again."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ThankPleaseComeAgain, 0x8023, 4, 0
    VMJump L_2427

L_240F:
    // "Oh yeah, you can also challenge\nArea [f000]ȁ\u0001\u0000 now![f000]븁\u0000\nGood luck in your future challenges.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_OhYeahCanAlso, 0x8023, 2, 0
    // "You come back, now."
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_ComeBackNow, 0x8023, 4, 0

L_2427:
    LastKeyWait
    ActorMsgClose
    VMReturn

L_242D:
    Plugin9_Cmd1023 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_244E
    SEPlay SEQ_SE_SW_MDUN_BL_05
    VMJump L_2452

L_244E:
    SEPlay SEQ_SE_SW_MDUN_WH_05

L_2452:
    VMReturn

L_2454:
    Plugin9_Cmd1023 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2475
    SEPlay SEQ_SE_SW_MDUN_BL_03
    VMJump L_2479

L_2475:
    SEPlay SEQ_SE_SW_MDUN_WH_03

L_2479:
    VMReturn

L_247B:
    WorkSetConst 0x8053, 0
    Plugin9_Cmd1024 2, 0, 0x8053
    Plugin9_Cmd1023 0x8020
    GameGetVersion 0x802c
    Plugin9_Cmd1006 0x8021
    VMStackPush 0x8053
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2511
    FlagGet EVENT_FLAG_0x0166, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2511
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2511
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_24FB
    ActorNew 6, 65535, 2, 251, 48, 0
    VMJump L_2509

L_24FB:
    ActorNew 6, 65535, 2, 251, 47, 0

L_2509:
    RTReserveScript 10749
    FlagSet EVENT_FLAG_0x0166

L_2511:
    VMStackPush 0x8053
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_258D
    FlagGet EVENT_FLAG_0x01db, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_258D
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_258D
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2577
    ActorNew 6, 7, 2, 251, 48, 0
    VMJump L_2585

L_2577:
    ActorNew 6, 7, 2, 251, 47, 0

L_2585:
    RTReserveScript 10750
    FlagSet EVENT_FLAG_0x01db

L_258D:
    VMStackPush 0x8053
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2609
    FlagGet EVENT_FLAG_0x0168, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2609
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2609
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_25F3
    ActorNew 65535, 7, 2, 251, 74, 0
    VMJump L_2601

L_25F3:
    ActorNew 65535, 7, 2, 251, 64, 0

L_2601:
    RTReserveScript 10751
    FlagSet EVENT_FLAG_0x0168

L_2609:
    WorkSetConst 0x8053, 0
    VMReturn

L_2611:
    WorkSetConst 0x8054, 0
    Plugin9_Cmd1024 5, 0, 0x8054
    FlagSet EVENT_FLAG_0x0352
    FlagSet EVENT_FLAG_0x0353
    FlagGet EVENT_FLAG_0x0167, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_26AE
    VMStackPush 0x8054
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_26AE
    Plugin9_Cmd1023 0x8020
    GameGetVersion 0x802c
    VMStackPush 0x8020
    VMStackPush 0x802c
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_26AE
    Plugin9_Cmd1006 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_26AE
    RTReserveScript 10752
    FlagSet EVENT_FLAG_0x0167
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_26AA
    FlagReset EVENT_FLAG_0x0352
    VMJump L_26AE

L_26AA:
    FlagReset EVENT_FLAG_0x0353

L_26AE:
    WorkSetConst 0x8054, 0
    VMReturn

L_26B6:
    Plugin9_Cmd1023 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_26D7
    SEPlay SEQ_SE_SW_MDUN_BL_04
    VMJump L_26DB

L_26D7:
    SEPlay SEQ_SE_SW_MDUN_WH_04

L_26DB:
    VMReturn

L_26DD:
    WorkSetConst 0x8055, 0
    WorkSetConst 0x8056, 0
    WorkSetConst 0x8057, 0
    KeysCmd_02D1 0x8024
    Plugin9_Cmd1012 0x802a
    Plugin9_Cmd1017 0x8055
    WordSetNumber 0, 0x8055, 2
    Plugin9_Cmd1028 0x8056
    WordSetNumber 2, 0x8056, 1
    Plugin9_Cmd1006 0x8021
    Plugin9_Cmd1023 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_285A
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_273B
    VMJump L_2746

L_273B:
    WordSetPokeSpeciesWithArticle 1, 521
    VMJump L_2854

L_2746:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_2759
    VMJump L_2764

L_2759:
    WordSetPokeSpeciesWithArticle 1, 505
    VMJump L_2854

L_2764:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_2777
    VMJump L_2782

L_2777:
    WordSetPokeSpeciesWithArticle 1, 626
    VMJump L_2854

L_2782:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_2795
    VMJump L_27A0

L_2795:
    WordSetPokeSpeciesWithArticle 1, 526
    VMJump L_2854

L_27A0:
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_27B3
    VMJump L_27BE

L_27B3:
    WordSetPokeSpeciesWithArticle 1, 437
    VMJump L_2854

L_27BE:
    WorkCmpConst 0x8021, 5
    VMJumpIf CMP_EQ, L_27D1
    VMJump L_27DC

L_27D1:
    WordSetPokeSpeciesWithArticle 1, 474
    VMJump L_2854

L_27DC:
    WorkCmpConst 0x8021, 6
    VMJumpIf CMP_EQ, L_27EF
    VMJump L_27FA

L_27EF:
    WordSetPokeSpeciesWithArticle 1, 130
    VMJump L_2854

L_27FA:
    WorkCmpConst 0x8021, 7
    VMJumpIf CMP_EQ, L_280D
    VMJump L_2818

L_280D:
    WordSetPokeSpeciesWithArticle 1, 392
    VMJump L_2854

L_2818:
    WorkCmpConst 0x8021, 8
    VMJumpIf CMP_EQ, L_282B
    VMJump L_2836

L_282B:
    WordSetPokeSpeciesWithArticle 1, 635
    VMJump L_2854

L_2836:
    WorkCmpConst 0x8021, 9
    VMJumpIf CMP_EQ, L_2849
    VMJump L_2854

L_2849:
    WordSetPokeSpeciesWithArticle 1, 381
    VMJump L_2854

L_2854:
    VMJump L_2986

L_285A:
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_286D
    VMJump L_2878

L_286D:
    WordSetPokeSpeciesWithArticle 1, 505
    VMJump L_2986

L_2878:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_288B
    VMJump L_2896

L_288B:
    WordSetPokeSpeciesWithArticle 1, 521
    VMJump L_2986

L_2896:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_28A9
    VMJump L_28B4

L_28A9:
    WordSetPokeSpeciesWithArticle 1, 508
    VMJump L_2986

L_28B4:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_28C7
    VMJump L_28D2

L_28C7:
    WordSetPokeSpeciesWithArticle 1, 560
    VMJump L_2986

L_28D2:
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_28E5
    VMJump L_28F0

L_28E5:
    WordSetPokeSpeciesWithArticle 1, 344
    VMJump L_2986

L_28F0:
    WorkCmpConst 0x8021, 5
    VMJumpIf CMP_EQ, L_2903
    VMJump L_290E

L_2903:
    WordSetPokeSpeciesWithArticle 1, 468
    VMJump L_2986

L_290E:
    WorkCmpConst 0x8021, 6
    VMJumpIf CMP_EQ, L_2921
    VMJump L_292C

L_2921:
    WordSetPokeSpeciesWithArticle 1, 350
    VMJump L_2986

L_292C:
    WorkCmpConst 0x8021, 7
    VMJumpIf CMP_EQ, L_293F
    VMJump L_294A

L_293F:
    WordSetPokeSpeciesWithArticle 1, 389
    VMJump L_2986

L_294A:
    WorkCmpConst 0x8021, 8
    VMJumpIf CMP_EQ, L_295D
    VMJump L_2968

L_295D:
    WordSetPokeSpeciesWithArticle 1, 373
    VMJump L_2986

L_2968:
    WorkCmpConst 0x8021, 9
    VMJumpIf CMP_EQ, L_297B
    VMJump L_2986

L_297B:
    WordSetPokeSpeciesWithArticle 1, 380
    VMJump L_2986

L_2986:
    Plugin9_Cmd1024 11, 0, 0x8057
    VMStackPush 0x8057
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_29E8
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_29D6
    // "The real battle awaits you.[f000]븁\u0000\nThe Boss Trainer is the strongest\nTrainer in this area.[f000]븁\u0000\nThe Boss Trainer is to the west\nof here.[f000]븁\u0000\nGo through the gate, and give it\nyour best!"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_RealBattleAwaitsBoss, 0x8011, 2, 0
    VMJump L_29E2

L_29D6:
    // "The real battle awaits you.[f000]븁\u0000\nThe Boss Trainer is the strongest\nTrainer in this area.[f000]븁\u0000\nThe Boss Trainer is to the west\nof here.[f000]븁\u0000\nGo through the gate, and give it\nyour best!"
    ActorMsg MSGFILE_SCRIPT, Global10735_Text_RealBattleAwaitsBoss_2, 0x8011, 2, 0

L_29E2:
    VMJump L_29F4

L_29E8:
    ActorMsg MSGFILE_SCRIPT, 0x802a, 0x8011, 2, 0

L_29F4:
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8057, 0
    WorkSetConst 0x8056, 0
    WorkSetConst 0x8055, 0
    VMReturn

L_2A0C:
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMReturn

L_2A2E:
    Plugin9_Cmd1030
    VMReturn

L_2A32:
    Plugin9_Cmd1023 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2ABD
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2A66
    FlagReset EVENT_FLAG_0x03f8
    VMJump L_2AB7

L_2A66:
    VMStackPush 0x8025
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2A83
    FlagReset EVENT_FLAG_0x03f9
    VMJump L_2AB7

L_2A83:
    VMStackPush 0x8025
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2AA0
    FlagReset EVENT_FLAG_0x03fa
    VMJump L_2AB7

L_2AA0:
    VMStackPush 0x8025
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2AB7
    FlagReset EVENT_FLAG_0x03fb

L_2AB7:
    VMJump L_2B2B

L_2ABD:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2ADA
    FlagReset EVENT_FLAG_0x03fc
    VMJump L_2B2B

L_2ADA:
    VMStackPush 0x8025
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2AF7
    FlagReset EVENT_FLAG_0x03fd
    VMJump L_2B2B

L_2AF7:
    VMStackPush 0x8025
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2B14
    FlagReset EVENT_FLAG_0x03fe
    VMJump L_2B2B

L_2B14:
    VMStackPush 0x8025
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2B2B
    FlagReset EVENT_FLAG_0x03ff

L_2B2B:
    VMReturn
    .balign 4, 0

Movement_2B30:
    Move 12, 1
    Move 14, 1
    Move 35, 1
    Move 3, 1
    MoveEnd

Movement_2B44:
    Move 12, 1
    Move 15, 1
    Move 34, 1
    Move 2, 1
    MoveEnd

Movement_2B58:
    Move 14, 1
    Move 13, 1
    MoveEnd

Movement_2B64:
    Move 15, 1
    Move 13, 1
    MoveEnd

Movement_2B70:
    Move 12, 2
    Move 14, 2
    Move 12, 1
    MoveEnd

Movement_2B80:
    Move 12, 2
    Move 15, 2
    Move 12, 1
    MoveEnd

Movement_2B90:
    Move 12, 2
    Move 33, 1
    Move 1, 1
    MoveEnd

Movement_2BA0:
    Move 32, 1
    Move 0, 1
    MoveEnd

Movement_2BAC:
    Move 33, 1
    Move 1, 1
    MoveEnd

Movement_2BB8:
    Move 35, 1
    Move 3, 1
    MoveEnd

Movement_2BC4:
    Move 34, 1
    Move 2, 1
    MoveEnd

Movement_2BD0:
    Move 13, 1
    Move 15, 2
    Move 13, 2
    Move 32, 1
    Move 0, 1
    MoveEnd

Movement_2BE8:
    Move 13, 1
    Move 14, 2
    Move 13, 2
    Move 32, 1
    Move 0, 1
    MoveEnd

Movement_2C00:
    Move 13, 2
    MoveEnd

Movement_2C08:
    Move 12, 3
    MoveEnd

Movement_2C10:
    Move 75, 1
    MoveEnd

Movement_2C18:
    Move 34, 1
    Move 2, 1
    Move 14, 1
    Move 35, 1
    Move 3, 1
    MoveEnd

Movement_2C30:
    Move 13, 1
    Move 34, 1
    Move 2, 1
    Move 14, 1
    Move 35, 1
    Move 3, 1
    MoveEnd

Movement_2C4C:
    Move 12, 2
    Move 33, 1
    Move 1, 1
    MoveEnd

Movement_2C5C:
    Move 15, 1
    Move 33, 1
    Move 1, 1
    MoveEnd

Movement_2C6C:
    Move 15, 1
    Move 33, 1
    Move 1, 1
    MoveEnd
    Move 14, 3
    MoveEnd

Movement_2C84:
    Move 12, 2
    MoveEnd

Movement_2C8C:
    Move 12, 1
    MoveEnd

Movement_2C94:
    Move 13, 3
    MoveEnd

Movement_2C9C:
    Move 13, 6
    MoveEnd

Movement_2CA4:
    Move 13, 1
    MoveEnd

Movement_2CAC:
    Move 13, 2
    MoveEnd

Movement_2CB4:
    Move 15, 1
    MoveEnd

Movement_2CBC:
    Move 14, 3
    MoveEnd

Movement_2CC4:
    Move 34, 1
    Move 2, 1
    Move 33, 1
    Move 1, 1
    MoveEnd

Movement_2CD8:
    Move 34, 1
    Move 2, 1
    Move 62, 1
    Move 33, 1
    MoveEnd

Movement_2CEC:
    Move 33, 1
    MoveEnd
