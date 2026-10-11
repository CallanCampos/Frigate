#include "asm/field_script.inc"
#include "text/script/battle_subway_2.h"

// Script plugin 1, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    BSubwayCmd_Tool 21, 0, 0, 32803
    ActorSetGPos 0, 17, 0, 14, 3
    ActorSetGPos 1, 76, 0, 14, 2
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008B
    BSubwayCmd_Tool 18, 0x8023, 1, 0
    ActorSetGPos 255, 75, 0, 12, 1
    BSubwayCmd_Tool 329, 0, 0, 0

L_008B:
    VMHalt

Script_2:
    BSubwayCmd_Tool 21, 0, 0, 32803
    FieldGetContinueFlag 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F0
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00F0
    Plugin1_Cmd1001 1, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00F0
    WorkSetConst EVENT_WORK_0x4179, 3

L_00F0:
    BSubwayCmd_Tool 18, 0x8023, 1, 0
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0117
    BSubwayCmd_Tool 20, 1, 0, 0

L_0117:
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AB
    // "What would you like to do?"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_WhatWouldLike, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 16, 65535, 0
    ListMenuAdd 17, 65535, 1
    ListMenuAdd 18, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0176
    VMJump L_0184

L_0176:
    MsgWinCloseAll
    VMCall L_01B7
    VMJump L_01A5

L_0184:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0197
    VMJump L_01A3

L_0197:
    VMCall L_0A9E
    VMJump L_01A5

L_01A3:
    MsgWinCloseAll

L_01A5:
    VMJump L_01B1

L_01AB:
    VMCall L_0A9E

L_01B1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01B7:
    BSubwayCmd_Tool 21, 0, 0, 32803
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_01D4
    VMJump L_01E0

L_01D4:
    VMCall L_02BB
    VMJump L_02B9

L_01E0:
    WorkCmpConst 0x8023, 5
    VMJumpIf CMP_EQ, L_01F3
    VMJump L_01FF

L_01F3:
    VMCall L_02BB
    VMJump L_02B9

L_01FF:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_0212
    VMJump L_021E

L_0212:
    VMCall L_02BB
    VMJump L_02B9

L_021E:
    WorkCmpConst 0x8023, 6
    VMJumpIf CMP_EQ, L_0231
    VMJump L_023D

L_0231:
    VMCall L_02BB
    VMJump L_02B9

L_023D:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_0250
    VMJump L_025C

L_0250:
    VMCall L_02BB
    VMJump L_02B9

L_025C:
    WorkCmpConst 0x8023, 7
    VMJumpIf CMP_EQ, L_026F
    VMJump L_027B

L_026F:
    VMCall L_02BB
    VMJump L_02B9

L_027B:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_028E
    VMJump L_029A

L_028E:
    VMCall L_0438
    VMJump L_02B9

L_029A:
    WorkCmpConst 0x8023, 8
    VMJumpIf CMP_EQ, L_02AD
    VMJump L_02B9

L_02AD:
    VMCall L_0438
    VMJump L_02B9

L_02B9:
    VMReturn

L_02BB:
    WorkSetConst 0x8024, 0
    WorkGet 0x8024, 0x8011
    RTCallGlobal 10345
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F0
    // "The selection was canceled."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_SelectionCanceled, 0x8024, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_02F0:
    WorkGet 0x8008, 0x8024
    RTCallGlobal 10346
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_031F
    // "The selection was canceled."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_SelectionCanceled, 0x8008, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_031F:
    BSubwayCmd_Tool 310, 0, 0, 32803
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0391
    VMCall L_03F4
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8008, 2
    RTCallGlobal 10347
    ActorCmdExec 255, Movement_03EC
    ActorCmdWait
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0391
    // "The selection was canceled."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_SelectionCanceled, 0x8024, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_0391:
    WorkSetConst EVENT_WORK_0x4176, 4
    WorkSetConst EVENT_WORK_0x4178, 1
    BSubwayCmd_Tool 322, 0, 0, 0
    BSubwayCmd_Tool 316, 0, 0, 0
    // "Saving...\nDon't turn off the power."
    SystemMsg BattleSubway2_Text_SavingDontTurnOff, 2
    SaveDataWrite 0x8010
    MsgWinCloseAll
    RecordAdd 48, 1
    VMCall L_157C
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    WorkSetConst EVENT_WORK_0x4179, 0
    MapChangeCore ZONE_BATTLE_SUBWAY, 7, 0, 4, 0
    WorkSetConst 0x8024, 0
    VMReturn
    .balign 4, 0

