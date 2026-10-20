#include "asm/field_script.inc"
#include "text/script/pokestar_studios.h"

// Script plugin 10, from the zones that use this file

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
    ScriptEntry Script_31
    ScriptEntry Script_32
    ScriptEntry Script_33
    ScriptEntry Script_34
    ScriptEntry Script_35
    ScriptEntry Script_36
    ScriptEntry Script_37
    ScriptEntry Script_38
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPush EVENT_WORK_0x413c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C7
    FlagReset EVENT_FLAG_0x03e3
    FlagReset EVENT_FLAG_0x03e2

L_00C7:
    Cmd_02CB EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E8
    FlagReset EVENT_FLAG_0x02b3
    VMJump L_0151

L_00E8:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0109
    FlagReset EVENT_FLAG_0x02b3
    FlagReset EVENT_FLAG_0x02b4
    VMJump L_0151

L_0109:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012E
    FlagReset EVENT_FLAG_0x02b3
    FlagReset EVENT_FLAG_0x02b4
    FlagReset EVENT_FLAG_0x02b5
    VMJump L_0151

L_012E:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0151
    FlagReset EVENT_FLAG_0x02b3
    FlagReset EVENT_FLAG_0x02b4
    FlagReset EVENT_FLAG_0x02b5
    FlagReset EVENT_FLAG_0x02b6

L_0151:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x410a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0184
    ActorSetGPos 0, 32, 0, 56, 2
    ActorSetGPos 26, 30, 0, 56, 3
    VMJump L_01A3

L_0184:
    VMStackPush EVENT_WORK_0x410a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A3
    ActorSetGPos 0, 45, 0, 19, 1

L_01A3:
    VMStackPush EVENT_WORK_0x413c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0200
    ActorSetGPos 0, 45, 2, 21, 0
    ActorSetGPos 26, 31, 2, 9, 0
    ActorDelete 17
    VMStackPushFlag EVENT_FLAG_0x02b5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E9
    ActorDelete 35

L_01E9:
    VMStackPushFlag EVENT_FLAG_0x02b6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0200
    ActorDelete 36

L_0200:
    VMHalt

