#include "asm/field_script.inc"
#include "text/script/global_10865.h"

// Script plugin 10, from the zones that start its scripts

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_4:
    VMStackPush EVENT_WORK_0x410a
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0041
    ActorSetGPos 25, 15, 0, 29, 1
    ActorSetGPos 26, 12, 0, 23, 3

L_0041:
    VMHalt
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

Script_3:
    ActorsPauseAll
    // "Here we are!\nThis is the reception area.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_HereWeReceptionArea, 25, 2, 0
    MsgWinCloseAll
    ActorWalkRoute 255, 15, 24, 1, 8, 1
    VMSleep 4
    ActorWalkRoute 25, 15, 23, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 26, Movement_02B0
    ActorCmdWait
    ActorCmdExec 26, Movement_0B7C
    VMSleep 8
    ActorCmdExec 255, Movement_0BCC
    ActorCmdExec 25, Movement_0BCC
    ActorCmdWait
    // "Oh, if it isn't the scout himself!\nThanks to you, I've made my movie debut![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_OhIfIsntScout, 26, 2, 0
    MsgWinCloseAll
    // "Wow!\nI'm glad to hear that![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WowImGladHear, 25, 1, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0B6C
    ActorCmdWait
    ActorCmdExec 26, Movement_0BD4
    ActorCmdWait
    // "And who would that be?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WhoWould, 26, 2, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    // "This is [f000]Ā\u0001\u0000![f000]븁\u0000\nI witnessed this Trainer challenging your\ndaughter. It struck me--here's someone[f000]븀\u0000\nwith real potential![f000]븁\u0000\nA fresh discovery for Pokéstar Studios![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WitnessedTrainerChallengingDaughter, 25, 1, 0
    MsgWinCloseAll
    // "You beat Roxie, I see.\nYou must be a pretty good Trainer.[f000]븁\u0000\nOh, you know what?\nThey're releasing my movie now![f000]븁\u0000\nAre you thinking about making a\nPokéstar Studios debut, too?[f000]븁\u0000\nThen, I do insist you watch my movie\nin order to gain acting tips from a[f000]븀\u0000\nprofessional such as myself.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_BeatRoxieSeeMust, 26, 2, 0
    MsgWinCloseAll
    ActorCmdExec 25, Movement_02B0
    ActorCmdWait
    // "That's a good idea![f000]븁\u0000\n[f000]Ā\u0001\u0000, it should be a good\nintroduction to filmmaking for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_ThatsGoodIdeaShould, 25, 1, 0
    MsgWinCloseAll
    // "Awesome! Let's go![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_AwesomeLetsGo, 26, 2, 0
    MsgWinCloseAll
    VMCall L_02CC
    // "...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_Empty, 26, 1, 0
    MsgWinCloseAll
    ActorSetGPos 25, 15, 0, 17, 0
    ActorWalkRoute 25, 15, 13, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0BC4
    ActorCmdWait
    // "I hope you had a good time!\nHow did you enjoy the movie?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_HopeHadGoodTime, 25, 2, 0
    MsgWinCloseAll
    // "...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_Empty_2, 26, 1, 0
    MsgWinCloseAll
    // "What's the matter?"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WhatsMatter, 25, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0B8C
    VMSleep 16
    ActorCmdExec 25, Movement_0BCC
    ActorCmdExec 255, Movement_0BCC
    ActorCmdWait
    // "Ha ha ha. I've always dreamed of making\nmy debut on the silver screen...[f000]븁\u0000\nNow I've fulfilled that dream...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_HaHaHaIve, 26, 1, 0
    MsgWinCloseAll
    // "What are you saying?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WhatSaying, 25, 2, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0B94
    ActorCmdWait
    // "Different people have different talents.\nI knew it all along.[f000]븁\u0000\nBut I see my daughter striving to\nbalance the responsibility of being a[f000]븀\u0000\nGym Leader while pursuing her own[f000]븀\u0000\ndream of being in a band.[f000]븁\u0000\nAlthough I'm a father, I really wanted\nto pursue my own long-held dream...[f000]븁\u0000\nIf you stop pursuing your dreams,\nyour life might as well be over, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_DifferentPeopleHaveDifferent, 26, 1, 0
    MsgWinCloseAll
    // "...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_Empty_3, 25, 2, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0BD4
    ActorCmdWait
    WordSetPlayerName 0
    // "I'm going back to my duties as a ship\ncaptain for now.[f000]븁\u0000\nSomeday, I'll return to the screen![f000]븁\u0000\nUntil that day comes, I'll leave\nall the starring roles to you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_ImGoingBackDuties, 26, 1, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_02C0
    VMSleep 10
    ActorCmdExec 25, Movement_0BC4
    ActorCmdExec 255, Movement_0BC4
    ActorCmdWait
    VMSleep 20
    ActorCmdExec 25, Movement_0BBC
    ActorCmdWait
    // "We had better get going.\nMr. Deeoh should be ready by now.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WeHadBetterGet, 25, 2, 0
    MsgWinCloseAll
    FadeOutBlack
    ActorWalkRoute 25, 15, 18, 1, 8, 1
    ActorWalkRoute 255, 15, 17, 1, 8, 1
    FadeWait
    ActorCmdWait
    MapChangeCore ZONE_POKESTAR_STUDIOS, 45, 3, 18, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 12, 6
    MoveEnd

