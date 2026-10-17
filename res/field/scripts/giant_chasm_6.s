#include "asm/field_script.inc"
#include "text/script/giant_chasm_6.h"

// Script plugin 14, from the zones that use this file

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0075
    WorkSetConst EVENT_WORK_0x4020, 121
    WorkSetConst EVENT_WORK_0x4021, 365
    WorkSetConst EVENT_WORK_0x4022, 368
    VMJump L_0087

L_0075:
    WorkSetConst EVENT_WORK_0x4020, 121
    WorkSetConst EVENT_WORK_0x4021, 364
    WorkSetConst EVENT_WORK_0x4022, 367

L_0087:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x4072
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AE
    ActorSetGPos 0, 17, 0, 16, 1
    VMJump L_00D9

L_00AE:
    VMStackPush EVENT_WORK_0x4072
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D9
    ActorSetGPos 3, 15, 0, 22, 0
    ActorSetGPos 0, 17, 0, 16, 2

L_00D9:
    VMStackPush EVENT_WORK_0x4072
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x410d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0108
    ActorSetGPos 11, 15, 0, 18, 1

L_0108:
    VMHalt

Script_3:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x108000, 40
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0143
    PlayerSetSpecialSequence 1

L_0143:
    ActorWalkRoute 255, 15, 20, 1, 8, 0
    ActorCmdWait
    EvCameraWait
    VMSleep 10
    BGMPlay SEQ_BGM_E_NEW_G_CIS
    // "Ghetsis: The Giant Chasm![f000]븁\u0000\nThis is the spot where\nKyurem's power resonates.[f000]븁\u0000\nHere, Kyurem can use the\nfull extent of its power[f000]븀\u0000\nand easily cover all of Unova in ice![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisGiantChasmSpot, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06AC
    VMSleep 76
    SEPlay SEQ_SE_SW_GHETSIS_STICK_01
    SEWait
    ActorCmdWait
    // "Kyurem, come![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_KyuremCome, 0, 0, 0
    MsgWinCloseAll
    PlayFieldEffect 109
    ActorSetGPos 2, 14, 0, 16, 1
    ActorSetGPos 0, 18, 0, 16, 1
    FadeEx 12, 16, 0, 1
    FadeExWait
    PVPlay 646, 0
    // "Haaahraaan!"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_Haaahraaan, 2, 5, 0
    PVWait
    MsgWaitAdvance
    ActorMsgClose
    // "Ghetsis: I have a memory that\nhas continued to haunt me.[f000]븀\u0000\nJust one.[f000]븁\u0000\nThat unpleasant look in your eyes\nreminds me of it.[f000]븁\u0000\nThat aside, this is my gift to you\nto show my respect for making[f000]븀\u0000\nit this far.[f000]븁\u0000\nI'll freeze you solid right here\nso you can watch my glorious ascent![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisHaveMemoryHas, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06BC
    VMSleep 30
    SEPlay SEQ_SE_SW_GHETSIS_STICK_01
    SEWait
    ActorCmdWait
    // "Kyurem!\nGlaciate![f000]븁\u0000"
    ScreamMsg GiantChasm6_Text_KyuremGlaciate, 1
    InfoMsgClose_0039
    PVPlay 646, 0
    // "Haahraa!"
    ScreamMsg GiantChasm6_Text_Haahraa, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    Plugin14_Cmd1002 0
    VMSleep 300
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x13d000, 40
    VMSleep 200
    EvCameraWait
    Cmd_02E8 2, 0
    BGMFadeOutAll 1
    VMSleep 20
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025D
    // "[f000]븉\u0001\u0001[f000][ff00]\u0001\u0002Zekrom!\nFusion Bolt!![f000]븉\u0001\u0000[f000][ff00]\u0001\u0000"
    ScreamMsg GiantChasm6_Text_ZekromFusionBolt, 2
    VMJump L_0262

L_025D:
    // "[f000]븉\u0001\u0001[f000][ff00]\u0001\u0001Reshiram!\nFusion Flare!![f000][ff00]\u0001\u0000[f000]븉\u0001\u0000"
    ScreamMsg GiantChasm6_Text_ReshiramFusionFlare, 2

L_0262:
    VMSleep 60
    InfoMsgClose_0039
    Plugin14_Cmd1001 1
    Cmd_02E9 2, 0
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028F
    PlayFieldEffect 107
    VMJump L_0293

L_028F:
    PlayFieldEffect 108

