#include "asm/field_script.inc"
#include "text/script/driftveil_city.h"

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
    ScriptEntry Script_25
    ScriptEntry Script_26
    ScriptEntry Script_27
    ScriptEntry Script_28
    ScriptEntry Script_29
    ScriptEntry Script_30
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_12:
    FlagSet EVENT_FLAG_0x03b6
    FlagSet EVENT_FLAG_0x03b7
    FlagSet EVENT_FLAG_0x03b9
    FlagSet EVENT_FLAG_0x03bb
    FlagSet EVENT_FLAG_0x03bd
    FlagReset EVENT_FLAG_0x03b4
    FlagReset EVENT_FLAG_0x03b5
    FlagReset EVENT_FLAG_0x03b8
    FlagReset EVENT_FLAG_0x03ba
    FlagReset EVENT_FLAG_0x03bc
    VMStackPush EVENT_WORK_0x40c3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E5
    ObjInitNPCGPos 2, 2, 198, 2, 396
    ObjInitNPCGPos 4, 3, 197, 2, 396
    VMJump L_015A

L_00E5:
    VMStackPush EVENT_WORK_0x40c3
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0116
    ObjInitNPCGPos 2, 1, 198, 2, 396
    ObjInitNPCGPos 4, 1, 197, 2, 396
    VMJump L_015A

L_0116:
    VMStackPush EVENT_WORK_0x40c3
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013B
    ObjInitNPCGPos 5, 1, 211, 0, 403
    VMJump L_015A

L_013B:
    VMStackPush EVENT_WORK_0x40c3
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015A
    ObjInitNPCGPos 5, 2, 213, 0, 405

L_015A:
    VMHalt

Script_13:
    VMHalt