Movement_03EC:
    Move 34, 1
    MoveEnd

L_03F4:
    FadeEx 3, 0, 16, 2
    FadeExWait
    BSubwayCmd_Tool 35, 0, 3, 0
    ActorSetGPos 255, 18, 0, 14, 3
    ActorSetGPos 2, 19, 0, 14, 2
    BSubwayCmd_Tool 36, 2, 0, 0
    FadeEx 3, 16, 0, 2
    VMReturn

L_0438:
    // "Communicating. Please stand by..."
    SystemMsg BattleSubway2_Text_CommunicatingPleaseStandBy, 2
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 100, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0475
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_0475:
    WorkSetConst EVENT_WORK_0x4000, 0
    BSubwayCmd_Tool 405, 2, EVENT_WORK_0x4000, 0
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04B2
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_04B2:
    BSubwayCmd_Tool 406, 2, 0, 16384
    MsgWinCloseAll
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E9
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_04E9:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0508
    VMCall L_0510
    VMJump L_050E

L_0508:
    VMCall L_052E

L_050E:
    VMReturn

L_0510:
    // "“Return to Nimbasa City\" was chosen, so\nyour challenge[f000]븀\u0000\nwill end for now."
    ParentActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_ReturnNimbasaCityChosen, 0, 0
    VMSleep 30
    MsgWinCloseAll
    VMCall L_1718
    VMCall L_0A7C
    VMReturn

L_052E:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 105, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_056F
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_056F:
    RTCallGlobal 10345
    WorkGet 0x8025, 0x8010
    VMSleep 30
    // "Awaiting your friend's response..."
    SystemMsg BattleSubway2_Text_AwaitingFriendsResponse, 2
    BSubwayCmd_Tool 402, 106, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05B0
    InfoMsgClose
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_05B0:
    BSubwayCmd_Tool 405, 5, 0x8025, 0
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05E7
    InfoMsgClose
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_05E7:
    BSubwayCmd_Tool 406, 5, 0, 32806
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_061E
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_061E:
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_06A5
    InfoMsgClose
    BSubwayCmd_Tool 402, 107, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_066E
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_066E:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0693
    // "The challenge was interrupted."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_ChallengeInterrupted, 0x8011, 2, 0
    VMJump L_069F

L_0693:
    // "The selection was canceled."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_SelectionCanceled, 0x8011, 2, 0

L_069F:
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_06A5:
    BSubwayCmd_Tool 402, 108, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06D2
    InfoMsgClose
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_06D2:
    InfoMsgClose
    WorkGet 0x8008, 0x8011
    RTCallGlobal 10346
    WorkGet 0x8025, 0x8010
    // "Awaiting your friend's response..."
    SystemMsg BattleSubway2_Text_AwaitingFriendsResponse, 2
    BSubwayCmd_Tool 402, 109, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0717
    InfoMsgClose
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_0717:
    BSubwayCmd_Tool 405, 6, 0x8025, 0
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_074E
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_074E:
    BSubwayCmd_Tool 406, 6, 0, 32806
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0785
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_0785:
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_080C
    InfoMsgClose
    BSubwayCmd_Tool 402, 110, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D5
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_07D5:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_07FA
    // "The challenge was interrupted."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_ChallengeInterrupted, 0x8011, 2, 0
    VMJump L_0806

L_07FA:
    // "The selection was canceled."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_SelectionCanceled, 0x8011, 2, 0

