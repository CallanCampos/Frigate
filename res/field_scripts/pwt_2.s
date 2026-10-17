#include "asm/field_script.inc"
#include "text/script/pwt_2.h"

// Script plugin 6, from the zones that use this file

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
    ScriptEntriesEnd

Script_9:
    VMStackPush EVENT_WORK_0x4135
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0091
    FlagReset EVENT_FLAG_0x037c
    FlagReset EVENT_FLAG_0x037b
    ActorAdd 14
    ActorAdd 5
    ActorSetGPos 13, 15, 0, 11, 0
    ActorSetGPos 14, 14, 0, 11, 0
    ActorSetGPos 5, 14, 0, 9, 1

L_0091:
    VMHalt

Script_8:
    ActorsPauseAll
    ActorWalkRoute 255, 15, 25, 1, 8, 1
    VMSleep 8
    FlagReset EVENT_FLAG_0x037b
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 5
    SEWait
    ActorCmdWait
    ActorCmdExec 255, Movement_04DC
    ActorCmdExec 5, Movement_0508
    ActorCmdWait
    // "Clay: This time, I'm gonna have ya\nparticipate in the Driftveil Tournament.[f000]븁\u0000\nAnything goes in this here tournament![f000]븁\u0000\nEight people will be participatin', and if\nya win three times, yer the champion![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_ClayTimeImGonna, 13, 0, 0
    MsgWinCloseAll
    ActorCmdExec 14, Movement_04BC
    ActorCmdWait
    // "Cheren: Why did you call me, too?\nI'm busy looking into something![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_CherenWhyDidCall, 14, 0, 0
    MsgWinCloseAll
    ActorCmdExec 13, Movement_04C4
    ActorCmdWait
    // "Clay: Ya know somethin', Cheren.\nThe one who's gotta show everyone[f000]븀\u0000\nwhat Pokémon battlin' means--is you.[f000]븁\u0000\nAnd yer pal ain't here, either![f000]븁\u0000\nI'm countin' on the power of youth,\n'cause everyone likes[f000]븀\u0000\nup-and-comin' stars![f000]븁\u0000\nOK! Whenever yer ready,\nget on over to reception![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_ClayYaKnowSomethin, 13, 0, 0
    MsgWinCloseAll
    // "Cheren: Man oh man...\nYou never change, Clay.[f000]븁\u0000\nBut the tournament itself\ndoes look pretty fun![f000]븁\u0000\nOK!\nI'll go register![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_CherenManOhMan, 14, 0, 0
    MsgWinCloseAll
    ActorCmdExec 14, Movement_04E4
    VMSleep 8
    ActorCmdExec 13, Movement_04CC
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Me, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_Too, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_04C4
    VMSleep 3
    ActorCmdExec 255, Movement_04BC
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000. C'mon! Let's have some fun.\nWe'll battle, plain and simple![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_CmonLetsHaveSome, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0518
    VMSleep 8
    ActorCmdExec 255, Movement_04CC
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 0, Movement_04EC
    VMSleep 16
    ActorCmdExec 14, Movement_04E4
    ActorCmdExec 5, Movement_0528
    ActorCmdWait
    ActorDelete 14
    ActorDelete 5
    FlagSet EVENT_FLAG_0x037c
    FlagSet EVENT_FLAG_0x037b
    FlagSet EVENT_FLAG_0x0989
    HollowRivalCmd_0262 2, 1
    HollowRivalCmd_0262 1, 13
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    ActorWalkRoute 255, 15, 9, 1, 8, 0
    ActorCmdWait
    // "Clay: An outstandin' battle, runts![f000]븁\u0000\nNow everybody's gonna want to\njoin in on this here tournament[f000]븀\u0000\nan' show their stuff![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_ClayOutstandinBattleRunts, 13, 0, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x037e
    ActorAdd 16
    ActorCmdExec 16, Movement_0530
    VMSleep 32
    ActorCmdExec 255, Movement_04BC
    ActorCmdExec 13, Movement_04BC
    VMSleep 3
    ActorCmdExec 5, Movement_04BC
    ActorCmdExec 14, Movement_04BC
    ActorCmdWait
    WordSetPlayerName 0
    // "Roxie: Hey! You two![f000]븁\u0000\nHaven't you got any wild and crazy\nPokémon battles to show me?![f000]븁\u0000\nGuess I'll have to enter the\ntournament myself and rock the[f000]븀\u0000\naudience right outta their seats![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_RoxieHeyTwoHavent, 16, 0, 0
    MsgWinCloseAll
    ActorCmdExec 16, Movement_053C
    VMSleep 24
    ActorCmdExec 13, Movement_04CC
    VMSleep 3
    ActorCmdExec 255, Movement_04D4
    VMSleep 3
    ActorCmdExec 5, Movement_04D4
    ActorCmdExec 14, Movement_04CC
    ActorCmdWait
    // "Clay: See what I mean?\nPeople are pourin' in already![f000]븁\u0000\nIf the strongest Trainers from\nall over join in, it'll raise up[f000]븀\u0000\neverybody's level of skill![f000]븁\u0000\nAn' then, li'l ol' Driftveil City\nwill grow even more and make[f000]븀\u0000\na heap of money![f000]븁\u0000\nSo keep on bustin' those battles\nand rilin' everybody up! See ya![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt2_Text_ClaySeeWhatMean, 13, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 13, 15, 19, 1, 8, 1
    VMSleep 8
    ActorCmdExec 14, Movement_04D4
    ActorCmdWait
    ActorDelete 16
    ActorDelete 13
    MapReplaceSetEvent 4, 1, 1
    MapReplaceSetEvent 3, 0, 0
    FlagSet EVENT_FLAG_0x037e
    FlagSet EVENT_FLAG_0x037d
    FlagSet EVENT_FLAG_0x037c
    FlagSet EVENT_FLAG_0x037b
    FlagReset EVENT_FLAG_0x037a
    WorkSetConst EVENT_WORK_0x40c6, 2
    WorkSetConst EVENT_WORK_PLASMA_FRIGATE_LOCATION, 0
    FlagReset EVENT_FLAG_0x0989
    FlagReset EVENT_FLAG_0x03e9
    HollowRivalCmd_0262 1, 14
    HollowRivalCmd_0262 2, 2
    WorkSetConst EVENT_WORK_0x4135, 2
    ActorAdd 17
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Clay: Whenever yer ready,\nget on over to reception!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_ClayWheneverYerReady, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Colress: By having battles with\nmany Trainers, I can bring out[f000]븀\u0000\nPokémon's abilities![f000]븁\u0000\nEventually, as I continue to battle,\nthe truth of my theory[f000]븀\u0000\nwill be evident to all!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_ColressByHavingBattles, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Cheren: The Unova Gym Leaders\nwill probably participate in order[f000]븀\u0000\nto improve their skills."
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_CherenUnovaGymLeaders, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0001: Aww...[f000]븁\u0000\nI wanted to win the tournament\nthe first time I participated!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_AwwWantedWinTournament, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Someday, I'll be a famous guy![f000]븁\u0000\nBut for now, I'm just a spectator here."
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_SomedayIllFamousGuy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Many Trainers from far away will come\nto participate!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_ManyTrainersFromFar, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Clay is awesome![f000]븁\u0000\nI heard he started all this to encourage\nDriftveil City's development!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_ClayAwesomeHeardHe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Two years ago, this was\nthe Cold Storage area!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_TwoYearsAgoCold, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That's the Pokémon World Tournament\nfor you! It's packed with spectators!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_ThatsPokemonWorldTournament, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's crazy popular! This is what\npacked to the rafters means!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_ItsCrazyPopularWhat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What can I do when\neverything's sold out?"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_WhatCanWhenEverythings, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's the Pokémon World Tournament!\nIt's all in the name![f000]븁\u0000\nTrainers have gathered\nfrom all over the world!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_ItsPokemonWorldTournament, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello there, Trainer.\nLet me fill you in about Battle Points.[f000]븁\u0000\nBattle Points, also known as BP, are\npoints you get for winning streaks in[f000]븀\u0000\neither Nimbasa City's Battle Subway[f000]븀\u0000\nor this tournament.[f000]븁\u0000\nWin a lot, save up lots of points, and\nyou can exchange them for useful items!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt2_Text_HelloThereTrainerLet, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 253
    WorkSet 0x8001, 2
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 254
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04BC:
    Move 35, 1
    MoveEnd

Movement_04C4:
    Move 34, 1
    MoveEnd

Movement_04CC:
    Move 32, 1
    MoveEnd

Movement_04D4:
    Move 33, 1
    MoveEnd

Movement_04DC:
    Move 12, 11
    MoveEnd

Movement_04E4:
    Move 12, 5
    MoveEnd

Movement_04EC:
    Move 12, 1
    Move 15, 1
    Move 34, 1
    Move 63, 6
    Move 14, 1
    Move 13, 1
    MoveEnd

Movement_0508:
    Move 12, 11
    Move 15, 1
    Move 12, 1
    MoveEnd

Movement_0518:
    Move 12, 6
    Move 14, 2
    Move 32, 1
    MoveEnd

Movement_0528:
    Move 12, 5
    MoveEnd

Movement_0530:
    Move 12, 5
    Move 34, 1
    MoveEnd

Movement_053C:
    Move 13, 6
    MoveEnd
