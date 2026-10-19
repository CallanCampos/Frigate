#include "asm/field_script.inc"
#include "text/script/nacrene_city_11.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9698, 0, 0xecf56, 0x58000, 0x5500f, 0x58000, 1
    EvCameraWait
    ActorCmdExec 255, Movement_0340
    ActorCmdWait
    FadeInBlackQ
    EvCameraReturn 60
    EvCameraWait
    FadeWait
    ActorCmdExec 1, Movement_02C8
    ActorCmdWait
    ActorCmdExec 0, Movement_0320
    ActorCmdWait
    // "Lenora: Why hello, Burgh.[f000]븁\u0000\nWhat's up?\nSuffering from artist's block again?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraWhyHelloBurgh, 0, 0, 0
    MsgWinCloseAll
    // "Burgh: Hmm...\nBones, perhaps...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghHmmBonesPerhaps, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0328
    ActorCmdWait
    // "Lenora: Bones...?[f000]븁\u0000\nThis is a museum,\nso of course we have bones.[f000]븁\u0000\nWhat are you looking for?\nYour next motif?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraBonesMuseumCourse, 0, 0, 0
    MsgWinCloseAll
    // "Burgh: Maybe...[f000]븁\u0000\nOr maybe not...[f000]븁\u0000\nMany Bug-type Pokémon have\nhard coverings, right?[f000]븀\u0000\nTheir so-called exoskeletons.[f000]븁\u0000\nSo I don't quite understand bones.\nNot at all, actually.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghMaybeMaybeNot, 1, 0, 0
    MsgWinCloseAll
    // "Lenora: So, did you think you would\nunderstand something about bones[f000]븀\u0000\nif you came here?[f000]븁\u0000\nSure thing!\nMake yourself at home![f000]븁\u0000\nIf you'd like, I can have my husband\nexplain them to you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraDidThinkWould, 0, 0, 0
    MsgWinCloseAll
    // "Burgh: Mmm...[f000]븁\u0000\nWell, for starters, what do bones\nmean to you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghMmmWellStarters, 1, 0, 0
    MsgWinCloseAll
    // "Lenora: When I was little, my dad\nworked in a mine, and he would always[f000]븀\u0000\nbring home bones he dug up.[f000]븁\u0000\nI was just a child, but I was brought\nunder the spell of bones.[f000]븁\u0000\nWhat did the bones look like\nwhen they were part of a living being?[f000]븁\u0000\nWhy are they shaped the way they are?\nThings like that captivated me.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraWhenLittleDad, 0, 0, 0
    MsgWinCloseAll
    // "Burgh: Oohhh...\nI kind of understand.[f000]븁\u0000\nI'm also fascinated by the functional,\nefficient designs of Bug-type Pokémon,[f000]븀\u0000\nsuch as the shape of their legs.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghOohhhKindUnderstand, 1, 0, 0
    MsgWinCloseAll
    // "???: Hey there!\nWhat's everyone all gathered up for?[f000]븁\u0000"
    InfoMsg NacreneCity11_Text_HeyThereWhatsEveryone, 1
    InfoMsgClose_0039
    ActorCmdExec 2, Movement_02C8
    ActorCmdWait
    ActorCmdExec 1, Movement_0310
    ActorCmdWait
    // "Burgh: What brings you here?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghWhatBringsHere, 1, 0, 0
    MsgWinCloseAll
    // "Clay: Found a Fossil\nin Twist Mountain.[f000]븁\u0000\nI came to have Lenora look at it and see\nif ya wanted to keep it here or somethin'.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_ClayFoundFossilTwist, 2, 0, 0
    MsgWinCloseAll
    // "Burgh: Clay.\nWhat do bones mean to you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghClayWhatBones, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0318
    ActorCmdWait
    // "Clay: Hmm?\nWell now...[f000]븁\u0000\nNo matter who ya are or when ya die,\nbones are all that'll be left of ya.[f000]븁\u0000\nWhen ya think about it, it's amazin'.\nEven if ya die, ya can make money![f000]븀\u0000\nSome Fossils are worth a lot, ya know![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_ClayHmmWellNow, 2, 0, 0
    MsgWinCloseAll
    // "Lenora: I can't tell if you're kidding,\nbut of course you would think that way.[f000]븁\u0000\nBurgh is trying hard to think\nabout what bones mean to him.[f000]븁\u0000\nDon't be so flippant![f000]븁\u0000\nSo what kind of Fossil have you got\nthere? Let me see.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraCantTellIf, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_02F8
    VMSleep 8
    ActorCmdExec 1, Movement_02F8
    ActorCmdWait
    // "Clay: Here ya go![f000]븁\u0000\nWhaddaya think?[f000]븁\u0000\nIt's gotta be worth a mint\nas a specimen or as a collectible![f000]븀\u0000\nWhich is it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_ClayHereYaGo, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0358
    ActorCmdWait
    ActorCmdExec 2, Movement_0310
    ActorCmdExec 1, Movement_0310
    ActorCmdWait
    VMSleep 60
    // "Lenora: Well, I mean this in a good way,\nbut it's an ordinary Fossil.[f000]븁\u0000\nIf it isn't worth much as a specimen,\nyou can't sell it for much.[f000]븁\u0000\nBut I'm curious about the soil\nattached to the Fossil...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraWellMeanGood, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0328
    ActorCmdWait
    // "Burgh: What do you mean?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghWhatMean, 1, 0, 0
    MsgWinCloseAll
    // "Lenora: It's very subtle,\nbut it's a bit different from the soil[f000]븀\u0000\nI usually see from Twist Mountain.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraItsVerySubtle, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0308
    ActorCmdWait
    // "Clay: That so?[f000]븁\u0000\nGuess my Twist Mountain\nstill has unlimited potential![f000]븁\u0000\nAll right!\nI'll keep on diggin' my Clay Tunnel![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_ClayGuessTwistMountain, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0368
    VMSleep 10
    ActorCmdExec 0, Movement_0300
    ActorCmdExec 1, Movement_0300
    ActorCmdWait
    EvCameraMoveTo 9688, 0, 0xed000, 0x58000, 0, 0x98000, 30
    EvCameraWait
    // "Here, Burgh![f000]븁\u0000\nThat plain ol' Fossil's yours![f000]븁\u0000\nHave a look at it\nand think about what bones mean to ya.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_HereBurghPlainOl, 2, 2, 0
    MsgWinCloseAll
    EvCameraReturn 30
    ActorCmdExec 2, Movement_0348
    ActorCmdWait
    EvCameraWait
    VMSleep 30
    ActorCmdExec 0, Movement_0318
    ActorCmdExec 1, Movement_0310
    ActorCmdWait
    // "Burgh: Thank you, Lenora.[f000]븁\u0000\nI guess it's OK that I have\nmy own perspective on what[f000]븀\u0000\nbones mean to me, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghThankLenoraGuess, 1, 0, 0
    MsgWinCloseAll
    // "Lenora: Why not?[f000]븁\u0000\nYou, me, Clay...\nWe all have a different way of thinking[f000]븀\u0000\nabout what bones mean to us.[f000]븁\u0000\nIt would be kind of creepy if we all felt\nthe exact same way, right?[f000]븁\u0000\nAnyway, I feel sorry for this Fossil.[f000]븁\u0000\nIt's just resting, minding its own\nbusiness, and everyone is saying[f000]븀\u0000\nthis and that about it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_LenoraWhyNotClay, 0, 0, 0
    MsgWinCloseAll
    // "Burgh: I know.[f000]븁\u0000\nI'll thank this Fossil\nby using it as a motif[f000]븀\u0000\nfor one of my works.[f000]븁\u0000\nSee you, Lenora.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity11_Text_BurghKnowIllThank, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0350
    EvCameraMoveTo 9698, 0, 0xecf56, 0x58000, 0x5500f, 0x58000, 30
    VMSleep 8
    ActorCmdExec 0, Movement_0300
    FadeOutBlack
    FadeWait
    ActorCmdWait
    EvCameraWait
    RTReserveScript 19
    EvCameraRebind
    EvCameraEnd
    MapChangeCore ZONE_NACRENE_CITY_2, 12, 3, 6, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_02C8:
    Move 12, 6
    MoveEnd
    Move 9, 1
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

Movement_02F8:
    Move 32, 1
    MoveEnd

Movement_0300:
    Move 33, 1
    MoveEnd

Movement_0308:
    Move 29, 1
    MoveEnd

Movement_0310:
    Move 34, 1
    MoveEnd

Movement_0318:
    Move 35, 1
    MoveEnd

Movement_0320:
    Move 75, 1
    MoveEnd

Movement_0328:
    Move 159, 1
    MoveEnd
    Move 71, 1
    Move 169, 1
    Move 72, 1
    MoveEnd

Movement_0340:
    Move 69, 1
    MoveEnd

Movement_0348:
    Move 13, 4
    MoveEnd

Movement_0350:
    Move 13, 6
    MoveEnd

Movement_0358:
    Move 14, 2
    Move 13, 3
    Move 35, 1
    MoveEnd

Movement_0368:
    Move 13, 1
    Move 15, 1
    Move 13, 1
    Move 9, 1
    Move 65, 1
    Move 32, 1
    MoveEnd