L_0806:
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_080C:
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 111, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0843
    InfoMsgClose
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_0843:
    BSubwayCmd_Tool 405, 0, 0, 0
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_087A
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_087A:
    BSubwayCmd_Tool 406, 0, 0, 32806
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08B1
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_08B1:
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_091F
    InfoMsgClose
    WorkGet 0x8008, 0x8011
    WorkGet 0x8009, 0x8026
    RTCallGlobal 10348
    // "Awaiting your friend's response..."
    SystemMsgAsync BattleSubway2_Text_AwaitingFriendsResponse, 2
    BSubwayCmd_Tool 402, 112, 0, 32802
    VMSleep 15
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_090D
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_090D:
    // "The challenge was interrupted."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_ChallengeInterrupted, 0x8011, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_091F:
    BSubwayCmd_Tool 402, 113, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_094C
    InfoMsgClose
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_094C:
    // "Saving...\nDon't turn off the power."
    SystemMsg BattleSubway2_Text_SavingDontTurnOff, 2
    WorkSetConst EVENT_WORK_0x4176, 4
    WorkSetConst EVENT_WORK_0x4178, 1
    BSubwayCmd_Tool 21, 0, 0, 32803
    BSubwayCmd_Tool 322, 0, 0, 0
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 102, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09A9
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_09A9:
    BSubwayCmd_Tool 407, 0, 0, 32801
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09E0
    BSubwayCmd_Tool 316, 0, 0, 0
    BSubwayCmd_Tool 405, 1, 0, 32801
    VMJump L_09EA

L_09E0:
    BSubwayCmd_Tool 406, 1, 0, 16384

L_09EA:
    BSubwayCmd_Tool 415, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A17
    MsgWinCloseAll
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_0A17:
    SaveDataWrite 0x8010
    MsgWinCloseAll
    BSubwayCmd_Tool 402, 103, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A48
    VMCall L_16F4
    VMCall L_0A7C
    VMReturn

L_0A48:
    RecordAdd 48, 1
    VMCall L_157C
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    WorkSetConst EVENT_WORK_0x4179, 0
    MapChangeCore ZONE_BATTLE_SUBWAY, 7, 0, 4, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    VMReturn

L_0A7C:
    WorkSetConst EVENT_WORK_0x4176, 2
    VMCall L_157C
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    WorkSetConst EVENT_WORK_0x4179, 0
    VMCall L_1776
    VMReturn

L_0A9E:
    WorkSetConst 0x8027, 0
    // "Do you want to return to Nimbasa City?"
    ParentActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_WantReturnNimbasaCity, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0AC9
    MsgWinCloseAll
    VMReturn

L_0AC9:
    // "Well then, we'll return to Nimbasa City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_WellThenWellReturn, 0, 0
    MsgWinCloseAll
    BSubwayCmd_Tool 331, 0, 0, 32807
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AF8
    VMCall L_0B20

L_0AF8:
    WorkSetConst EVENT_WORK_0x4176, 2
    VMCall L_157C
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    WorkSetConst EVENT_WORK_0x4179, 0
    VMCall L_1776
    WorkSetConst 0x8027, 0
    VMReturn

L_0B20:
    // "Awaiting your friend's response..."
    SystemMsg BattleSubway2_Text_AwaitingFriendsResponse, 2
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 100, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B5B
    MsgWinCloseAll
    VMCall L_16F4
    VMJump L_0B7D

L_0B5B:
    WorkSetConst EVENT_WORK_0x4000, 1
    BSubwayCmd_Tool 405, 2, EVENT_WORK_0x4000, 0
    BSubwayCmd_Tool 406, 2, 0, 16384
    VMCall L_1718
    MsgWinCloseAll

L_0B7D:
    VMReturn

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    BSubwayCmd_Tool 21, 0, 0, 32803
    BSubwayCmd_Tool 6, 0x8023, 0, 32809
    TrainerCardGetSex 0x8028
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BFC
    WorkSetConst 0x8020, 23
    VMStackPush 0x8029
    VMStackPushConst 49
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0BDD
    WorkSetConst 0x8020, 27
    VMJump L_0BF6

L_0BDD:
    VMStackPush 0x8029
    VMStackPushConst 21
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0BF6
    WorkSetConst 0x8020, 25