Script_3:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1f8000, 0, 0x398000, 30
    EvCameraWait
    // "Hm... I see...\nThat kid sounds promising...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HmSeeKidSounds, 26, 0, 0
    MsgWinCloseAll
    // "Indeed... And that kid should\nbe here any moment![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_IndeedKidShouldHere, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0E60
    ActorCmdWait
    ActorCmdExec 0, Movement_0E88
    ActorCmdWait
    // "Oh!\nAnd look who should appear![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_OhLookWhoShould, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0E60
    ActorWalkRoute 255, 31, 58, 1, 8, 0
    ActorCmdWait
    WordSetPlayerName 0
    // "Welcome...to Pokéstar Studios![f000]븁\u0000\nHey, [f000]Ā\u0001\u0000!\nWe've been waiting for you![f000]븁\u0000\nThis is our boss,\nMr. Stu Deeoh![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_WelcomePokestarStudiosHey, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0550
    ActorCmdWait
    ActorCmdExec 26, Movement_0E08
    ActorCmdWait
    ActorCmdExec 26, Movement_0E60
    ActorCmdWait
    // "Bonsoir! Hello!\nI'm Stu Deeoh! Charmed, I'm sure![f000]븁\u0000\nSo you must be [f000]Ā\u0001\u0000.\nWe were just talking about you![f000]븁\u0000\nI'd like to explain Pokéstar Studios,\nbut I need a little time to prepare![f000]븁\u0000\nI'm so sorry, dahling,\ncould you wait an eensy moment?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_BonsoirHelloImStu, 26, 3, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0DF8
    ActorCmdWait
    // "In the meantime, I'll show you around\nPokéstar Studios![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_MeantimeIllShowAround, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0E70
    VMSleep 8
    ActorCmdExec 0, Movement_0E68
    ActorCmdWait
    // "Oh, that would be maaarvelous![f000]븁\u0000\nMovies! They're amazing![f000]븁\u0000\nPokéstar Studios inspires and\nmoves people all over the world![f000]븀\u0000\nYou can make sure [f000]Ā\u0001\u0000[f000]븀\u0000\nexperiences its many charms![f000]븁\u0000\nWell then, I absolutely must\nbe off and start my preparations![f000]븀\u0000\nI'll see you in a minute!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_OhWouldMaaarvelousMovies, 26, 3, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 26, 31, 45, 0, 4, 1
    VMSleep 12
    ActorCmdExec 0, Movement_0E58
    ActorCmdWait
    ActorDelete 26
    ActorCmdExec 0, Movement_0E10
    ActorCmdWait
    ActorCmdExec 0, Movement_0E60
    ActorCmdWait
    // "That's my boss for you!\nWhat graceful footwork![f000]븁\u0000\nWell then, [f000]Ā\u0001\u0000,\ncome with me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ThatsBossWhatGraceful, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 15
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 0, Movement_0DF8
    ActorCmdWait
    ActorCmdExec 0, Movement_0E60
    ActorCmdWait
    ActorWalkRoute 0, 31, 32, 0, 8, 0
    VMSleep 4
    ActorWalkRoute 255, 31, 33, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0E60
    ActorCmdWait
    // "First, let me tell you briefly\nwhat Pokéstar Studios is all about![f000]븁\u0000\nPokéstar Studios was built for making\nfilms--it's a movie metropolis![f000]븁\u0000\nMany films are made and released\nright here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_FirstLetTellBriefly, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0508
    VMSleep 4
    ActorCmdExec 255, Movement_04E8
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 5848, 0, 0xed000, 0x2d8000, 0x3b01f, 0x138000, 40
    EvCameraWait
    // "This is the theater![f000]븁\u0000\nOf course, this is where\nthe films are shown.[f000]븀\u0000\nMovie fans from all over[f000]븀\u0000\nalso gather here![f000]븁\u0000\nHow about we have a look inside?[f000]븁\u0000"
    InfoMsg PokestarStudios_Text_TheaterCourseWhereFilms, 2
    InfoMsgClose_0039
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorWalkRoute 0, 45, 20, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 0, Movement_0E60
    ActorCmdWait
    ActorCmdExec 0, Movement_0524
    VMSleep 4
    ActorCmdExec 255, Movement_0500
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x410a, 2
    MapChangeWarp ZONE_POKESTAR_STUDIOS_4, 15, 31, 0
    RTReserveScript 10867
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_33:
    ActorsPauseAll
    FadeInBlack
    ActorCmdExec 0, Movement_0560
    VMSleep 4
    ActorCmdExec 255, Movement_0530
    FadeWait
    ActorCmdWait
    ActorCmdExec 0, Movement_0570
    VMSleep 8
    ActorCmdExec 255, Movement_053C
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 5592, 0, 0x105000, 0x148000, 0x3000f, 0x118000, 40
    EvCameraWait
    // "This is the filming studio![f000]븁\u0000\nThis is where Pokéstar Studios\nmovies are born![f000]븁\u0000\nWell now, come inside!\nMr. Deeoh should be waiting![f000]븁\u0000"
    InfoMsg PokestarStudios_Text_FilmingStudioWherePokestar, 2
    InfoMsgClose_0039
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 0, Movement_058C
    ActorCmdWait
    ActorCmdExec 0, Movement_059C
    VMSleep 4
    ActorCmdExec 255, Movement_0584
    ActorCmdWait
    FlagSet EVENT_FLAG_0x0988
    RTReserveScript 10819
    MapChangeWarp ZONE_POKESTAR_STUDIOS_3, 14, 21, 0
    WorkSetConst EVENT_WORK_0x410a, 3
    WorkSetConst EVENT_WORK_0x40ac, 4
    FlagReset EVENT_FLAG_0x02d4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 15, 1
    Move 12, 18
    MoveEnd
    Move 12, 18
    MoveEnd
    Move 14, 1
    Move 12, 18
    MoveEnd

Movement_04E8:
    Move 12, 1
    Move 15, 4
    Move 12, 5
    Move 15, 10
    Move 12, 6
    MoveEnd

Movement_0500:
    Move 12, 3
    MoveEnd

Movement_0508:
    Move 15, 4
    Move 12, 5
    Move 15, 10
    Move 12, 6
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_0524:
    Move 12, 2
    Move 69, 1
    MoveEnd

Movement_0530:
    Move 13, 9
    Move 34, 1
    MoveEnd

Movement_053C:
    Move 14, 10
    Move 12, 3
    Move 14, 15
    Move 32, 1
    MoveEnd

Movement_0550:
    Move 71, 1
    Move 12, 1
    Move 72, 1
    MoveEnd

Movement_0560:
    Move 13, 8
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_0570:
    Move 14, 9
    Move 12, 3
    Move 14, 16
    Move 32, 1
    MoveEnd

