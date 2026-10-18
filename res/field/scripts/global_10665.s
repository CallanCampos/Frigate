#include "asm/field_script.inc"
#include "text/script/global_10665.h"

// Script plugin 7, from the only plugin whose commands it decodes with

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0

Script_1:
    ActorsPauseAll
    VMCall L_0256
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_008F
    WbtCmd_GetTournament 0x8010
    Cmd_01DD 11, 0x8010, 0
    VMCall L_0139
    VMJump L_0131

L_008F:
    WbtCmd_GetTournament 0x8010
    Cmd_01DD 10, 0x8010, 1
    VMCall L_053D
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00CC
    WbtCmd_GetTournament 0x8010
    Cmd_01DD 11, 0x8010, 1
    VMCall L_0139
    VMJump L_0131

L_00CC:
    WbtCmd_GetTournament 0x8010
    Cmd_01DD 10, 0x8010, 2
    VMCall L_07E0
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_010F
    WorkSetConst 0x8027, 0
    VMCall L_0B14
    WbtCmd_GetTournament 0x8010
    Cmd_01DD 11, 0x8010, 2
    VMJump L_012B

L_010F:
    WorkSetConst 0x8027, 1
    VMCall L_0B14
    WbtCmd_SetRound 5
    WbtCmd_GetTournament 0x8010
    Cmd_01DD 10, 0x8010, 3

L_012B:
    VMCall L_0139

L_0131:
    EvCameraEnd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0139:
    BGMChangeMap
    SEStop
    RTReserveScript 10658
    Plugin7_Cmd1016 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0168
    MapChangeWarp ZONE_PWT_2, 14, 0, 1
    VMJump L_0185

L_0168:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0185
    MapChangeWarp ZONE_PWT_2, 16, 0, 1

L_0185:
    VMReturn

Script_2:
    ActorsPauseAll
    WbtCmd_Create
    WbtCmd_SetTournament 4
    WbtCmd_SetStyle 0
    WbtCmd_SetRound 2
    Plugin7_Cmd1005
    Plugin7_Cmd1040 EVENT_WORK_0x4020
    ActorNew 21, 15, 3, 251, 162, 0
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 1
    EvCameraWait
    VMCall L_053D
    MapChangeWarp ZONE_PWT_2, 15, 0, 1
    EvCameraEnd
    RTReserveScript 10658
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WbtCmd_Create
    WbtCmd_SetTournament 4
    WbtCmd_SetStyle 0
    WbtCmd_SetRound 3
    Plugin7_Cmd1005
    Plugin7_Cmd1040 EVENT_WORK_0x4020
    ActorNew 21, 15, 3, 251, 162, 0
    VMCall L_07E0
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0234
    WorkSetConst 0x8027, 0
    VMCall L_0B14
    VMJump L_0240

L_0234:
    WorkSetConst 0x8027, 1
    VMCall L_0B14

L_0240:
    MapChangeWarp ZONE_PWT_2, 15, 0, 1
    EvCameraEnd
    RTReserveScript 10658
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0256:
    SEPlay SEQ_SE_SW_WBT_07
    Plugin7_Cmd1047 0
    WbtCmd_SetRound 2
    WbtCmd_GetTournament 0x8029
    VMStackPush 0x8029
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0315
    Plugin7_Cmd1040 EVENT_WORK_0x4020
    ActorNew 3, 15, 3, 251, 162, 0
    ActorSetGPos 255, 42, 0, 15, 2
    ActorCmdExec 251, Movement_0AD4
    ActorCmdExec 255, Movement_0AD4
    ActorCmdWait
    Plugin7_Cmd1052 3
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 3544, 0, 0xed000, 0x2ac000, 0x2001f, 0x108000, 1
    EvCameraWait
    SEPlay SEQ_SE_SW_WBT_07
    FadeInBlackQ
    FadeWait
    WordSetPlayerName 1
    // "The Driftveil Tournament\nstarts now![f000]븁\u0000\nFirst-timer [f000]Ā\u0001\u0001 enters![f000]븁\u0000"
    InfoMsg Global10665_Text_DriftveilTournamentStartsNow, 1
    InfoMsgClose_0039
    ActorCmdExec 255, Movement_0ADC
    ActorCmdWait
    ActorCmdExec 255, Movement_0AE4
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 146
    ActorCmdWait
    EvCameraWait
    VMSleep 30
    VMJump L_037F

L_0315:
    Plugin7_Cmd1040 EVENT_WORK_0x4020
    ActorNew 3, 15, 3, 251, 162, 0
    ActorCmdExec 251, Movement_0AD4
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x4a5bf, 0xe7000, 1
    EvCameraWait
    Plugin7_Cmd1052 3
    FadeInBlack
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 48
    ActorCmdExec 255, Movement_0A84
    EvCameraWait
    FadeWait
    ActorCmdWait
    VMSleep 30

L_037F:
    EvCameraMoveTo 3544, 0, 0xed000, 0x33000, 0x2001f, 0x108000, 60
    FadeEx 3, 0, 16, 2
    FadeExWait
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin7_Cmd1043 1
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0ADC
    ActorCmdWait
    ActorCmdExec 251, Movement_0A8C
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 78
    VMSleep 20
    FadeEx 3, 0, 16, 2
    FadeExWait
    Plugin7_Cmd1060 0
    VMSleep 50
    FadeEx 3, 16, 0, 2
    FadeExWait
    ActorCmdWait
    EvCameraWait
    WbtCmd_MakeOpponentParty
    VMStackPush 0x8029
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMStackPush 0x8029
    VMStackPushConst 15
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0436
    VMCall L_0BDD