Movement_02B0:
    Move 75, 1
    MoveEnd
    Move 12, 3
    MoveEnd

Movement_02C0:
    Move 13, 7
    Move 69, 1
    MoveEnd

L_02CC:
    ActorCmdExec 26, Movement_03C8
    ActorCmdWait
    ActorCmdExec 26, Movement_03D4
    VMSleep 4
    ActorCmdExec 255, Movement_03E4
    ActorCmdExec 25, Movement_0BBC
    ActorCmdWait
    // "Welcome to the Pokéstar Studios theater.\nThis is the reception counter.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WelcomePokestarStudiosTheater, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0BCC
    VMSleep 8
    ActorCmdExec 255, Movement_0BD4
    ActorCmdWait
    // "This is where you choose\nthe movie you want to watch.[f000]븁\u0000\nThere are eight screens in the theater\nhere in Pokéstar Studios![f000]븀\u0000\nPretty impressive, huh?[f000]븁\u0000\nNow...\nHuh?[f000]븁\u0000\nIt seems my movie will be\nshown on a special screen.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WhereChooseMovieWant, 26, 1, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_03F8
    VMSleep 8
    ActorCmdExec 255, Movement_03BC
    ActorCmdWait
    PlayerGetGPos 0x8027, 0x8028
    WorkSub 0x8028, 1
    BMCreateHandleByGPos 0x8025, 1, 15, 8
    BMHndAudioVisualAnmPlay 0x8025, 0
    BMHndAnmWait 0x8025
    ActorCmdExec 26, Movement_0B74
    VMSleep 4
    ActorCmdExec 255, Movement_0B74
    ActorCmdWait
    FadeOutBlackQ
    FadeWait
    BMReleaseHandle 0x8025
    PokewoodCmd_Create
    Plugin10_Cmd1026
    PokewoodCmd_Free
    PlayerGetGPos 0x8027, 0x8028
    BMCreateHandleByGPos 0x8025, 1, 0x8027, 0x8028
    BMHndAudioVisualAnmPlay 0x8025, 1
    BMHndAnmPause 0x8025
    FadeInBlackQ
    FadeWait
    ActorCmdExec 255, Movement_041C
    ActorCmdExec 26, Movement_040C
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8025, 1
    BMHndAnmWait 0x8025
    VMReturn

Movement_03BC:
    Move 15, 7
    Move 12, 1
    MoveEnd

Movement_03C8:
    Move 15, 1
    Move 35, 1
    MoveEnd

Movement_03D4:
    Move 12, 11
    Move 14, 5
    Move 12, 2
    MoveEnd

Movement_03E4:
    Move 14, 1
    Move 12, 11
    Move 14, 6
    Move 12, 2
    MoveEnd

Movement_03F8:
    Move 15, 6
    Move 12, 2
    MoveEnd
    Move 13, 3
    MoveEnd

Movement_040C:
    Move 13, 2
    Move 14, 1
    Move 13, 1
    MoveEnd

Movement_041C:
    Move 13, 2
    Move 63, 2
    Move 34, 1
    MoveEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokewoodCmd_Create
    WorkSetConst 0x8020, 2

L_043C:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0510
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0462
    VMJump L_047E

