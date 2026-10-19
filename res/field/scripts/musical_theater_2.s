#include "asm/field_script.inc"
#include "text/script/musical_theater_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    VMReturn

L_0048:
    MusicalCmd_0165 31, 0, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 100
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0074
    // "Good job![f000]븁\u0000\nIt was a musical with some very\nexciting moments![f000]븁\u0000\nBy the way, I received comments for you\nfrom the audience.[f000]븁\u0000\nI am quite pleased.\nI will give them to you in order.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_GoodJobMusicalSome, 3, 0, 0
    VMJump L_00D1

L_0074:
    MusicalCmd_0165 32, 0, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 70
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00A0
    // "Good job!\nThe Pokémon were in total sync![f000]븁\u0000\nIt was the best musical ever! I am the\nowner, but even I was greatly moved.[f000]븁\u0000\nSince it's the best musical ever seen,\nwe have received many comments from[f000]븀\u0000\nthe audience![f000]븁\u0000\nThere are comments for everyone,\nso let me give them to you in order![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_GoodJobPokemonWere, 3, 0, 0
    VMJump L_00D1

L_00A0:
    VMStackPush 0x8022
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00C5
    // "Hmmm... Good job!\nIt was a great musical![f000]븁\u0000\nIn fact, I've received comments for you\nfrom the audience![f000]븁\u0000\nI am really pleased.\nI will give them to you in order![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_HmmmGoodJobGreat, 3, 0, 0
    VMJump L_00D1

L_00C5:
    // "Good job!\nIt was quite an interesting musical![f000]븁\u0000\nI've received comments for you\nfrom the audience![f000]븁\u0000\nWell, I am so pleased.\nI will give them to you in order![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_GoodJobQuiteInteresting, 3, 0, 0

L_00D1:
    ActorMsgClose
    VMReturn

L_00D5:
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_00E8
    VMJump L_0172

L_00E8:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_00FB
    VMJump L_0109

L_00FB:
    ActorCmdExec 3, Movement_0BBC
    VMJump L_016C

L_0109:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_011C
    VMJump L_012A

L_011C:
    ActorCmdExec 3, Movement_0BCC
    VMJump L_016C

L_012A:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_013D
    VMJump L_014B

L_013D:
    ActorCmdExec 3, Movement_0BDC
    VMJump L_016C

L_014B:
    WorkCmpConst 0x8024, 5
    VMJumpIf CMP_EQ, L_015E
    VMJump L_016C

L_015E:
    ActorCmdExec 3, Movement_0B6C
    VMJump L_016C

L_016C:
    VMJump L_03E6

L_0172:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_0185
    VMJump L_020F

L_0185:
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_0198
    VMJump L_01A6

L_0198:
    ActorCmdExec 3, Movement_0BEC
    VMJump L_0209

L_01A6:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_01B9
    VMJump L_01C7

L_01B9:
    ActorCmdExec 3, Movement_0BBC
    VMJump L_0209

L_01C7:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_01DA
    VMJump L_01E8

L_01DA:
    ActorCmdExec 3, Movement_0BCC
    VMJump L_0209

L_01E8:
    WorkCmpConst 0x8024, 5
    VMJumpIf CMP_EQ, L_01FB
    VMJump L_0209

L_01FB:
    ActorCmdExec 3, Movement_0B80
    VMJump L_0209

L_0209:
    VMJump L_03E6

L_020F:
    WorkCmpConst 0x8025, 2
    VMJumpIf CMP_EQ, L_0222
    VMJump L_02AC

L_0222:
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_0235
    VMJump L_0243

L_0235:
    ActorCmdExec 3, Movement_0BFC
    VMJump L_02A6

L_0243:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_0256
    VMJump L_0264

L_0256:
    ActorCmdExec 3, Movement_0BEC
    VMJump L_02A6

L_0264:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_0277
    VMJump L_0285