L_0BF6:
    VMJump L_0C3A

L_0BFC:
    WorkSetConst 0x8020, 24
    VMStackPush 0x8029
    VMStackPushConst 49
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0C21
    WorkSetConst 0x8020, 28
    VMJump L_0C3A

L_0C21:
    VMStackPush 0x8029
    VMStackPushConst 21
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0C3A
    WorkSetConst 0x8020, 26

L_0C3A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    BSubwayCmd_Tool 21, 0, 0, 32803
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C95
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    BSubwayCmd_Tool 37, 0x8011, 0, 0
    VMJump L_0CDC

L_0C95:
    BSubwayCmd_Tool 39, 0x8011, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CBE
    VMCall L_0CE2
    VMJump L_0CDC

L_0CBE:
    BSubwayCmd_Tool 26, 0x8011, 0, 32800
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose

L_0CDC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0CE2:
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkGet 0x802d, 0x8011
    BSubwayCmd_Tool 40, 0x802d, 0, 32810
    BSubwayCmd_Tool 41, 0x802d, 0, 32811
    BSubwayCmd_Tool 42, 0x802d, 0, 32812
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 0x802c
    WorkSet 0x8001, 1
    WorkSet 0x8002, 1
    WorkSet 0x8003, 0x802a
    WorkSet 0x8004, 0x802b
    WorkSet 0x8005, 0x802b
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    VMReturn

Script_6:
    ActorsPauseAll
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x802e, 1
    BSubwayCmd_Tool 310, 0, 0, 32803
    BSubwayCmd_Tool 312, 0, 0, 32815
    WorkSetConst EVENT_WORK_0x4179, 2
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_0DEF
    VMJump L_0E0E

L_0DEF:
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E08
    WorkSetConst EVENT_WORK_0x4179, 3

L_0E08:
    VMJump L_0ECD

L_0E0E:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_0E21
    VMJump L_0E40

L_0E21:
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E3A
    WorkSetConst EVENT_WORK_0x4179, 3

L_0E3A:
    VMJump L_0ECD

L_0E40:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_0E53
    VMJump L_0E72

L_0E53:
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E6C
    WorkSetConst EVENT_WORK_0x4179, 3

L_0E6C:
    VMJump L_0ECD

L_0E72:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_0E85
    VMJump L_0EAE

L_0E85:
    BSubwayCmd_Tool 358, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 21
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0EA8
    WorkSetConst EVENT_WORK_0x4179, 3

L_0EA8:
    VMJump L_0ECD

L_0EAE:
    WorkCmpConst 0x8023, 4
    VMJumpIf CMP_EQ, L_0EC1
    VMJump L_0ECD

L_0EC1:
    WorkSetConst EVENT_WORK_0x4179, 3
    VMJump L_0ECD

L_0ECD:
    VMCall L_13CB
    BSubwayCmd_Tool 5, 0, 0, 0
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1113
    BSubwayCmd_Tool 326, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FFE
    // "Congratulations![f000]븁\u0000\nYou had a seven-win streak and\nbrilliantly beat the Subway Boss![f000]븁\u0000\nTo commemorate this,\nI present you with these Battle Points.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_CongratulationsHadSevenWin, 0x802e, 2, 0
    MsgWinCloseAll
    BSubwayCmd_Tool 313, 0, 0, 32816
    WordSetPlayerName 0
    WordSetNumber 1, 0x8030, 2
    // "[f000]Ā\u0001\u0000 received [f000]ȁ\u0001\u0001 BP!"
    SystemMsg BattleSubway2_Text_ReceivedBp, 2
    MEPlay SEQ_ME_BPGET
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    BSubwayCmd_Tool 14, 0x8023, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FF8
    // "Also, to commemorate this, I give you\nthis trophy.[f000]븁\u0000\nPlease display it in your home!"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_AlsoCommemorateGiveTrophy, 0x802e, 2, 0
    MEPlay SEQ_ME_HYOUKA6
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll
    BSubwayCmd_Tool 15, 0x8023, 0, 0
    Cmd_01DD 8, 0, 0
    WorkCmpConst 0x8023, 5
    VMJumpIf CMP_EQ, L_0F97
    VMJump L_0FA1