L_0436:
    Plugin7_Cmd1045 0
    WordSetPlayerName 1
    // "The first round!\n[f000]Ā\u0001\u0000 vs. [f000]Ā\u0001\u0001.[f000]븁\u0000"
    InfoMsg Global10665_Text_FirstRoundVs, 1
    InfoMsgClose_0039
    SEPlay SEQ_SE_SW_WBT_16
    Plugin7_Cmd1052 0
    VMSleep 29
    SEPlay SEQ_SE_SW_WBT_21
    VMSleep 31
    SEPlay SEQ_SE_SW_WBT_22
    VMSleep 30
    Plugin7_Cmd1041 0, 251
    ActorMsgClose
    SEStop
    ActorCmdExec 255, Movement_0B08
    ActorCmdExec 251, Movement_0AFC
    ActorCmdWait
    Plugin7_Cmd1042 0x8027
    SEPlay SEQ_SE_SW_WBT_07
    Plugin7_Cmd1052 3
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 1
    EvCameraWait
    FadeInBlackQ_
    FadeWait
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0519
    WordSetPlayerName 0
    SEPlay SEQ_SE_SW_WBT_02
    // "The winner of the first round is...\n[f000]Ā\u0001\u0000![f000]븁\u0000"
    ScreamMsg Global10665_Text_WinnerFirstRound, 1
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8029
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_04F1
    VMCall L_0DA8

L_04F1:
    InfoMsgClose_0039
    Plugin7_Cmd1041 5, 251
    ActorMsgClose
    ActorCmdExec 251, Movement_0A9C
    ActorCmdWait
    ActorCmdExec 255, Movement_0AA4
    ActorCmdWait
    ActorDelete 251
    VMJump L_0539

L_0519:
    Plugin7_Cmd1045 0
    SEPlay SEQ_SE_SW_WBT_02
    // "The winner of the first round is...\n[f000]Ā\u0001\u0000![f000]븁\u0000"
    ScreamMsg Global10665_Text_WinnerFirstRound, 1
    InfoMsgClose_0039
    Plugin7_Cmd1041 6, 251
    ActorMsgClose
    ActorCmdExec 255, Movement_0AF4
    ActorCmdWait

L_0539:
    EvCameraEnd
    VMReturn

L_053D:
    // "Let's check out\nthe other matches![f000]븁\u0000\nTurn your attention to\nthe giant screen![f000]븁\u0000"
    InfoMsg Global10665_Text_LetsCheckOutOther, 1
    InfoMsgClose_0039
    FadeOutBlack
    SEPlay SEQ_SE_SW_WBT_20
    EvCameraMoveTo 3554, 0, 0xecf4c, 0x170000, 0x2a01f, 0x56000, 18
    FadeWait
    EvCameraWait
    Plugin7_Cmd1047 1
    WbtCmd_SetRound 3
    Plugin7_Cmd1040 EVENT_WORK_0x4020
    ActorNew 3, 15, 3, 251, 162, 0
    ActorSetGPos 255, 31, 0, 15, 2
    ActorCmdExec 251, Movement_0AD4
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x4a5bf, 0xe7000, 1
    EvCameraWait
    Plugin7_Cmd1052 3
    FadeInBlack
    SEPlay SEQ_SE_SW_WBT_02
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 30
    ActorCmdExec 255, Movement_0AAC
    EvCameraWait
    FadeWait
    WorkSetConst 0x802c, 0
    VMCall L_0E8B
    ActorCmdWait
    VMSleep 30
    EvCameraMoveTo 3544, 0, 0xed000, 0x33000, 0x2001f, 0x108000, 60
    FadeEx 3, 0, 16, 2
    FadeExWait
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin7_Cmd1043 1
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0ADC
    ActorCmdWait
    SEPlay SEQ_SE_SW_WBT_07
    ActorCmdExec 251, Movement_0AB4
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 78
    VMSleep 20
    FadeEx 3, 0, 16, 2
    FadeExWait
    Plugin7_Cmd1060 0
    VMSleep 50
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkSetConst 0x802c, 1
    VMCall L_0E8B
    ActorCmdWait
    EvCameraWait
    WbtCmd_MakeOpponentParty
    VMStackPush 0x8029
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMStackPush 0x8029
    VMStackPushConst 15
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_06BB
    VMCall L_0BDD

L_06BB:
    Plugin7_Cmd1045 0
    WordSetPlayerName 1
    VMSleep 30
    // "The second round!\n[f000]Ā\u0001\u0000 vs. [f000]Ā\u0001\u0001.[f000]븁\u0000"
    InfoMsg Global10665_Text_SecondRoundVs, 1
    InfoMsgClose_0039
    SEPlay SEQ_SE_SW_WBT_16
    Plugin7_Cmd1052 0
    VMSleep 29
    SEPlay SEQ_SE_SW_WBT_21
    VMSleep 31
    SEPlay SEQ_SE_SW_WBT_22
    VMSleep 30
    Plugin7_Cmd1041 1, 251
    ActorMsgClose
    SEStop
    ActorCmdExec 255, Movement_0B08
    ActorCmdExec 251, Movement_0AFC
    ActorCmdWait
    Plugin7_Cmd1042 0x8027
    SEPlay SEQ_SE_SW_WBT_07
    Plugin7_Cmd1052 3
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 1
    EvCameraWait
    FadeInBlackQ_
    FadeWait
    SEPlay SEQ_SE_SW_WBT_02
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07B2
    WordSetPlayerName 0
    SEPlay SEQ_SE_SW_WBT_02
    // "The winner of the second round is...\n[f000]Ā\u0001\u0000![f000]븁\u0000"
    ScreamMsg Global10665_Text_WinnerSecondRound, 1
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8029
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_077E
    VMCall L_0DA8