L_0277:
    ActorCmdExec 3, Movement_0BBC
    VMJump L_02A6

L_0285:
    WorkCmpConst 0x8024, 5
    VMJumpIf CMP_EQ, L_0298
    VMJump L_02A6

L_0298:
    ActorCmdExec 3, Movement_0B94
    VMJump L_02A6

L_02A6:
    VMJump L_03E6

L_02AC:
    WorkCmpConst 0x8025, 3
    VMJumpIf CMP_EQ, L_02BF
    VMJump L_0349

L_02BF:
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_02D2
    VMJump L_02E0

L_02D2:
    ActorCmdExec 3, Movement_0C0C
    VMJump L_0343

L_02E0:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_02F3
    VMJump L_0301

L_02F3:
    ActorCmdExec 3, Movement_0BFC
    VMJump L_0343

L_0301:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_0314
    VMJump L_0322

L_0314:
    ActorCmdExec 3, Movement_0BEC
    VMJump L_0343

L_0322:
    WorkCmpConst 0x8024, 5
    VMJumpIf CMP_EQ, L_0335
    VMJump L_0343

L_0335:
    ActorCmdExec 3, Movement_0BA8
    VMJump L_0343

L_0343:
    VMJump L_03E6

L_0349:
    WorkCmpConst 0x8025, 5
    VMJumpIf CMP_EQ, L_035C
    VMJump L_03E6

L_035C:
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_036F
    VMJump L_037D

L_036F:
    ActorCmdExec 3, Movement_0B1C
    VMJump L_03E0

L_037D:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_0390
    VMJump L_039E

L_0390:
    ActorCmdExec 3, Movement_0B30
    VMJump L_03E0

L_039E:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_03B1
    VMJump L_03BF

L_03B1:
    ActorCmdExec 3, Movement_0B44
    VMJump L_03E0

L_03BF:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_03D2
    VMJump L_03E0

L_03D2:
    ActorCmdExec 3, Movement_0B58
    VMJump L_03E0

L_03E0:
    VMJump L_03E6

L_03E6:
    ActorCmdWait
    VMReturn

L_03EA:
    WorkAdd 0x8027, 1
    Cmd_0167 14, 0x8027, 0, 0
    VMCall L_00D5
    WordSetMusicalInfo 5, 0, 0x8024
    MusicalCmd_0165 38, 0x8024, 0x8023
    WordSetMusicalInfo 4, 1, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0438
    // "[f000]Ā\u0001\u0000's Dress Up performance\nwas very [f000]ģ\u0001\u0001![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_SDressUpPerformance, 3, 0, 0
    VMJump L_0444

L_0438:
    // "[f000]Ā\u0001\u0000's Dress Up performance\nwas very distinctive![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_SDressUpPerformance_2, 3, 0, 0

L_0444:
    MusicalCmd_0165 33, 0x8024, 0x8022
    WorkAdd 0x8027, 1
    Cmd_0167 14, 0x8027, 0, 0
    VMStackPush 0x8022
    VMStackPushConst 100
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0480
    // "In this musical, it's not an\noverstatement to say that [f000]Ā\u0001\u0000's[f000]븀\u0000\nPokémon was the lead role.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_MusicalItsNotOverstatement, 3, 0, 0
    VMJump L_0520

L_0480:
    VMStackPush 0x8022
    VMStackPushConst 70
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04A5
    // "The Pokémon used Props to convey such\nabundant expression, just as if they[f000]븀\u0000\nwere actors![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_PokemonUsedPropsConvey, 3, 0, 0
    VMJump L_0520

L_04A5:
    VMStackPush 0x8022
    VMStackPushConst 40
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04CA
    // "Watching your Pokémon's sharp moves,\nI also felt [f000]ģ\u0001\u0001![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_WatchingPokemonsSharpMoves, 3, 0, 0
    VMJump L_0520