L_0462:
    // "Please come again!"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_PleaseComeAgain, 0x8011, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    VMJump L_050A

L_047E:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0491
    VMJump L_049D

L_0491:
    VMCall L_0518
    VMJump L_050A

L_049D:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_04B0
    VMJump L_04BC

L_04B0:
    VMCall L_05F3
    VMJump L_050A

L_04BC:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_04CF
    VMJump L_04DF

L_04CF:
    FlagReset EVENT_FLAG_0x09f3
    VMCall L_0632
    VMJump L_050A

L_04DF:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_04F2
    VMJump L_0504

L_04F2:
    VMCall L_08D1
    WorkSetConst 0x8020, 0
    VMJump L_050A

L_0504:
    WorkSetConst 0x8020, 0

L_050A:
    VMJump L_043C

L_0510:
    PokewoodCmd_Free
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0518:
    WorkSetConst 0x8022, 1

L_051E:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05F1
    // "Welcome to Pokéstar Studios.\nHow may I help you?"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WelcomePokestarStudiosHow, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32801
    ListMenuAdd 0, 65535, 0
    ListMenuAdd 1, 65535, 1
    ListMenuAdd 2, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_0573
    VMJump L_05BA

L_0573:
    PokewoodCmd_CountDownloadedMovies 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05A8
    // "I apologize.\nWe have no movies available to show.[f000]븁\u0000\nI wonder if anyone would be interested\nin helping to create new movies, hmm...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_ApologizeWeHaveNo, 0x8011, 2, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8020, 1
    VMJump L_05B4

L_05A8:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8020, 3

L_05B4:
    VMJump L_05EB

L_05BA:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_05CD
    VMJump L_05DF

L_05CD:
    // "This is where the movies developed\nat the film studio are released.[f000]븁\u0000\nYou'll find a monitor in the movie theater\nto check the records of these movies.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WhereMoviesDevelopedFilm, 0x8011, 2, 0
    VMJump L_05EB

L_05DF:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8020, 1

L_05EB:
    VMJump L_051E

L_05F1:
    VMReturn

L_05F3:
    // "Please select the movie\nyou'd like to watch.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_PleaseSelectMovieYoud, 0x8011, 2, 0
    MsgWinCloseAll
    Plugin10_Cmd1003 0, 0x8023
    DebugPrint 0x8023
    VMStackPush 0x8023
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_062A
    WorkSetConst 0x8020, 1
    VMJump L_0630

L_062A:
    WorkSetConst 0x8020, 4

L_0630:
    VMReturn

L_0632:
    FunfestBGMReturn
    // "The movie is about to begin.\nPlease enjoy.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_MovieAboutBeginPlease, 0x8011, 2, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0748
    ActorCmdWait
    PlayerGetGPos 0x8027, 0x8028
    WorkSub 0x8028, 1
    BMCreateHandleByGPos 0x8025, 1, 0x8027, 0x8028
    BMHndAudioVisualAnmPlay 0x8025, 0
    BMHndAnmWait 0x8025
    ActorCmdExec 255, Movement_0754
    ActorCmdWait
    FadeOutBlackQ
    FadeWait
    BMReleaseHandle 0x8025
    Plugin10_Cmd1001 0x8023, 0x8026
    Plugin10_Cmd1017 0x8023, 0x8010
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06B3
    VMCall L_0764

L_06B3:
    VMStackPush EVENT_WORK_0x413c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06E3
    Plugin10_Cmd1021 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06E3
    WorkSetConst EVENT_WORK_0x413c, 1

L_06E3:
    PlayerGetGPos 0x8027, 0x8028
    BMCreateHandleByGPos 0x8025, 1, 0x8027, 0x8028
    BMHndAudioVisualAnmPlay 0x8025, 1
    BMHndAnmPause 0x8025
    ActorCmdExec 255, Movement_0BA4
    ActorCmdWait
    FadeInBlackQ
    FadeWait
    ActorCmdExec 255, Movement_075C
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8025, 1
    BMHndAnmWait 0x8025
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_073E
    WorkSetConst 0x8020, 5
    VMJump L_0744

L_073E:
    WorkSetConst 0x8020, 0

L_0744:
    VMReturn
    .balign 4, 0