L_0293:
    Plugin14_Cmd1002 1
    VMSleep 50
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x108000, 20
    EvCameraWait
    // "Ghetsis: So you came...[f000]븁\u0000\nThe freak without a human heart...\nN![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisCameFreakWithout, 0, 0, 0
    MsgWinCloseAll
    BGMPlay SEQ_BGM_EV_GIANTHOLE_01
    FlagSet EVENT_FLAG_0x09fc
    BGMAmbienceResume
    Plugin14_Cmd1004 1
    VMSleep 100
    FadeOutBlack
    FadeWait
    EvCameraRebind
    EvCameraEnd
    FlagReset EVENT_FLAG_0x0376
    FlagReset EVENT_FLAG_0x0378
    MapChangeCore ZONE_GIANT_CHASM_6, 15, 0, 21, 0
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9816, 0, 0xed000, 0xf8000, 0, 0x12c000, 1
    EvCameraWait
    ActorSetGPos 2, 14, 0, 16, 1
    ActorSetGPos 0, 17, 0, 16, 1
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0348
    Plugin14Cmd_ShiftActor 3, 0, 9
    VMJump L_0350

L_0348:
    Plugin14Cmd_ShiftActor 3, 0, 9

L_0350:
    Plugin14Cmd_ShiftActor 4, 4, 0
    FadeInBlackQ
    FadeWait
    FlagSet EVENT_FLAG_0x09fb
    FlagReset EVENT_FLAG_0x09fc
    VMSleep 7
    // "N: [f000]븉\u0001\u0001Reshiram told me\nKyurem is suffering![f000]븁\u0000\nI can't allow selfish humans\nto make Pokémon suffer![f000]븁\u0000\nAnd I like Unova.[f000]븁\u0000\nIt's the place that taught me\nhow to live as a human...[f000]븁\u0000\nIt's the place that made me notice the\nharmony between Pokémon and humans[f000]븀\u0000\nliving together...[f000]븁\u0000\nI will protect the Pokémon\nand humans who live here![f000]븉\u0001\u0000[f000]븁\u0000"
    // "N: [f000]븉\u0001\u0001Zekrom told me\nKyurem is suffering![f000]븁\u0000\nI can't allow selfish humans\nto make Pokémon suffer![f000]븁\u0000\nAnd I like Unova.[f000]븁\u0000\nIt's the place that taught me\nhow to live as a human...[f000]븁\u0000\nIt's the place that made me notice the\nharmony between Pokémon and humans[f000]븀\u0000\nliving together...[f000]븁\u0000\nI will protect the Pokémon\nand humans who live here![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_NReshiramToldKyurem, GiantChasm6_Text_NZekromToldKyurem, 4, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03A2
    PVPlay 644, 0
    // "Bazzazzazzash!"
    ScreamMsg GiantChasm6_Text_Bazzazzazzash, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMJump L_03B3

L_03A2:
    PVPlay 643, 0
    // "Preeeeaah!"
    ScreamMsg GiantChasm6_Text_Preeeeaah, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039

L_03B3:
    // "Ghetsis: Excellent!\nThat was a moving expression[f000]븀\u0000\nof your determination![f000]븁\u0000\nSo the education I provided\nto make you king wasn't[f000]븀\u0000\na complete waste, then![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisExcellentMovingExpression, 0, 1, 0
    MsgWinCloseAll
    // "But I still haven't forgotten that even\nthough I was kind enough to find you[f000]븀\u0000\nwhen you were living in the forest with[f000]븀\u0000\nPokémon, and take you in, and care[f000]븀\u0000\nfor you, in the end you were selfish[f000]븀\u0000\nand disrupted my plans.[f000]븁\u0000\nI was supposed to use your\nabilities to rule Unova![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_ButStillHaventForgotten, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_1190
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 0, Movement_1180
    ActorCmdWait
    // "But I'll forgive you for that as well.[f000]븁\u0000\nReshiram, which you were kind enough\nto bring with you, can melt ice![f000]븁\u0000\nNow you've saved me the work\nof searching for it![f000]븁\u0000\nWell, actually, I knew you'd appear\nif we fired ice missiles into Opelucid City[f000]븀\u0000\nand you noticed the change![f000]븁\u0000"
    // "But I'll forgive you for that as well.[f000]븁\u0000\nZekrom, which you were kind enough\nto bring with you, can shatter ice![f000]븁\u0000\nNow you've saved me the work\nof searching for it![f000]븁\u0000\nWell, actually, I knew you'd appear\nif we fired ice missiles into Opelucid City[f000]븀\u0000\nand you noticed the change![f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_ButIllForgiveWell_2, GiantChasm6_Text_ButIllForgiveWell, 0, 1, 0
    MsgWinCloseAll
    // "N: [f000]븉\u0001\u0001That's an ugly formula!\nIt won't work![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_NThatsUglyFormula, 4, 2, 0
    MsgWinCloseAll
    BGMFadeOutAll 12
    // "Ghetsis: Oh, but it will![f000]븁\u0000\nIf I use these![f000]븁\u0000\nThe DNA Splicers![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisOhButWill, 0, 1, 0
    MsgWinCloseAll
    BGMPlay SEQ_BGM_EV_GIANTHOLE_02
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0xf0000, 35
    VMSleep 20
    ActorCmdExec 0, Movement_06D4
    VMSleep 23
    SEPlay SEQ_SE_SW_GHETSIS_STICK_02
    BGMAmbienceResume
    EvCameraWait
    ActorCmdWait
    SEWait
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046C
    PlayFieldEffect 110
    VMJump L_0470