L_077E:
    InfoMsgClose_0039
    SEPlay SEQ_SE_SW_WBT_17
    Plugin7_Cmd1056 2
    VMSleep 60
    Plugin7_Cmd1041 5, 251
    ActorMsgClose
    ActorCmdExec 251, Movement_0AC4
    ActorCmdWait
    ActorCmdExec 255, Movement_0ACC
    ActorCmdWait
    ActorDelete 251
    VMJump L_07DE

L_07B2:
    Plugin7_Cmd1045 0
    SEPlay SEQ_SE_SW_WBT_02
    // "The winner of the second round is...\n[f000]Ā\u0001\u0000![f000]븁\u0000"
    ScreamMsg Global10665_Text_WinnerSecondRound, 1
    InfoMsgClose_0039
    SEPlay SEQ_SE_SW_WBT_17
    Plugin7_Cmd1056 2
    VMSleep 60
    Plugin7_Cmd1041 6, 251
    ActorMsgClose
    ActorCmdExec 255, Movement_0AF4
    ActorCmdWait

L_07DE:
    VMReturn

L_07E0:
    // "Let's check out the other match\non the giant screen![f000]븁\u0000"
    InfoMsg Global10665_Text_LetsCheckOutOther_2, 1
    InfoMsgClose_0039
    FadeOutBlack
    SEPlay SEQ_SE_SW_WBT_20
    EvCameraMoveTo 3554, 0, 0xecf4c, 0x170000, 0x2a01f, 0x56000, 18
    FadeWait
    EvCameraWait
    Plugin7_Cmd1047 2
    WbtCmd_SetRound 4
    Plugin7_Cmd1040 EVENT_WORK_0x4020
    ActorNew 3, 15, 3, 251, 162, 0
    ActorSetGPos 255, 42, 0, 15, 2
    ActorCmdExec 251, Movement_0AD4
    ActorCmdExec 255, Movement_0AD4
    ActorCmdWait
    Plugin7_Cmd1052 3
    Plugin7_Cmd1055 0, 1
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 3544, 0, 0xed000, 0x2ac000, 0x2001f, 0x108000, 1
    EvCameraWait
    SEStop
    BGMFadeOutAll 30
    FadeInBlackQ
    FadeWait
    VMSleep 30
    VMCall L_16D1
    SEPlay SEQ_SE_SW_WBT_07
    ActorCmdExec 255, Movement_0ADC
    ActorCmdWait
    Plugin7_Cmd1055 1, 1
    BGMChangeMap
    VMCall L_17EE
    Plugin7_Cmd1053 0x802a, 1
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08B6
    VMSleep 40

L_08B6:
    SEPlay SEQ_SE_SW_WBT_01
    ActorCmdExec 255, Movement_0AE4
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 146
    VMCall L_0FEF
    ActorCmdWait
    EvCameraWait
    SEPlay SEQ_SE_SW_WBT_18
    Plugin7_Cmd1056 1
    VMSleep 60
    WbtCmd_GetTournament 0x8010
    Plugin7_Cmd1044 0, 0x8010
    // "[f000]Ļ\u0001\u0000!\nThe final round![f000]븀\u0000\nThe opponent is entering the arena![f000]븁\u0000"
    InfoMsg Global10665_Text_FinalRoundOpponentEntering, 1
    InfoMsgClose_0039
    EvCameraMoveTo 3544, 0, 0xed000, 0x33000, 0x2001f, 0x108000, 60
    FadeEx 3, 0, 16, 2
    FadeExWait
    Plugin7_Cmd1060 0
    VMSleep 60
    Plugin7_Cmd1055 0, 0
    SEStop
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin7_Cmd1043 1
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0ADC
    ActorCmdWait
    Plugin7_Cmd1055 1, 0
    VMCall L_17EE
    Plugin7_Cmd1053 0x802a, 0
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0979
    VMSleep 40

L_0979:
    SEPlay SEQ_SE_SW_WBT_01
    ActorCmdExec 251, Movement_0AEC
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 146
    VMCall L_11FA
    ActorCmdWait
    EvCameraWait
    SEPlay SEQ_SE_SW_WBT_18
    Plugin7_Cmd1056 0
    VMSleep 60
    WbtCmd_MakeOpponentParty
    VMStackPush 0x8029
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMStackPush 0x8029
    VMStackPushConst 15
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_09DE
    VMCall L_0BDD

L_09DE:
    Plugin7_Cmd1045 0
    WordSetPlayerName 1
    // "The final round!\n[f000]Ā\u0001\u0000 vs. [f000]Ā\u0001\u0001.[f000]븁\u0000"
    InfoMsg Global10665_Text_FinalRoundVs, 1
    InfoMsgClose_0039
    SEPlay SEQ_SE_SW_WBT_16
    Plugin7_Cmd1052 0
    VMSleep 29
    SEPlay SEQ_SE_SW_WBT_21
    VMSleep 31
    SEPlay SEQ_SE_SW_WBT_22
    VMSleep 30
    Plugin7_Cmd1041 2, 251
    ActorMsgClose
    SEStop
    ActorCmdExec 255, Movement_0B08
    ActorCmdExec 251, Movement_0AFC
    ActorCmdWait
    SEPlay SEQ_SE_SW_WBT_19
    Plugin7_Cmd1056 3
    VMSleep 30
    SEWait
    Plugin7_Cmd1042 0x8027
    BGMFadeOutAll 1
    Plugin7_Cmd1052 3
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 2008, 0, 0xfd000, 0x172000, 0x2001f, 0xe7000, 1
    EvCameraWait
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A78
    Plugin7_Cmd1060 1
    VMJump L_0A7C

L_0A78:
    Plugin7_Cmd1060 2

L_0A7C:
    FadeInBlackQ_
    FadeWait
    VMReturn
    .balign 4, 0

Movement_0A84:
    Move 14, 6
    MoveEnd

Movement_0A8C:
    Move 15, 6
    Move 23, 4
    Move 15, 7
    MoveEnd

