#include "asm/field_script.inc"
#include "text/script/global_10815.h"

// Script plugin 10, from the zones that start its scripts

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd

Script_1:
    FlagSet EVENT_FLAG_0x0392
    FlagReset EVENT_FLAG_0x0393
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004F
    FlagReset EVENT_FLAG_0x03c3
    FlagReset EVENT_FLAG_0x03c4
    VMJump L_0057

L_004F:
    FlagSet EVENT_FLAG_0x03c3
    FlagSet EVENT_FLAG_0x03c4

L_0057:
    VMHalt

Script_2:
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B5
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PlayerGetGPos 0x8020, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 21
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_009D
    ActorSetGPos 1, 14, 0, 20, 0

L_009D:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_00BB

L_00B5:
    WorkSetConst EVENT_WORK_0x4000, 0

L_00BB:
    VMStackPushFlag EVENT_FLAG_0x01ca
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DA
    ActorSetGPos 4, 2, 3, 7, 3

L_00DA:
    VMHalt
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0

Script_3:
    ActorsPauseAll
    PokewoodCmd_Create
    WorkSetConst 0x8022, 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x09f3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_015E
    WordSetPlayerName 0
    // "Hey, [f000]Ā\u0001\u0000!\nDo you want to film a new movie?[f000]븁\u0000\nI'm sorry, but new scripts are\nbeing written as we speak![f000]븁\u0000\nFor now, go and see your debut\nlike the boss told you to do!"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyWantFilmNew, 0x8011, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0164

L_015E:
    VMCall L_016C

L_0164:
    PokewoodCmd_Free
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_016C:
    // "Welcome to the soundstage\nof Pokéstar Studios![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_WelcomeSoundstagePokestarStudios, 0x8011, 2, 0
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BF
    Plugin10_Cmd1016 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B9
    WordSetPlayerName 0
    // "Hey, [f000]Ā\u0001\u0000![f000]븁\u0000\nRecently, you've even begun\nto look like a movie star!"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyRecentlyYouveEven, 0x8011, 2, 0
    SEPlay SEQ_SE_TDEMO_001
    SEWait
    MsgWaitAdvance

L_01B9:
    VMCall L_09F1

L_01BF:
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0348
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_01E5
    VMJump L_01F1

L_01E5:
    VMCall L_04A9
    VMJump L_0342

L_01F1:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_0204
    VMJump L_0235

L_0204:
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0229
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8022, 3
    VMJump L_022F

L_0229:
    VMCall L_0526

L_022F:
    VMJump L_0342

L_0235:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_0248
    VMJump L_0254

L_0248:
    VMCall L_0565
    VMJump L_0342

L_0254:
    WorkCmpConst 0x8022, 4
    VMJumpIf CMP_EQ, L_0267
    VMJump L_0273

L_0267:
    VMCall L_0620
    VMJump L_0342

L_0273:
    WorkCmpConst 0x8022, 5
    VMJumpIf CMP_EQ, L_0286
    VMJump L_02AA

L_0286:
    Cmd_02C5 3
    Cmd_01DD 14, 0x8024, 0
    VMCall L_0655
    VMCall L_034A
    VMCall L_0714
    VMJump L_0342

L_02AA:
    WorkCmpConst 0x8022, 6
    VMJumpIf CMP_EQ, L_02BD
    VMJump L_02C9

L_02BD:
    VMCall L_043A
    VMJump L_0342

L_02C9:
    WorkCmpConst 0x8022, 7
    VMJumpIf CMP_EQ, L_02DC
    VMJump L_030D

L_02DC:
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0301
    WorkSetConst EVENT_WORK_0x4001, 1
    WorkSetConst 0x8022, 0
    VMJump L_0307

L_0301:
    VMCall L_0969

L_0307:
    VMJump L_0342

L_030D:
    WorkCmpConst 0x8022, 8
    VMJumpIf CMP_EQ, L_0320
    VMJump L_033C

L_0320:
    // "Oh, I see!\nCome back later, then![f000]븁\u0000\nThe silver screen is waiting!"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OhSeeComeBack, 0x8011, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8022, 0
    VMJump L_0342