Movement_0584:
    Move 12, 3
    MoveEnd

Movement_058C:
    Move 12, 1
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_059C:
    Move 12, 3
    MoveEnd
    Move 34, 1
    Move 62, 1
    Move 35, 1
    Move 62, 1
    Move 33, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    ActorCmdExec 0, Movement_0E88
    ActorCmdWait
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PlayerGetGPos 0x8023, 0x8024
    WorkAdd 0x8024, 1
    ActorWalkRoute 0, 0x8023, 0x8024, 0, 8, 0
    ActorCmdWait
    WordSetPlayerName 0
    // "Hey! [f000]Ā\u0001\u0000!\nToday, Pokéstar Studios is having[f000]븀\u0000\na special ceremony![f000]븁\u0000\nMr. Stu Deeoh is waiting!\nCome with me, won't you?!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HeyTodayPokestarStudios, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 43, 27, 4, 8, 1
    VMSleep 4
    ActorWalkRoute 255, 44, 27, 4, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0E60
    ActorCmdExec 0, Movement_0E60
    ActorCmdExec 20, Movement_0E20
    ActorCmdWait
    // "[f000]Ā\u0001\u0000, congratulations![f000]븁\u0000\nToday is a special day for you\nand for Pokéstar Studios![f000]븁\u0000\nI treasure the time I've\nspent acting with you!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_CongratulationsTodaySpecialDay, 20, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0884
    VMSleep 4
    ActorCmdExec 255, Movement_0884
    VMSleep 8
    ActorCmdExec 20, Movement_0E68
    ActorCmdWait
    ActorCmdExec 21, Movement_0E20
    ActorCmdWait
    // "Hey, [f000]Ā\u0001\u0000![f000]븁\u0000\nI'm honored to have\nbeen in a movie with you![f000]븁\u0000\nMe? You don't remember me?[f000]븁\u0000\nYou can't be serious!\nI was the UFO!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HeyImHonoredHave, 21, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0890
    VMSleep 4
    ActorCmdExec 255, Movement_0890
    VMSleep 8
    ActorCmdExec 21, Movement_0E68
    ActorCmdWait
    ActorCmdExec 25, Movement_0E00
    ActorCmdWait
    ActorCmdExec 25, Movement_0E80
    ActorCmdWait
    // "If it isn't [f000]Ā\u0001\u0000!\nYou're too much! I'm so moved![f000]븀\u0000\nCongratulations and everything![f000]븁\u0000\nI-I'm so glad I was your\nfan, [f000]Ā\u0001\u0000!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_IfIsntYoureToo, 25, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_089C
    VMSleep 2
    ActorCmdExec 255, Movement_08B0
    ActorCmdWait
    // "Acting with you...[f000]븁\u0000\nIt was pretty fun and a\ngood experience.[f000]븁\u0000\nComing clear out here\nwas worth it."
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ActingPrettyFunGood, 24, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_08C8
    VMSleep 2
    ActorCmdExec 255, Movement_08C8
    ActorCmdWait
    // "I'm proud that I was\nable to make movies with you![f000]븁\u0000\nCongratulations, [f000]Ā\u0001\u0000!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ImProudAbleMake, 23, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_08D4
    VMSleep 2
    ActorCmdExec 255, Movement_08D4
    ActorCmdWait
    ActorCmdExec 22, Movement_0E10
    ActorCmdWait
    // "How great, [f000]Ā\u0001\u0000![f000]븁\u0000\nNow listen!\nI was the director of your debut![f000]븁\u0000\nSo I'm almost like your parent!\nDon't forget to be grateful!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HowGreatNowListen, 22, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_08EC
    VMSleep 4
    ActorCmdExec 255, Movement_08EC
    VMSleep 4
    ActorCmdExec 22, Movement_08E0
    ActorCmdWait
    // "Hey, boss!\nWe're here!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HeyBossWereHere, 0, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_08F4
    VMSleep 8
    ActorCmdExec 255, Movement_0E00
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 27, Movement_0E60
    VMSleep 15
    ActorCmdExec 26, Movement_0E60
    ActorCmdWait
    // "Hm...[f000]븁\u0000\n[f000]Ā\u0001\u0000,\ntoday's a day to be remembered.[f000]븁\u0000\nWhen I built this place,\nI made a wish and a promise.[f000]븁\u0000\nFor the day Pokéstar Studios\nwould grow until it was a temple[f000]븀\u0000\nof entertainment that would[f000]븀\u0000\namaze the whole world...[f000]븁\u0000\nAnd for the day that Pokéstar Studios\ncreated a new star worthy of it...[f000]븁\u0000\nI wouldn't build anything\non this platform.[f000]븁\u0000\nBut look...[f000]븁\u0000\nThere's a bronze statue here now.\nIn other words, my wish has come true,[f000]븀\u0000\nand I fulfilled my promise.[f000]븁\u0000\nThat's right, [f000]Ā\u0001\u0000...\nIt's all thanks to you!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HmTodaysDayRemembered, 26, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    // "Your acting excites the staff\nand the other actors...[f000]븁\u0000\nYour acting charms audiences...[f000]븁\u0000\nYou're amazing...[f000]븁\u0000\nBetter said...[f000]븁\u0000\nYou're the best..."
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ActingExcitesStaffOther, 27, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 26, Movement_0DF8
    ActorCmdWait
    // "[f000]Ā\u0001\u0000![f000]븁\u0000\nYou are the true star\nof Pokéstar Studios![f000]븁\u0000\nYou're the shooting star across\nthe night sky that is the silver screen!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_TrueStarPokestarStudios, 26, 2, 0
    MEPlay SEQ_ME_POKEWOOD
    MEWait
    MsgWaitAdvance
    // "So that's why we got this\nsmall gift for you."
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ThatsWhyWeGot, 26, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0E08
    VMSleep 8
    ActorCmdExec 255, Movement_0E68
    ActorCmdWait
    // "Yessir, boss![f000]븁\u0000\nOK! [f000]Ā\u0001\u0000!\nCome this way!"
    ActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_YessirBossOkCome, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    RTReserveScript 2
    MapChangeWarp ZONE_POKESTAR_STUDIOS_5, 11, 16, 3
    FlagSet EVENT_FLAG_0x01ca
    FlagSet EVENT_FLAG_0x03e3
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    Move 15, 1
    Move 1, 1
    MoveEnd
    Move 13, 1
    Move 14, 1
    Move 1, 1
    MoveEnd
    Move 13, 6
    Move 14, 1
    Move 1, 1
    MoveEnd
    Move 13, 7
    MoveEnd

