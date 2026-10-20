#include "asm/field_script.inc"
#include "text/script/humilau_city.h"

// Script plugin 15, from the zones that use this file

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

Script_1:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0adc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007D
    WorkSetConst EVENT_WORK_0x4000, 1

L_007D:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40df
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009E
    ActorSetGPos 0, 787, 65532, 176, 2

L_009E:
    VMStackPushFlag EVENT_FLAG_0x0137
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_010F
    VMStackPushFlag EVENT_FLAG_0x0326
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0327
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00F4
    FlagSet EVENT_FLAG_0x0326
    FlagSet EVENT_FLAG_0x0327
    ActorDelete 2
    ActorDelete 3

L_00F4:
    VMStackPushFlag EVENT_FLAG_0x0328
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010F
    FlagReset EVENT_FLAG_0x0328
    ActorAdd 5

L_010F:
    VMHalt

Script_3:
    ActorsPauseAll
    PlayerGetDir 0x8020
    ActorCmdExec 0, Movement_0780
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_013A
    ActorCmdExec 255, Movement_0770

L_013A:
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: We'll get the DNA Splicers\nback for sure![f000]븁\u0000\nSo you should focus on\ndefeating the Gym Leader first!"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_WellGetDnaSplicers, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40de, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    ActorNew 792, 100, 1, 251, 230, 0
    Plugin15_Cmd1000 791, 65531, 151
    Plugin15_Cmd1001 251, 791, 156, 1
    // "Uihaa![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_Uihaa, 251, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorCmdExec 255, Movement_0768
    VMStackPush 0x8022
    VMStackPushConst 156
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BA
    ActorCmdExec 251, Movement_0220
    VMJump L_01C2

L_01BA:
    ActorCmdExec 251, Movement_0228

L_01C2:
    ActorCmdWait
    // "Sup, you must be here to\nchallenge the Pokémon Gym![f000]븁\u0000\nI'm the Gym Leader, Marlon.\nSorry to make you look for me, yo.[f000]븁\u0000\nI was swimmin' with the Pokémon,\nand it felt real good,[f000]븀\u0000\nso I kept goin' and goin'.[f000]븁\u0000\nI'll be waitin' in the Gym, 'K?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_SupMustHereChallenge, 251, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 156
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F7
    ActorCmdExec 251, Movement_0234
    VMSleep 40
    VMJump L_0203

L_01F7:
    ActorCmdExec 251, Movement_0240
    VMSleep 32

L_0203:
    ActorCmdExec 255, Movement_0778
    ActorCmdWait
    ActorDelete 251
    WorkSetConst EVENT_WORK_0x411f, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0220:
    Move 15, 2
    MoveEnd

Movement_0228:
    Move 13, 1
    Move 15, 2
    MoveEnd

Movement_0234:
    Move 15, 1
    Move 13, 9
    MoveEnd

Movement_0240:
    Move 15, 1
    Move 13, 8
    MoveEnd