L_033C:
    WorkSetConst 0x8022, 0

L_0342:
    VMJump L_01BF

L_0348:
    VMReturn

L_034A:
    ActorCmdExec 255, Movement_0410
    ActorCmdWait
    WorkSetConst 0x8025, 1

L_035A:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F7
    WorkSetConst 0x8010, 30
    VMCall L_0420
    MsgWinCloseAll
    Plugin10_Cmd1000 0x8026, 0x8024, 0x8010
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D7
    WorkSetConst 0x8010, 41
    VMCall L_0420
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_03C3
    VMJump L_03D1

L_03C3:
    MsgWinCloseAll
    WorkSetConst 0x8022, 8
    WorkSetConst 0x8025, 0

L_03D1:
    VMJump L_03F1

L_03D7:
    WorkSetConst 0x8010, 36
    VMCall L_0420
    MsgWinCloseAll
    WorkSetConst 0x8022, 6
    WorkSetConst 0x8025, 0

L_03F1:
    VMJump L_035A

L_03F7:
    ActorCmdExec 255, Movement_0418
    ActorCmdWait
    MapChangeWarp ZONE_POKESTAR_STUDIOS_3, 14, 11, 1
    VMReturn
    .balign 4, 0

Movement_0410:
    Move 12, 6
    MoveEnd

Movement_0418:
    Move 13, 6
    MoveEnd

L_0420:
    Plugin10_Cmd1020 0x8024, 0x8027
    WorkAdd 0x8010, 0x8027
    ActorMsg MSGFILE_SCRIPT, 0x8010, 1, 2, 0
    VMReturn

L_043A:
    WorkSetConst 0x8011, 0
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0463
    WorkSetConst EVENT_WORK_0x4001, 1
    FlagSet EVENT_FLAG_0x02bd
    WorkSetConst EVENT_WORK_0x40ab, 4

L_0463:
    VMCall L_0744
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0488
    WorkSetConst 0x8022, 7
    VMJump L_04A7

L_0488:
    WorkSetConst 0x8022, 8
    VMStackPushFlag EVENT_FLAG_0x0988
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A7
    WorkSetConst EVENT_WORK_0x4001, 0

L_04A7:
    VMReturn

L_04A9:
    // "Would you like to\ntry to shoot a film?"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_WouldLikeTryShoot, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 25, 65535, 0
    ListMenuAdd 26, 65535, 1
    ListMenuAdd 27, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_04EB
    VMJump L_04F7

L_04EB:
    WorkSetConst 0x8022, 2
    VMJump L_0524

L_04F7:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_050A
    VMJump L_051E

L_050A:
    // "You can make movies here\nat Pokéstar Studios![f000]븁\u0000\nYou act with other actors\nas determined by the script.[f000]븁\u0000\nIf you meet all of the conditions\nfor completing the movie,[f000]븀\u0000\nthen it's a wrap.[f000]븁\u0000\nThen, we use the most\ncutting-edge VFX technology[f000]븀\u0000\nand finish the movie in an instant![f000]븁\u0000\nBe careful, because the\nnecessary conditions for[f000]븀\u0000\nmaking a good movie are[f000]븀\u0000\ndifferent from script to script![f000]븁\u0000\nThe final movie is released\nin the Pokéstar Studios Theater,[f000]븀\u0000\nso be sure to check it out![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_CanMakeMoviesHere, 0x8011, 2, 0
    MsgWinCloseAll
    VMJump L_0524

L_051E:
    WorkSetConst 0x8022, 8

L_0524:
    VMReturn

L_0526:
    // "OK! Pick which script\nyou want to shoot![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OkPickWhichScript, 0x8011, 2, 0
    MsgWinCloseAll
    WorkSetConst 0x8024, 41
    Plugin10_Cmd1002 0x8024
    DebugPrint 0x8024
    VMStackPush 0x8024
    VMStackPushConst 41
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_055D
    WorkSetConst 0x8022, 8
    VMReturn

L_055D:
    WorkSetConst 0x8022, 3
    VMReturn