L_0F97:
    FlagReset EVENT_FLAG_0x02dd
    VMJump L_0FF8

L_0FA1:
    WorkCmpConst 0x8023, 6
    VMJumpIf CMP_EQ, L_0FB4
    VMJump L_0FBE

L_0FB4:
    FlagReset EVENT_FLAG_0x02de
    VMJump L_0FF8

L_0FBE:
    WorkCmpConst 0x8023, 7
    VMJumpIf CMP_EQ, L_0FD1
    VMJump L_0FDB

L_0FD1:
    FlagReset EVENT_FLAG_0x02df
    VMJump L_0FF8

L_0FDB:
    WorkCmpConst 0x8023, 8
    VMJumpIf CMP_EQ, L_0FEE
    VMJump L_0FF8

L_0FEE:
    FlagReset EVENT_FLAG_0x02df
    VMJump L_0FF8

L_0FF8:
    VMJump L_110D

L_0FFE:
    // "Congratulations![f000]븁\u0000\nYou had a seven-win streak and\nbrilliantly beat the Subway Boss![f000]븁\u0000\nTo commemorate this,\nI present you with these Battle Points.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_CongratulationsHadSevenWin, 0x802e, 2, 0
    MsgWinCloseAll
    BSubwayCmd_Tool 313, 0, 0, 32816
    WordSetPlayerName 0
    WordSetNumber 1, 0x8030, 2
    // "[f000]Ā\u0001\u0000 received [f000]ȁ\u0001\u0001 BP!"
    SystemMsg BattleSubway2_Text_ReceivedBp, 2
    MEPlay SEQ_ME_BPGET
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    BSubwayCmd_Tool 358, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 21
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_110D
    BSubwayCmd_Tool 14, 0x8023, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_110D
    WordSetPlayerName 0
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_1080
    VMJump L_1092

L_1080:
    // "And, [f000]Ā\u0001\u0000, now you have earned\nthe right to challenge[f000]븀\u0000\nthe Super Single Train![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_NowHaveEarnedRight, 0x802e, 2, 0
    VMJump L_1101

L_1092:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_10A5
    VMJump L_10B7

L_10A5:
    // "And, [f000]Ā\u0001\u0000, now you have earned\nthe right to challenge[f000]븀\u0000\nthe Super Double Train![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_NowHaveEarnedRight_2, 0x802e, 2, 0
    VMJump L_1101

L_10B7:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_10CA
    VMJump L_10DC

L_10CA:
    // "And, [f000]Ā\u0001\u0000, now you have earned\nthe right to challenge[f000]븀\u0000\nthe Super Multi Train![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_NowHaveEarnedRight_3, 0x802e, 2, 0
    VMJump L_1101

L_10DC:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_10EF
    VMJump L_1101

L_10EF:
    // "And, [f000]Ā\u0001\u0000, now you have earned\nthe right to challenge[f000]븀\u0000\nthe Super Multi Train![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_NowHaveEarnedRight_3, 0x802e, 2, 0
    VMJump L_1101

L_1101:
    MsgWinCloseAll
    BSubwayCmd_Tool 15, 0x8023, 0, 0

L_110D:
    VMJump L_11AA

L_1113:
    // "Congratulations! You've successfully\nreached a seven-win streak![f000]븁\u0000\nSince you've won seven in a row,\nI present you with these Battle Points![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_CongratulationsYouveSuccessfullyReached, 0x802e, 2, 0
    MsgWinCloseAll
    BSubwayCmd_Tool 313, 0, 0, 32816
    WordSetPlayerName 0
    WordSetNumber 1, 0x8030, 2
    // "[f000]Ā\u0001\u0000 received [f000]ȁ\u0001\u0001 BP!"
    SystemMsg BattleSubway2_Text_ReceivedBp, 2
    MEPlay SEQ_ME_BPGET
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11AA
    BSubwayCmd_Tool 358, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 21
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_11AA
    BSubwayCmd_Tool 14, 0x8023, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11AA
    // "And, [f000]Ā\u0001\u0000, now you have earned\nthe right to challenge[f000]븀\u0000\nthe Super Multi Train![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_NowHaveEarnedRight_3, 0x802e, 2, 0
    MsgWinCloseAll
    BSubwayCmd_Tool 15, 0x8023, 0, 0