Movement_0A9C:
    Move 14, 7
    MoveEnd

Movement_0AA4:
    Move 15, 7
    MoveEnd

Movement_0AAC:
    Move 14, 6
    MoveEnd

Movement_0AB4:
    Move 15, 6
    Move 23, 4
    Move 15, 7
    MoveEnd

Movement_0AC4:
    Move 14, 7
    MoveEnd

Movement_0ACC:
    Move 15, 7
    MoveEnd

Movement_0AD4:
    Move 69, 1
    MoveEnd

Movement_0ADC:
    Move 70, 1
    MoveEnd

Movement_0AE4:
    Move 14, 17
    MoveEnd

Movement_0AEC:
    Move 15, 17
    MoveEnd

Movement_0AF4:
    Move 15, 7
    MoveEnd

Movement_0AFC:
    Move 63, 1
    Move 170, 1
    MoveEnd

Movement_0B08:
    Move 63, 1
    Move 169, 1
    MoveEnd

L_0B14:
    // "The winner is...[f000]븁\u0000"
    InfoMsg Global10665_Text_Winner, 1
    InfoMsgClose_0039
    VMSleep 30
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B3B
    WordSetPlayerName 0
    VMJump L_0B3E

L_0B3B:
    Plugin7_Cmd1045 0

L_0B3E:
    SEPlay SEQ_SE_SW_WBT_15
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B5F
    Plugin7_Cmd1052 1
    VMJump L_0B63

L_0B5F:
    Plugin7_Cmd1052 2

L_0B63:
    Plugin7_Cmd1054
    VMSleep 80
    SEPlay SEQ_SE_SW_WBT_07
    SEPlay SEQ_SE_SW_WBT_03
    // "[f000]Ā\u0001\u0000!"
    ScreamMsg Global10665_Text_Empty, 1
    BGMPlay SEQ_BGM_WBT_FANFARE
    VMSleep 120
    MsgWaitAdvance
    InfoMsgClose_0039
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BA1
    Plugin7_Cmd1041 5, 251
    VMJump L_0BA7

L_0BA1:
    Plugin7_Cmd1041 6, 251

L_0BA7:
    ActorMsgClose
    WbtCmd_GetTournament 0x8010
    Plugin7_Cmd1044 0, 0x8010
    // "This concludes the\n[f000]Ļ\u0001\u0000![f000]븀\u0000\nSee you in the next tournament![f000]븁\u0000"
    InfoMsg Global10665_Text_ConcludesSeeNextTournament, 1
    InfoMsgClose_0039
    VMSleep 60
    VMReturn
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0

L_0BDD:
    Plugin7_Cmd1031
    // "How strong are your Pokémon\nwhen they face your own party?[f000]븁\u0000\nLet's mix them![f000]븁\u0000"
    ScreamMsg Global10665_Text_HowStrongPokemonWhen, 1
    SEPlay SEQ_SE_SW_WBT_01
    VMSleep 30
    MsgWinCloseAll
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 2
    VMCall L_0C3C
    WbtCmd_GetStyle 0x8010
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C38
    // "Let's swap another Pokémon![f000]븁\u0000"
    ScreamMsg Global10665_Text_LetsSwapAnotherPokemon, 1
    SEPlay SEQ_SE_SW_WBT_01
    VMSleep 30
    MsgWinCloseAll
    WorkSetConst 0x8030, 1
    WorkSetConst 0x8031, 3
    VMCall L_0C3C

L_0C38:
    Plugin7_Cmd1037
    VMReturn

L_0C3C:
    Plugin7_Cmd1032 0x8030
    Plugin7_Cmd1045 0
    Plugin7_Cmd1034 1, 0x8030
    // "[f000]Ā\u0001\u0000 chose\n[f000]ā\u0001\u0001.[f000]븁\u0000"
    SystemMsg Global10665_Text_Chose, 1
    WorkSetConst 0x802d, 1

L_0C54:
    VMStackPush 0x802d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CE9
    Plugin7_Cmd1029
    // "Select one Pokémon\nfrom the opponent's party."
    SystemMsg Global10665_Text_SelectOnePokemonFrom, 1
    ListMenu_AnchorTopRight 31, 7, 0, 0, 32815
    Plugin7_Cmd1035
    ListMenuShow
    Plugin7_Cmd1030
    Plugin7_Cmd1024 0x802e
    DebugPrint 0x802f
    DebugPrint 0x802e
    VMStackPush 0x802f
    VMStackPush 0x802e
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0CDD
    Plugin7_Cmd1033 0x8031, 0x802f
    Plugin7_Cmd1034 0, 0x8031
    // "Are you OK with [f000]ā\u0001\u0000?"
    SystemMsg Global10665_Text_Ok, 1
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CD1
    WorkSetConst 0x802d, 0
    VMJump L_0CD7

L_0CD1:
    Plugin7_Cmd1033 0x8031, 0x802e

L_0CD7:
    VMJump L_0CE3

L_0CDD:
    // "You've selected\nthat Pokémon already.[f000]븁\u0000"
    SystemMsg Global10665_Text_YouveSelectedPokemonAlready, 1

L_0CE3:
    VMJump L_0C54