L_0565:
    // "OK! What kind of Pokémon do you\nwant to have perform with you?"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OkWhatKindPokemon, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 29, 65535, 0
    ListMenuAdd 28, 65535, 1
    ListMenuAdd 27, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_05A7
    VMJump L_05C5

L_05A7:
    // "OK! All right![f000]븁\u0000\nThen we'll provide you with the\nperfect Pokémon for the part![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OkAllRightThen_2, 0x8011, 2, 0
    MsgWinCloseAll
    WorkSetConst 0x8022, 5
    Plugin10_Cmd1014 0
    VMJump L_061E

L_05C5:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_05D8
    VMJump L_0618

L_05D8:
    PokewoodCmd_GetMovieFlag 2, 0x8024, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0603
    WorkSetConst 0x8022, 4
    Plugin10_Cmd1014 1
    VMJump L_0612

L_0603:
    WordSetPlayerName 0
    // "So sorry![f000]븁\u0000\nI know you want to use\nyour own cool Pokémon.[f000]븁\u0000\nBut, would you film the movie with\nPokéstar Studios' Pokémon first?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_SorryKnowWantUse, 0x8011, 2, 0

L_0612:
    VMJump L_061E

L_0618:
    WorkSetConst 0x8022, 8

L_061E:
    VMReturn

L_0620:
    // "OK! All right![f000]븁\u0000\nThen pick the Pokémon\nthat will perform with you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OkAllRightThen, 0x8011, 2, 0
    MsgWinCloseAll
    Plugin10_Cmd1013 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_064D
    WorkSetConst 0x8022, 8
    VMReturn

L_064D:
    WorkSetConst 0x8022, 5
    VMReturn

L_0655:
    FunfestBGMReturn
    // "OK!\nThen let's start the shoot![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OkThenLetsStart, 0x8011, 2, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8029, 0x802a
    WorkCmpConst 0x8029, 12
    VMJumpIf CMP_EQ, L_067E
    VMJump L_068C

L_067E:
    ActorCmdExec 255, Movement_06F0
    VMJump L_06CE

L_068C:
    WorkCmpConst 0x8029, 13
    VMJumpIf CMP_EQ, L_069F
    VMJump L_06AD

L_069F:
    ActorCmdExec 255, Movement_0700
    VMJump L_06CE

L_06AD:
    WorkCmpConst 0x8029, 14
    VMJumpIf CMP_EQ, L_06C0
    VMJump L_06CE

L_06C0:
    ActorCmdExec 255, Movement_070C
    VMJump L_06CE

L_06CE:
    ActorCmdWait
    Plugin10_Cmd1019 0x8024, EVENT_WORK_0x4020, EVENT_WORK_0x4021
    FlagReset EVENT_FLAG_0x0392
    FlagSet EVENT_FLAG_0x0393
    FlagSet EVENT_FLAG_0x03c3
    MapChangeWarp ZONE_POKESTAR_STUDIOS_2, 15, 16, 0
    VMReturn

Movement_06F0:
    Move 13, 1
    Move 15, 2
    Move 12, 2
    MoveEnd

Movement_0700:
    Move 15, 1
    Move 12, 2
    MoveEnd

Movement_070C:
    Move 12, 1
    MoveEnd

L_0714:
    FlagSet EVENT_FLAG_0x0392
    FlagReset EVENT_FLAG_0x0393
    ActorCmdExec 0, Movement_0730
    ActorCmdExec 255, Movement_0738
    ActorCmdWait
    VMReturn

Movement_0730:
    Move 35, 1
    MoveEnd

Movement_0738:
    Move 13, 1
    Move 34, 1
    MoveEnd

L_0744:
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 1
    WorkSetConst 0x802c, 0

L_075C:
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0955
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_0782
    VMJump L_07E8

L_0782:
    // "Good work on the shoot![f000]븁\u0000\nWould you like to release\nthe film you just shot[f000]븀\u0000\nin the theater?"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_GoodWorkShootWould, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_07B1
    WorkSetConst 0x802b, 2
    VMJump L_07E2