L_046C:
    PlayFieldEffect 111

L_0470:
    FadeOutBlackQ
    BGMPush 6
    FadeWait
    ActorDelete 2
    WorkSetConst EVENT_WORK_0x4020, 366
    ActorAdd 2
    FlagSet EVENT_FLAG_0x0378
    ActorDelete 3
    FieldClose
    Call3DDemo 16, 0
    FieldOpen
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04BB
    FieldClose
    Call3DDemo 17, 0
    FieldOpen
    VMJump L_04C5

L_04BB:
    FieldClose
    Call3DDemo 20, 0
    FieldOpen

L_04C5:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x125000, 1
    EvCameraWait
    ActorSetGPos 0, 17, 0, 16, 1
    ActorSetGPos 2, 15, 0, 16, 1
    ActorSetGPos 4, 17, 0, 19, 2
    Plugin14Cmd_ShiftActor 4, 65532, 0
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0536
    ActorNew 15, 18, 1, 251, 143, 0
    VMJump L_0544

L_0536:
    ActorNew 15, 18, 1, 251, 144, 0

L_0544:
    Plugin14Cmd_ShiftActor 251, 0, 8
    Plugin14_Cmd1003 251
    FadeInBlackQ
    BGMPop 0, 60
    FadeWait
    VMSleep 18
    // "N: [f000]븉\u0001\u0001?![f000]븁\u0000\nR-Reshiram!![f000]븉\u0001\u0000[f000]븁\u0000"
    // "N: [f000]븉\u0001\u0001?![f000]븁\u0000\nZ-Zekrom![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_NRReshiram, GiantChasm6_Text_NZZekrom, 4, 2, 0
    MsgWinCloseAll
    // "Kyurem!\nAbsorb Reshiram![f000]븀\u0000\nUse Absofusion![f000]븁\u0000"
    // "Kyurem!\nAbsorb Zekrom![f000]븀\u0000\nUse Absofusion![f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_KyuremAbsorbReshiramUse, GiantChasm6_Text_KyuremAbsorbZekromUse, 0, 1, 0
    MsgWinCloseAll
    FadeOutBlackQ
    BGMPush 6
    FadeWait
    ActorDelete 251
    FlagSet EVENT_FLAG_0x0377
    ActorDelete 2
    EvCameraRebind
    EvCameraEnd
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05C3
    FieldClose
    Call3DDemo 18, 0
    FieldOpen
    FieldClose
    Call3DDemo 19, 0
    FieldOpen
    VMJump L_05D7

L_05C3:
    FieldClose
    Call3DDemo 21, 0
    FieldOpen
    FieldClose
    Call3DDemo 22, 0
    FieldOpen

L_05D7:
    FlagReset EVENT_FLAG_0x0379
    ActorAdd 1
    ActorSetGPos 0, 17, 0, 16, 1
    ActorSetGPos 255, 15, 0, 20, 0
    ActorSetGPos 4, 17, 0, 19, 0
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0x108000, 1
    EvCameraWait
    FadeInBlackQ
    BGMPop 0, 60
    FadeWait
    // "N: [f000]븉\u0001\u0001...![f000]븁\u0000\nI never would have believed that\nPokémon could fuse together...[f000]븀\u0000\nThat there was a formula like this...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_NNeverWouldHave, 4, 0, 0
    MsgWinCloseAll
    // "Ghetsis: You fool...[f000]븁\u0000\nLast time, I was going to use\nyou to capture people's hearts[f000]븀\u0000\nand minds to rule them![f000]븁\u0000\nBut this time, I'm simply going to use\noverwhelming power and rule[f000]븀\u0000\nwith an iron fist![f000]븁\u0000\nDo you understand?\nIf you had simply become king,[f000]븀\u0000\nUnova would have remained beautiful![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisFoolLastTime, 0, 0, 0
    MsgWinCloseAll
    VMSleep 8
    // "Come now, Trainer.\nThis time no one will save you![f000]븁\u0000\nBut to make things interesting,\nI'll give you a chance.[f000]븁\u0000\nLet's see if you can stop\nTHIS Kyurem![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_ComeNowTrainerTime, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_11A0
    ActorCmdWait
    // "What's this?\nYour Poké Balls are trembling.[f000]븁\u0000\nCould your Pokémon\nbe shaking with rage?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_WhatsPokeBallsTrembling, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06D4
    VMSleep 25
    SEPlay SEQ_SE_SW_GHETSIS_STICK_01
    SEWait
    ActorCmdWait
    // "No!\nThat's not possible![f000]븁\u0000\nSimple tools don't have emotion\nor thought![f000]븁\u0000\nCome!\nChallenge Kyurem![f000]븁\u0000\nJust so you know,\ncatching it is impossible![f000]븁\u0000\nMy cane emits signals that disrupt\nthe function of all Poké Balls![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_NoThatsNotPossible, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 10
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x4072, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_06AC:
    Move 11, 3
    Move 1, 1
    Move 182, 1
    MoveEnd