L_0CE9:
    InfoMsgClose
    // "Let's mix them![f000]븁\u0000"
    ScreamMsg Global10665_Text_LetsMixThem, 1
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0D80
    ActorCmdExec 255, Movement_0D78
    ActorCmdWait
    Plugin7_Cmd1034 0, 0x8030
    MultiMsg 56, 3, 3, 1
    Plugin7_Cmd1034 0, 0x8031
    MultiMsg 56, 19, 16, 2
    VMSleep 60
    MsgWinCloseNo 1
    MsgWinCloseNo 2
    ActorCmdExec 251, Movement_0D98
    ActorCmdExec 255, Movement_0D88
    ActorCmdWait
    WordSetPlayerName 0
    Plugin7_Cmd1034 1, 0x8030
    Plugin7_Cmd1045 2
    Plugin7_Cmd1034 3, 0x8031
    // "[f000]ā\u0001\u0001 from [f000]Ā\u0001\u0000's party\nand [f000]ā\u0001\u0003 from [f000]Ā\u0001\u0002's party[f000]븀\u0000\nhave been swapped.[f000]븁\u0000"
    SystemMsg Global10665_Text_FromSPartyFrom, 1
    InfoMsgClose
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x31
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x30
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x2f
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x2e
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x2d
    .byte 0x80
    .balign 4, 0

Movement_0D78:
    Move 14, 1
    MoveEnd

Movement_0D80:
    Move 15, 1
    MoveEnd

Movement_0D88:
    Move 71, 1
    Move 15, 1
    Move 72, 1
    MoveEnd

Movement_0D98:
    Move 71, 1
    Move 14, 1
    Move 72, 1
    MoveEnd

L_0DA8:
    Plugin7_Cmd1019 0x8010
    VMStackPush 0x8010
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DCA
    // "In this tough tournament, where\neverybody is expected to face a[f000]븀\u0000\ntough road, we just witnessed a[f000]븀\u0000\ntotal victory! Great![f000]븁\u0000"
    ScreamMsg Global10665_Text_ToughTournamentWhereEverybody, 1
    VMJump L_0DFB

L_0DCA:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DEF
    WordSetNumber 0, 0x8010, 1
    // "[f000]Ȁ\u0001\u0000 Pokémon left![f000]븁\u0000\nCan we still call this raw power?[f000]븁\u0000"
    ScreamMsg Global10665_Text_PokemonLeftCanWe, 1
    VMJump L_0DFB

L_0DEF:
    WordSetNumber 0, 0x8010, 1
    // "Is this what they call raw power?\nA great victory with [f000]Ȁ\u0001\u0000 Pokémon left![f000]븁\u0000"
    ScreamMsg Global10665_Text_WhatTheyCallRaw, 1

L_0DFB:
    VMReturn

L_0DFD:
    WbtCmd_GetWinCount 0, 17, 0x8020
    DebugPrint 0x8020
    VMStackPush 0x8020
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0E28
    WorkSetConst 0x8023, 99
    VMJump L_0E85

L_0E28:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0E47
    WorkSetConst 0x8023, 79
    VMJump L_0E85

L_0E47:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0E66
    WorkSetConst 0x8023, 59
    VMJump L_0E85

L_0E66:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0E85
    WorkSetConst 0x8023, 39
    VMJump L_0E85

L_0E85:
    DebugPrint 0x8023
    VMReturn

L_0E8B:
    WorkCmpConst 0x802c, 0
    VMJumpIf CMP_EQ, L_0E9E
    VMJump L_0EAA

L_0E9E:
    VMCall L_0DFD
    VMJump L_0EC9

L_0EAA:
    WorkCmpConst 0x802c, 1
    VMJumpIf CMP_EQ, L_0EBD
    VMJump L_0EC9

L_0EBD:
    WorkSetConst 0x8023, 99
    VMJump L_0EC9

L_0EC9:
    Random 0x8021, 100
    DebugPrint 0x8021
    WorkSetConst EVENT_WORK_0x400e, 0
    WorkSetConst EVENT_WORK_0x400f, 0
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0F48
    WorkSetConst 0x8024, 3
    WorkSetConst 0x8025, 3
    WorkSetConst 0x8026, 1
    WorkCmpConst 0x802c, 0
    VMJumpIf CMP_EQ, L_0F17
    VMJump L_0F20

L_0F17:
    WordSetPlayerName 0
    VMJump L_0F3C

L_0F20:
    WorkCmpConst 0x802c, 1
    VMJumpIf CMP_EQ, L_0F33
    VMJump L_0F3C

L_0F33:
    Plugin7_Cmd1045 0
    VMJump L_0F3C

L_0F3C:
    VMCall L_1405
    WorkSetConst EVENT_WORK_0x400e, 1

L_0F48:
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0FBB
    WorkSetConst 0x8024, 19
    WorkSetConst 0x8025, 16
    WorkSetConst 0x8026, 2
    WorkCmpConst 0x802c, 0
    VMJumpIf CMP_EQ, L_0F8A
    VMJump L_0F93

L_0F8A:
    WordSetPlayerName 0
    VMJump L_0FAF

L_0F93:
    WorkCmpConst 0x802c, 1
    VMJumpIf CMP_EQ, L_0FA6
    VMJump L_0FAF

L_0FA6:
    Plugin7_Cmd1045 0
    VMJump L_0FAF

L_0FAF:
    VMCall L_1405
    WorkSetConst EVENT_WORK_0x400f, 1

L_0FBB:
    VMSleep 60
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FD6
    MsgWinCloseNo 1

L_0FD6:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FED
    MsgWinCloseNo 2

L_0FED:
    VMReturn

L_0FEF:
    VMCall L_0DFD
    WordSetPlayerName 0
    Random 0x8021, 100
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_102F
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8025, 1
    WorkSetConst 0x8026, 1
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4001, 1

L_102F:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_106E
    WorkSetConst 0x8024, 18
    WorkSetConst 0x8025, 16
    WorkSetConst 0x8026, 2
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4002, 1

L_106E:
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1085
    MsgWinCloseNo 1

L_1085:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_10C4
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8025, 18
    WorkSetConst 0x8026, 3
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4003, 1

L_10C4:
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10DB
    MsgWinCloseNo 2

L_10DB:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_111A
    WorkSetConst 0x8024, 18
    WorkSetConst 0x8025, 1
    WorkSetConst 0x8026, 4
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4004, 1