L_07B1:
    PokewoodCmd_CountDownloadedMovies 0x8028
    VMStackPush 0x8028
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D4
    WorkSetConst 0x802b, 3
    VMJump L_07E2

L_07D4:
    VMCall L_09AF
    MsgWinCloseAll
    WorkSetConst 0x802b, 4

L_07E2:
    VMJump L_094F

L_07E8:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_07FB
    VMJump L_083C

L_07FB:
    // "It's really OK to not release\nthe movie you shot?"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_ItsReallyOkNot, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0830
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    VMJump L_0836

L_0830:
    WorkSetConst 0x802b, 1

L_0836:
    VMJump L_094F

L_083C:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_084F
    VMJump L_08BD

L_084F:
    // "Whoa! The screens are full![f000]븁\u0000\nIf you want to release a new\nmovie, you're going to have[f000]븀\u0000\nto end another movie's run![f000]븀\u0000\nIs that OK?"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_WhoaScreensFullIf, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08B1
    // "OK. Decide which film\nto remove from the theater.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OkDecideWhichFilm, 0x8011, 2, 0
    MsgWinCloseAll
    Plugin10_Cmd1003 1, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08A5
    WorkSetConst 0x802b, 2
    VMJump L_08AB

L_08A5:
    WorkSetConst 0x802b, 4

L_08AB:
    VMJump L_08B7

L_08B1:
    WorkSetConst 0x802b, 2

L_08B7:
    VMJump L_094F

L_08BD:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_08D0
    VMJump L_0943

L_08D0:
    DebugPrint 0x8026
    Plugin10_Cmd1004 0x8026
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    RTCallGlobal 2003
    WorkSet 0x8010, 0x8000
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0937
    WorkSetConst 0x802c, 1
    WorkSetConst 0x802b, 0
    VMJump L_093D

L_0937:
    WorkSetConst 0x802b, 2

L_093D:
    VMJump L_094F

L_0943:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0

L_094F:
    VMJump L_075C

L_0955:
    WorkGet 0x8010, 0x802c
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    VMReturn

L_0969:
    WordSetPlayerName 0
    // "OK! We'll send this straight\noff to the theater![f000]븁\u0000\n[f000]Ā\u0001\u0000, you're interested in\nhow the finished film turned out, right?[f000]븁\u0000\nWill you go to the theater right away?"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OkWellSendStraight, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09A7
    MsgWinCloseAll
    MapChangeWarp ZONE_POKESTAR_STUDIOS_4, 9, 11, 0
    WorkSetConst 0x8022, 0
    VMJump L_09AD

L_09A7:
    WorkSetConst 0x8022, 8

L_09AD:
    VMReturn

L_09AF:
    WorkSetConst 0x8026, 0

L_09B5:
    VMStackPush 0x8026
    VMStackPushConst 8
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_09EF
    PokewoodCmd_CheckDownloadedMovie 0x8026, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09E3
    VMReturn

L_09E3:
    WorkAdd 0x8026, 1
    VMJump L_09B5

L_09EF:
    VMReturn

L_09F1:
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8024, 0

L_0A15:
    VMStackPush 0x8024
    VMStackPushConst 41
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0B8C
    Plugin10_Cmd1015 0x8024, 0x802d, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B80
    WordSetPlayerName 0
    Plugin10_Cmd1005 1, 0x8024
    WorkCmpConst 0x802d, 0
    VMJumpIf CMP_EQ, L_0A5E
    VMJump L_0A70

L_0A5E:
    // "Hey, [f000]Ā\u0001\u0000!\nThe movie you were just in[f000]븀\u0000\nwas a smash hit, right?[f000]븁\u0000\nThanks to that, the screenwriter\nwrote a new script in the series.[f000]븁\u0000\nThe sequel is called\n“[f000][ff00]\u0001\u0001[f000]Ŀ\u0001\u0001[f000][ff00]\u0001\u0000.\"[f000]븁\u0000\nWe'd love for you to give it a try\nand make another smash hit![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyMovieWereJust, 0x8011, 2, 0
    VMJump L_0B80