Script_14:
    ActorsPauseAll
    ActorCmdExec 5, Movement_0274
    VMSleep 16
    ActorWalkRoute 255, 213, 403, 1, 8, 0
    ActorCmdExec 6, Movement_0E24
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: You got a Gym Badge, too!\nI knew you could do it![f000]븁\u0000\nYou know... My partners are the reason\nI was able to get that Badge.[f000]븁\u0000\nBut I'm sure there's more connecting\nus to each other than Poké Balls![f000]븁\u0000\nIf that's all there is, the stolen\nPurrloin's feelings will never be[f000]븀\u0000\nwhat they were![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_GotGymBadgeToo, 5, 5, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0280
    VMSleep 16
    ActorCmdExec 5, Movement_0E34
    ActorCmdExec 255, Movement_0E34
    ActorCmdWait
    // "Clay: Oh, so you two squirts\nknow each other, huh?[f000]븁\u0000\nYa both ain't bad, so\nI wanna show ya somethin'.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ClayOhTwoSquirts, 6, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Wait!\nI just remembered. Clay...[f000]븁\u0000\nWhy? What's the reason?\nWhy have you forgiven Team Plasma?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_WaitJustRememberedClay, 5, 6, 0
    MsgWinCloseAll
    // "Clay: There's always room for folks\nto grow and change, ain't there?[f000]븁\u0000\nAnd, if ya only go after what ya think is\nright, ya might end up rejectin' all[f000]븀\u0000\nthoughts and opinions other than[f000]븀\u0000\nyer own. That's mighty dangerous.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ClayTheresAlwaysRoom, 6, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Hmph...\nIs that one of those compromises[f000]븀\u0000\nadults are supposed to make?[f000]븁\u0000\nWhatever!\nI'm gonna fight Team Plasma![f000]븁\u0000\nOh yeah, what were you wanting\nto show us?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_HmphOneThoseCompromises, 5, 6, 0
    MsgWinCloseAll
    // "Clay: Ya ever heard of the\nPokémon World Tournament?[f000]븁\u0000\nTrainers from all over the world\ngather on up to see who's toughest![f000]븁\u0000\nWell then, I'll be waitin' for you at the\nsouth end of town![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ClayYaEverHeard, 6, 3, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_029C
    ActorCmdWait
    ActorCmdExec 255, Movement_0E2C
    ActorCmdExec 5, Movement_0E24
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: A tournament\nto decide who's strongest, huh?[f000]븁\u0000\nRight on!\nIt's time for some special training![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_TournamentDecideWhosStrongest, 5, 6, 0
    MsgWinCloseAll
    ActorWalkRoute 5, 202, 405, 1, 4, 1
    VMSleep 8
    ActorCmdExec 255, Movement_0E34
    ActorCmdWait
    ActorDelete 6
    ActorDelete 5
    WorkSetConst EVENT_WORK_0x40c3, 5
    FlagSet EVENT_FLAG_0x02cc
    FlagSet EVENT_FLAG_0x02cd
    FlagSet EVENT_FLAG_0x02cd
    FlagSet EVENT_FLAG_0x02c4
    FlagSet EVENT_FLAG_0x03e9
    WorkSetConst EVENT_WORK_0x40c5, 1
    HollowRivalCmd_0262 1, 12
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0274:
    Move 75, 1
    Move 12, 1
    MoveEnd

Movement_0280:
    Move 12, 2
    Move 35, 1
    Move 63, 1
    Move 33, 1
    Move 63, 1
    Move 35, 1
    MoveEnd

Movement_029C:
    Move 13, 2
    Move 14, 6
    MoveEnd
    Move 14, 1
    Move 33, 1
    MoveEnd

Script_11:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 255, 220, 432, 1, 8, 0
    ActorCmdWait
    VMStackPush 0x8022
    VMStackPushConst 432
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02E9
    ActorCmdExec 255, Movement_0E34
    ActorCmdWait

L_02E9:
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0304
    PlayerSetSpecialSequence 1

L_0304:
    // "Team Plasma: C'mon![f000]븁\u0000\nLet's have fun stealing Pokémon\ntogether, like we did before![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_TeamPlasmaCmonLets, 3, 3, 0
    MsgWinCloseAll
    // "???: I can't.[f000]븁\u0000\nI've learned the hard way\nthat stealing from others is wrong![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_CantIveLearnedHard, 2, 4, 0
    MsgWinCloseAll
    // "Team Plasma: Oh, come on! It's too late\nto start acting all goody-two-shoes now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_TeamPlasmaOhCome, 3, 3, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 218, 432, 1, 4, 1
    VMSleep 7
    SEPlay SEQ_SE_FLD_04
    ActorCmdExec 2, Movement_04B0
    ActorCmdWait
    SEWait
    // "Team Plasma: People don't understand\nour just cause![f000]븁\u0000\nDon't they call you a villain\nwho was plotting world domination?[f000]븁\u0000\nEven though you quit Team Plasma, people\nare still really cold to you, right?[f000]븁\u0000\nSo, you might as well just come\nsteal Pokémon with us and[f000]븀\u0000\ntake over the world![f000]븁\u0000\nThe people who are mean to you now\nwill be groveling at your feet[f000]븀\u0000\nand saying how great you are![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_TeamPlasmaPeopleDont, 3, 3, 0
    MsgWinCloseAll
    // "Ex-Team Plasma: I can't...[f000]븁\u0000\nMy lord N will be sad...\nI can't do that to him...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ExTeamPlasmaCant, 2, 4, 0
    MsgWinCloseAll
    // "Team Plasma: N![f000]븁\u0000\nTeam Plasma's king... What a joke!\nHe's nothing more than a traitor![f000]븁\u0000\nHe disappeared somewhere and abandoned\nus when we needed him![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_TeamPlasmaNTeam, 3, 3, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x02cc
    ActorAdd 5
    // "Hey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_Hey, 5, 6, 1
    MsgWinCloseAll
    ActorCmdExec 255, Movement_04A4
    ActorCmdExec 3, Movement_04A4
    ActorCmdExec 2, Movement_04A4
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 5, 219, 432, 1, 4, 0
    VMSleep 28
    ActorCmdExec 255, Movement_04D0
    VMSleep 20
    SEPlay SEQ_SE_W001_01
    ActorCmdExec 3, Movement_04C0
    ActorCmdExec 2, Movement_0E24
    ActorCmdWait
    SEWait
    ActorCmdExec 255, Movement_0E34
    ActorCmdWait
    WordSetLoadRivalName 1
    // "Start talking, you Team Plasma trash![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_StartTalkingTeamPlasma, 5, 5, 0
    MsgWinCloseAll
    // "Team Plasma: Oww...[f000]븁\u0000\nYou're gonna pay for that![f000]븁\u0000\nOh, yeah. Almost forgot...\nI'm not supposed to cause any trouble.[f000]븁\u0000\nI'll get you next time![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_TeamPlasmaOwwYoure, 3, 3, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 212, 432, 1, 4, 1
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: You're not getting away![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_YoureNotGettingAway, 5, 5, 0
    ActorCmdWait
    MsgWinCloseAll
    ActorWalkRoute 5, 212, 432, 1, 4, 0
    ActorCmdWait
    ActorDelete 5
    ActorDelete 3
    ActorWalkRoute 2, 219, 433, 1, 8, 1
    ActorCmdWait
    // "Ex-Team Plasma: I'm OK![f000]븁\u0000\nWe were friends when we\nwere both in Team Plasma...[f000]븁\u0000\nBut two years ago, Team Plasma\nsplit into a group that follows Lord N,[f000]븀\u0000\nwho just wants to save Pokémon,[f000]븀\u0000\nand a group that follows Ghetsis,[f000]븀\u0000\nwho plans to take over the world.[f000]븁\u0000\nYou can hear the rest\nof the story in our home.[f000]븁\u0000\nIt's on that little hill next\nto the Pokémon Gym.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ExTeamPlasmaIm, 2, 4, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 212, 433, 1, 8, 1
    ActorCmdWait
    ActorSetGPos 2, 198, 2, 396, 2
    FlagSet EVENT_FLAG_0x02cc
    FlagSet EVENT_FLAG_0x02c3
    WorkSetConst EVENT_WORK_0x40c3, 1
    FlagSet EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY
    HollowRivalCmd_0262 1, 9
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04A4:
    Move 75, 1
    Move 35, 1
    MoveEnd

Movement_04B0:
    Move 71, 1
    Move 17, 1
    Move 72, 1
    MoveEnd

Movement_04C0:
    Move 71, 1
    Move 18, 2
    Move 72, 1
    MoveEnd

Movement_04D0:
    Move 0, 1
    Move 71, 1
    Move 17, 1
    Move 72, 1
    MoveEnd
    VMStackAdd
    VMReturn
    Move 15, 4
    Move 12, 3
    Move 15, 12
    Move 34, 1
    MoveEnd
    Move 12, 6
    Move 15, 4
    Move 12, 3
    Move 15, 11
    MoveEnd

Script_29:
    ActorsPauseAll
    ActorCmdExec 2, Movement_0E44
    ActorCmdWait
    // "Ex-Team Plasma: Sir, that's the person\nI was talking about![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ExTeamPlasmaSir, 2, 5, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8021, 1
    ActorWalkRoute 2, 0x8021, 0x8022, 1, 8, 1
    VMSleep 16
    ActorCmdExec 4, Movement_0E2C
    ActorCmdWait
    // "Over here! This way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_OverHereWay, 2, 5, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8022, 397
    VMJumpIf CMP_EQ, L_0579
    VMJump L_0587

L_0579:
    ActorCmdExec 255, Movement_064C
    VMJump L_05C9

L_0587:
    WorkCmpConst 0x8022, 398
    VMJumpIf CMP_EQ, L_059A
    VMJump L_05A8

L_059A:
    ActorCmdExec 255, Movement_065C
    VMJump L_05C9

L_05A8:
    WorkCmpConst 0x8022, 399
    VMJumpIf CMP_EQ, L_05BB
    VMJump L_05C9

L_05BB:
    ActorCmdExec 255, Movement_0668
    VMJump L_05C9

L_05C9:
    ActorWalkRoute 2, 198, 396, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 2, Movement_0E2C
    ActorCmdWait
    // "Rood: Oh! So you're interested\nin Team Plasma, are you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodOhYoureInterested, 4, 3, 0
    MsgWinCloseAll
    // "Ex-Team Plasma: If you hear what\nwe have to say, you might be able[f000]븀\u0000\nto understand us.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ExTeamPlasmaIf, 2, 5, 0
    MsgWinCloseAll
    // "Rood: My guest.[f000]븁\u0000\nIf you're going to come inside,\nI would like to see what kind[f000]븀\u0000\nof person you are, Trainer.[f000]븁\u0000\nThat's right. In a Pokémon battle.\nDo you find this acceptable?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodGuestIfYoure, 4, 3, 0
    WorkSetConst EVENT_WORK_0x40c3, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_063E
    // "Rood: Then, I'm afraid I must\nask you to leave."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodThenImAfraid, 4, 3, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0644

L_063E:
    VMCall L_06E0

L_0644:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_064C:
    Move 13, 1
    Move 15, 3
    Move 32, 1
    MoveEnd

Movement_065C:
    Move 15, 3
    Move 32, 1
    MoveEnd

Movement_0668:
    Move 15, 3
    Move 12, 1
    MoveEnd

Script_10:
    ActorsPauseAll
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: Challenging the Gym, huh?\nNice! Keep getting stronger![f000]븁\u0000\nLet me tell you, though,\nClay's tough![f000]븁\u0000\nEven if all you have to use against\nGround types is Water-type Pokémon,[f000]븀\u0000\nyou might still be in for a rough fight!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ChallengingGymHuhNice, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rood: If you're going to come inside,\nI would like to see what kind[f000]븀\u0000\nof person you are, Trainer.[f000]븁\u0000\nThat's right. In a Pokémon battle.\nDo you find this acceptable?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodIfYoureGoing, 4, 3, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06D4
    // "Rood: Then, I'm afraid I must\nask you to leave."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodThenImAfraid, 4, 3, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_06DA

L_06D4:
    VMCall L_06E0

L_06DA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_06E0:
    // "Rood: Let us begin![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodLetUsBegin, 4, 3, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_ROOD, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_072D
    ActorSetGPos 4, 197, 2, 396, 1
    ActorSetGPos 255, 197, 2, 397, 0
    CallTrainerBattleEnd
    VMJump L_072F

L_072D:
    CallTrainerLose

L_072F:
    // "Rood: I apologize for testing you.[f000]븁\u0000\nBeing former members of Team Plasma,\nwe must deal with a lot...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodApologizeTestingBeing, 4, 3, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x02cc
    ActorAdd 5
    ActorSetGPos 5, 185, 2, 398, 3
    ActorCmdExec 5, Movement_07DC
    VMSleep 16
    ActorCmdExec 255, Movement_0E34
    ActorCmdExec 2, Movement_0E34
    ActorCmdExec 4, Movement_0E34
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: He got away![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_HeGotAway, 5, 4, 0
    MsgWinCloseAll
    // "Rood: And that is?[f000]븁\u0000\n...[f000]븁\u0000\nYour friend?\nHe may join us.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RoodFriendHeMay, 4, 3, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0DEC
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 4
    SEWait
    ActorCmdExec 255, Movement_07E4
    ActorCmdExec 5, Movement_07EC
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40c3, 3
    FlagSet EVENT_FLAG_0x02cb
    FlagSet EVENT_FLAG_0x02c2
    RTReserveScript 1
    MapChangeWarp ZONE_DRIFTVEIL_CITY_6, 7, 25, 0
    VMReturn
    .balign 4, 0

Movement_07DC:
    Move 19, 10
    MoveEnd

Movement_07E4:
    Move 12, 2
    MoveEnd

Movement_07EC:
    Move 15, 2
    Move 12, 1
    MoveEnd

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ex-Team Plasma: I'm sorry...[f000]븁\u0000\nSage Rood is only saying\nthat in order to protect us."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ExTeamPlasmaIm_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Gym Leader, Clay, is currently\nin the middle of something.[f000]븀\u0000\nPlease come back again later."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_GymLeaderClayCurrently, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Welcome to Driftveil City!"
    MsgPlaceSign DriftveilCity_Text_WelcomeDriftveilCity, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Driftveil Drawbridge"
    MsgPlaceSign DriftveilCity_Text_DriftveilDrawbridge, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Driftveil City\nA City of Billowing Sails"
    MsgPlaceSign DriftveilCity_Text_DriftveilCityCityBillowing, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Driftveil Market"
    MsgPlaceSign DriftveilCity_Text_DriftveilMarket, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Driftveil City Pokémon Gym\nLeader: Clay[f000]븀\u0000\nThe Underground Boss"
    MsgPlaceSign DriftveilCity_Text_DriftveilCityPokemonGym, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_26:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Clay Tunnel Ahead"
    MsgPlaceSign DriftveilCity_Text_ClayTunnelAhead, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8023, 0
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0990
    VMStackPush EVENT_WORK_0x4097
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_095B
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI wanted to get the attention of a girl\nI like, so I learned a new style of[f000]븀\u0000\nPokémon battling.[f000]븁\u0000\nIts name... Triple Battle!\nWant to learn about it?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles_5, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0945
    WorkSetConst EVENT_WORK_0x4097, 1
    // "In Triple Battles, you send out three\nPokémon at a time and battle![f000]븁\u0000\nThe rules are simple: just make all of\nyour opponent's Pokémon faint.[f000]븁\u0000\nAnd that's a rough explanation\nof Triple Battles.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_TripleBattlesSendOut, 0, 0, 0
    VMCall L_0AF2
    VMJump L_0955

L_0945:
    // "Oh, man! Getting someone's attention is\nreally hard."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_OhManGettingSomeones_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0955:
    VMJump L_098A

L_095B:
    VMStackPush EVENT_WORK_0x4097
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_097A
    VMCall L_0AF2
    VMJump L_098A

L_097A:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nRiding a bike and becoming the wind fits a\nbad boy like me."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles_8, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_098A:
    VMJump L_0A29

L_0990:
    VMStackPush EVENT_WORK_0x4097
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09FA
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI wanted to get the attention of a girl\nI like, so I learned a new style of[f000]븀\u0000\nPokémon battling.[f000]븁\u0000\nIts name... Rotation Battle!\nWant to learn about it?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09E4
    WorkSetConst EVENT_WORK_0x4097, 1
    // "In Rotation Battles, you send out three\nPokémon at a time and battle![f000]븁\u0000\nOne Pokémon takes the lead position,\nand the other two stand on each side.[f000]븁\u0000\nThe trick is, each turn you can change\ntheir positions...[f000]븁\u0000\nAnd that's a rough explanation\nof Rotation Battles.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_RotationBattlesSendOut, 0, 0, 0
    VMCall L_0A2F
    VMJump L_09F4

L_09E4:
    // "Oh, man! Getting someone's attention is\nreally hard."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_OhManGettingSomeones, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_09F4:
    VMJump L_0A29

L_09FA:
    VMStackPush EVENT_WORK_0x4097
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A19
    VMCall L_0A2F
    VMJump L_0A29

L_0A19:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nRiding a bike and becoming the wind fits\na bad boy like me."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles_4, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0A29:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0A2F:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nHey! If you're a Trainer, how about a\nRotation Battle?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles_2, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ADA
    WorkSetConst 0x8024, 0
    PokePartyGetCount 0x8024, 2
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0A87
    // "I hate to burst your bubble when you're\nall fired up, but...[f000]븁\u0000\nIn Rotation Battles, you need three or\nmore Pokémon to battle."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_HateBurstBubbleWhen, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0AD4

L_0A87:
    // "You've got a good attitude, don't you![f000]븁\u0000\nI'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI'm always at full throttle.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_YouveGotGoodAttitude, 0, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_MOTORCYCLIST_CHARLES_4, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ABC
    CallTrainerBattleEnd
    VMJump L_0ABE

L_0ABC:
    CallTrainerLose

L_0ABE:
    // "Sheesh. That's embarrassing. Getting\nschooled when I was planning to teach.[f000]븁\u0000\nStill, you have potential![f000]븁\u0000\nYou have to understand your Pokémon\nto win in a Rotation Battle.[f000]븁\u0000\nIf you want more Rotation Battles,\ngo to the Pokémon World Tournament!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_SheeshThatsEmbarrassingGetting, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x4097, 2

L_0AD4:
    VMJump L_0AEA

L_0ADA:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI have some advice for you.\nChallenge is the essence of life!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles_3, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0AEA:
    WorkSetConst 0x8024, 0
    VMReturn

L_0AF2:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nHey! If you're a Trainer, how about a\nTriple Battle?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles_6, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B9D
    WorkSetConst 0x8025, 0
    PokePartyGetCount 0x8025, 2
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0B4A
    // "I hate to burst your bubble when you're\nall fired up, but...[f000]븁\u0000\nIn Triple Battles, you need three or\nmore Pokémon to battle."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_HateBurstBubbleWhen_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0B97

L_0B4A:
    // "You've got a good attitude, don't you![f000]븁\u0000\nI'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI'm always at full throttle.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_YouveGotGoodAttitude_2, 0, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_MOTORCYCLIST_CHARLES_3, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B7F
    CallTrainerBattleEnd
    VMJump L_0B81

L_0B7F:
    CallTrainerLose

L_0B81:
    // "Sheesh. That's embarrassing. Getting\nschooled when I was planning to teach.[f000]븁\u0000\nStill, you have potential![f000]븁\u0000\nYou have to understand your Pokémon\nto win in a Triple Battle.[f000]븁\u0000\nIf you want more Triple Battles,\ngo to the Pokémon World Tournament."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_SheeshThatsEmbarrassingGetting_2, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x4097, 2

L_0B97:
    VMJump L_0BAD

L_0B9D:
    // "I'm a heartbreaker...\nMy name... Charles.[f000]븁\u0000\nI have some advice for you.[f000]븁\u0000\nChallenge is the essence of life!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ImHeartbreakerNameCharles_7, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0BAD:
    WorkSetConst 0x8025, 0
    VMReturn

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So how about this city's pride and joy,\nthe drawbridge?[f000]븁\u0000\nWe also call it the Charizard Bridge\ndue to its elegant form!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_HowAboutCitysPride, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Elite Four of the Pokémon League\nare extremely tough![f000]븁\u0000\nI hear you can't battle them unless\nyou have eight Gym Badges!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_EliteFourPokemonLeague, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When there's an item in your Bag\nyou want to switch,[f000]븀\u0000\njust press SELECT[f000]븀\u0000\nand give it a new niche! ♪[f000]븁\u0000\nDoesn't that jingle take you back?"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_WhenTheresItemBag, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A long time ago, Team Plasma\nstole my Pokémon...[f000]븁\u0000\nWell, they did give it back later!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_LongTimeAgoTeam, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to level up my dear Pokémon\nso they never have to feel[f000]븀\u0000\nthe sting of defeat!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_WantLevelUpDear, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_27:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Rokorroook!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_Rokorroook, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Whoooa, dude! If a Pokémon uses the move\nSurf, it can catch a wave!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_WhoooaDudeIfPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Even though I used to look\nat the Cold Storage every day...[f000]븁\u0000\nI've already forgotten\nwhat it looked like..."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_EvenThoughUsedLook, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A Pokémon of thunder and\na Pokémon of wind were roaming[f000]븀\u0000\neverywhere and causing trouble![f000]븁\u0000\nThen they were punished\nby a Pokémon of the soil.[f000]븁\u0000\nI like that story!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_PokemonThunderPokemonWind, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "On the other side of the ocean...\nAnd all over the world, there sure[f000]븀\u0000\nare a lot of different Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_OtherSideOceanAll, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are former members of\nTeam Plasma in there...[f000]븁\u0000\nI'm worried that they might\nbe up to no good again..."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ThereFormerMembersTeam, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bridges connect different lands.[f000]븁\u0000\nTrading and battling with Pokémon\ncan connect different people.[f000]븁\u0000\nI guess that means Pokémon\nare a kind of bridge as well!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_BridgesConnectDifferentLands, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_28:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Construction on a shortcut\nto Twist Mountain has started.[f000]븁\u0000\nBut it's going to take a while\n'cause digging's difficult."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ConstructionShortcutTwistMountain, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_30:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    PlayerGetDir 0x8020
    VMStackPush 0x8022
    VMStackPushConst 397
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D77
    ActorCmdExec 20, Movement_0DCC
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0D6F
    VMSleep 16
    ActorCmdExec 255, Movement_0E2C

L_0D6F:
    ActorCmdWait
    VMJump L_0DA0

L_0D77:
    ActorCmdExec 20, Movement_0DD8
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0D9E
    VMSleep 16
    ActorCmdExec 255, Movement_0E24

L_0D9E:
    ActorCmdWait

L_0DA0:
    // "Construction on a shortcut\nto Twist Mountain has started.[f000]븁\u0000\nBut it's going to take a while\n'cause digging's difficult.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity_Text_ConstructionShortcutTwistMountain_2, 20, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0DF4
    VMSleep 8
    ActorCmdExec 20, Movement_0E3C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0DCC:
    Move 32, 1
    Move 75, 1
    MoveEnd

Movement_0DD8:
    Move 33, 1
    Move 75, 1
    MoveEnd
    Move 13, 1
    MoveEnd

Movement_0DEC:
    Move 12, 1
    MoveEnd

Movement_0DF4:
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

Movement_0E24:
    Move 32, 1
    MoveEnd

Movement_0E2C:
    Move 33, 1
    MoveEnd

Movement_0E34:
    Move 34, 1
    MoveEnd

Movement_0E3C:
    Move 35, 1
    MoveEnd

Movement_0E44:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