L_04CA:
    VMStackPush 0x8022
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04EF
    // "Your Pokémon played its role very well.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_PokemonPlayedItsRole, 3, 0, 0
    VMJump L_0520

L_04EF:
    VMStackPush 0x8022
    VMStackPushConst 20
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0514
    // "It looked like your Pokémon was making an\neffort to live up to your expectations.[f000]븁\u0000\nI will support your Pokémon on the side![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_LookedLikePokemonMaking, 3, 0, 0
    VMJump L_0520

L_0514:
    // "It was not highly noticeable...\nBut I liked it! Keep it up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_NotHighlyNoticeableBut, 3, 0, 0

L_0520:
    ActorMsgClose
    VMReturn

Script_1:
    MusicalCmd_0165 39, 0, 0x8021
    WorkGet EVENT_WORK_0x4021, 0x8021
    MusicalCmd_0165 39, 1, 0x8021
    WorkGet EVENT_WORK_0x4022, 0x8021
    MusicalCmd_0165 39, 2, 0x8021
    WorkGet EVENT_WORK_0x4023, 0x8021
    MusicalCmd_0165 39, 3, 0x8021
    WorkGet EVENT_WORK_0x4024, 0x8021
    VMHalt

Script_2:
    VMHalt

Script_3:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9694, 0, 0xed02b, 0xb8000, 0, 0x98000, 1
    EvCameraWait
    FadeInBlackQ
    FadeWait
    Cmd_0167 15, 0, 0, 0
    Cmd_0167 19, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06B2
    Cmd_0167 20, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05C8
    MsgSetAutoscrolls 1

L_05C8:
    MusicalCmd_0165 36, 0, 0x8023
    WordSetMusicalInfo 2, 0, 0
    WordSetMusicalInfo 3, 1, 0x8023
    // "Welcome to the musical!\nI'm very pleased with your participation![f000]븁\u0000\nParticipants for this musical are:[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_WelcomeMusicalImVery, 3, 0, 0
    Cmd_0167 14, 111, 0, 0
    WordSetMusicalInfo 6, 2, 0
    WordSetMusicalInfo 5, 3, 0
    // "[f000]Ā\u0001\u0003 will participate\nwith [f000]ā\u0001\u0002![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_WillParticipate, 3, 0, 0
    Cmd_0167 14, 112, 0, 0
    WordSetMusicalInfo 6, 2, 1
    WordSetMusicalInfo 5, 3, 1
    // "[f000]Ā\u0001\u0003 will participate\nwith [f000]ā\u0001\u0002![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_WillParticipate, 3, 0, 0
    Cmd_0167 14, 113, 0, 0
    WordSetMusicalInfo 6, 2, 2
    WordSetMusicalInfo 5, 3, 2
    // "[f000]Ā\u0001\u0003 will participate\nwith [f000]ā\u0001\u0002![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_WillParticipate, 3, 0, 0
    Cmd_0167 14, 114, 0, 0
    WordSetMusicalInfo 6, 2, 3
    WordSetMusicalInfo 5, 3, 3
    // "[f000]Ā\u0001\u0003 will participate\nwith [f000]ā\u0001\u0002![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_WillParticipate, 3, 0, 0
    Cmd_0167 14, 115, 0, 0
    // "And the show you'll be performing\nis [f000]Ģ\u0001\u0000.[f000]븁\u0000\nThat is a popular show\namong [f000]ĳ\u0001\u0001.[f000]븁\u0000\nWell, without further ado...\nLet's play Dress Up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_ShowYoullPerformingPopular, 2, 0, 0
    ActorMsgClose
    Cmd_0167 14, 11, 0, 0
    Cmd_0167 20, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06B2
    MsgSetAutoscrolls 0

L_06B2:
    FadeOutBlackQ
    FadeWait
    EvCameraEnd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9694, 0, 0xed02b, 0xb8000, 0, 0x98000, 1
    EvCameraWait
    FadeInWhiteQ
    FadeWait
    Cmd_0167 20, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0703
    MsgSetAutoscrolls 1