Movement_06BC:
    Move 1, 1
    Move 182, 1
    MoveEnd
    Move 10, 1
    Move 1, 1
    MoveEnd

Movement_06D4:
    Move 182, 1
    MoveEnd
    Move 35, 1
    Move 63, 1
    Move 33, 1
    MoveEnd

L_06EC:
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0710
    // "Bazzash!"
    ScreamMsg GiantChasm6_Text_Bazzash, 2
    PVPlay 644, 0
    VMJump L_071B

L_0710:
    // "Preeeeaah!"
    ScreamMsg GiantChasm6_Text_Preeeeaah_2, 2
    PVPlay 643, 0

L_071B:
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    PVPlay 646, 0
    // "Haaahraaan!"
    ScreamMsg GiantChasm6_Text_Haaahraaan_2, 5
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0750
    // "Kyurem and Zekrom\nwere separated!"
    InfoMsg GiantChasm6_Text_KyuremZekromWereSeparated, 2
    VMJump L_0755

L_0750:
    // "Kyurem and Reshiram\nwere separated!"
    InfoMsg GiantChasm6_Text_KyuremReshiramWereSeparated, 2

L_0755:
    MsgWaitAdvance
    InfoMsgClose_0039
    Cmd_02E9 1, 0
    ActorCmdExec 255, Movement_1190
    ActorCmdWait
    // "Ghetsis: I can't believe it!\nThe White Kyurem I went to[f000]븀\u0000\nall the trouble of preparing![f000]븁\u0000\nHow irritating![f000]븁\u0000\nNow I have to go recapture\nKyurem, don't I?[f000]븁\u0000\nBut first, I'll take down this disgusting\nTrainer with my own hand![f000]븁\u0000\nThis time I WILL succeed!\nNo matter what they try,[f000]븀\u0000\nno one will be able to stop me![f000]븁\u0000"
    // "Ghetsis: I can't believe it!\nThe Black Kyurem I went to[f000]븀\u0000\nall the trouble of preparing![f000]븁\u0000\nHow irritating![f000]븁\u0000\nNow I have to go recapture\nKyurem, don't I?[f000]븁\u0000\nBut first, I'll take down this disgusting\nTrainer with my own hand![f000]븁\u0000\nThis time I WILL succeed!\nNo matter what they try,[f000]븀\u0000\nno one will be able to stop me![f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_GhetsisCantBelieveWhite, GiantChasm6_Text_GhetsisCantBelieveBlack, 0, 5, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    // "N: [f000]븉\u0001\u0001...![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_N, 4, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_FLD_176
    PokePartyRecoverAll
    SEWait
    WorkSetConst EVENT_WORK_0x4072, 3
    VMCall L_08D4
    VMReturn

Script_10:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C3
    PlayerSetSpecialSequence 1

L_07C3:
    WorkCmpConst 0x8022, 13
    VMJumpIf CMP_EQ, L_07D6
    VMJump L_07E4

L_07D6:
    ActorCmdExec 255, Movement_0888
    VMJump L_0868

L_07E4:
    WorkCmpConst 0x8022, 14
    VMJumpIf CMP_EQ, L_07F7
    VMJump L_0805

L_07F7:
    ActorCmdExec 255, Movement_0894
    VMJump L_0868

L_0805:
    WorkCmpConst 0x8022, 15
    VMJumpIf CMP_EQ, L_0818
    VMJump L_0826

L_0818:
    ActorCmdExec 255, Movement_08A4
    VMJump L_0868

L_0826:
    WorkCmpConst 0x8022, 16
    VMJumpIf CMP_EQ, L_0839
    VMJump L_0847

L_0839:
    ActorCmdExec 255, Movement_08B4
    VMJump L_0868

L_0847:
    WorkCmpConst 0x8022, 17
    VMJumpIf CMP_EQ, L_085A
    VMJump L_0868

L_085A:
    ActorCmdExec 255, Movement_08C4
    VMJump L_0868

L_0868:
    ActorCmdWait
    // "Ghetsis: I'll take down this\ndisgusting Trainer with my own hand![f000]븁\u0000\nThis time I WILL succeed!\nNo matter what they try,[f000]븀\u0000\nno one will be able to stop me![f000]븁\u0000"
    // "Ghetsis: I'll take down this\ndisgusting Trainer with my own hand![f000]븁\u0000\nThis time I WILL succeed!\nNo matter what they try,[f000]븀\u0000\nno one will be able to stop me![f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_GhetsisIllTakeDown_2, GiantChasm6_Text_GhetsisIllTakeDown, 0, 5, 0
    MsgWinCloseAll
    VMCall L_08D4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0888:
    Move 12, 7
    Move 15, 2
    MoveEnd

Movement_0894:
    Move 14, 1
    Move 12, 7
    Move 15, 2
    MoveEnd

