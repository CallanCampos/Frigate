#include "asm/field_script.inc"
#include "text/script/virbank_city.h"

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
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_24:
    VMStackPush EVENT_WORK_0x40ac
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C3
    ObjInitNPCGPos 8, 3, 240, 0, 671
    ObjInitNPCGPos 7, 3, 240, 0, 669
    ObjInitNPCGPos 9, 2, 240, 0, 670
    ObjInitNPCGPos 11, 2, 241, 0, 671
    ObjInitNPCGPos 10, 2, 241, 0, 669
    VMJump L_00EE

L_00C3:
    VMStackPush EVENT_WORK_0x40ac
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EE
    ObjInitNPCGPos 1, 3, 241, 0, 670
    ObjInitNPCGPos 0, 2, 243, 0, 670

L_00EE:
    VMHalt

Script_14:
    VMStackPush EVENT_WORK_0x40ac
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013F
    ActorSetGPos 8, 240, 0, 671, 3
    ActorSetGPos 7, 240, 0, 669, 3
    ActorSetGPos 9, 240, 0, 670, 2
    ActorSetGPos 11, 241, 0, 671, 2
    ActorSetGPos 10, 241, 0, 669, 2

L_013F:
    VMHalt

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xd28000, 0, 0x28b8000, 45
    EvCameraWait
    // "???: Roxie,\ndon't try to stop me![f000]븁\u0000\nI'm off to Pokéstar Studios\nto live up to my true potential![f000]븁\u0000\nMy dream is to be a ship captain\nand a movie star![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieDontTryStop, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0214
    ActorCmdWait
    // "Roxie: Get real!\nYou're a captain already, aren't you?[f000]븁\u0000\nIf that ship doesn't move,\nyou're going to cause lots of trouble![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieGetRealYoure, 1, 4, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0254
    ActorCmdWait
    // "Captain: Oh, dear daughter.[f000]븁\u0000\nYou split your time between your\nresponsibilities as a Gym Leader and[f000]븀\u0000\nwith your band, right?[f000]븀\u0000\nI can do that, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_CaptainOhDearDaughter, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_01E8
    ActorCmdWait
    ActorDelete 0
    // "Roxie: AAAAAAH![f000]븁\u0000\nYou dim-witted...dense...dumb...daft...\ndippy...dorky...doltish DOOFUS![f000]븁\u0000\nDoing double duty isn't the problem!\nYou're causing problems for people![f000]븁\u0000\nKeeping people from getting where\nthey're going because of sheer[f000]븀\u0000\nselfishness is unforgivable![f000]븁\u0000\nI've HAD it!\nI'm going to the Gym![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieAaaaaahDimWitted, 1, 4, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_01FC
    ActorCmdWait
    ActorDelete 1
    EvCameraMoveToDefault 45
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x40ac, 2
    FlagSet EVENT_FLAG_0x02d6
    FlagSet EVENT_FLAG_ARRIVED_VIRBANK_CITY
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01E8:
    Move 15, 5
    Move 12, 10
    MoveEnd
    Move 39, 4
    MoveEnd

Movement_01FC:
    Move 13, 8
    MoveEnd

Movement_0204:
    Move 13, 1
    MoveEnd

Movement_020C:
    Move 12, 1
    MoveEnd

Movement_0214:
    Move 15, 1
    MoveEnd

Movement_021C:
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_022C:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0

Movement_023C:
    Move 3, 1
    MoveEnd

Movement_0244:
    Move 32, 1
    MoveEnd

Movement_024C:
    Move 33, 1
    MoveEnd

Movement_0254:
    Move 34, 1
    MoveEnd

Movement_025C:
    Move 35, 1
    MoveEnd

Movement_0264:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_0274:
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01ec
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Roxie's poison is intense!\nIt stings, stings, and stiiiings![f000]븁\u0000\nBut, know what I did?\nI caught a Magnemite in the complex,[f000]븀\u0000\nand I was just fine!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxiesPoisonIntenseStings_2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02EB