L_0703:
    // "I guess everyone is ready.\nLet's go up on stage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_GuessEveryoneReadyLets, 3, 0, 0
    ActorMsgClose
    Cmd_0167 17, 0, 0, 0
    FadeOutBlackQ
    FadeWait
    EvCameraEnd
    Cmd_0167 20, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0742
    MsgSetAutoscrolls 0

L_0742:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8028, 0
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9694, 0, 0xed02b, 0xb8000, 0, 0x98000, 1
    EvCameraWait
    FadeInBlackQ
    FadeWait
    Cmd_0167 20, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0793
    MsgSetAutoscrolls 1

L_0793:
    Cmd_0167 14, 12, 0, 0
    MusicalCmd_0165 30, 0, 0x8020
    WorkSetConst 0x8027, 120
    VMCall L_0048
    WorkSetConst 0x8028, 1
    WorkSetConst 0x8025, 5
    WorkSetConst 0x8026, 3

L_07C2:
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0813
    MusicalCmd_0165 37, 0x8026, 0x8024
    VMCall L_03EA
    WorkGet 0x8025, 0x8024
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0807
    WorkSetConst 0x8028, 0
    VMJump L_080D

L_0807:
    WorkSub 0x8026, 1

L_080D:
    VMJump L_07C2

L_0813:
    Cmd_0167 14, 141, 0, 0
    WorkSetConst 0x8024, 5
    VMCall L_00D5
    // "Thank you very much for participating in\nthe musical today![f000]븁\u0000\nPlease join another musical again!\nHave a great day![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MusicalTheater2_Text_ThankVeryMuchParticipating, 3, 0, 0
    Cmd_0167 20, 0, 0, 0x8010
    ActorMsgClose
    Cmd_0167 14, 142, 0, 0
    WorkSetConst 0x8029, 0
    MusicalCmd_0166 0, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0875
    FlagSet EVENT_FLAG_0x026e
    VMJump L_0879

L_0875:
    FlagReset EVENT_FLAG_0x026e

L_0879:
    MusicalCmd_0166 1, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_089D
    FlagSet EVENT_FLAG_0x026f
    VMJump L_08A1

L_089D:
    FlagReset EVENT_FLAG_0x026f

L_08A1:
    MusicalCmd_0166 2, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08C5
    FlagSet EVENT_FLAG_0x0270
    VMJump L_08C9

L_08C5:
    FlagReset EVENT_FLAG_0x0270

L_08C9:
    MusicalCmd_0166 3, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08ED
    FlagSet EVENT_FLAG_0x0271
    VMJump L_08F1

L_08ED:
    FlagReset EVENT_FLAG_0x0271

L_08F1:
    MusicalCmd_0166 4, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0915
    FlagSet EVENT_FLAG_0x0272
    VMJump L_0919

L_0915:
    FlagReset EVENT_FLAG_0x0272

L_0919:
    MusicalCmd_0166 5, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_093D
    FlagSet EVENT_FLAG_0x0295
    VMJump L_0941

L_093D:
    FlagReset EVENT_FLAG_0x0295

L_0941:
    MusicalCmd_0166 6, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0965
    FlagSet EVENT_FLAG_0x0296
    VMJump L_0969

L_0965:
    FlagReset EVENT_FLAG_0x0296

L_0969:
    MusicalCmd_0166 7, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_098D
    FlagSet EVENT_FLAG_0x0297
    VMJump L_0991

L_098D:
    FlagReset EVENT_FLAG_0x0297

L_0991:
    MusicalCmd_0166 8, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09B5
    FlagSet EVENT_FLAG_0x0298
    VMJump L_09B9

L_09B5:
    FlagReset EVENT_FLAG_0x0298

