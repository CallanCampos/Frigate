#include "asm/field_script.inc"
#include "text/script/floccesy_town_5.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9694, 0, 0xecf12, 0x68000, 0x5d007, 0x38000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_03B8
    ActorCmdWait
    FadeInBlackQ
    EvCameraReturn 60
    EvCameraWait
    FadeWait
    MultiMsg 0, 8, 5, 1
    VMSleep 25
    MultiMsg 1, 19, 14, 2
    VMSleep 25
    MsgWinCloseNo 1
    VMSleep 10
    MsgWinCloseNo 2
    VMSleep 40
    MultiMsg 2, 8, 5, 3
    VMSleep 25
    MultiMsg 3, 19, 14, 4
    VMSleep 25
    MsgWinCloseNo 3
    VMSleep 10
    MsgWinCloseNo 4
    VMSleep 40
    MultiMsg 4, 8, 5, 5
    VMSleep 40
    MsgWinCloseNo 5
    VMSleep 50
    // "Sensei!\nYou called?[f000]븁\u0000"
    ScreamMsg FloccesyTown5_Text_SenseiCalled, 0
    InfoMsgClose_0039
    ActorCmdExec 0, Movement_0360
    VMSleep 10
    ActorCmdExec 1, Movement_0360
    ActorCmdWait
    ActorCmdExec 2, Movement_03D8
    // "Alder: You're looking well![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_AlderYoureLookingWell, 0, 0, 0
    ActorCmdWait
    MsgWinCloseAll
    VMSleep 20
    ActorCmdExec 2, Movement_03E0
    VMSleep 20
    ActorCmdExec 0, Movement_0370
    ActorCmdWait
    // "Marshal: Hey, I heard you're\ngoing back to acting![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_MarshalHeyHeardYoure, 2, 0, 0
    MsgWinCloseAll
    // "Brycen: Indeed![f000]븁\u0000\nTeam Plasma's actions\nstirred the hearts of the public.[f000]븁\u0000\nPeople even worry about\nhow to interact with Pokémon.[f000]븁\u0000\nOf course, it's good to think\nabout that relationship, but still...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_BrycenIndeedTeamPlasmas, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0368
    VMSleep 10
    ActorCmdExec 1, Movement_0368
    ActorCmdWait
    // "Alder: Even if you're told to find\nan answer on your own...[f000]븁\u0000\nIt's not exactly an easy thing\nto do now, is it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_AlderEvenIfYoure, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_03C8
    ActorCmdWait
    ActorCmdExec 0, Movement_0358
    ActorCmdExec 2, Movement_0358
    ActorCmdWait
    // "Brycen: That's why...[f000]븁\u0000\nIf people can see my bond with my Pokémon\nwhen they're watching my movies,[f000]븀\u0000\nit might ease their worries...[f000]븁\u0000\nThat was my reasoning...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_BrycenThatsWhyIf, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0318
    ActorCmdWait
    // "Marshal: Oh! I get it now![f000]븁\u0000\nCan't communicate that just\nby holing up in the Gym and[f000]븀\u0000\ntalking to challengers.[f000]븁\u0000\nBut, are you gonna be all right?\nWhat if you get hurt during filming again?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_MarshalOhGetNow, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0368
    ActorCmdExec 0, Movement_0370
    ActorCmdWait
    // "Alder: Marshal...\nBrycen has already decided.[f000]븁\u0000\nSometimes worrying can\ndestroy potential.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_AlderMarshalBrycenHas, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0380
    ActorCmdWait
    // "Brycen: I'm fine.\nI have no doubts about my decision.[f000]븁\u0000\nAlso, I appreciate Marshal's concern.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_BrycenImFineHave, 1, 0, 0
    ActorCmdExec 1, Movement_0378
    ActorCmdWait
    // "I was young...[f000]븁\u0000\nI thought I could do\nany kind of action scene.[f000]븁\u0000\nSo I would put myself in\nunnecessary danger.[f000]븁\u0000\nI thought this bravery was\nmy value as a person.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_YoungThoughtCouldAny, 1, 0, 0
    ActorCmdExec 1, Movement_0380
    ActorCmdWait
    // "So... Instead of trying to communicate\nsomething through my acting,[f000]븀\u0000\nI simply sought amazing action.[f000]븁\u0000\nThat was how I got hurt.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_InsteadTryingCommunicateSomething, 1, 0, 0
    MsgWinCloseAll
    // "Marshal: I think I understand.[f000]븁\u0000\nIf you seek strength for its own sake,\nthe reason why you wanted it[f000]븀\u0000\nin the first place becomes vague.[f000]븁\u0000\nAnd you end up desiring\nvictory and nothing else.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_MarshalThinkUnderstandIf, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_03F8
    ActorCmdWait
    ActorCmdExec 1, Movement_0368
    ActorCmdWait
    // "Alder: That's why I've also made\nan important decision.[f000]븁\u0000\nI'm going to resign as Champion![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_AlderThatsWhyIve, 0, 0, 0
    ActorCmdExec 0, Movement_0388
    ActorCmdWait
    // "We in particular need to take another\nlook at what it means to live together[f000]븀\u0000\nwith Pokémon.[f000]븁\u0000\nThe world at large is still shaken\nby uncertainty.[f000]븁\u0000\nSo I have to show everyone\na new hope.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_WeParticularNeedTake, 0, 0, 0
    MsgWinCloseAll
    // "Marshal: Well, if that's what\nyou've decided, Sensei.[f000]븁\u0000\nEven if I argue, that won't\nchange your decision, will it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_MarshalWellIfThats, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0390
    ActorCmdWait
    // "Alder: You've grown so much since you\nsought me out and asked to be my pupil.[f000]븁\u0000\nCould that be it?[f000]븁\u0000\nDid becoming part of the Elite Four\nhelp you grow up in its own way?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_AlderYouveGrownMuch, 0, 0, 0
    MsgWinCloseAll
    // "Marshal: I thought that if I were strong,\nmy Pokémon would become stronger.[f000]븁\u0000\nMaybe I grew up because my Pokémon\ntaught me that this isn't always[f000]븀\u0000\nthe case...[f000]븁\u0000\nWell then, will you spar with me?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_MarshalThoughtIfWere, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_040C
    ActorCmdWait
    // "Brycen: I will observe.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_BrycenWillObserve, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0414
    VMSleep 15
    ActorCmdExec 0, Movement_0360
    ActorCmdWait
    // "Alder: Thank you.[f000]븁\u0000\nI want to be sure my Pokémon are in\ntop condition to challenge the Trainer[f000]븀\u0000\nwho will become the next Champion.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_AlderThankWantSure, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0388
    ActorCmdExec 0, Movement_0390
    ActorCmdWait
    // "Alder: To strength![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_AlderStrength, 0, 0, 1
    ActorMsgClose
    VMSleep 20
    // "Marshal: To Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown5_Text_MarshalPokemon, 2, 0, 1
    ActorMsgClose
    ActorCmdExec 2, Movement_034C
    ActorCmdExec 0, Movement_0340
    ActorCmdWait
    EvCameraMoveTo 9694, 0, 0xecf12, 0x68000, 0x5d007, 0x38000, 30
    FadeOutBlack
    FadeWait
    EvCameraWait
    RTReserveScript 16
    EvCameraRebind
    EvCameraEnd
    MapChangeCore ZONE_ICIRRUS_CITY_2, 17, 0, 15, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_0318:
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