L_111A:
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1131
    MsgWinCloseNo 3

L_1131:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1170
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8025, 18
    WorkSetConst 0x8026, 5
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4005, 1

L_1170:
    VMStackPush EVENT_WORK_0x4004
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1187
    MsgWinCloseNo 4

L_1187:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_11C6
    WorkSetConst 0x8024, 19
    WorkSetConst 0x8025, 1
    WorkSetConst 0x8026, 6
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4006, 1

L_11C6:
    VMStackPush EVENT_WORK_0x4005
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11DD
    MsgWinCloseNo 5

L_11DD:
    VMSleep 20
    VMStackPush EVENT_WORK_0x4006
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11F8
    MsgWinCloseNo 6

L_11F8:
    VMReturn

L_11FA:
    WorkSetConst 0x8023, 99
    Plugin7_Cmd1045 0
    Random 0x8021, 100
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_123E
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8025, 1
    WorkSetConst 0x8026, 1
    VMCall L_156B
    VMSleep 20
    WorkSetConst EVENT_WORK_0x4007, 1

L_123E:
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1279
    WorkSetConst 0x8024, 18
    WorkSetConst 0x8025, 18
    WorkSetConst 0x8026, 2
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4008, 1

L_1279:
    VMStackPush EVENT_WORK_0x4007
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1290
    MsgWinCloseNo 1

L_1290:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_12CF
    WorkSetConst 0x8024, 18
    WorkSetConst 0x8025, 1
    WorkSetConst 0x8026, 3
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x4009, 1

L_12CF:
    VMStackPush EVENT_WORK_0x4008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12E6
    MsgWinCloseNo 2

L_12E6:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1325
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8025, 18
    WorkSetConst 0x8026, 4
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x400a, 1

L_1325:
    VMStackPush EVENT_WORK_0x4009
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_133C
    MsgWinCloseNo 3

L_133C:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_137B
    WorkSetConst 0x8024, 19
    WorkSetConst 0x8025, 5
    WorkSetConst 0x8026, 5
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x400b, 1

L_137B:
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1392
    MsgWinCloseNo 4

L_1392:
    VMSleep 20
    Random 0x8021, 100
    DebugPrint 0x8021
    VMStackPush 0x8023
    VMStackPush 0x8021
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_13D1
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8025, 19
    WorkSetConst 0x8026, 6
    VMCall L_156B
    WorkSetConst EVENT_WORK_0x400c, 1

L_13D1:
    VMStackPush EVENT_WORK_0x400b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_13E8
    MsgWinCloseNo 5

L_13E8:
    VMSleep 20
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1403
    MsgWinCloseNo 6

L_1403:
    VMReturn

L_1405:
    Random 0x8022, 10
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_141E
    VMJump L_142E

L_141E:
    MultiMsg 11, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_142E:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_1441
    VMJump L_1451

L_1441:
    MultiMsg 12, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_1451:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_1464
    VMJump L_1474

L_1464:
    MultiMsg 13, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_1474:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_1487
    VMJump L_1497

L_1487:
    MultiMsg 14, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_1497:
    WorkCmpConst 0x8022, 4
    VMJumpIf CMP_EQ, L_14AA
    VMJump L_14BA

L_14AA:
    MultiMsg 15, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_14BA:
    WorkCmpConst 0x8022, 5
    VMJumpIf CMP_EQ, L_14CD
    VMJump L_14DD

L_14CD:
    MultiMsg 16, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_14DD:
    WorkCmpConst 0x8022, 6
    VMJumpIf CMP_EQ, L_14F0
    VMJump L_1500

L_14F0:
    MultiMsg 17, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_1500:
    WorkCmpConst 0x8022, 7
    VMJumpIf CMP_EQ, L_1513
    VMJump L_1523

L_1513:
    MultiMsg 18, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_1523:
    WorkCmpConst 0x8022, 8
    VMJumpIf CMP_EQ, L_1536
    VMJump L_1546

L_1536:
    MultiMsg 19, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_1546:
    WorkCmpConst 0x8022, 9
    VMJumpIf CMP_EQ, L_1559
    VMJump L_1569

L_1559:
    MultiMsg 20, 0x8024, 0x8025, 0x8026
    VMJump L_1569

L_1569:
    VMReturn

L_156B:
    Random 0x8022, 10
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_1584
    VMJump L_1594

L_1584:
    MultiMsg 21, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_1594:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_15A7
    VMJump L_15B7

L_15A7:
    MultiMsg 22, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_15B7:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_15CA
    VMJump L_15DA

L_15CA:
    MultiMsg 23, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_15DA:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_15ED
    VMJump L_15FD

L_15ED:
    MultiMsg 24, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_15FD:
    WorkCmpConst 0x8022, 4
    VMJumpIf CMP_EQ, L_1610
    VMJump L_1620

L_1610:
    MultiMsg 25, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_1620:
    WorkCmpConst 0x8022, 5
    VMJumpIf CMP_EQ, L_1633
    VMJump L_1643

L_1633:
    MultiMsg 26, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_1643:
    WorkCmpConst 0x8022, 6
    VMJumpIf CMP_EQ, L_1656
    VMJump L_1666

L_1656:
    MultiMsg 27, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_1666:
    WorkCmpConst 0x8022, 7
    VMJumpIf CMP_EQ, L_1679
    VMJump L_1689

L_1679:
    MultiMsg 28, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_1689:
    WorkCmpConst 0x8022, 8
    VMJumpIf CMP_EQ, L_169C
    VMJump L_16AC

L_169C:
    MultiMsg 29, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_16AC:
    WorkCmpConst 0x8022, 9
    VMJumpIf CMP_EQ, L_16BF
    VMJump L_16CF