Movement_08A4:
    Move 14, 2
    Move 12, 7
    Move 15, 2
    MoveEnd

Movement_08B4:
    Move 14, 3
    Move 12, 7
    Move 15, 2
    MoveEnd

Movement_08C4:
    Move 14, 4
    Move 12, 7
    Move 15, 2
    MoveEnd

L_08D4:
    CallTrainerBattle TRAINER_TEAM_PLASMA_GHETSIS, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0941
    FlagSet EVENT_FLAG_0x0375
    FlagReset EVENT_FLAG_0x03c8
    FlagReset EVENT_FLAG_0x03c9
    FlagSet EVENT_FLAG_0x0379
    ActorDelete 0
    ActorAdd 5
    ActorSetGPos 255, 15, 0, 16, 3
    ActorSetGPos 5, 17, 0, 16, 2
    ActorSetGPos 3, 15, 0, 22, 0
    ActorCmdExec 4, Movement_1158
    ActorCmdWait
    CallTrainerBattleEnd
    VMJump L_0943

L_0941:
    CallTrainerLose

L_0943:
    ActorCmdExec 5, Movement_0C6C
    ActorCmdWait
    // "Ghetsis: How can this be?[f000]븁\u0000\nI'm the creator of Team Plasma!\nI'm perfect![f000]븁\u0000\nI'm the absolute ruler\nwho will change the world![f000]븁\u0000\nAnd I've lost to some unknown\nTrainer not once, but TWICE?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisHowCanIm, 5, 5, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0C58
    ActorCmdWait
    // "I can't accept this!\nThis isn't possible![f000]븁\u0000\nI can't be bested by\nfools who can't even[f000]븀\u0000\nuse Pokémon correctly![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_CantAcceptIsntPossible, 5, 5, 0
    MsgWinCloseAll
    BGMPlay SEQ_BGM_EV_GIANTHOLE_03
    // "N: [f000]븉\u0001\u0001It's hard to call you this, but...[f000]븁\u0000\nFather!\nPlease understand.[f000]븁\u0000\nPokémon are not tools.[f000]븁\u0000\nPokémon and humans take\neach other to greater heights.[f000]븀\u0000\nThey are our wonderful partners.[f000]븁\u0000\nSome humans understand this.[f000]븁\u0000\nWhy can't you?[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_NItsHardCall, 4, 6, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_1178
    ActorCmdWait
    // "Ghetsis: Shut your mouth![f000]븁\u0000\nShut up! Shut up! Shut UP![f000]븁\u0000\nDon't talk like a person, you freak!\nNo real person could talk to Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisShutMouthShut, 5, 5, 1
    MsgWinCloseAll
    ActorNew 18, 17, 1, 251, 182, 0
    ActorCmdExec 251, Movement_0C7C
    ActorCmdWait
    // "Shadow Triad: Lord Ghetsis has...\nlost control...[f000]븁\u0000\nWe'll take it from here...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_ShadowTriadLordGhetsis, 251, 5, 0
    MsgWinCloseAll
    // "N: [f000]븉\u0001\u0001OK...[f000]븁\u0000\nWithout Father,\nTeam Plasma is...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_NOkWithoutFather, 4, 0, 0
    MsgWinCloseAll
    // "Shadow Triad: Farewell...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_ShadowTriadFarewell, 251, 5, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_1178
    ActorCmdWait
    ActorCmdExec 251, Movement_0C84
    VMSleep 2
    ActorDelete 5
    ActorCmdWait
    ActorDelete 251
    ActorAdd 6
    VMSleep 16
    ActorCmdExec 4, Movement_0C90
    VMSleep 8
    ActorCmdExec 255, Movement_1180
    ActorCmdWait
    // "N: [f000]븉\u0001\u0001On behalf of everyone...[f000]븁\u0000\nThank you.[f000]븁\u0000\nKyurem is fine.[f000]븁\u0000\nNow, it has lost its power,\nbut it will come here again.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_NBehalfEveryoneThank, 4, 6, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_1180
    ActorCmdWait
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A63
    // "Bazzash!"
    InfoMsg GiantChasm6_Text_Bazzash_2, 1
    PVPlay 644, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0A74

L_0A63:
    // "Preeeeaah!"
    InfoMsg GiantChasm6_Text_Preeeeaah_3, 1
    PVPlay 643, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll

L_0A74:
    ActorCmdExec 4, Movement_1178
    ActorCmdWait
    // "N: [f000]븉\u0001\u0001Reshiram says thank you as well![f000]븁\u0000\nThat's right...\nI can talk with Pokémon.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "N: [f000]븉\u0001\u0001Zekrom says thank you as well![f000]븁\u0000\nThat's right...\nI can talk with Pokémon.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_NReshiramSaysThank, GiantChasm6_Text_NZekromSaysThank, 4, 6, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_1190
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 4, Movement_1178
    ActorCmdWait
    Cmd_02B4 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0AD7
    Cmd_02B5 0, 1
    // "[f000]븉\u0001\u0001On that day two years ago, [f000]Ā\u0001\u0001\nand Alder taught me something...[f000]븁\u0000\nBy accepting different ideas, this world\ncreates a chemical reaction...[f000]븁\u0000\nSo I met many different Pokémon\nand people and heard so much...[f000]븁\u0000\nAnd that's how my world\nquietly grew bigger...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_DayTwoYearsAgo_2, 4, 6, 0
    VMJump L_0AE3

