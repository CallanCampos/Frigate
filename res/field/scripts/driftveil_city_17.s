#include "asm/field_script.inc"
#include "text/script/driftveil_city_17.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9690, 0, 0xecfe0, 0x78000, 0x3f000, 0x128000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_0330
    ActorCmdWait
    FadeInBlack
    EvCameraReturn 60
    EvCameraWait
    FadeWait
    // "Grunt 1: Did you hear?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1DidHear, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0310
    ActorCmdWait
    // "Grunt 2: Did I hear what?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt2DidHear, 1, 1, 0
    MsgWinCloseAll
    // "Grunt 1: Well, I ran into some of\nour old allies.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1WellRan, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0308
    ActorCmdWait
    // "Grunt 2: Oh! Those who are still\nin Team Plasma?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt2OhThose, 1, 1, 0
    MsgWinCloseAll
    // "Grunt 1: They tried to convince me\nto rejoin Team Plasma.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1TheyTried, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0318
    ActorCmdWait
    // "Grunt 2: Seriously?[f000]븁\u0000\nWell, quite a few of them\nare still in Team Plasma.[f000]븁\u0000\nLord Ghetsis... No, just Ghetsis.[f000]븁\u0000\nI guess he was actually pretty good.[f000]븁\u0000\nHe's awesome!\nHaha. Just kidding![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt2SeriouslyWell, 1, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0338
    ActorCmdWait
    // "Grunt 1: What? How dare you say that?[f000]븁\u0000\nHow can you say such a thing even after\nyou know what he's done to Lord N?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1WhatHow, 0, 1, 0
    MsgWinCloseAll
    // "Grunt 2: Of course,\nthat was disgusting.[f000]븁\u0000\nBut even if you live honestly,\nI'm telling you, the world is cold.[f000]븁\u0000\nAs if you were surrounded by Cryogonal.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt2CourseDisgusting, 1, 1, 0
    MsgWinCloseAll
    // "Grunt 1: Yeah, you're right.[f000]븁\u0000\nIt has flaws, but\nTeam Plasma is Team Plasma.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1YeahYoure, 0, 1, 0
    MsgWinCloseAll
    // "Grunt 2: Don't you think we could have\ngone to a region far away?[f000]븁\u0000\nOrganizations like Team Plasma\nmust exist everywhere.[f000]븀\u0000\nWe might have gotten favorable treatment[f000]븀\u0000\nbecause of our experience![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt2DontThink, 1, 1, 0
    MsgWinCloseAll
    // "Grunt 1: Stupid, stupid, stupid![f000]븁\u0000\nAnd did I mention stupid?[f000]븁\u0000\nIt all adds up to nincompoop![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1StupidStupid, 0, 1, 1
    ActorMsgClose
    ActorCmdExec 0, Movement_0344
    SEPlay SEQ_SE_FLD_04
    ActorCmdWait
    SEWait
    ActorCmdExec 0, Movement_0344
    VMSleep 4
    SEPlay SEQ_SE_FLD_04
    ActorCmdWait
    SEWait
    // "Ouch, ouch, ouch, ouch, ouch!\nOwwwww![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_OuchOuchOuchOuch, 1, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0358
    ActorCmdWait
    // "Grunt 1: We have to do something\nfor Pokémon. Did you forget that?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1WeHave, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0360
    ActorCmdWait
    // "Grunt 2: Honestly, I'm sorry.[f000]븁\u0000\nBut Pokémon have a hard time, too.\nDon't you think?[f000]븁\u0000\nOnce they are in Poké Balls, that's it...[f000]븁\u0000\nThat might happen. Still, do they really\nlike their Trainers?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt2HonestlyIm, 1, 1, 0
    MsgWinCloseAll
    // "Grunt 1: Yeah![f000]븁\u0000\nBut Lord N was listening to such feelings\nand words from Pokémon...[f000]븁\u0000\nJust imagining it\ncan break my heart.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt1YeahBut, 0, 1, 0
    MsgWinCloseAll
    // "Grunt 2: Lord N...\nI wonder where he is...[f000]븁\u0000\nNow, let's go find the Pokémon\nin N's Castle.[f000]븀\u0000\nWe have to protect them.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_Grunt2LordN, 1, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0368
    VMSleep 8
    ActorCmdExec 1, Movement_0374
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 2, Movement_0380
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 2, Movement_02E8
    ActorCmdWait
    // "N: [f000]븉\u0001\u0001I'm here, though...[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_NImHereThough, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_038C
    ActorCmdWait
    // "[f000]븉\u0001\u0001I've been worried about\nthe people who believed in me...[f000]븁\u0000\nBut it seems they know\nwhat they can do for Pokémon[f000]븀\u0000\nin their own ways.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_IveBeenWorriedAbout, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0280
    ActorCmdWait
    // "[f000]븉\u0001\u0001Compared to them...\nWhat was I doing?[f000]븁\u0000\nWhat I really should have done was\ntell people about how Pokémon feel...![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_ComparedThemWhatDoing, 2, 1, 0
    // "[f000]븉\u0001\u0001The legendary Dragon-type\nPokémon knew that.[f000]븁\u0000\nIt has lived long\nand known many people.[f000]븁\u0000\nIt knew humans and Pokémon have lived\nand will live together.[f000]븁\u0000\nIt knew that in this relationship,\nhumans' actions have an enormous impact[f000]븀\u0000\non Pokémon.[f000]븁\u0000\nThat's why it helps the one\nwho searches for truth...[f000]븀\u0000\nthe one who opens the way to the future.[f000]븉\u0001\u0000[f000]븁\u0000"
    // "[f000]븉\u0001\u0001The legendary Dragon-type\nPokémon knew that.[f000]븁\u0000\nIt has lived long\nand known many people.[f000]븁\u0000\nIt knew humans and Pokémon have lived\nand will live together.[f000]븁\u0000\nIt knew that in this relationship,\nhumans' actions have an enormous impact[f000]븀\u0000\non Pokémon.[f000]븁\u0000\nThat's why it helps the one\nwho searches for truth...[f000]븀\u0000\nthe one who opens the way to the future.[f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsgVersioned 1024, DriftveilCity17_Text_LegendaryDragonTypePokemon_2, DriftveilCity17_Text_LegendaryDragonTypePokemon, 2, 1, 0
    MsgWinCloseAll
    VMSleep 15
    // "[f000]븉\u0001\u0001It's not necessary...[f000]븁\u0000\nSeparating black from white\nand humans from Pokémon![f000]븁\u0000\nIf you think in terms of each\nindividual life, this world was in a state[f000]븀\u0000\nthat couldn't be divided any further.[f000]븁\u0000\nPossibilities are born out of\ncombining and fusing these[f000]븀\u0000\ndifferent lives![f000]븁\u0000\nThere are some things we can\nunderstand only by doing this.[f000]븁\u0000\nIt will give form to unseen things.[f000]븁\u0000\nThese formulae will restructure\nthe world and make it richer![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_ItsNotNecessarySeparating, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_02E8
    ActorCmdWait
    // "[f000]븉\u0001\u0001From their Poké Balls, I can hear the\nmany different feelings Pokémon have[f000]븀\u0000\nabout their Trainers![f000]븁\u0000\nMore than anything, I can hear their joy\nthat they met people who need them![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_FromTheirPokeBalls, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_02F0
    ActorCmdWait
    // "[f000]븉\u0001\u0001I'll go...[f000]븁\u0000\nFor Pokémon,\nfor Trainers,[f000]븀\u0000\nand for all lives...[f000]븁\u0000\nAnd for my friends\nwho saved me![f000]븉\u0001\u0000[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity17_Text_IllGoPokemonTrainers, 2, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_03A0
    EvCameraMoveTo 9690, 0, 0xecfe0, 0x78000, 0x3f000, 0x128000, 30
    FadeOutBlack
    FadeWait
    ActorCmdWait
    FlagSet EVENT_FLAG_0x02b9
    EvCameraWait
    RTReserveScript 17
    EvCameraRebind
    EvCameraEnd
    MapChangeCore ZONE_DRIFTVEIL_CITY_6, 7, 0, 25, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0280:
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 12, 6
    MoveEnd
    Move 9, 1
    MoveEnd
    Move 39, 1
    Move 19, 1
    MoveEnd
    Move 38, 1
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

Movement_02E8:
    Move 32, 1
    MoveEnd

Movement_02F0:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_0308:
    Move 75, 1
    MoveEnd

Movement_0310:
    Move 159, 1
    MoveEnd

Movement_0318:
    Move 161, 1
    MoveEnd
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_0330:
    Move 69, 1
    MoveEnd

Movement_0338:
    Move 75, 1
    Move 39, 4
    MoveEnd

Movement_0344:
    Move 71, 1
    Move 19, 1
    Move 18, 1
    Move 72, 1
    MoveEnd

Movement_0358:
    Move 39, 4
    MoveEnd

Movement_0360:
    Move 38, 4
    MoveEnd

Movement_0368:
    Move 15, 1
    Move 13, 8
    MoveEnd

Movement_0374:
    Move 14, 1
    Move 13, 8
    MoveEnd

Movement_0380:
    Move 15, 2
    Move 13, 8
    MoveEnd

Movement_038C:
    Move 35, 1
    Move 34, 1
    Move 33, 1
    Move 182, 1
    MoveEnd

Movement_03A0:
    Move 13, 8
    MoveEnd