L_11AA:
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1252
    BSubwayCmd_Tool 109, 0, 0, 32801
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1252
    BSubwayCmd_Tool 28, 0, 0, 32801
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1217
    BSubwayCmd_Tool 27, 0, 0, 32801
    WordSetPlayerName 0
    WordSetNumber 1, 0x8021, 2
    // "[f000]Ā\u0001\u0000, you have been promoted\nto Rank [f000]ȁ\u0001\u0001![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_HaveBeenPromotedRank, 0x802e, 2, 0

L_1217:
    // "Would you like to send these results\nusing Nintendo WFC?"
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_WouldLikeSendThese, 0x802e, 2, 0
    YesNoWin 0x8010
    MsgWinCloseAll
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1248
    WorkSetConst 0x8031, 1
    VMJump L_1252

L_1248:
    BSubwayCmd_Tool 100, 1, 0, 0

L_1252:
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_126F
    BSubwayCmd_Tool 349, 0, 0, 0

L_126F:
    VMStackPush EVENT_WORK_0x4165
    VMStackPushConst 10
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_1288
    WorkAdd EVENT_WORK_0x4165, 1

L_1288:
    BSubwayCmd_Tool 321, 0, 0, 0
    VMStackPush 0x8031
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12B5
    // "Saving...\nDon't turn off the power."
    SystemMsg BattleSubway2_Text_SavingDontTurnOff, 2
    SaveDataWrite 0x8010
    MsgWinCloseAll
    RTCallGlobal 10344

L_12B5:
    // "Saving your record data...\nDon't turn off the power."
    SystemMsg BattleSubway2_Text_SavingRecordDataDont, 2
    SaveDataWrite 0x8010
    InfoMsgClose
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12E6
    // "Please enjoy your time here.[f000]븁\u0000\nIf you would like to continue your\nchallenge or go back to Nimbasa City,[f000]븀\u0000\nplease talk to me."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_PleaseEnjoyTimeHere, 0x802e, 2, 0
    VMJump L_139F

L_12E6:
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_12F9
    VMJump L_130B

L_12F9:
    // "This is the end of the Single Train line.[f000]븁\u0000\nWhen you want to return to Nimbasa City,\nplease talk to me again.[f000]븁\u0000\nPlease enjoy your time here."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_EndSingleTrainLine, 0x802e, 2, 0
    VMJump L_139F

L_130B:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_131E
    VMJump L_1330

L_131E:
    // "This is the end of the Double Train line.[f000]븁\u0000\nWhen you want to return to Nimbasa City,\nplease talk to me again.[f000]븁\u0000\nPlease enjoy your time here."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_EndDoubleTrainLine, 0x802e, 2, 0
    VMJump L_139F

L_1330:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_1343
    VMJump L_1355

L_1343:
    // "This is the end of the Multi Train line.[f000]븁\u0000\nWhen you want to return to Nimbasa City,\nplease talk to me again.[f000]븁\u0000\nPlease enjoy your time here."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_EndMultiTrainLine, 0x802e, 2, 0
    VMJump L_139F

L_1355:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_1368
    VMJump L_137A

L_1368:
    // "This is the end of the Multi Train line.[f000]븁\u0000\nWhen you want to return to Nimbasa City,\nplease talk to me again.[f000]븁\u0000\nPlease enjoy your time here."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_EndMultiTrainLine, 0x802e, 2, 0
    VMJump L_139F

L_137A:
    WorkCmpConst 0x8023, 4
    VMJumpIf CMP_EQ, L_138D
    VMJump L_139F