L_0A70:
    WorkCmpConst 0x802d, 1
    VMJumpIf CMP_EQ, L_0A83
    VMJump L_0AAE

L_0A83:
    VMStackPush 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AA8
    WorkSetConst 0x802e, 1
    // "Hey, [f000]Ā\u0001\u0000![f000]븁\u0000\nThere's been a lot of buzz about\nyou lately! Several scripts have arrived[f000]븀\u0000\nfor movies they want you to be in![f000]븁\u0000\nHave a look at them when\nyou're deciding which film to try![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyTheresBeenLot, 0x8011, 2, 0

L_0AA8:
    VMJump L_0B80

L_0AAE:
    WorkCmpConst 0x802d, 2
    VMJumpIf CMP_EQ, L_0AC1
    VMJump L_0AD3

L_0AC1:
    // "Hey, [f000]Ā\u0001\u0000![f000]븁\u0000\nYour popularity's been amazing lately!\nThey've decided to make a film[f000]븀\u0000\nwith you in mind![f000]븁\u0000\nThe movie is called\n“[f000][ff00]\u0001\u0001[f000]Ŀ\u0001\u0001[f000][ff00]\u0001\u0000.\"[f000]븁\u0000\nWe'd love for you to give it your best\nand make another smash hit![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyPopularitysBeenAmazing, 0x8011, 2, 0
    VMJump L_0B80

L_0AD3:
    WorkCmpConst 0x802d, 3
    VMJumpIf CMP_EQ, L_0AE6
    VMJump L_0AF8

L_0AE6:
    // "Hey, [f000]Ā\u0001\u0000!!\nI've got some big news![f000]븁\u0000\nThe movie you were in\nshattered past box-office records![f000]븁\u0000\nMr. Deeoh is so happy that\nhe decided to make a movie[f000]븀\u0000\nto commemorate that![f000]븁\u0000\nThe movie is called\n“[f000][ff00]\u0001\u0001[f000]Ŀ\u0001\u0001[f000][ff00]\u0001\u0000.\"[f000]븁\u0000\nPlease film the movie\nand make it another smash hit![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyIveGotSome, 0x8011, 2, 0
    VMJump L_0B80

L_0AF8:
    WorkCmpConst 0x802d, 4
    VMJumpIf CMP_EQ, L_0B0B
    VMJump L_0B1D

L_0B0B:
    // "Hey, [f000]Ā\u0001\u0000![f000]븁\u0000\nYour Pokéstar Studios career\nhas been going on for a long time now.[f000]븁\u0000\nA script has been finished that's perfect\nfor a seasoned pro such as yourself![f000]븁\u0000\nIts title is\n“[f000][ff00]\u0001\u0001[f000]Ŀ\u0001\u0001[f000][ff00]\u0001\u0000.\"[f000]븁\u0000\nWe'd love for you to give it your best\nand make another smash hit![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyPokestarStudiosCareer, 0x8011, 2, 0
    VMJump L_0B80

L_0B1D:
    WorkCmpConst 0x802d, 5
    VMJumpIf CMP_EQ, L_0B30
    VMJump L_0B5B

L_0B30:
    VMStackPush 0x802f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B55
    WorkSetConst 0x802f, 1
    // "Hey, [f000]Ā\u0001\u0000!\nHow did your big-screen debut turn out?[f000]븁\u0000\nAt the very least, the boss seemed\nquite satisfied with your performance.[f000]븁\u0000\nHe brought a new script by\nto commemorate your[f000]븀\u0000\nPokéstar Studios debut![f000]븁\u0000\nGo have a look at it when\nyou want to shoot a film.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyHowDidBig, 0x8011, 2, 0

L_0B55:
    VMJump L_0B80

L_0B5B:
    WorkCmpConst 0x802d, 6
    VMJumpIf CMP_EQ, L_0B6E
    VMJump L_0B80