Movement_0884:
    Move 14, 4
    Move 1, 1
    MoveEnd

Movement_0890:
    Move 14, 4
    Move 1, 1
    MoveEnd

Movement_089C:
    Move 12, 3
    Move 14, 4
    Move 12, 5
    Move 3, 1
    MoveEnd

Movement_08B0:
    Move 14, 1
    Move 12, 3
    Move 14, 4
    Move 12, 4
    Move 3, 1
    MoveEnd

Movement_08C8:
    Move 12, 2
    Move 2, 1
    MoveEnd

Movement_08D4:
    Move 12, 3
    Move 3, 1
    MoveEnd

Movement_08E0:
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_08EC:
    Move 12, 2
    MoveEnd

Movement_08F4:
    Move 14, 2
    Move 35, 1
    MoveEnd

Script_5:
    ActorsPauseAll
    WordSetPlayerName 0
    Cmd_02CB EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0954
    WorkSetConst 0x8025, 0
    Random 0x8025, 4
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8026, 12
    WorkAdd 0x8026, 0x8025
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8026, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0968

L_0954:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Why, if it isn't [f000]Ā\u0001\u0000!\nI thought you were my AD![f000]븁\u0000\nA star as big as you\ncan be in one of my films![f000]븁\u0000\nWould you replace that Lillipup\nand play the Pokémon's part?"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_WhyIfIsntThought, 0, 0
    LastKeyWait
    ActorMsgClose

L_0968:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WordSetPlayerName 0
    Cmd_02CB EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_09AE
    SEPlay SEQ_SE_MESSAGE
    // "Well now...[f000]븁\u0000\nI'd like to see a love story\njust like ours!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_WellNowIdLike, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_09C0