L_02B3:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Roxie's poison is intense!\nIt stings, stings, and stiiiings![f000]븁\u0000\nBut, know what I did?\nI caught a Magnemite in the complex,[f000]븀\u0000\nand I was just fine![f000]븁\u0000\nHere, I'll give you these,\nso go catch a Magnemite or something![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxiesPoisonIntenseStings, 2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 4
    WorkSet 0x8001, 5
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01ec

L_02EB:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If I didn't have Pokémon, all of the work\nat the complex would make me a wreck!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_IfDidntHavePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Kid!\nDo you know about the Battle Box?[f000]븁\u0000\nIt's a convenient feature you can use\non the PCs at the Pokémon Center."
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_KidKnowAboutBattle, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This Watchog has been with me\nsince I was born![f000]븁\u0000\nIts Keen Eye Ability\nhas helped me so many times!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_WatchogHasBeenSince, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Roxie's band![f000]븁\u0000\nIt's getting hard to buy\ntickets to their shows lately! ♪[f000]븁\u0000\nMaybe they'll go on a world\ntour soon!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxiesBandItsGetting, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Past here is the charming wonderland--\nPokéstar Studios![f000]븁\u0000\nIt's a movie studio, but right\nnow auditions are in progress...[f000]븁\u0000\nThey're having a look at the captain's\nacting. No unauthorized people allowed!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_PastHereCharmingWonderland, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    PlayerGetGPos 0x8020, 0x8021
    WorkCmpConst 0x8020, 215
    VMJumpIf CMP_EQ, L_0396
    VMJump L_03AE

L_0396:
    ActorCmdExec 6, Movement_04C4
    ActorCmdWait
    ActorCmdExec 255, Movement_025C
    VMJump L_042F

L_03AE:
    WorkCmpConst 0x8020, 216
    VMJumpIf CMP_EQ, L_03C1
    VMJump L_03D9

L_03C1:
    ActorCmdExec 6, Movement_04D4
    ActorCmdWait
    ActorCmdExec 255, Movement_025C
    VMJump L_042F

L_03D9:
    WorkCmpConst 0x8020, 218
    VMJumpIf CMP_EQ, L_03EC
    VMJump L_0404

L_03EC:
    ActorCmdExec 6, Movement_04E0
    ActorCmdWait
    ActorCmdExec 255, Movement_0254
    VMJump L_042F

L_0404:
    WorkCmpConst 0x8020, 219
    VMJumpIf CMP_EQ, L_0417
    VMJump L_042F

L_0417:
    ActorCmdExec 6, Movement_04EC
    ActorCmdWait
    ActorCmdExec 255, Movement_0254
    VMJump L_042F

L_042F:
    ActorCmdWait
    // "Past here is the charming wonderland--\nPokéstar Studios![f000]븁\u0000\nIt's a movie studio, but right\nnow auditions are in progress...[f000]븁\u0000\nThey're having a look at the captain's\nacting. No unauthorized people allowed![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_PastHereCharmingWonderland_2, 6, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0204
    ActorCmdWait
    WorkCmpConst 0x8020, 215
    VMJumpIf CMP_EQ, L_045C
    VMJump L_046A

L_045C:
    ActorCmdExec 6, Movement_04FC
    VMJump L_04B9

L_046A:
    WorkCmpConst 0x8020, 216
    VMJumpIf CMP_EQ, L_048A
    WorkCmpConst 0x8020, 218
    VMJumpIf CMP_EQ, L_048A
    VMJump L_0498

L_048A:
    ActorCmdExec 6, Movement_022C
    VMJump L_04B9

L_0498:
    WorkCmpConst 0x8020, 219
    VMJumpIf CMP_EQ, L_04AB
    VMJump L_04B9

L_04AB:
    ActorCmdExec 6, Movement_0508
    VMJump L_04B9

L_04B9:
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04C4:
    Move 2, 1
    Move 75, 1
    Move 14, 1
    MoveEnd

Movement_04D4:
    Move 2, 1
    Move 75, 1
    MoveEnd

Movement_04E0:
    Move 3, 1
    Move 75, 1
    MoveEnd

Movement_04EC:
    Move 3, 1
    Move 75, 1
    Move 15, 1
    MoveEnd

Movement_04FC:
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_0508:
    Move 14, 1
    Move 33, 1
    MoveEnd

Script_9:
    ActorsPauseAll
    MEPlay SEQ_ME_CALL
    // "The Xtransceiver is ringing!"
    SystemMsg VirbankCity_Text_XtransceiverRinging, 2
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 picked up the\nXtransceiver![f000]븁\u0000"
    SystemMsg VirbankCity_Text_PickedUpXtransceiver, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    CallXTransceiver 1, 0
    FadeInBlackQ
    FadeWait
    WorkSetConst EVENT_WORK_0x40ac, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    VMCall L_0555
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0555:
    BGMPlay SEQ_BGM_E_PLASMA
    // "Roxie: So, are you guys\nTeam Plasma, then?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieGuysTeamPlasma, 7, 0, 0
    MsgWinCloseAll
    // "That's right!\nWe're Team Plasma![f000]븁\u0000\nTwo years ago...\nWe were betrayed by the man[f000]븀\u0000\nwe respected as our king and hero![f000]븁\u0000\nSo our conquest of the Unova region\nended in failure![f000]븁\u0000\nHowever! We'll never give up!\nWe will persevere![f000]븁\u0000\nWe already have another\nplan in motion.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_ThatsRightWereTeam, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 8, Movement_0214
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Shut it![f000]븁\u0000\nYou guys are the worst.\nYou talk about saving Pokémon,[f000]븀\u0000\nbut you're just Pokémon thieves![f000]븀\u0000\nDon't think I'll ever forgive you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_ShutGuysWorstTalk, 8, 0, 0
    MsgWinCloseAll
    // "Team Plasma: In the ranch,\nwe got chased by Lillipup...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_TeamPlasmaRanchWe, 11, 0, 0
    MsgWinCloseAll
    // "Team Plasma: And now we're getting\ninvolved in trouble. This kinda stinks.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_TeamPlasmaNowWere, 10, 0, 0
    MsgWinCloseAll
    // "Team Plasma: Well, it's OK, isn't it?\nIf we steal their Pokémon...[f000]븁\u0000\nOK, kiddos, when you're\nready, give it your best shot![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_TeamPlasmaWellIts, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_021C
    VMSleep 8
    ActorCmdExec 11, Movement_021C
    ActorCmdExec 10, Movement_021C
    ActorCmdWait
    VMSleep 20
    ActorCmdExec 8, Movement_0214
    ActorCmdExec 7, Movement_0214
    ActorCmdWait
    ActorCmdExec 8, Movement_0988
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: I'll crush you\nand your new plans![f000]븁\u0000\nI can never forgive Team Plasma![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_IllCrushNewPlans, 8, 0, 0
    MsgWinCloseAll
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40ac, 5
    VMReturn

Script_11:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40ac
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_064C
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Roxie: [f000]Ā\u0001\u0000!\nHelp out!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieHelpOut, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 7, Movement_025C
    ActorCmdWait
    VMJump L_0666

L_064C:
    SEPlay SEQ_SE_MESSAGE
    ActorWalkRoute 255, 239, 670, 0, 8, 1
    ActorCmdWait
    VMCall L_0555

L_0666:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40ac
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_069F
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0001: Dirty Pokémon thieves..."
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_DirtyPokemonThieves, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0703

L_069F:
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8022, 0
    PlayerGetDir 0x8022
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_06C0
    VMJump L_06D4

L_06C0:
    ActorWalkRoute 255, 239, 670, 0, 8, 1
    VMJump L_06FB

L_06D4:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_06E7
    VMJump L_06FB

L_06E7:
    ActorWalkRoute 255, 239, 670, 0, 8, 0
    VMJump L_06FB

L_06FB:
    ActorCmdWait
    VMCall L_0555

L_0703:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: What?\nThink you can beat me?"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_TeamPlasmaWhatThink, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_091B
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_47
    // "Ha ha! Your Pokémon will be\nhelping us take over the world[f000]븀\u0000\nin a few seconds![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_HaHaPokemonWill, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_44, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0769
    CallTrainerBattleEnd
    VMJump L_076B

L_0769:
    CallTrainerLose

L_076B:
    ActorCmdExec 8, Movement_023C
    ActorCmdExec 7, Movement_023C
    ActorCmdWait
    ActorCmdExec 9, Movement_09B0
    VMSleep 8
    ActorCmdExec 11, Movement_09B0
    ActorCmdExec 10, Movement_09B0
    ActorCmdWait
    // "Team Plasma: I can't believe\nkids like these caught us off guard![f000]븁\u0000\nTch... Let's run![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_TeamPlasmaCantBelieve, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_09C0
    ActorCmdWait
    ActorCmdExec 9, Movement_09CC
    ActorCmdExec 255, Movement_0990
    ActorCmdWait
    ActorWalkRoute 9, 228, 670, 1, 4, 0
    ActorWalkRoute 10, 228, 670, 1, 4, 1
    VMSleep 4
    ActorWalkRoute 11, 228, 670, 1, 4, 1
    VMSleep 16
    ActorCmdExec 255, Movement_0254
    ActorCmdExec 7, Movement_0254
    ActorCmdExec 8, Movement_0254
    ActorCmdWait
    ActorDelete 9
    ActorDelete 10
    ActorDelete 11
    ActorCmdExec 8, Movement_021C
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Tsk!\nWhat a bunch of creeps![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_TskWhatBunchCreeps, 8, 0, 0
    MsgWinCloseAll
    // "Roxie: Split up! You,\ngo check Route 20![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieSplitUpGo, 7, 0, 0
    MsgWinCloseAll
    ActorCmdExec 7, Movement_0930
    ActorCmdWait
    ActorWalkRoute 7, 237, 669, 0, 8, 1
    ActorCmdWait
    // "If I'd lost, my precious\nPokémon would've been taken![f000]븁\u0000\nThanks, you two![f000]븁\u0000\nUse this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_IfIdLostPrecious, 7, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 420
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorWalkRoute 7, 228, 669, 0, 4, 0
    ActorCmdWait
    ActorDelete 7
    ActorWalkRoute 8, 238, 670, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 8, Movement_0244
    VMSleep 8
    ActorCmdExec 255, Movement_024C
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: That's...\na Hidden Machine, right?[f000]븀\u0000\nIt's Cut![f000]븁\u0000\nPokémon that know Cut\ncan use it even when they're not[f000]븀\u0000\nin battle to cut down small trees![f000]븁\u0000\nOh! But more importantly,\nlet's go after Team Plasma![f000]븁\u0000\nC’mon! We’re splitting up\nand searching Route 20![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_ThatsHiddenMachineRight, 8, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 8, 228, 670, 0, 4, 0
    VMSleep 8
    ActorCmdExec 255, Movement_0254
    ActorCmdWait
    ActorDelete 8
    SEWait
    WorkSetConst EVENT_WORK_0x40ac, 6
    FlagSet EVENT_FLAG_0x02d4
    FlagReset EVENT_FLAG_0x02bd
    FlagReset EVENT_FLAG_0x02be
    FlagSet EVENT_FLAG_0x09fe
    WorkSetConst EVENT_WORK_0x40ab, 5
    VMJump L_0929

L_091B:
    // "That's right!\nDon't try opposing us!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_ThatsRightDontTry, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0929:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0930:
    Move 17, 1
    Move 18, 5
    Move 10, 1
    Move 63, 2
    Move 3, 1
    MoveEnd
    Move 13, 1
    Move 15, 3
    MoveEnd
    Move 14, 8
    MoveEnd
    Move 79, 2
    MoveEnd
    Move 14, 1
    Move 32, 1
    MoveEnd
    VMStackDiv
    VMSleep 12
    VMHalt
    .byte 0xfe
    .balign 4, 0
    Move 12, 1
    Move 69, 1
    MoveEnd

Movement_0988:
    Move 100, 1
    MoveEnd

Movement_0990:
    Move 71, 1
    Move 18, 1
    Move 72, 1
    Move 1, 1
    Move 71, 1
    Move 16, 1
    Move 72, 1
    MoveEnd

Movement_09B0:
    Move 71, 1
    Move 11, 1
    Move 72, 1
    MoveEnd

Movement_09C0:
    Move 38, 4
    Move 18, 1
    MoveEnd

Movement_09CC:
    Move 18, 2
    MoveEnd

Script_15:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "This is Virbank City!\nCity of falling fog and rising stars!"
    MsgPlaceSign VirbankCity_Text_VirbankCityCityFalling, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Pokéstar Studios Ahead\nBringing a new golden age of cinema!"
    MsgPlaceSign VirbankCity_Text_PokestarStudiosAheadBringing, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Virbank City Pokémon Gym\nLeader: Roxie[f000]븀\u0000\nPoison days, poison on the stage!"
    MsgPlaceSign VirbankCity_Text_VirbankCityPokemonGym, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Hoooog!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_Hoooog, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Medal's really pretty, isn't it?![f000]븁\u0000\nIf you solve the Hint Medal riddles,\nyou can collect more and more of them!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_MedalsReallyPrettyIsnt, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "First, turn the C-Gear's power on![f000]븁\u0000\nThen tap [f000][ff00]\u0001\u0001CONNECTED[f000][ff00]\u0001\u0000\nin the center of the C-Gear screen[f000]븀\u0000\nto check the Tag Log!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_FirstTurnCGears, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "First, turn the C-Gear's power on![f000]븁\u0000\nThen tap [f000][ff00]\u0001\u0001WIRELESS[f000][ff00]\u0001\u0000.\nFinally, tap [f000][ff00]\u0001\u0001ENTRALINK[f000][ff00]\u0001\u0000![f000]븀\u0000\nIf you have some time, give it a try![f000]븀\u0000\nIt's amazing!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_FirstTurnCGears_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "One, two, three, four, five, six![f000]븁\u0000\nThe number of Poké Balls\nyou can put in your belt is six.[f000]븀\u0000\nSo you can take six Pokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCity_Text_OneTwoThreeFour, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xf18000, 0, 0x29e8000, 20
    EvCameraWait
    // "Roxie: I thought your movie\nwas pretty good.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieThoughtMoviePretty, 1, 4, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0274
    ActorCmdWait
    // "Pop Roxie: So you saw it...[f000]븁\u0000\nThat movie was...[f000]븁\u0000\nNo, it wasn't the movie. It was me.\nI was so excited that I didn't really[f000]븀\u0000\nbecome Riolu-Man![f000]븁\u0000\nActing like that won't\nthrill or excite anyone...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_PopRoxieSawMovie, 0, 6, 0
    MsgWinCloseAll
    // "Roxie: C'mon!\nDon't get down![f000]븁\u0000\nYou can keep trying while\nyou continue on as captain![f000]븁\u0000\nAs long as you don't cause anyone\ntrouble, I won't get mad.[f000]븁\u0000\nI don't know how the Pokéstar Studios\npeople feel about that, though![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_RoxieCmonDontGet, 1, 4, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0254
    ActorCmdWait
    ActorCmdExec 1, Movement_0264
    ActorCmdWait
    PlayerGetGPos 0x8020, 0x8021
    WorkCmpConst 0x8021, 669
    VMJumpIf CMP_EQ, L_0B37
    VMJump L_0B43

L_0B37:
    WorkSetConst 0x8021, 670
    VMJump L_0B81

L_0B43:
    WorkCmpConst 0x8021, 670
    VMJumpIf CMP_EQ, L_0B56
    VMJump L_0B62

L_0B56:
    WorkSetConst 0x8020, 240
    VMJump L_0B81

L_0B62:
    WorkCmpConst 0x8021, 671
    VMJumpIf CMP_EQ, L_0B75
    VMJump L_0B81

L_0B75:
    WorkSetConst 0x8021, 670
    VMJump L_0B81

L_0B81:
    ActorWalkRoute 1, 0x8020, 0x8021, 1, 8, 1
    ActorCmdWait
    PlayerGetGPos 0x8020, 0x8021
    WorkCmpConst 0x8021, 669
    VMJumpIf CMP_EQ, L_0BAA
    VMJump L_0BC6

L_0BAA:
    ActorCmdExec 1, Movement_0244
    VMSleep 8
    ActorCmdExec 255, Movement_024C
    ActorCmdWait
    VMJump L_0C0E

L_0BC6:
    WorkCmpConst 0x8021, 670
    VMJumpIf CMP_EQ, L_0BD9
    VMJump L_0BDF

L_0BD9:
    VMJump L_0C0E

L_0BDF:
    WorkCmpConst 0x8021, 671
    VMJumpIf CMP_EQ, L_0BF2
    VMJump L_0C0E

L_0BF2:
    ActorCmdExec 1, Movement_024C
    VMSleep 8
    ActorCmdExec 255, Movement_0244
    ActorCmdWait
    VMJump L_0C0E

L_0C0E:
    WordSetLoadRivalName 1
    // "Oh...\nDid you hear us?[f000]븁\u0000\nWell, it looks like everyone\nfrom Team Plasma got away...[f000]븁\u0000\nI don't feel really good about it,\nbut at least they're gone.[f000]븁\u0000\nOh! There was a lot of ruckus, but\nyou can sail to Castelia City now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_OhDidHearUs, 1, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8020, 0x8021
    WorkCmpConst 0x8021, 669
    VMJumpIf CMP_EQ, L_0C38
    VMJump L_0C44

L_0C38:
    WorkSetConst 0x8021, 670
    VMJump L_0C82

L_0C44:
    WorkCmpConst 0x8021, 670
    VMJumpIf CMP_EQ, L_0C57
    VMJump L_0C63

L_0C57:
    WorkSetConst 0x8021, 669
    VMJump L_0C82

L_0C63:
    WorkCmpConst 0x8021, 671
    VMJumpIf CMP_EQ, L_0C76
    VMJump L_0C82

L_0C76:
    WorkSetConst 0x8021, 670
    VMJump L_0C82

L_0C82:
    ActorWalkRoute 1, 228, 0x8021, 1, 8, 1
    VMSleep 12
    ActorCmdExec 255, Movement_0254
    ActorCmdWait
    ActorDelete 1
    // "Pop Roxie: OK! I guess\nI'll do my best as a captain![f000]븁\u0000\nI'll excite and thrill everyone\nby barreling through rough waves![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCity_Text_PopRoxieOkGuess, 0, 6, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_025C
    ActorWalkRoute 0, 242, 669, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_020C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    EvCameraMoveToDefault 20
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FlagSet EVENT_FLAG_0x02d6
    FlagReset EVENT_FLAG_0x02fb
    WorkSetConst EVENT_WORK_0x40ac, 8
    FlagReset EVENT_FLAG_0x02d2
    FlagReset EVENT_FLAG_0x02d3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