L_0AD7:
    // "[f000]븉\u0001\u0001On that day two years ago, a certain\nTrainer and Alder taught me something...[f000]븁\u0000\nBy accepting different ideas, this world\ncreates a chemical reaction...[f000]븁\u0000\nSo I met many different Pokémon\nand people and heard so much...[f000]븁\u0000\nAnd that's how my world\nquietly grew bigger...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_DayTwoYearsAgo, 4, 6, 0

L_0AE3:
    // "[f000]븉\u0001\u0001By being with Pokémon, humans\ncan continue toward new horizons.[f000]븁\u0000\nBy being with humans, Pokémon\ncan exhibit their true power.[f000]븁\u0000\nThat's what Reshiram taught me:\nthe truth for Pokémon and me.[f000]븁\u0000\nAnd someday both truth and ideals\nwill come together...[f000]븁\u0000\nThen Pokémon and humans will be\nfree from the oppression of Poké Balls.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001By being with Pokémon, humans\ncan continue toward new horizons.[f000]븁\u0000\nBy being with humans, Pokémon\ncan exhibit their true power.[f000]븁\u0000\nThat's what Zekrom taught me:\nthe ideal for Pokémon and me.[f000]븁\u0000\nAnd someday both ideals and truth\nwill come together...[f000]븁\u0000\nThen Pokémon and humans will be\nfree from the oppression of Poké Balls.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_ByBeingPokemonHumans_2, GiantChasm6_Text_ByBeingPokemonHumans, 4, 6, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_1140
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]븉\u0001\u0001You![f000]븁\u0000\nWhat are you and your Pokémon\nstriving for?[f000]븁\u0000\nYou should head to the Pokémon League\nand put your truths to the test![f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001You![f000]븁\u0000\nWhat are you and your Pokémon\nstriving for?[f000]븁\u0000\nYou should head to the Pokémon League\nand put your ideals to the test![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_WhatPokemonStrivingShould_2, GiantChasm6_Text_WhatPokemonStrivingShould, 4, 6, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 4
    ActorCmdExec 4, Movement_0C9C
    VMSleep 8
    ActorCmdExec 255, Movement_0C9C
    ActorCmdWait
    FadeExWait
    ActorDelete 4
    ActorDelete 3
    ActorSetGPos 255, 17, 0, 18, 2
    FadeEx 3, 16, 0, 4
    Plugin14_Cmd1004 0
    VMSleep 10
    FadeExWait
    VMSleep 160
    Plugin14_Cmd1005
    FlagReset EVENT_FLAG_0x09fb
    FlagReset EVENT_FLAG_0x0407
    ActorAdd 11
    BGMChangeMap
    ActorWalkRoute 11, 15, 18, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 11, Movement_1190
    ActorCmdWait
    // "Team Plasma's ship flew away...\nIs it over?"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_TeamPlasmasShipFlew, 11, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BBD
    // "I see...\nSo it's finally over.[f000]븁\u0000\nMore importantly, you rescued\nKyurem from Team Plasma, right?[f000]븀\u0000\nYou're really something![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_SeeItsFinallyOver, 11, 0, 0
    VMJump L_0BC9

L_0BBD:
    // "I suppose... You have to be the one\nwho decides when it's over.[f000]븁\u0000\nMore importantly, you rescued\nKyurem from Team Plasma, right?[f000]븀\u0000\nYou're really something![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_SupposeHaveOneWho, 11, 0, 0

L_0BC9:
    MsgWinCloseAll
    ActorCmdExec 11, Movement_1180
    ActorCmdWait
    // "I'm...gonna make sure Purrloin\ngets back to my sister![f000]븁\u0000\nStill can't take it out of\nthe Poké Ball yet, though...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_ImGonnaMakeSure, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_0CA8
    ActorCmdWait
    // "What are you gonna do?[f000]븁\u0000\n...\n...[f000]븁\u0000\nHe said go to the Pokémon League?\nThat's a good idea![f000]븁\u0000\nI mean, now you're the\nstrongest in Unova, right?[f000]븀\u0000\nGo prove it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_WhatGonnaHeSaid, 11, 0, 0
    // "Do you remember where we first\nmet up in the Giant Chasm?[f000]븁\u0000\nIf you follow the path from there,\nit goes out onto Route 23.[f000]븁\u0000\nVictory Road and the Pokémon League\nare just past there!"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_RememberWhereWeFirst, 11, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x016c
    WorkSetConst EVENT_WORK_0x4072, 4
    FlagSet EVENT_FLAG_0x036c
    FlagSet EVENT_FLAG_0x036e
    FlagSet EVENT_FLAG_0x036f
    FlagSet EVENT_FLAG_0x0372
    FlagSet EVENT_FLAG_0x0375
    FlagSet EVENT_FLAG_0x03c8
    FlagSet EVENT_FLAG_0x0376
    FlagSet EVENT_FLAG_0x0378
    FlagSet EVENT_FLAG_0x0356
    HollowRivalCmd_0262 0, 6
    HollowRivalCmd_0262 1, 36
    HollowRivalCmd_0262 2, 12
    HollowRivalCmd_0262 3, 7
    HollowRivalCmd_0262 4, 0
    VMReturn
    .balign 4, 0

Movement_0C58:
    Move 10, 1
    Move 11, 2
    Move 14, 1
    Move 33, 1
    MoveEnd

Movement_0C6C:
    Move 71, 1
    Move 11, 1
    Move 72, 1
    MoveEnd

Movement_0C7C:
    Move 184, 1
    MoveEnd

Movement_0C84:
    Move 185, 1
    Move 69, 1
    MoveEnd

Movement_0C90:
    Move 14, 2
    Move 12, 1
    MoveEnd

Movement_0C9C:
    Move 13, 1
    Move 9, 2
    MoveEnd

Movement_0CA8:
    Move 161, 1
    Move 35, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ghetsis: Come now![f000]븁\u0000\nI want to see your face at the moment\nyou lose all hope![f000]븁\u0000\nBattle to protect Unova![f000]븁\u0000\nI've prepared the finest stage, and\nit's wasted on a bit player like you![f000]븀\u0000\nLose and go down in flames!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_GhetsisComeNowWant, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "N: [f000]븉\u0001\u0001It's faint, but I can hear my friend.\nI can hear Reshiram's voice.[f000]븀\u0000\nIt says they can be separated again![f000]븁\u0000\nI beg you!\nPlease save my friend![f000]븁\u0000\nAnd all of Unova's\nPokémon and humans...[f000]븉\u0001\u0000"
    // "N: [f000]븉\u0001\u0001It's faint, but I can hear my friend.\nI can hear Zekrom's voice.[f000]븀\u0000\nIt says they can be separated again![f000]븁\u0000\nI beg you!\nPlease save my friend![f000]븁\u0000\nAnd all of Unova's\nPokémon and humans...[f000]븉\u0001\u0000"
    ActorMsgVersioned 1024, GiantChasm6_Text_NItsFaintBut_2, GiantChasm6_Text_NItsFaintBut, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you remember where we first\nmet up in the Giant Chasm?[f000]븁\u0000\nIf you follow the path from there,\nit goes out onto Route 23.[f000]븁\u0000\nVictory Road and the Pokémon League\nare just past there!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_RememberWhereWeFirst, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D29
    PlayerSetSpecialSequence 1

L_0D29:
    SEPlay SEQ_SE_MESSAGE
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf8000, 0, 0xff000, 6
    EvCameraWait
    GameGetVersion 0x8020
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 2048
    WorkOr 0x8024, 1
    WorkOr 0x8024, 16
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DA5
    ActorCmdExec 1, Movement_0EE0
    ActorCmdWait
    // "Bazzakyurom!"
    ScreamMsg GiantChasm6_Text_Bazzakyurom, 2
    PVPlay 646, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattleEx 646, 55, 2, 0x8024
    VMJump L_0DCA

L_0DA5:
    ActorCmdExec 1, Movement_0ED8
    ActorCmdWait
    // "Preeeahkyuram!"
    ScreamMsg GiantChasm6_Text_Preeeahkyuram, 2
    PVPlay 646, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattleEx 646, 55, 1, 0x8024

L_0DCA:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WildBattleIsVictory 0x8025
    WildBattleGetResult 0x8026
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E8D
    VMStackPush 0x8026
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E58
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FlagSet EVENT_FLAG_0x0379
    FlagReset EVENT_FLAG_0x0378
    ActorDelete 1
    ActorAdd 3
    ActorSetGPos 255, 15, 0, 16, 0
    ActorSetGPos 3, 15, 0, 22, 0
    ActorSetGPos 0, 17, 0, 16, 2
    ActorCmdExec 4, Movement_1158
    ActorCmdWait
    Plugin14_Cmd1007
    VMJump L_0E87

L_0E58:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E87
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorDelete 1
    ActorAdd 1
    FlagReset EVENT_FLAG_0x0379
    CallWildBattleEnd

L_0E87:
    VMJump L_0EA6

L_0E8D:
    VMStackPushFlag EVENT_FLAG_0x0379
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EA4
    FlagReset EVENT_FLAG_0x0379

L_0EA4:
    CallWildLose

L_0EA6:
    WorkCmpConst 0x8026, 2
    VMJumpIf CMP_EQ, L_0EB9
    VMJump L_0EC5

L_0EB9:
    VMCall L_06EC
    VMJump L_0EC5

L_0EC5:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0ED8:
    Move 191, 1
    MoveEnd

Movement_0EE0:
    Move 191, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's the cane Ghetsis was holding.[f000]븁\u0000\nWas he controlling Kyurem with it?"
    InfoMsg GiantChasm6_Text_ItsCaneGhetsisHolding, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorDelete 7
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 628
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x03eb
    FlagSet EVENT_FLAG_0x01e1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    PVPlay 646, 0
    // "Haaahraaan!"
    ScreamMsg GiantChasm6_Text_Haaahraaan_3, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 646, 70, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FB5
    FlagSet EVENT_FLAG_0x03ec
    ActorDelete 8
    VMStackPushFlag EVENT_FLAG_0x01e1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F8E
    FlagReset EVENT_FLAG_0x03eb
    ActorAdd 7

L_0F8E:
    VMStackPushFlag EVENT_FLAG_0x01e3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FAD
    ActorSetGPos 255, 15, 0, 16, 0

L_0FAD:
    CallWildBattleEnd
    VMJump L_0FB7

L_0FB5:
    CallWildLose

L_0FB7:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0FCE
    VMJump L_0FDC

L_0FCE:
    FlagSet EVENT_FLAG_0x00f9
    Cmd_00E4 2
    VMJump L_100C

L_0FDC:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0FFC
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0FFC
    VMJump L_100C

L_0FFC:
    // "Kyurem vanished into the darkness\nof the cave..."
    SystemMsg GiantChasm6_Text_KyuremVanishedIntoDarkness, 0
    MsgWaitAdvance
    InfoMsgClose
    VMJump L_100C

L_100C:
    VMStackPushFlag EVENT_FLAG_0x01e3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1025
    VMCall L_102B

L_1025:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_102B:
    FlagReset EVENT_FLAG_0x03f3
    ActorAdd 9
    ActorAdd 10
    ActorWalkRoute 9, 15, 18, 0, 8, 0
    VMSleep 4
    ActorWalkRoute 10, 14, 18, 0, 8, 0
    VMSleep 40
    ActorCmdExec 255, Movement_1180
    ActorCmdWait
    WordSetPlayerName 0
    // "Cheren: [f000]Ā\u0001\u0000!\nThat was Kyurem just now, right?[f000]븁\u0000\nSo that's the legendary Pokémon\nTeam Plasma, or should I say Ghetsis,[f000]븀\u0000\nwas using...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_CherenKyuremJustNow, 9, 6, 0
    MsgWinCloseAll
    // "Bianca: [f000]Ā\u0001\u0000, you're amazing!\nYou've met so many Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_BiancaYoureAmazingYouve, 10, 4, 0
    VMStackPushFlag EVENT_FLAG_0x0184
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10B3
    MsgWinCloseAll
    ActorCmdExec 10, Movement_1198
    ActorCmdWait
    // "Hey now! Your Pokédex should be\npretty full, right?[f000]븁\u0000\nI'm sure Professor Juniper will\nbe really happy if you show her![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_HeyNowPokedexShould, 10, 4, 0
    VMJump L_10BF

L_10B3:
    // "You showed Professor Juniper\nyour Pokédex, didn't you?[f000]븁\u0000\nShe told me about how\nhappy she was![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_ShowedProfessorJuniperPokedex, 10, 4, 0

L_10BF:
    MsgWinCloseAll
    // "Cheren: I'm sure she was![f000]븁\u0000\nIn Unova, actually in the whole world,\nthere are still many Pokémon[f000]븀\u0000\nthat are waiting to meet you, though![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_CherenImSureShe, 9, 6, 0
    MsgWinCloseAll
    // "Bianca: Still...\nHow many Pokémon[f000]븀\u0000\ncould there be in all?[f000]븁\u0000\nI think I'll go back to Nuvema Town\nand ask Professor Juniper![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm6_Text_BiancaStillHowMany, 10, 4, 0
    MsgWinCloseAll
    ActorWalkRoute 9, 15, 25, 0, 8, 0
    VMSleep 4
    ActorWalkRoute 10, 14, 25, 0, 8, 0
    ActorCmdWait
    ActorDelete 9
    ActorDelete 10
    FlagSet EVENT_FLAG_0x01e3
    FlagSet EVENT_FLAG_0x03f3
    FlagReset EVENT_FLAG_0x02e3
    VMReturn
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1134
    CallTrainerBattleEnd
    VMJump L_1136

L_1134:
    CallTrainerLose

L_1136:
    VMReturn
    Move 13, 1
    MoveEnd

Movement_1140:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_1158:
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

Movement_1178:
    Move 32, 1
    MoveEnd

Movement_1180:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_1190:
    Move 35, 1
    MoveEnd

Movement_1198:
    Move 75, 1
    MoveEnd

Movement_11A0:
    Move 159, 1
    MoveEnd