L_16BF:
    MultiMsg 30, 0x8024, 0x8025, 0x8026
    VMJump L_16CF

L_16CF:
    VMReturn

L_16D1:
    WordSetPlayerName 0
    WbtCmd_GetWinCount 0, 17, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_16FA
    // "It comes down to this match!\n[f000]Ā\u0001\u0000 enters![f000]븀\u0000\nThis would be the first title win![f000]븁\u0000"
    InfoMsg Global10665_Text_ComesDownMatchEnters, 1
    VMJump L_17EA

L_16FA:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1718
    // "There are two groups who participate\nin this tournament...[f000]븀\u0000\nThe ones who've won and those[f000]븀\u0000\nwho haven't.[f000]븁\u0000\nHere comes someone who has!\n[f000]Ā\u0001\u0000 enters![f000]븁\u0000"
    InfoMsg Global10665_Text_ThereTwoGroupsWho, 1
    VMJump L_17EA

L_1718:
    VMStackPush 0x8020
    VMStackPushConst 100
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1736
    // "This Trainer has achieved over\n100 victories![f000]븁\u0000\nBut the pursuit for true strength\nbeyond mere numbers continues![f000]븁\u0000\nI present you [f000]Ā\u0001\u0000![f000]븁\u0000"
    InfoMsg Global10665_Text_TrainerHasAchievedOver, 1
    VMJump L_17EA

L_1736:
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1754
    // "The one who tasted many victories\nand witnessed many lose their battles.[f000]븁\u0000\nWhat is going through the mind of\nthis amazing Trainer?[f000]븀\u0000\n[f000]Ā\u0001\u0000 enters![f000]븁\u0000"
    InfoMsg Global10665_Text_OneWhoTastedMany, 1
    VMJump L_17EA

L_1754:
    VMStackPush 0x8020
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1772
    // "Call down a storm!\nGenerate a cyclone![f000]븁\u0000\nHere comes the highflyer of\nthis tournament![f000]븀\u0000\n[f000]Ā\u0001\u0000![f000]븀\u0000\nYou are the eye of the typhoon![f000]븁\u0000"
    InfoMsg Global10665_Text_CallDownStormGenerate, 1
    VMJump L_17EA

L_1772:
    VMStackPush 0x8020
    VMStackPushConst 20
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1790
    // "This Trainer makes the crowds go\nmad with excitement simply by[f000]븀\u0000\nparticipating in a tournament![f000]븁\u0000\n[f000]Ā\u0001\u0000 makes\na dramatic appearance![f000]븁\u0000"
    InfoMsg Global10665_Text_TrainerMakesCrowdsGo, 1
    VMJump L_17EA

L_1790:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_17AE
    // "We need both hands to count\nhow many times this Trainer has won[f000]븀\u0000\nthe tournament![f000]븁\u0000\nStarting to have the presence of\na winner, I should say![f000]븀\u0000\nCome on down! [f000]Ā\u0001\u0000![f000]븁\u0000"
    InfoMsg Global10665_Text_WeNeedBothHands, 1
    VMJump L_17EA

L_17AE:
    VMStackPush 0x8020
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_17CC
    // "Recently winning a few times,\nthis Trainer is becoming popular[f000]븀\u0000\naround Driftveil City![f000]븀\u0000\nHere comes our [f000]Ā\u0001\u0000![f000]븁\u0000"
    InfoMsg Global10665_Text_RecentlyWinningFewTimes, 1
    VMJump L_17EA

L_17CC:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_17EA
    // "Go all out![f000]븁\u0000\nGet another win,\n[f000]Ā\u0001\u0000![f000]븁\u0000"
    InfoMsg Global10665_Text_GoAllOutGet, 1
    VMJump L_17EA

L_17EA:
    InfoMsgClose_0039
    VMReturn

L_17EE:
    WbtCmd_GetTournament 0x8029
    WorkCmpConst 0x8029, 1
    VMJumpIf CMP_EQ, L_1805
    VMJump L_1815

L_1805:
    SEPlay SEQ_SE_SW_WBT_13
    WorkSetConst 0x802a, 20
    VMJump L_1C0C

L_1815:
    WorkCmpConst 0x8029, 2
    VMJumpIf CMP_EQ, L_1828
    VMJump L_1A45

L_1828:
    WbtCmd_GetType 0x802b
    SEPlay SEQ_SE_SW_WBT_04
    WorkCmpConst 0x802b, 0
    VMJumpIf CMP_EQ, L_1843
    VMJump L_184F

L_1843:
    WorkSetConst 0x802a, 0
    VMJump L_1A3F

L_184F:
    WorkCmpConst 0x802b, 9
    VMJumpIf CMP_EQ, L_1862
    VMJump L_186E

L_1862:
    WorkSetConst 0x802a, 1
    VMJump L_1A3F

L_186E:
    WorkCmpConst 0x802b, 10
    VMJumpIf CMP_EQ, L_1881
    VMJump L_188D

L_1881:
    WorkSetConst 0x802a, 2
    VMJump L_1A3F

L_188D:
    WorkCmpConst 0x802b, 11
    VMJumpIf CMP_EQ, L_18A0
    VMJump L_18AC

L_18A0:
    WorkSetConst 0x802a, 4
    VMJump L_1A3F

L_18AC:
    WorkCmpConst 0x802b, 12
    VMJumpIf CMP_EQ, L_18BF
    VMJump L_18CB

L_18BF:
    WorkSetConst 0x802a, 3
    VMJump L_1A3F

L_18CB:
    WorkCmpConst 0x802b, 14
    VMJumpIf CMP_EQ, L_18DE
    VMJump L_18EA

L_18DE:
    WorkSetConst 0x802a, 14
    VMJump L_1A3F

L_18EA:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_18FD
    VMJump L_1909