L_138D:
    // "This is the end of the Wi-Fi Train line.[f000]븁\u0000\nWhen you want to return to Nimbasa City,\nplease talk to me again.[f000]븁\u0000\nPlease enjoy your time here."
    ActorMsg MSGFILE_SCRIPT, BattleSubway2_Text_EndWiFiTrain, 0x802e, 2, 0
    VMJump L_139F

L_139F:
    LastKeyWait
    MsgWinCloseAll
    BSubwayCmd_Tool 333, 0, 0, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_13CB:
    WorkSetConst 0x8032, 0
    BSubwayCmd_Tool 310, 0, 0, 32803
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_140A
    WorkSetConst 0x8032, 1
    VMJump L_1410

L_140A:
    WorkSetConst 0x8032, 0

L_1410:
    BSubwayCmd_Tool 13, 255, 1, 0
    BSubwayCmd_Tool 23, 255, 0, 0
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_147D
    BSubwayCmd_Tool 31, 0, 0, 16416
    FlagReset EVENT_FLAG_0x025c
    ActorAdd 2
    BSubwayCmd_Tool 23, 2, 0, 0
    BSubwayCmd_Tool 13, 2, 1, 0
    BSubwayCmd_Tool 36, 2, 0, 0
    BSubwayCmd_Tool 35, 2, 1, 0
    ActorSetGPos 2, 75, 0, 12, 1

L_147D:
    BSubwayCmd_Tool 19, 2, 0, 0
    FadeInBlackQ
    FadeWait
    VMSleep 10
    SEPlay SEQ_SE_BDEMO_03
    VMSleep 60
    SEPlay SEQ_SE_BDEMO_01
    ActorCmdExec 255, Movement_1548
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14BE
    ActorCmdExec 2, Movement_155C

L_14BE:
    ActorCmdWait
    BSubwayCmd_Tool 24, 255, 0, 0
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14E7
    BSubwayCmd_Tool 24, 2, 0, 0

L_14E7:
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_153E
    VMSleep 30
    BSubwayCmd_Tool 19, 0, 0, 0
    SEPlay SEQ_SE_BDEMO_01
    VMSleep 20
    SEPlay SEQ_SE_BDEMO_04
    VMSleep 30
    FadeEx 3, 0, 16, 2
    FadeExWait
    BSubwayCmd_Tool 20, 1, 0, 0
    VMSleep 30
    FadeEx 3, 16, 0, 2
    FadeExWait

L_153E:
    WorkSetConst 0x8032, 0
    VMReturn
    .balign 4, 0

Movement_1548:
    Move 70, 1
    Move 60, 4
    Move 13, 2
    Move 35, 1
    MoveEnd

Movement_155C:
    Move 60, 14
    Move 70, 1
    Move 60, 4
    Move 13, 1
    Move 14, 1
    Move 13, 1
    Move 35, 1
    MoveEnd

L_157C:
    WorkSetConst 0x8033, 0
    FadeEx 3, 0, 16, 2
    FadeExWait
    BSubwayCmd_Tool 35, 0, 3, 0
    BSubwayCmd_Tool 310, 0, 0, 32803
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_15D1
    WorkSetConst 0x8033, 1
    VMJump L_15D7

L_15D1:
    WorkSetConst 0x8033, 0

L_15D7:
    ActorSetGPos 255, 18, 0, 14, 2
    BSubwayCmd_Tool 23, 255, 0, 0
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1620
    ActorSetGPos 2, 19, 0, 14, 2
    BSubwayCmd_Tool 23, 2, 0, 0
    BSubwayCmd_Tool 36, 2, 0, 0

L_1620:
    FadeEx 3, 16, 0, 2
    VMStackPush EVENT_WORK_0x4179
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1661
    BSubwayCmd_Tool 19, 2, 1, 0
    BSubwayCmd_Tool 20, 0, 0, 0
    SEPlay SEQ_SE_BDEMO_03
    VMSleep 70
    SEPlay SEQ_SE_BDEMO_01
    VMSleep 20

L_1661:
    FadeExWait
    ActorCmdExec 255, Movement_16B0
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1686
    ActorCmdExec 2, Movement_16C4