Movement_0340:
    Move 39, 1
    Move 19, 1
    MoveEnd

Movement_034C:
    Move 38, 1
    Move 18, 1
    MoveEnd

Movement_0358:
    Move 0, 1
    MoveEnd

Movement_0360:
    Move 1, 1
    MoveEnd

Movement_0368:
    Move 2, 1
    MoveEnd

Movement_0370:
    Move 3, 1
    MoveEnd

Movement_0378:
    Move 32, 1
    MoveEnd

Movement_0380:
    Move 33, 1
    MoveEnd

Movement_0388:
    Move 34, 1
    MoveEnd

Movement_0390:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_03B8:
    Move 69, 1
    MoveEnd
    Move 13, 4
    MoveEnd

Movement_03C8:
    Move 8, 1
    Move 10, 1
    Move 32, 1
    MoveEnd

Movement_03D8:
    Move 16, 6
    MoveEnd

Movement_03E0:
    Move 3, 1
    Move 75, 1
    Move 19, 2
    Move 16, 1
    Move 32, 1
    MoveEnd

Movement_03F8:
    Move 10, 1
    Move 65, 1
    Move 1, 1
    Move 3, 1
    MoveEnd

Movement_040C:
    Move 15, 1
    MoveEnd

Movement_0414:
    Move 13, 3
    Move 32, 1
    MoveEnd