L_09B9:
    MusicalCmd_0166 9, 0, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09DD
    FlagSet EVENT_FLAG_0x0299
    VMJump L_09E1

L_09DD:
    FlagReset EVENT_FLAG_0x0299

L_09E1:
    WorkSetConst 0x8029, 0
    FlagReset EVENT_FLAG_0x00f2
    Cmd_0167 20, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A0C
    FlagSet EVENT_FLAG_0x00f3

L_0A0C:
    FadeOutBlackQ
    FadeWait
    EvCameraEnd
    Cmd_0167 20, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A33
    MsgSetAutoscrolls 0

L_0A33:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .byte 0x00
    VMNop
    VMNop
    VMNop2
    VMStackAdd
    VMStackPush 3
    VMNop2
    VMStackDiv
    DebugPrint 0
    VMNop2
    VMStackAdd
    VMNop2
    PokePartyGetSpecies 0, 0
    VMNop2
    VMStackAdd
    VMStackPush 3
    VMNop2
    VMStackDiv
    VMStackPushConst 0
    VMNop2
    VMStackAdd
    VMNop2
    PokePartyGetSpecies 0, 0
    VMNop2
    VMStackAdd
    VMStackPush 3
    VMNop2
    VMStackDiv
    VMStackPop 0
    VMNop2
    VMStackAdd
    VMNop2
    PokePartyGetSpecies 0, 0
    VMNop2
    VMStackAdd
    VMStackPush 3
    VMNop2
    VMStackDiv
    VMStackAdd
    VMNop
    VMNop2
    VMStackAdd
    VMNop2
    PokePartyGetSpecies 0, 1
    VMNop2
    VMStackSub
    VMNop2
    VMHalt
    VMNop2
    VMStackMul
    DebugPrint 1
    VMNop2
    VMStackSub
    VMStackPush 254
    VMNop
    VMNop2
    VMNop2
    VMStackSub
    VMNop2
    VMHalt
    VMNop2
    VMStackMul
    VMStackPushConst 1
    VMNop2
    VMStackSub
    VMStackPush 254
    VMNop
    VMNop2
    VMNop2
    VMStackSub
    VMNop2
    VMHalt
    VMNop2
    VMStackMul
    VMStackPop 1
    VMNop2
    VMStackSub
    VMStackPush 254
    VMNop
    VMNop2
    VMNop2
    VMStackSub
    VMNop2
    VMHalt
    .byte 0x01
    .balign 4, 0
    Move 14, 12
    Move 1, 1
    Move 13, 9
    MoveEnd

Movement_0B1C:
    Move 1, 1
    Move 13, 2
    Move 14, 3
    Move 33, 1
    MoveEnd

Movement_0B30:
    Move 1, 1
    Move 13, 2
    Move 14, 1
    Move 33, 1
    MoveEnd

Movement_0B44:
    Move 1, 1
    Move 13, 2
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_0B58:
    Move 1, 1
    Move 13, 2
    Move 15, 3
    Move 33, 1
    MoveEnd

Movement_0B6C:
    Move 3, 1
    Move 15, 3
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_0B80:
    Move 3, 1
    Move 15, 1
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_0B94:
    Move 2, 1
    Move 14, 1
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_0BA8:
    Move 2, 1
    Move 14, 3
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_0BBC:
    Move 3, 1
    Move 15, 2
    Move 33, 1
    MoveEnd

Movement_0BCC:
    Move 3, 1
    Move 15, 4
    Move 33, 1
    MoveEnd

Movement_0BDC:
    Move 3, 1
    Move 15, 6
    Move 33, 1
    MoveEnd

Movement_0BEC:
    Move 2, 1
    Move 14, 2
    Move 33, 1
    MoveEnd

Movement_0BFC:
    Move 2, 1
    Move 14, 4
    Move 33, 1
    MoveEnd

Movement_0C0C:
    Move 2, 1
    Move 14, 6
    Move 33, 1
    MoveEnd