L_0B6E:
    // "Hey, [f000]Ā\u0001\u0000![f000]븁\u0000\nThere's a present for you today--\na new script![f000]븁\u0000\nThe title is\n“[f000][ff00]\u0001\u0001[f000]Ŀ\u0001\u0001[f000][ff00]\u0001\u0000.\"[f000]븁\u0000\nAs the title suggests,\nit was written because many people want[f000]븀\u0000\nto see you and Brycen together again![f000]븁\u0000\nWe'd love for you to give it a try\nand make another smash hit![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyTheresPresentToday, 0x8011, 2, 0
    VMJump L_0B80

L_0B80:
    WorkAdd 0x8024, 1
    VMJump L_0A15

L_0B8C:
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802d, 0
    VMReturn

Script_5:
    ActorsPauseAll
    VMSleep 10
    WordSetPlayerName 0
    // "Hey, boss![f000]븁\u0000\nI brought [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyBossBrought, 1, 2, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0E64
    ActorCmdWait
    ActorCmdExec 1, Movement_0CAC
    ActorCmdWait
    ActorCmdExec 255, Movement_0CB8
    ActorCmdWait
    // "Well now, thanks for coming![f000]븁\u0000\nFirst, let me reintroduce myself.\nMy name is Stu Deeoh![f000]븀\u0000\nI'm the owner of Pokéstar Studios![f000]븁\u0000\nSo, [f000]Ā\u0001\u0000, dahling,\nI brought you here because I have[f000]븀\u0000\na very important request of you![f000]븁\u0000\nI'll bet you've figured it out,\nbut I want you to be in[f000]븀\u0000\nPokéstar Studios' movies![f000]븁\u0000\nThe scout said you were absolutely,\npositively oozing with star potential![f000]븁\u0000\nAnd when I saw you,\nyour potential struck me[f000]븀\u0000\nlike a lightning bolt![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_WellNowThanksComing, 2, 2, 0
    MsgWinCloseAll
    // "Yessir, boss![f000]븁\u0000\nThere's no doubt in my mind that\nthis Trainer will become a top star[f000]븀\u0000\nof Pokéstar Studios' silver screen![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_YessirBossTheresNo, 1, 2, 0
    MsgWinCloseAll
    // "I know! Isn't it fabulous?![f000]븁\u0000\nI am sure you'll be\na big star, dahling![f000]븁\u0000\nSo I beg of you! Be in a movie![f000]븁\u0000\nToday I've even called on\nan amazing, astounding[f000]븀\u0000\ncostar for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_KnowIsntFabulousAm, 2, 2, 0
    ActorCmdExec 2, Movement_0E5C
    ActorCmdWait
    // "Brycen!\nWould you join us?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_BrycenWouldJoinUs, 2, 2, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorNew 14, 11, 1, 251, 91, 0
    ActorCmdExec 251, Movement_0CC0
    ActorCmdWait
    ActorCmdExec 2, Movement_0E74
    ActorCmdWait
    // "I'm Brycen...\nPleased to meet you...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_ImBrycenPleasedMeet, 251, 2, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0E64
    ActorCmdWait
    // "Brycen is Pokéstar Studios'\npride and joy--our marquee star![f000]븁\u0000\nI've prepared a positively perfect\nscript for a big, veteran star like him[f000]븀\u0000\nand a fresh, new talent like you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_BrycenPokestarStudiosPride, 2, 2, 0
    MsgWinCloseAll
    // "Those eyes...\nI look forward to acting with you...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_ThoseEyesLookForward, 251, 2, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0CD0
    ActorCmdWait
    ActorDelete 251
    SEPlay SEQ_SE_KAIDAN
    SEWait
    // "Mhm!\nAs cool as ever![f000]븁\u0000\nSo that's the situation![f000]븁\u0000\nIf you talk to that fine staff member\nover there, you can shoot the film![f000]븁\u0000\nDon't be afraid of making mistakes!\nTo start with, try going big!"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_MhmCoolEverThats, 2, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    // "Well now, [f000]Ā\u0001\u0000!\nLooking forward to working with you![f000]븁\u0000\nPlease do your best until we make\na movie to release in the theater!"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_WellNowLookingForward, 1, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0CAC:
    Move 14, 1
    Move 0, 1
    MoveEnd

Movement_0CB8:
    Move 12, 3
    MoveEnd