L_09AE:
    SEPlay SEQ_SE_MESSAGE
    // "Hm... What movie to see...[f000]븁\u0000\n[f000]Ā\u0001\u0000 is the one who\nalways acts really well, right?"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HmWhatMovieSee, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_09C0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 506, 0
    // "Bwoo! Bowoof!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_BwooBowoof, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm a rookie assistant director.[f000]븁\u0000\nI love movies, but\nI'm not good at making them yet.[f000]븁\u0000\nThe director is really strict,\nand he's always getting mad at me.[f000]븁\u0000\nBut...[f000]븁\u0000\nWorking for him is teaching me a lot."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ImRookieAssistantDirector, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Special sets and movie props\nare stored inside![f000]븁\u0000\nBefore VFX became so advanced,\nwe actually had to make a lot of things!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_SpecialSetsMovieProps, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Man... That director's\ngoing off again...[f000]븁\u0000\nHis films may be pretty good,\nbut there's a fine line between[f000]븀\u0000\ngenius and insanity..."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ManDirectorsGoingOff, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Just looking at the posters is exciting!\nWhich movie should I watch today?"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_JustLookingPostersExciting, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You're a Pokéstar Studios\nactor as well, right?[f000]븁\u0000\nA single line from an actor\ncan change an entire film![f000]븁\u0000\nMovie shoots are full of\npossibilities at Pokéstar Studios!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_YourePokestarStudiosActor, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm rehearsing right now.[f000]븁\u0000\nIn order to make a good film,\ndoing a lot of work before the shoot[f000]븀\u0000\nis really important!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ImRehearsingRightNow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There's a dressing room\ninside this trailer.[f000]븁\u0000\nInside they're doing costume fitting,\nmakeup, and script checks![f000]븁\u0000\nI just finished changing!\nI'm going to give it my best today, too!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_TheresDressingRoomInside, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WordSetPlayerName 0
    Cmd_02CB EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0AE2
    SEPlay SEQ_SE_MESSAGE
    // "Hey, darling!\nWhat are we going to watch today?"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HeyDarlingWhatWe, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0AF4

L_0AE2:
    SEPlay SEQ_SE_MESSAGE
    // "C'mon, darling!\nLet's watch one of [f000]Ā\u0001\u0000's[f000]븀\u0000\nmovies today!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_CmonDarlingLetsWatch, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0AF4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WordSetPlayerName 0
    Cmd_02CB EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0B30
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello there! How about a portrait of one\nof Pokéstar Studios' famous stars?[f000]븁\u0000\nRight now...[f000]븁\u0000\nBrycen's is a hot ticket\nwith women and kids.[f000]븁\u0000\nSabrina's is extremely popular with guys![f000]븁\u0000\nHuh? I'm afraid we don't carry one\nof [f000]Ā\u0001\u0000."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HelloThereHowAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0B71

L_0B30:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0B5D
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Good day! How about a portrait of one of\nPokéstar Studios' famous stars?[f000]븁\u0000\nRight now...[f000]븁\u0000\nBrycen's is very popular with\nwomen and kids.[f000]븁\u0000\nSabrina's is extremely popular with guys![f000]븁\u0000\nHuh? We've started stocking portraits\nof [f000]Ā\u0001\u0000 recently, but the sales[f000]븀\u0000\nare nothing to write home about."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_GoodDayHowAbout, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0B71

L_0B5D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello there! How about a portrait of one\nof Pokéstar Studios' famous stars?[f000]븁\u0000\nRight now, the most popular is...[f000]븁\u0000\n[f000]Ā\u0001\u0000. There's no doubt about it!\nHey, has anyone ever mentioned[f000]븀\u0000\nyou look kinda like [f000]Ā\u0001\u0000?[f000]븁\u0000\nI'm really jealous!\nI wish I resembled a star like that."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HelloThereHowAbout_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_0B71:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    WordSetPlayerName 0
    Cmd_02CB EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0BAD
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Someday, I'm going to be in pictures,\nbecome a famous actor,[f000]븀\u0000\nand buy a mansion![f000]븁\u0000\nA really big mansion! A huge one!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_SomedayImGoingPictures, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0BC1

L_0BAD:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh! It's the star who was in the movie\nI just watched--[f000]Ā\u0001\u0000![f000]븁\u0000\n[f000]Ā\u0001\u0000, you're a star,\nso you're living in a mansion, right?[f000]븀\u0000\nA really big one? A huge one?"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_OhItsStarWho, 0, 0
    LastKeyWait
    ActorMsgClose