Movement_0748:
    Move 15, 6
    Move 12, 2
    MoveEnd

Movement_0754:
    Move 12, 1
    MoveEnd

Movement_075C:
    Move 13, 3
    MoveEnd

L_0764:
    Cmd_02CB 0x8010
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPush 0
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x03d4
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0797
    FlagReset EVENT_FLAG_0x03d4
    ActorAdd 1

L_0797:
    VMStackPush 0x8010
    VMStackPush 1
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x03d5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07CA
    FlagReset EVENT_FLAG_0x03d5
    ActorAdd 2
    ActorAdd 3
    ActorAdd 4

L_07CA:
    VMStackPush 0x8010
    VMStackPush 2
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x03d6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0801
    FlagReset EVENT_FLAG_0x03d6
    ActorAdd 5
    ActorAdd 10
    ActorAdd 7
    ActorAdd 8

L_0801:
    VMStackPush 0x8010
    VMStackPush 3
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x03d7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0840
    FlagReset EVENT_FLAG_0x03d7
    ActorAdd 9
    ActorAdd 6
    ActorAdd 11
    ActorAdd 12
    ActorAdd 13
    ActorAdd 14

L_0840:
    VMStackPush 0x8010
    VMStackPush 4
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x03d8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0887
    FlagReset EVENT_FLAG_0x03d8
    ActorAdd 15
    ActorAdd 16
    ActorAdd 17
    ActorAdd 18
    ActorAdd 19
    ActorAdd 20
    ActorAdd 21
    ActorAdd 22

L_0887:
    WorkGet EVENT_WORK_0x4002, 0x8023
    WorkSetConst EVENT_WORK_0x4005, 0
    WorkSetConst EVENT_WORK_0x4006, 0
    WorkSetConst EVENT_WORK_0x4007, 0
    WorkSetConst EVENT_WORK_0x4008, 0
    WorkSetConst EVENT_WORK_0x4009, 0
    WorkSetConst EVENT_WORK_0x400a, 0
    WorkSetConst EVENT_WORK_0x400b, 0
    WorkSetConst EVENT_WORK_0x400c, 0
    WorkSetConst EVENT_WORK_0x400d, 0
    WorkSetConst EVENT_WORK_0x400e, 0
    WorkSetConst EVENT_WORK_0x400f, 0
    VMReturn

L_08D1:
    Plugin10_Cmd1017 0x8023, 0x8010
    Plugin10_Cmd1022 0x8023
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0906
    WorkSetConst EVENT_WORK_0x4000, 1
    WorkGet EVENT_WORK_0x4001, 0x8023
    WorkSetConst EVENT_WORK_0x4003, 1
    VMJump L_090C

L_0906:
    WorkSetConst EVENT_WORK_0x4000, 0

L_090C:
    VMReturn

Script_5:
    ActorsPauseAll
    VMCall L_0922
    WorkSetConst EVENT_WORK_0x4000, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0922:
    ActorNew 15, 31, 0, 251, 345, 0
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorCmdExec 251, Movement_0AD8
    ActorCmdWait
    WordSetPlayerName 0
    // "Well, well!\nIf it isn't [f000]Ā\u0001\u0000, dahling![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_WellWellIfIsnt, 251, 1, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8027, 0x8028
    WorkCmpConst 0x8027, 14
    VMJumpIf CMP_EQ, L_096A
    VMJump L_0984

L_096A:
    ActorCmdExec 251, Movement_0AE0
    ActorCmdWait
    ActorCmdExec 255, Movement_0B00
    ActorCmdWait
    VMJump L_09D4

L_0984:
    WorkCmpConst 0x8027, 15
    VMJumpIf CMP_EQ, L_0997
    VMJump L_09A7

L_0997:
    ActorCmdExec 251, Movement_0AEC
    ActorCmdWait
    VMJump L_09D4

L_09A7:
    WorkCmpConst 0x8027, 16
    VMJumpIf CMP_EQ, L_09BA
    VMJump L_09D4

L_09BA:
    ActorCmdExec 251, Movement_0AF4
    ActorCmdWait
    ActorCmdExec 255, Movement_0B08
    ActorCmdWait
    VMJump L_09D4

L_09D4:
    WordSetPlayerName 0
    Plugin10_Cmd1010 EVENT_WORK_0x4001, 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_09F0
    VMJump L_0A02