L_1686:
    ActorCmdWait
    BSubwayCmd_Tool 19, 3, 1, 0
    SEPlay SEQ_SE_BDEMO_01
    VMSleep 30
    SEPlay SEQ_SE_BDEMO_02
    VMSleep 30
    FadeOutBlackQ
    FadeWait
    WorkSetConst 0x8033, 0
    VMReturn
    .balign 4, 0

Movement_16B0:
    Move 32, 1
    Move 12, 2
    Move 60, 8
    Move 69, 1
    MoveEnd

Movement_16C4:
    Move 60, 16
    Move 14, 1
    Move 12, 2
    Move 60, 8
    Move 69, 1
    MoveEnd

L_16DC:
    BSubwayCmd_Tool 401, 0, 0, 0
    Cmd_013C
    BSubwayCmd_Tool 330, 0, 0, 0
    VMReturn

L_16F4:
    BSubwayCmd_Tool 413, 0, 0, 0
    VMCall L_16DC
    VMReturn
    BSubwayCmd_Tool 414, 0, 0, 0
    VMCall L_16DC
    VMReturn

L_1718:
    BSubwayCmd_Tool 402, 104, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_173F
    BSubwayCmd_Tool 413, 0, 0, 0

L_173F:
    VMCall L_16DC
    VMReturn
    BSubwayCmd_Tool 402, 104, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_176E
    BSubwayCmd_Tool 414, 0, 0, 0

L_176E:
    VMCall L_16DC
    VMReturn

L_1776:
    BSubwayCmd_Tool 202, 99, 0, 0
    BSubwayCmd_Tool 21, 0, 0, 32803
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_179D
    VMJump L_17AF

L_179D:
    MapChangeCore ZONE_GEAR_STATION_2, 11, 0, 15, 3
    VMJump L_18E3

L_17AF:
    WorkCmpConst 0x8023, 5
    VMJumpIf CMP_EQ, L_17C2
    VMJump L_17D4

L_17C2:
    MapChangeCore ZONE_GEAR_STATION_3, 11, 0, 15, 3
    VMJump L_18E3

L_17D4:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_17E7
    VMJump L_17F9

L_17E7:
    MapChangeCore ZONE_GEAR_STATION_4, 11, 0, 15, 3
    VMJump L_18E3

L_17F9:
    WorkCmpConst 0x8023, 6
    VMJumpIf CMP_EQ, L_180C
    VMJump L_181E

L_180C:
    MapChangeCore ZONE_GEAR_STATION_5, 11, 0, 15, 3
    VMJump L_18E3

L_181E:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_1831
    VMJump L_1843

L_1831:
    MapChangeCore ZONE_GEAR_STATION_6, 11, 0, 15, 3
    VMJump L_18E3

L_1843:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_1856
    VMJump L_1868

L_1856:
    MapChangeCore ZONE_GEAR_STATION_6, 11, 0, 15, 3
    VMJump L_18E3

L_1868:
    WorkCmpConst 0x8023, 7
    VMJumpIf CMP_EQ, L_187B
    VMJump L_188D

L_187B:
    MapChangeCore ZONE_GEAR_STATION_7, 11, 0, 15, 3
    VMJump L_18E3

L_188D:
    WorkCmpConst 0x8023, 8
    VMJumpIf CMP_EQ, L_18A0
    VMJump L_18B2

L_18A0:
    MapChangeCore ZONE_GEAR_STATION_7, 11, 0, 15, 3
    VMJump L_18E3

L_18B2:
    WorkCmpConst 0x8023, 4
    VMJumpIf CMP_EQ, L_18C5
    VMJump L_18D7

L_18C5:
    MapChangeCore ZONE_GEAR_STATION_8, 11, 0, 15, 3
    VMJump L_18E3

L_18D7:
    MapChangeCore ZONE_GEAR_STATION_2, 11, 0, 15, 3

L_18E3:
    BSubwayCmd_Tool 202, 100, 0, 0
    VMReturn

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a subway map of the Unova region.[f000]븁\u0000"
    InfoMsg BattleSubway2_Text_ItsSubwayMapUnova, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 2, 0
    FieldOpen
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