Movement_0CC0:
    Move 13, 1
    Move 15, 1
    Move 13, 4
    MoveEnd

Movement_0CD0:
    Move 12, 4
    Move 14, 1
    Move 12, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    ActorCmdExec 1, Movement_0E7C
    ActorCmdWait
    ActorCmdExec 1, Movement_0E74
    ActorCmdWait
    WordSetPlayerName 0
    // "Well now, [f000]Ā\u0001\u0000!\nLooking forward to working with you![f000]븁\u0000\nPlease do your best until we make\na movie to release in the theater!"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_WellNowLookingForward, 1, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0D1C
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0D1C:
    Move 12, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Movies! They're amazement itself![f000]븁\u0000\nCome now, [f000]Ā\u0001\u0000, dahling,\nbe surprised and moved![f000]븀\u0000\nTry the experience for yourself!"
    ParentActorMsg MSGFILE_SCRIPT, Global10815_Text_MoviesTheyreAmazementItself, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Well now, [f000]Ā\u0001\u0000!\nLooking forward to working with you![f000]븁\u0000\nPlease do your best until we make\na movie to release in the theater!"
    ParentActorMsg MSGFILE_SCRIPT, Global10815_Text_WellNowLookingForward, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokéstar Studios...[f000]븁\u0000\nThis is a stage of dreams that only\nchosen Trainers can stand on![f000]븁\u0000\nPlease finish the procedures for\nfilming with the gentleman by the door..."
    ParentActorMsg MSGFILE_SCRIPT, Global10815_Text_PokestarStudiosStageDreams, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    ActorCmdExec 2, Movement_0E34
    ActorCmdExec 255, Movement_0E64
    ActorCmdWait
    WordSetPlayerName 0
    // "Great! Good work!\nThat was stirring acting![f000]븀\u0000\nI can't wait to see the finished film![f000]븁\u0000\nAnd this is where\nwe're really amazing![f000]븁\u0000\nThe movie you just filmed...\nwill be finished in an instant![f000]븀\u0000\nAnd released on the silver screen![f000]븁\u0000\nCome now, [f000]Ā\u0001\u0000, dahling!\nLet's be off to the theater![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_GreatGoodWorkStirring, 2, 2, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0E3C
    ActorCmdWait
    // "Hey! Boss!\nSorry to interrupt, but the time...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_HeyBossSorryInterrupt, 1, 1, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0E64
    ActorCmdWait
    // "Oh, that's right...\nGot it...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_OhThatsRightGot, 2, 2, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0E5C
    ActorCmdWait
    // "Boo! So sorry, [f000]Ā\u0001\u0000, dahling!\nI have to hurry off![f000]븁\u0000\nBut, [f000]Ā\u0001\u0000, you should go see\nhow your debut turned out![f000]븁\u0000\nI'm sure it will be an amazing movie.\nYou are in it, after all![f000]븁\u0000\nCiao! See you again soon!\nPokéstar Studios is always[f000]븀\u0000\nwaiting for you, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10815_Text_BooSorryDahlingHave, 2, 2, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0E44
    ActorCmdExec 1, Movement_0E4C
    ActorCmdWait
    FlagSet EVENT_FLAG_0x03c3
    FlagSet EVENT_FLAG_0x03c4
    ActorDelete 2
    ActorDelete 1
    FlagReset EVENT_FLAG_0x0988
    FlagSet EVENT_FLAG_0x09f3
    MedalDiscover 216
    MedalDiscover 218
    MedalDiscover 219
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkSetConst EVENT_WORK_0x4001, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0E34:
    Move 12, 3
    MoveEnd

Movement_0E3C:
    Move 12, 4
    MoveEnd

Movement_0E44:
    Move 13, 8
    MoveEnd

Movement_0E4C:
    Move 35, 1
    Move 63, 3
    Move 13, 4
    MoveEnd

Movement_0E5C:
    Move 32, 1
    MoveEnd

Movement_0E64:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0E74:
    Move 35, 1
    MoveEnd

Movement_0E7C:
    Move 75, 1
    MoveEnd