L_0BC1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hear Pokéstar Studios\nmakes horror films as well.[f000]븁\u0000\nMan, who even watches kids'\nstuff like that anyway?!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HearPokestarStudiosMakes, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, I know him![f000]븁\u0000\nHe was the guy yelling “Mama!\"\nduring the movie with ghosts!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HeyKnowHimHe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When I look at this from here,\nI feel like I've become a monster![f000]븁\u0000\nRoar![f000]븁\u0000\nHa ha ha..."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_WhenLookFromHere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Brilliant actors come to Pokéstar Studios\nfrom all over the world to make movies![f000]븁\u0000\nThat's why the titles of movies are\nin so many different languages!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_BrilliantActorsComePokestar, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Not just anybody can take part\nin filming at Pokéstar Studios.[f000]븁\u0000\nOnly Trainers approved by the owner\ncan participate."
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_NotJustAnybodyCan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorCmdExec 18, Movement_0E78
    ActorCmdWait
    // "Good grief...\nI've been waiting for three hours...[f000]븁\u0000\nHow long does it take\nto put on makeup anyway?"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_GoodGriefIveBeen, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The captain of the illustrious guard\nprotecting the star Sabrina is me![f000]븁\u0000\nI rushed here when I heard she had\nmade a shocking debut as an actress.[f000]븁\u0000\nBut recently, I've been interested\nin a star named [f000]Ā\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_CaptainIllustriousGuardProtecting, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Recently, films starring\n[f000]Ā\u0001\u0000 are getting a lot of buzz.[f000]븁\u0000\n...Wait? [f000]Ā\u0001\u0000?\nI wish I had something for you to sign!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_RecentlyFilmsStarringGetting, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_26:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ciao! Come va?\nAre you used to Pokéstar Studios yet?[f000]븁\u0000\nThe staff here is very international!\nIt's very exciting, isn't it?[f000]븁\u0000\nSee you during a shoot someday!\nBuona giornata!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_CiaoComeVaUsed, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_27:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "¡Todos me consideran\nuna niña prodigio de la actuación! ¡Por no[f000]븀\u0000\nhablar de que soy una auténtica estrella![f000]븁\u0000\n¡Mi popularidad y mi destreza como actriz\nestán a años luz de ti, principiante!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_TodosConsideranUnaNi, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_28:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Excuse me, I'm her manager.[f000]븁\u0000\nWhat she just said is,[f000]븁\u0000\n“People know me as a\nbrilliant child actress![f000]븀\u0000\nNot to mention I’m a top star![f000]븀\u0000\nMy popularity and acting skills[f000]븀\u0000\nare way beyond yours, rookie!\""
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_ExcuseImHerManager, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_29:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Guten Tag![f000]븁\u0000\nSince you've come to Pokéstar Studios,\nwe suit actors have been busy.[f000]븁\u0000\nBis bald!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_GutenTagSinceYouve, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_30:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey! It's [f000]Ā\u0001\u0000!\nC'mon, give me an autograph!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HeyItsCmonGive, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_31:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Why, [f000]Ā\u0001\u0000...[f000]븁\u0000\nLet me ask you dis... When will you do me\nda honor of being in one of my films?"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_WhyLetAskDis, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_32:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "¡Hola! ¿Qué tal?\nYou've become so famous![f000]븁\u0000\nEven me, the top star in my country,\ncan't compete with you in Unova!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios_Text_HolaQueTalYouve, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_34:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Pokéstar Studios Sound Stage\nNo public access!"
    InfoMsg PokestarStudios_Text_PokestarStudiosSoundStage, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_35:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This fake building is so\ndetailed it looks real."
    InfoMsg PokestarStudios_Text_FakeBuildingDetailedLooks, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_36:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's filled with movie props\nfor shooting films."
    InfoMsg PokestarStudios_Text_ItsFilledMovieProps, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_37:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This detailed model of the Royal Unova\nis a set for a movie."
    InfoMsg PokestarStudios_Text_DetailedModelRoyalUnova, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_38:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a set for a movie.[f000]븁\u0000\nIt's a model of the Skyarrow Bridge\ndone to 1/144 scale."
    InfoMsg PokestarStudios_Text_ItsSetMovieIts, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0DF8:
    Move 13, 1
    MoveEnd

Movement_0E00:
    Move 12, 1
    MoveEnd

Movement_0E08:
    Move 15, 1
    MoveEnd

Movement_0E10:
    Move 14, 1
    MoveEnd
    Move 9, 1
    MoveEnd

Movement_0E20:
    Move 8, 1
    MoveEnd
    Move 11, 1
    MoveEnd
    Move 10, 1
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

Movement_0E58:
    Move 32, 1
    MoveEnd

Movement_0E60:
    Move 33, 1
    MoveEnd

Movement_0E68:
    Move 34, 1
    MoveEnd

Movement_0E70:
    Move 35, 1
    MoveEnd

Movement_0E78:
    Move 36, 4
    MoveEnd

Movement_0E80:
    Move 48, 2
    MoveEnd

Movement_0E88:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