Script_4:
    ActorsPauseAll
    ActorCmdExec 0, Movement_0780
    ActorCmdWait
    ActorWalkRoute 0, 784, 176, 1, 8, 0
    ActorCmdExec 255, Movement_0388
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: You got all of the Badges!\nYou're really something![f000]븁\u0000\nUsually, you'd go to\nthe Pokémon League now, but...[f000]븀\u0000\ndealing with Team Plasma comes first![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_GotAllBadgesYoure, 0, 5, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x0325
    ActorAdd 1
    ActorWalkRoute 1, 783, 178, 1, 8, 1
    VMSleep 32
    ActorCmdExec 255, Movement_0778
    ActorCmdExec 0, Movement_0778
    ActorCmdWait
    // "Marlon: Sup yo![f000]븁\u0000\nWhat's this Team Plasma\nyou're talking about do?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_MarlonSupYoWhats, 1, 6, 0
    MsgWinCloseAll
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Team Plasma does things\nlike steal my sister's...[f000]븀\u0000\nI mean people's Pokémon.[f000]븁\u0000\nThey plan on conquering Unova\nby using Pokémon to freeze it solid![f000]븀\u0000\nThey're really evil![f000]븁\u0000\nHaven't you heard of them, Marlon?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_TeamPlasmaDoesThings, 0, 5, 0
    MsgWinCloseAll
    // "Marlon: Nope![f000]븁\u0000\nWhen the ocean's your home,\nyou don't worry about things like that.[f000]븁\u0000\n'Cause the ocean accepts all rivers![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_MarlonNopeWhenOceans, 1, 6, 0
    // "So you think Team Plasma's bad, then?"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_ThinkTeamPlasmasBad, 1, 6, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030F
    // "Marlon: I get it.\nThey're bad, so you fight 'em.[f000]븁\u0000\nBut, first, you got to say\nthat in your own words.[f000]븁\u0000\nWhen you do, you'll understand\nbetter what you want to do[f000]븀\u0000\nand what you're hopin' for![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_MarlonGetTheyreBad, 1, 6, 0
    MsgWinCloseAll
    VMJump L_031D

L_030F:
    // "Marlon: Shoots! Not bad...\nYou think that but still fight![f000]븁\u0000\nBut, first, you got to say\nthat in your own words.[f000]븁\u0000\nWhen you do, you'll understand\nbetter what you want to do[f000]븀\u0000\nand what you're hopin' for![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_MarlonShootsNotBad, 1, 6, 0
    MsgWinCloseAll

L_031D:
    ActorCmdExec 1, Movement_0778
    ActorCmdWait
    // "Well then...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_WellThen, 1, 6, 0
    MsgWinCloseAll
    ActorWalkRoute 1, 783, 184, 1, 8, 1
    ActorCmdWait
    ActorDelete 1
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Man, I don't know if that\nguy is laid back or just irresponsible.[f000]븁\u0000\nThat kinda got me down,\nbut our opponent is Team Plasma![f000]븁\u0000\nWe have to focus! But, before that,\nwe have to find where they are![f000]븁\u0000\nOK! We'll split up!\nYou check Route 22! Got it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_ManDontKnowIf, 0, 5, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 783, 184, 1, 8, 1
    ActorCmdWait
    ActorDelete 0
    WorkSetConst EVENT_WORK_0x40df, 2
    FlagSet EVENT_FLAG_0x0324
    FlagSet EVENT_FLAG_0x0325
    HollowRivalCmd_0262 1, 28
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0388:
    Move 13, 3
    Move 35, 1
    MoveEnd

Script_5:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40de
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CE
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: We'll get the DNA Splicers\nback for sure![f000]븁\u0000\nSo you should focus on\ndefeating the Gym Leader first!"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_WellGetDnaSplicers, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40de, 2
    VMJump L_03E5

L_03CE:
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "After you get the Badge,\nwe'll look for Team Plasma![f000]븁\u0000\nI'm not gonna let the Unova region\nbecome an ice sculpture!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_AfterGetBadgeWell, 0, 0
    LastKeyWait
    ActorMsgClose

L_03E5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Eek! Hee-hee-hee!\nJust try and catch me!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_EekHeeHeeHee, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A ha ha! Hey, wait up!\nI'm gonna catch you!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_HaHaHeyWait, 0, 0
    LastKeyWait
    ActorMsgClose
    VMStackPushFlag EVENT_FLAG_0x0326
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0434
    FlagSet EVENT_FLAG_0x0137

L_0434:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0993
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0482
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "With Pokémon and people,\ntreasure every meeting.[f000]븀\u0000\nThere may not be another...[f000]븁\u0000\nThat's why you have to give\nit your best during that moment...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_PokemonPeopleTreasureEvery_2, 0, 0
    MsgWinCloseAll
    Cmd_0275 0, 40, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink!"
    SystemMsg HumilauCity_Text_FunfestMissionHasBeen, 0
    SEWait
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0993
    VMJump L_0496

L_0482:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "With Pokémon and people,\ntreasure every meeting.[f000]븀\u0000\nThere may not be another...[f000]븁\u0000\nThat's why you have to give\nit your best during that moment..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_PokemonPeopleTreasureEvery, 0, 0
    LastKeyWait
    ActorMsgClose

L_0496:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_04C3
    ActorCmdExec 255, Movement_0778
    ActorCmdWait

L_04C3:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04EF
    // "It's a face board..."
    InfoMsg HumilauCity_Text_ItsFaceBoard, 2
    LastKeyWait
    InfoMsgClose_0039
    FlagSet EVENT_FLAG_DAILY_0x0adc
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_070A

L_04EF:
    FlagReset EVENT_FLAG_0x03d9
    WorkSetConst EVENT_WORK_0x4000, 1
    // "There's a face board![f000]븁\u0000"
    InfoMsg HumilauCity_Text_TheresFaceBoard, 2
    InfoMsgClose_0039
    ActorAdd 6
    Random EVENT_WORK_0x4001, 4
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0531
    ActorNew 777, 182, 3, 251, 230, 0
    VMJump L_05A0

L_0531:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0558
    ActorNew 777, 182, 3, 251, 22, 0
    VMJump L_05A0

L_0558:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_057F
    ActorNew 777, 182, 3, 251, 52, 0
    VMJump L_05A0

L_057F:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05A0
    ActorNew 777, 182, 3, 251, 315, 0

L_05A0:
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 786
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05CF
    ActorCmdExec 6, Movement_0788
    ActorCmdExec 251, Movement_07B0
    VMJump L_05DF

L_05CF:
    ActorCmdExec 6, Movement_079C
    ActorCmdExec 251, Movement_07C4

L_05DF:
    ActorCmdWait
    // "Say cheese!\nOne, two, three![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_SayCheeseOneTwo, 6, 0, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_GYM_E02
    FadeEx 12, 16, 0, 2
    FadeExWait
    SEWait
    VMSleep 8
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_062A
    // "This is a souvenir![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_Souvenir, 251, 0, 0
    VMJump L_069D

L_062A:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_064F
    // "It's lonely taking it by myself![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_ItsLonelyTakingBy, 251, 0, 0
    VMJump L_069D

L_064F:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0674
    // "I'm a superfan of face boards![f000]븁\u0000\nBy the way, some people\ncall them photo boards![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_ImSuperfanFaceBoards, 251, 0, 0
    VMJump L_069D

L_0674:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_069D
    PVPlay 575, 0
    // "Thita! ♪"
    ActorMsg MSGFILE_SCRIPT, HumilauCity_Text_Thita, 251, 0, 0
    PVWait
    MsgWaitAdvance

L_069D:
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 786
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06D4
    ActorCmdExec 6, Movement_07D0
    ActorCmdExec 251, Movement_07F0
    VMSleep 16
    ActorCmdExec 255, Movement_0768
    VMJump L_06F0

L_06D4:
    ActorCmdExec 6, Movement_07E0
    ActorCmdExec 251, Movement_0804
    VMSleep 8
    ActorCmdExec 255, Movement_0768

L_06F0:
    ActorCmdWait
    ActorDelete 6
    ActorDelete 251
    VMSleep 16
    MedalGive 98
    FlagSet EVENT_FLAG_0x03d9
    FlagSet EVENT_FLAG_DAILY_0x0adc

L_070A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Humilau City\nCalm and Sparkling Seas"
    MsgPlaceSign HumilauCity_Text_HumilauCityCalmSparkling, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Marine Tube Ahead\nThe Walk-Through Aquarium"
    MsgPlaceSign HumilauCity_Text_MarineTubeAheadWalk, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Humilau City Pokémon Gym\nGym Leader: Marlon[f000]븀\u0000\nMore Splash than the Sea"
    MsgPlaceSign HumilauCity_Text_HumilauCityPokemonGym, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 35, 1
    MoveEnd

Movement_0768:
    Move 34, 1
    MoveEnd

Movement_0770:
    Move 32, 1
    MoveEnd

Movement_0778:
    Move 33, 1
    MoveEnd

Movement_0780:
    Move 75, 1
    MoveEnd

Movement_0788:
    Move 15, 7
    Move 13, 1
    Move 15, 2
    Move 32, 1
    MoveEnd

Movement_079C:
    Move 15, 7
    Move 13, 1
    Move 15, 3
    Move 32, 1
    MoveEnd

Movement_07B0:
    Move 15, 7
    Move 12, 1
    Move 15, 3
    Move 13, 1
    MoveEnd

Movement_07C4:
    Move 15, 9
    Move 33, 1
    MoveEnd

Movement_07D0:
    Move 14, 2
    Move 12, 1
    Move 14, 7
    MoveEnd

Movement_07E0:
    Move 14, 3
    Move 12, 1
    Move 14, 7
    MoveEnd

Movement_07F0:
    Move 12, 1
    Move 14, 3
    Move 13, 1
    Move 14, 7
    MoveEnd

Movement_0804:
    Move 14, 9
    MoveEnd

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This dress is comfy and easy\nto wear..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_DressComfyEasyWear, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, are you taking on the Gym Leader?[f000]븁\u0000\nBut can you find ol' Marlon?\nHe does whatever he wants!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_OhTakingGymLeader, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When I float between the waves\nlike this, I'm like a mermaid.[f000]븁\u0000\nNow that I think of it,\nthere was a tomboyish-mermaid[f000]븀\u0000\nGym Leader in Kanto."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_WhenFloatBetweenWaves, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you know about Seaside Cave?[f000]븁\u0000\nIf you use the HM Surf\nto go down Route 21,[f000]븀\u0000\nyou'll find the cave there.[f000]븁\u0000\nIf you go through it,\nyou'll reach Undella Town."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_KnowAboutSeasideCave, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Controlling the ocean...nature...\nIt's not possible.[f000]븁\u0000\nPeople and Pokémon have to\nfigure out how to live with nature!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_ControllingOceanNatureIts, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, thanks to you,\nthe guest rooms are all full!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_HeyThanksGuestRooms, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0996
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_090A
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ahhhhhh!\nThe weather's great today, too![f000]븁\u0000\nI wonder how many days have passed\nsince I came here on my vacation.[f000]븁\u0000\nSpending every day in such abundance\nmakes my brain a little mushy.[f000]븁\u0000\nI wonder if there will be an event\nthat will stimulate me a little.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_AhhhhhhWeathersGreatToday, 0, 0
    MsgWinCloseAll
    Cmd_0275 0, 25, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink!"
    SystemMsg HumilauCity_Text_FunfestMissionHasBeen_2, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "Ahhhh...\nWas today Sunday?"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_AhhhhTodaySunday, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0996
    VMJump L_091E

L_090A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ahhhh...\nWas today Sunday?"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity_Text_AhhhhTodaySunday, 0, 0
    LastKeyWait
    ActorMsgClose

L_091E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