L_18FD:
    WorkSetConst 0x802a, 6
    VMJump L_1A3F

L_1909:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_191C
    VMJump L_1928

L_191C:
    WorkSetConst 0x802a, 7
    VMJump L_1A3F

L_1928:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_193B
    VMJump L_1947

L_193B:
    WorkSetConst 0x802a, 8
    VMJump L_1A3F

L_1947:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_195A
    VMJump L_1966

L_195A:
    WorkSetConst 0x802a, 9
    VMJump L_1A3F

L_1966:
    WorkCmpConst 0x802b, 13
    VMJumpIf CMP_EQ, L_1979
    VMJump L_1985

L_1979:
    WorkSetConst 0x802a, 5
    VMJump L_1A3F

L_1985:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_1998
    VMJump L_19A4

L_1998:
    WorkSetConst 0x802a, 11
    VMJump L_1A3F

L_19A4:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_19B7
    VMJump L_19C3

L_19B7:
    WorkSetConst 0x802a, 12
    VMJump L_1A3F

L_19C3:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_19D6
    VMJump L_19E2

L_19D6:
    WorkSetConst 0x802a, 13
    VMJump L_1A3F

L_19E2:
    WorkCmpConst 0x802b, 15
    VMJumpIf CMP_EQ, L_19F5
    VMJump L_1A01

L_19F5:
    WorkSetConst 0x802a, 10
    VMJump L_1A3F

L_1A01:
    WorkCmpConst 0x802b, 16
    VMJumpIf CMP_EQ, L_1A14
    VMJump L_1A20

L_1A14:
    WorkSetConst 0x802a, 16
    VMJump L_1A3F

L_1A20:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_1A33
    VMJump L_1A3F

L_1A33:
    WorkSetConst 0x802a, 15
    VMJump L_1A3F

L_1A3F:
    VMJump L_1C0C

L_1A45:
    WorkCmpConst 0x8029, 3
    VMJumpIf CMP_EQ, L_1A58
    VMJump L_1A68

L_1A58:
    SEPlay SEQ_SE_SW_WBT_11
    WorkSetConst 0x802a, 18
    VMJump L_1C0C

L_1A68:
    WorkCmpConst 0x8029, 4
    VMJumpIf CMP_EQ, L_1A7B
    VMJump L_1A8B

L_1A7B:
    SEPlay SEQ_SE_SW_WBT_10
    WorkSetConst 0x802a, 17
    VMJump L_1C0C

L_1A8B:
    WorkCmpConst 0x8029, 11
    VMJumpIf CMP_EQ, L_1A9E
    VMJump L_1AAE

L_1A9E:
    SEPlay SEQ_SE_SW_WBT_10
    WorkSetConst 0x802a, 17
    VMJump L_1C0C

L_1AAE:
    WorkCmpConst 0x8029, 12
    VMJumpIf CMP_EQ, L_1AC1
    VMJump L_1AD1

L_1AC1:
    SEPlay SEQ_SE_SW_WBT_10
    WorkSetConst 0x802a, 17
    VMJump L_1C0C

L_1AD1:
    WorkCmpConst 0x8029, 14
    VMJumpIf CMP_EQ, L_1AE4
    VMJump L_1AF4

L_1AE4:
    SEPlay SEQ_SE_SW_WBT_10
    WorkSetConst 0x802a, 17
    VMJump L_1C0C

L_1AF4:
    WorkCmpConst 0x8029, 5
    VMJumpIf CMP_EQ, L_1B07
    VMJump L_1B17

L_1B07:
    SEPlay SEQ_SE_SW_WBT_11
    WorkSetConst 0x802a, 18
    VMJump L_1C0C

L_1B17:
    WorkCmpConst 0x8029, 6
    VMJumpIf CMP_EQ, L_1B2A
    VMJump L_1B3A

L_1B2A:
    SEPlay SEQ_SE_SW_WBT_11
    WorkSetConst 0x802a, 18
    VMJump L_1C0C

L_1B3A:
    WorkCmpConst 0x8029, 7
    VMJumpIf CMP_EQ, L_1B4D
    VMJump L_1B5D

L_1B4D:
    SEPlay SEQ_SE_SW_WBT_11
    WorkSetConst 0x802a, 18
    VMJump L_1C0C

L_1B5D:
    WorkCmpConst 0x8029, 8
    VMJumpIf CMP_EQ, L_1B70
    VMJump L_1B80

L_1B70:
    SEPlay SEQ_SE_SW_WBT_11
    WorkSetConst 0x802a, 18
    VMJump L_1C0C

L_1B80:
    WorkCmpConst 0x8029, 9
    VMJumpIf CMP_EQ, L_1B93
    VMJump L_1BA3

L_1B93:
    SEPlay SEQ_SE_SW_WBT_11
    WorkSetConst 0x802a, 18
    VMJump L_1C0C

L_1BA3:
    WorkCmpConst 0x8029, 10
    VMJumpIf CMP_EQ, L_1BB6
    VMJump L_1BC6

L_1BB6:
    SEPlay SEQ_SE_SW_WBT_12
    WorkSetConst 0x802a, 19
    VMJump L_1C0C

L_1BC6:
    WorkCmpConst 0x8029, 13
    VMJumpIf CMP_EQ, L_1BD9
    VMJump L_1BE9

L_1BD9:
    SEPlay SEQ_SE_SW_WBT_12
    WorkSetConst 0x802a, 19
    VMJump L_1C0C

L_1BE9:
    WorkCmpConst 0x8029, 15
    VMJumpIf CMP_EQ, L_1BFC
    VMJump L_1C0C

L_1BFC:
    SEPlay SEQ_SE_SW_WBT_12
    WorkSetConst 0x802a, 19
    VMJump L_1C0C

L_1C0C:
    VMReturn
    .balign 4, 0