L_09F0:
    // "[f000]Ā\u0001\u0000--this particular effort\nturned out to be...[f000]븁\u0000\n...a spec-tac-ulah hit, dahling![f000]븁\u0000\nKeep it up, and you'll be loved by\nmovie buffs everywhere--and our staff.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_ParticularEffortTurnedOut_2, 251, 2, 0
    VMJump L_0A4C

L_0A02:
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0A15
    VMJump L_0A27

L_0A15:
    // "[f000]Ā\u0001\u0000--this particular effort\nturned out to be...[f000]븁\u0000\n...a complete flop, dahling.\nBut I thought it was quite a unique[f000]븀\u0000\nlittle film in its own way. I liked it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_ParticularEffortTurnedOut, 251, 2, 0
    VMJump L_0A4C

L_0A27:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0A3A
    VMJump L_0A4C

L_0A3A:
    // "[f000]Ā\u0001\u0000, this time your movie\nturned out to...[f000]븁\u0000\n...exceed everyone's expectations!\nEverybody's talking about it, dahling.[f000]븁\u0000\nWith an amazing film like this, movie\nbuffs--and our staff--will definitely[f000]븀\u0000\nnotice you, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_TimeMovieTurnedOut, 251, 2, 0
    VMJump L_0A4C

L_0A4C:
    Plugin10_Cmd1018 EVENT_WORK_0x4001, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A71
    // "Dahling, [f000]Ā\u0001\u0000, I must tell you...\nYour Pokémon that performed[f000]븀\u0000\nin this movie may be showing more[f000]븀\u0000\nacting chops than before.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_DahlingMustTellPokemon, 251, 2, 0

L_0A71:
    // "Bye for now, [f000]Ā\u0001\u0000! I'm so looking\nforward to your next picture, dahling![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10865_Text_ByeNowImLooking, 251, 2, 0
    MsgWinCloseAll
    WorkCmpConst 0x8027, 14
    VMJumpIf CMP_EQ, L_0A9F
    WorkCmpConst 0x8027, 16
    VMJumpIf CMP_EQ, L_0A9F
    VMJump L_0AAD

L_0A9F:
    ActorCmdExec 251, Movement_0B10
    VMJump L_0ACE

L_0AAD:
    WorkCmpConst 0x8027, 15
    VMJumpIf CMP_EQ, L_0AC0
    VMJump L_0ACE

L_0AC0:
    ActorCmdExec 251, Movement_0B18
    VMJump L_0ACE

L_0ACE:
    ActorCmdWait
    ActorDelete 251
    VMReturn
    .balign 4, 0

Movement_0AD8:
    Move 75, 1
    MoveEnd

Movement_0AE0:
    Move 12, 5
    Move 34, 1
    MoveEnd

Movement_0AEC:
    Move 12, 4
    MoveEnd

Movement_0AF4:
    Move 12, 5
    Move 35, 1
    MoveEnd

Movement_0B00:
    Move 35, 1
    MoveEnd

Movement_0B08:
    Move 34, 1
    MoveEnd

Movement_0B10:
    Move 12, 9
    MoveEnd

Movement_0B18:
    Move 14, 1
    Move 12, 10
    MoveEnd

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8029, 0
    PokewoodCmd_CountMovieFlags 5, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0B53
    SEPlay SEQ_SE_MESSAGE
    Plugin10_Cmd1023 0x8010
    VMJump L_0B5D

L_0B53:
    // "This monitor is currently being tested."
    SystemMsg Global10865_Text_MonitorCurrentlyBeingTested, 2
    LastKeyWait
    InfoMsgClose

L_0B5D:
    WorkSetConst 0x8029, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0B6C:
    Move 13, 1
    MoveEnd

Movement_0B74:
    Move 12, 1
    MoveEnd

Movement_0B7C:
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_0B8C:
    Move 10, 1
    MoveEnd

Movement_0B94:
    Move 9, 1
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_0BA4:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_0BBC:
    Move 32, 1
    MoveEnd

Movement_0BC4:
    Move 33, 1
    MoveEnd

Movement_0BCC:
    Move 34, 1
    MoveEnd

Movement_0BD4:
    Move 35, 1
    MoveEnd
    Move 42, 4
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
