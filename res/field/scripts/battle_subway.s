#include "asm/field_script.inc"
#include "text/script/battle_subway.h"

// Script plugin 1, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    WorkSetConst 0x8021, 0
    VMStackPush EVENT_WORK_0x4178
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00AF
    BSubwayCmd_Tool 13, 255, 1, 0
    BSubwayCmd_Tool 311, 0, 0, 0
    BSubwayCmd_Tool 310, 0, 0, 32801
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_007A
    VMCall L_0152
    VMJump L_00AF

L_007A:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00A9
    VMCall L_0209
    VMJump L_00AF

L_00A9:
    VMCall L_00B7

L_00AF:
    WorkSetConst 0x8021, 0
    VMHalt

L_00B7:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    BSubwayCmd_Tool 307, 0, 0, 0
    BSubwayCmd_Tool 8, 0, 0, 32803
    VMCall L_1A18
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0126
    BSubwayCmd_Tool 311, 1, 0, 0
    BSubwayCmd_Tool 351, 0, 0, 32776
    VMCall L_02B7
    WorkGet EVENT_WORK_0x4022, 0x8010
    VMJump L_0136

L_0126:
    BSubwayCmd_Tool 308, 0, 0, 32784
    WorkGet EVENT_WORK_0x4022, 0x8010

L_0136:
    ActorNew 9, 4, 2, 226, 164, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    VMReturn

L_0152:
    WorkSetConst 0x8024, 0
    BSubwayCmd_Tool 307, 0, 0, 0
    BSubwayCmd_Tool 8, 0, 0, 32804
    VMCall L_1A18
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01D1
    BSubwayCmd_Tool 311, 1, 0, 0
    BSubwayCmd_Tool 351, 0, 0, 32776
    VMCall L_02B7
    WorkGet EVENT_WORK_0x4022, 0x8010
    BSubwayCmd_Tool 351, 1, 0, 32776
    VMCall L_02B7
    WorkGet EVENT_WORK_0x4023, 0x8010
    VMJump L_01E5

L_01D1:
    BSubwayCmd_Tool 308, 0, 0, 16418
    BSubwayCmd_Tool 308, 1, 0, 16419

L_01E5:
    ActorNew 9, 5, 2, 226, 164, 0
    ActorNew 9, 3, 2, 227, 165, 0
    WorkSetConst 0x8024, 0
    VMReturn

L_0209:
    BSubwayCmd_Tool 307, 0, 0, 0
    BSubwayCmd_Tool 351, 0, 0, 32776
    VMCall L_02B7
    VMStackPush 0x8010
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0246
    BSubwayCmd_Tool 308, 0, 0, 32784
    VMJump L_0250

L_0246:
    BSubwayCmd_Tool 311, 1, 0, 0

L_0250:
    WorkGet EVENT_WORK_0x4022, 0x8010
    BSubwayCmd_Tool 351, 1, 0, 32776
    VMCall L_02B7
    VMStackPush 0x8010
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0289
    BSubwayCmd_Tool 308, 1, 0, 32784
    VMJump L_0293

L_0289:
    BSubwayCmd_Tool 311, 1, 0, 0

L_0293:
    WorkGet EVENT_WORK_0x4023, 0x8010
    ActorNew 9, 5, 2, 226, 164, 0
    ActorNew 9, 3, 2, 227, 165, 0
    VMReturn

L_02B7:
    WorkCmpConst 0x8008, 306
    VMJumpIf CMP_EQ, L_02CA
    VMJump L_02D6

L_02CA:
    WorkSetConst 0x8010, 207
    VMJump L_03B5

L_02D6:
    WorkCmpConst 0x8008, 307
    VMJumpIf CMP_EQ, L_02E9
    VMJump L_02F5

L_02E9:
    WorkSetConst 0x8010, 207
    VMJump L_03B5

L_02F5:
    WorkCmpConst 0x8008, 308
    VMJumpIf CMP_EQ, L_0308
    VMJump L_0314

L_0308:
    WorkSetConst 0x8010, 107
    VMJump L_03B5

L_0314:
    WorkCmpConst 0x8008, 309
    VMJumpIf CMP_EQ, L_0327
    VMJump L_0333

L_0327:
    WorkSetConst 0x8010, 107
    VMJump L_03B5

L_0333:
    WorkCmpConst 0x8008, 310
    VMJumpIf CMP_EQ, L_0346
    VMJump L_0352

L_0346:
    WorkSetConst 0x8010, 207
    VMJump L_03B5

L_0352:
    WorkCmpConst 0x8008, 311
    VMJumpIf CMP_EQ, L_0365
    VMJump L_0371

L_0365:
    WorkSetConst 0x8010, 107
    VMJump L_03B5

L_0371:
    WorkCmpConst 0x8008, 312
    VMJumpIf CMP_EQ, L_0384
    VMJump L_0390

L_0384:
    WorkSetConst 0x8010, 207
    VMJump L_03B5

L_0390:
    WorkCmpConst 0x8008, 313
    VMJumpIf CMP_EQ, L_03A3
    VMJump L_03AF

L_03A3:
    WorkSetConst 0x8010, 107
    VMJump L_03B5

L_03AF:
    WorkSetConst 0x8010, 10

L_03B5:
    VMReturn

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst EVENT_WORK_0x4178, 0
    BSubwayCmd_Tool 310, 0, 0, 32805
    BSubwayCmd_Tool 22, 0x8025, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0481
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_044B
    BSubwayCmd_Tool 411, 0, 0, 16416
    ActorNew 4, 3, 3, 224, 162, 0
    BSubwayCmd_Tool 412, 0, 0, 16417
    ActorNew 4, 5, 3, 225, 163, 0
    VMJump L_047B

L_044B:
    BSubwayCmd_Tool 12, 0, 0, 16416
    ActorNew 4, 3, 3, 224, 162, 0
    BSubwayCmd_Tool 31, 0, 0, 16417
    ActorNew 4, 5, 3, 225, 163, 0

L_047B:
    VMJump L_0499

L_0481:
    BSubwayCmd_Tool 12, 0, 0, 16416
    ActorNew 4, 4, 3, 224, 162, 0

L_0499:
    BSubwayCmd_Tool 334, 0, 0, 0
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C6
    VMCall L_04E6
    VMJump L_04CC

L_04C6:
    VMCall L_04E4

L_04CC:
    VMCall L_0684
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_04E4:
    VMReturn

L_04E6:
    VMReturn

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst EVENT_WORK_0x4178, 0
    BSubwayCmd_Tool 334, 0, 0, 0
    FadeInBlackQ
    FadeWait
    BSubwayCmd_Tool 310, 0, 0, 32807
    BSubwayCmd_Tool 22, 0x8027, 0, 32808
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_053D
    VMCall L_058C
    VMJump L_0543

L_053D:
    VMCall L_055B

L_0543:
    VMCall L_0684
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_055B:
    BSubwayCmd_Tool 12, 0, 0, 16416
    ActorNew 0, 4, 3, 224, 162, 0
    ActorCmdExec 224, Movement_0580
    ActorCmdWait
    VMReturn
    .balign 4, 0

Movement_0580:
    Move 62, 1
    Move 15, 4
    MoveEnd

L_058C:
    WorkSetConst 0x8029, 0
    BSubwayCmd_Tool 310, 0, 0, 32809
    VMStackPush 0x8029
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8029
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_05FF
    BSubwayCmd_Tool 411, 0, 0, 16416
    ActorNew 0, 4, 3, 224, 162, 0
    ActorCmdExec 224, Movement_0654
    ActorCmdWait
    BSubwayCmd_Tool 412, 0, 0, 16417
    ActorNew 0, 4, 3, 225, 163, 0
    VMJump L_0639

L_05FF:
    BSubwayCmd_Tool 12, 0, 0, 16416
    ActorNew 0, 4, 3, 224, 162, 0
    ActorCmdExec 224, Movement_0654
    ActorCmdWait
    BSubwayCmd_Tool 31, 0, 0, 16417
    ActorNew 0, 4, 3, 225, 163, 0

L_0639:
    ActorCmdExec 224, Movement_0660
    ActorCmdExec 225, Movement_0670
    ActorCmdWait
    WorkSetConst 0x8029, 0
    VMReturn
    .balign 4, 0

Movement_0654:
    Move 62, 1
    Move 15, 1
    MoveEnd

Movement_0660:
    Move 15, 3
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_0670:
    Move 62, 1
    Move 15, 4
    Move 13, 1
    Move 35, 1
    MoveEnd

L_0684:
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    BSubwayCmd_Tool 202, 0, 0, 0
    BSubwayCmd_Tool 310, 0, 0, 32811
    BSubwayCmd_Tool 200, 4, 0, 32810
    VMStackPush 0x802a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07FE
    WorkCmpConst 0x802b, 0
    VMJumpIf CMP_EQ, L_06DA
    VMJump L_06E6

L_06DA:
    VMCall L_1B6F
    VMJump L_07DE

L_06E6:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_06F9
    VMJump L_0705

L_06F9:
    VMCall L_1B6F
    VMJump L_07DE

L_0705:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_0718
    VMJump L_0724

L_0718:
    VMCall L_1B6F
    VMJump L_07DE

L_0724:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_0737
    VMJump L_0743

L_0737:
    VMCall L_1B6F
    VMJump L_07DE

L_0743:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_0756
    VMJump L_0762

L_0756:
    VMCall L_1B83
    VMJump L_07DE

L_0762:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_0775
    VMJump L_0781

L_0775:
    VMCall L_1B83
    VMJump L_07DE

L_0781:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_0794
    VMJump L_07A0

L_0794:
    VMCall L_1B83
    VMJump L_07DE

L_07A0:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_07B3
    VMJump L_07BF

L_07B3:
    VMCall L_1B83
    VMJump L_07DE

L_07BF:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_07D2
    VMJump L_07DE

L_07D2:
    VMCall L_1B6F
    VMJump L_07DE

L_07DE:
    FadeEx 3, 0, 16, 2
    FadeExWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMCall L_098F
    VMReturn

L_07FE:
    BSubwayCmd_Tool 343, 0, 0, 0
    BSubwayCmd_Tool 354, 0, 0, 0
    WorkCmpConst 0x802b, 0
    VMJumpIf CMP_EQ, L_0825
    VMJump L_0831

L_0825:
    VMCall L_13B8
    VMJump L_0929

L_0831:
    WorkCmpConst 0x802b, 5
    VMJumpIf CMP_EQ, L_0844
    VMJump L_0850

L_0844:
    VMCall L_13B8
    VMJump L_0929

L_0850:
    WorkCmpConst 0x802b, 1
    VMJumpIf CMP_EQ, L_0863
    VMJump L_086F

L_0863:
    VMCall L_13B8
    VMJump L_0929

L_086F:
    WorkCmpConst 0x802b, 6
    VMJumpIf CMP_EQ, L_0882
    VMJump L_088E

L_0882:
    VMCall L_13B8
    VMJump L_0929

L_088E:
    WorkCmpConst 0x802b, 2
    VMJumpIf CMP_EQ, L_08A1
    VMJump L_08AD

L_08A1:
    VMCall L_13F2
    VMJump L_0929

L_08AD:
    WorkCmpConst 0x802b, 7
    VMJumpIf CMP_EQ, L_08C0
    VMJump L_08CC

L_08C0:
    VMCall L_13F2
    VMJump L_0929

L_08CC:
    WorkCmpConst 0x802b, 3
    VMJumpIf CMP_EQ, L_08DF
    VMJump L_08EB

L_08DF:
    VMCall L_142C
    VMJump L_0929

L_08EB:
    WorkCmpConst 0x802b, 8
    VMJumpIf CMP_EQ, L_08FE
    VMJump L_090A

L_08FE:
    VMCall L_142C
    VMJump L_0929

L_090A:
    WorkCmpConst 0x802b, 4
    VMJumpIf CMP_EQ, L_091D
    VMJump L_0929

L_091D:
    VMCall L_13B8
    VMJump L_0929

L_0929:
    WorkGet 0x802c, 0x8010
    WorkCmpConst 0x802c, 1
    VMJumpIf CMP_EQ, L_0942
    VMJump L_094E

L_0942:
    VMCall L_098F
    VMJump L_097B

L_094E:
    WorkCmpConst 0x802c, 2
    VMJumpIf CMP_EQ, L_0961
    VMJump L_0971

L_0961:
    VMSleep 40
    VMCall L_1916
    VMJump L_097B

L_0971:
    VMSleep 40
    VMCall L_162B

L_097B:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    VMReturn

L_098F:
    WorkSetConst 0x802d, 0
    BSubwayCmd_Tool 310, 0, 0, 32813
    BSubwayCmd_Tool 8, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_09C4
    Cmd_01DD 7, 0x8010, 0

L_09C4:
    BSubwayCmd_Tool 357, 0, 0, 32784
    BSubwayCmd_Tool 303, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09F7
    VMCall L_0A05
    VMJump L_09FD

L_09F7:
    VMCall L_0CA0

L_09FD:
    WorkSetConst 0x802d, 0
    VMReturn

L_0A05:
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    BSubwayCmd_Tool 310, 0, 0, 32814
    VMStackPush 0x802e
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x802e
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0A65
    BSubwayCmd_Tool 416, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A65
    BSubwayCmd_Tool 354, 1, 0, 0

L_0A65:
    VMSleep 40
    BSubwayCmd_Tool 355, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AAB
    // "Would you like to save the last battle as\nyour Battle Video?"
    SystemMsg BattleSubway_Text_WouldLikeSaveLast, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AAB
    VMCall L_19EE

L_0AAB:
    BSubwayCmd_Tool 305, 0, 0, 0
    BSubwayCmd_Tool 22, 0x802e, 0, 32815
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ADE
    VMCall L_0C76
    VMJump L_0AE4

L_0ADE:
    VMCall L_0C5C

L_0AE4:
    BSubwayCmd_Tool 350, 0, 0, 0
    VMStackPush 0x802f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B0B
    VMSleep 120
    VMJump L_0B0F

L_0B0B:
    VMSleep 100

L_0B0F:
    FadeOutBlackQ
    FadeWait
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x4179, 1
    BSubwayCmd_Tool 347, 0, 0, 0
    BSubwayCmd_Tool 202, 1, 0, 0
    MapChangeCore ZONE_BATTLE_SUBWAY_2, 75, 0, 12, 1
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    VMReturn
    .balign 4, 0

Movement_0B4C:
    Move 62, 1
    Move 12, 2
    Move 33, 1
    MoveEnd

Movement_0B5C:
    Move 62, 1
    Move 12, 2
    Move 14, 1
    Move 12, 1
    Move 33, 1
    MoveEnd

Movement_0B74:
    Move 62, 1
    Move 12, 1
    Move 33, 1
    MoveEnd

Movement_0B84:
    Move 63, 3
    Move 15, 8
    Move 12, 2
    MoveEnd

Movement_0B94:
    Move 62, 1
    Move 63, 3
    Move 15, 6
    Move 12, 2
    MoveEnd

Movement_0BA8:
    Move 63, 4
    Move 15, 8
    Move 12, 1
    MoveEnd

Movement_0BB8:
    Move 63, 4
    Move 15, 7
    Move 12, 3
    MoveEnd

Movement_0BC8:
    Move 62, 1
    Move 63, 4
    Move 15, 6
    Move 12, 2
    MoveEnd

Movement_0BDC:
    Move 63, 3
    Move 15, 6
    Move 13, 4
    MoveEnd

Movement_0BEC:
    Move 62, 1
    Move 63, 3
    Move 79, 4
    MoveEnd

Movement_0BFC:
    Move 63, 4
    Move 15, 6
    Move 13, 5
    MoveEnd

Movement_0C0C:
    Move 62, 1
    Move 63, 4
    Move 79, 4
    MoveEnd

Movement_0C1C:
    Move 63, 4
    Move 15, 5
    Move 63, 3
    Move 15, 1
    Move 13, 2
    MoveEnd

Movement_0C34:
    Move 13, 2
    Move 15, 1
    Move 31, 1
    MoveEnd

Movement_0C44:
    Move 13, 2
    Move 15, 1
    Move 31, 1
    MoveEnd

Movement_0C54:
    Move 13, 2
    MoveEnd

L_0C5C:
    ActorCmdExec 226, Movement_0B4C
    ActorCmdExec 224, Movement_0BDC
    ActorCmdExec 255, Movement_0BEC
    VMReturn

L_0C76:
    ActorCmdExec 226, Movement_0B5C
    ActorCmdExec 227, Movement_0B74
    ActorCmdExec 224, Movement_0BFC
    ActorCmdExec 225, Movement_0C1C
    ActorCmdExec 255, Movement_0C0C
    VMReturn

L_0CA0:
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    BSubwayCmd_Tool 310, 0, 0, 32816
    BSubwayCmd_Tool 22, 0x8030, 0, 32817
    VMStackPush 0x8031
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CDF
    VMCall L_135C
    VMJump L_0CE5

L_0CDF:
    VMCall L_1340

L_0CE5:
    MEPlay SEQ_ME_ASA
    MEWait
    VMStackPush 0x8030
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8030
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0D1A
    VMCall L_0FBB
    VMJump L_0D20

L_0D1A:
    VMCall L_0D2E

L_0D20:
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8030, 0
    VMReturn

L_0D2E:
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    BSubwayCmd_Tool 310, 0, 0, 32820
    BSubwayCmd_Tool 22, 0x8034, 0, 32821
    WorkSetConst 0x8033, 1

L_0D66:
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F9B
    BSubwayCmd_Tool 358, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 9999
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0D9C
    WorkSetConst 0x8010, 9999

L_0D9C:
    WordSetNumber 0, 0x8010, 4
    BSubwayCmd_Tool 11, 0, 0, 32784
    WordSetNumber 1, 0x8010, 1
    // "Current winning streak: [f000]ȃ\u0001\u0000![f000]븁\u0000\nNext car: No. [f000]Ȁ\u0001\u0001.\nContinue to battle?"
    SystemMsg BattleSubway_Text_CurrentWinningStreakNext, 2
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    BSubwayCmd_Tool 355, 0, 0, 32822
    VMStackPush 0x8036
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E06
    ListMenuAdd 1, 65535, 0
    ListMenuAdd 2, 65535, 1
    ListMenuAdd 3, 65535, 2
    ListMenuAdd 4, 65535, 3
    VMJump L_0E1E

L_0E06:
    ListMenuAdd 1, 65535, 0
    ListMenuAdd 3, 65535, 2
    ListMenuAdd 4, 65535, 3

L_0E1E:
    ListMenuShow
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0E33
    VMJump L_0E9A

L_0E33:
    InfoMsgClose
    VMStackPush 0x8035
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E54
    VMCall L_139C
    VMJump L_0E5A

L_0E54:
    VMCall L_1388

L_0E5A:
    BSubwayCmd_Tool 350, 0, 0, 0
    FadeOutBlackQ
    FadeWait
    WorkSetConst EVENT_WORK_0x4178, 2
    BSubwayCmd_Tool 347, 0, 0, 0
    BSubwayCmd_Tool 202, 1, 0, 0
    MapChangeCore ZONE_BATTLE_SUBWAY, 7, 0, 4, 0
    WorkSetConst 0x8033, 0
    VMJump L_0F95

L_0E9A:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0EAD
    VMJump L_0EB9

L_0EAD:
    VMCall L_19EE
    VMJump L_0F95

L_0EB9:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0ECC
    VMJump L_0F4B

L_0ECC:
    // "Save and quit the game?"
    SystemMsg BattleSubway_Text_SaveQuitGame, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F45
    WorkSetConst EVENT_WORK_0x4176, 1
    BSubwayCmd_Tool 322, 0, 0, 0
    BSubwayCmd_Tool 306, 0, 0, 0
    BSubwayCmd_Tool 347, 0, 0, 0
    BSubwayCmd_Tool 202, 1, 0, 0
    Plugin1_Cmd1002
    // "Saving...\nDon't turn off the power."
    SystemMsg BattleSubway_Text_SavingDontTurnOff, 2
    SaveDataWrite 0x8010
    FadeEx 3, 0, 16, 2
    FadeExWait
    InfoMsgClose
    GameCommDisconnect 0x8010
    FieldSubscreenDisable
    VMSleep 8
    BSubwayCmd_Tool 1, 0, 0, 0

L_0F45:
    VMJump L_0F95

L_0F4B:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0F5E
    VMJump L_0F95

L_0F5E:
    // "Cancel your challenge?"
    SystemMsg BattleSubway_Text_CancelChallenge, 2
    BSubwayCmd_Tool 44, 1, 0, 32784
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F8F
    VMCall L_1890
    WorkSetConst 0x8033, 0

L_0F8F:
    VMJump L_0F95

L_0F95:
    VMJump L_0D66

L_0F9B:
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8032, 0
    VMReturn

L_0FBB:
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8039, 0
    BSubwayCmd_Tool 402, 51, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FF2
    VMCall L_1916
    VMReturn

L_0FF2:
    BSubwayCmd_Tool 416, 0, 0, 32823
    VMStackPush 0x8037
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1019
    BSubwayCmd_Tool 354, 1, 0, 0

L_1019:
    WorkSetConst 0x8038, 1

L_101F:
    VMStackPush 0x8038
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_132C
    BSubwayCmd_Tool 358, 0, 0, 32784
    WordSetNumber 0, 0x8010, 4
    BSubwayCmd_Tool 11, 0, 0, 32784
    WordSetNumber 1, 0x8010, 1
    // "Current winning streak: [f000]ȃ\u0001\u0000![f000]븁\u0000\nNext car: No. [f000]Ȁ\u0001\u0001.\nContinue to battle?"
    SystemMsg BattleSubway_Text_CurrentWinningStreakNext, 2
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    BSubwayCmd_Tool 355, 0, 0, 32825
    VMStackPush 0x8039
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_109E
    ListMenuAdd 1, 65535, 0
    ListMenuAdd 2, 65535, 1
    ListMenuAdd 4, 65535, 3
    VMJump L_10AE

L_109E:
    ListMenuAdd 1, 65535, 0
    ListMenuAdd 4, 65535, 3

L_10AE:
    ListMenuShow
    WorkSetConst EVENT_WORK_0x4000, 0
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_10C9
    VMJump L_1202

L_10C9:
    // "Awaiting your friend's selection."
    SystemMsg BattleSubway_Text_AwaitingFriendsSelection, 2
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 52, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1100
    InfoMsgClose
    VMCall L_1916
    VMReturn

L_1100:
    WorkSetConst EVENT_WORK_0x4000, 0
    BSubwayCmd_Tool 405, 2, EVENT_WORK_0x4000, 0
    BSubwayCmd_Tool 415, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1137
    InfoMsgClose
    VMCall L_1916
    VMReturn

L_1137:
    BSubwayCmd_Tool 406, 2, 0, 16384
    InfoMsgClose
    BSubwayCmd_Tool 415, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1168
    VMCall L_1916
    VMReturn

L_1168:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11BC
    // "You have chosen to retire from\nthis challenge."
    SystemMsg BattleSubway_Text_HaveChosenRetireFrom, 2
    VMSleep 30
    BSubwayCmd_Tool 402, 54, 0, 32800
    InfoMsgClose
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11B0
    VMCall L_1916
    VMJump L_11B6

L_11B0:
    VMCall L_1890

L_11B6:
    VMJump L_11F6

L_11BC:
    VMCall L_139C
    BSubwayCmd_Tool 350, 0, 0, 0
    FadeOutBlackQ
    FadeWait
    BSubwayCmd_Tool 347, 0, 0, 0
    BSubwayCmd_Tool 202, 1, 0, 0
    WorkSetConst EVENT_WORK_0x4178, 2
    MapChangeCore ZONE_BATTLE_SUBWAY, 7, 0, 4, 0

L_11F6:
    WorkSetConst 0x8038, 0
    VMJump L_1326

L_1202:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_1215
    VMJump L_1221

L_1215:
    VMCall L_19EE
    VMJump L_1326

L_1221:
    // "Cancel your challenge?"
    SystemMsg BattleSubway_Text_CancelChallenge, 2
    BSubwayCmd_Tool 44, 1, 0, 32784
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1326
    // "Awaiting your friend's selection."
    SystemMsg BattleSubway_Text_AwaitingFriendsSelection, 2
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 52, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_127D
    InfoMsgClose
    VMCall L_1916
    VMReturn

L_127D:
    WorkSetConst EVENT_WORK_0x4000, 1
    BSubwayCmd_Tool 405, 2, EVENT_WORK_0x4000, 0
    BSubwayCmd_Tool 415, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12B4
    InfoMsgClose
    VMCall L_1916
    VMReturn

L_12B4:
    BSubwayCmd_Tool 406, 2, 0, 16384
    BSubwayCmd_Tool 415, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12E5
    InfoMsgClose
    VMCall L_1916
    VMReturn

L_12E5:
    // "You have chosen to retire from\nthis challenge."
    SystemMsg BattleSubway_Text_HaveChosenRetireFrom, 2
    VMSleep 30
    BSubwayCmd_Tool 402, 54, 0, 32800
    InfoMsgClose
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_131A
    VMCall L_1916
    VMJump L_1320

L_131A:
    VMCall L_1890

L_1320:
    WorkSetConst 0x8038, 0

L_1326:
    VMJump L_101F

L_132C:
    WorkSetConst 0x8039, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8037, 0
    VMReturn

L_1340:
    ActorCmdExec 226, Movement_0B4C
    ActorCmdExec 224, Movement_0B84
    ActorCmdExec 255, Movement_0B94
    ActorCmdWait
    VMReturn

L_135C:
    ActorCmdExec 226, Movement_0B5C
    ActorCmdExec 227, Movement_0B74
    ActorCmdExec 224, Movement_0BA8
    ActorCmdExec 225, Movement_0BB8
    ActorCmdExec 255, Movement_0BC8
    ActorCmdWait
    VMReturn

L_1388:
    ActorCmdExec 224, Movement_0C34
    ActorCmdExec 255, Movement_0C54
    ActorCmdWait
    VMReturn

L_139C:
    ActorCmdExec 224, Movement_0C34
    ActorCmdExec 225, Movement_0C44
    ActorCmdExec 255, Movement_0C54
    ActorCmdWait
    VMReturn

L_13B8:
    BSubwayCmd_Tool 352, 0, 226, 0
    VMCall L_1B6F
    BSubwayCmd_Tool 309, 0, 0, 0
    BSubwayCmd_Tool 353, 0, 0, 0
    BSubwayCmd_Tool 345, 0, 0, 0
    BSubwayCmd_Tool 332, 0, 0, 32784
    VMReturn

L_13F2:
    BSubwayCmd_Tool 320, 1, 227, 0
    BSubwayCmd_Tool 352, 0, 226, 0
    VMCall L_1B83
    BSubwayCmd_Tool 309, 0, 0, 0
    BSubwayCmd_Tool 345, 0, 0, 0
    BSubwayCmd_Tool 332, 0, 0, 32784
    VMReturn

L_142C:
    WorkSetConst 0x803a, 0
    WorkSetConst 0x803b, 0
    BSubwayCmd_Tool 320, 1, 227, 0
    BSubwayCmd_Tool 352, 0, 226, 0
    // "Communicating. Please stand by..."
    SystemMsgAsync BattleSubway_Text_CommunicatingPleaseStandBy, 2
    VMSleep 15
    BSubwayCmd_Tool 402, 53, 0, 32800
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_147D
    WorkSetConst 0x8010, 2
    VMReturn

L_147D:
    VMCall L_1B83
    BSubwayCmd_Tool 402, 50, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14A8
    WorkSetConst 0x8010, 2
    VMReturn

L_14A8:
    BSubwayCmd_Tool 309, 0, 0, 0
    BSubwayCmd_Tool 415, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14D7
    WorkSetConst 0x8010, 2
    VMReturn

L_14D7:
    BSubwayCmd_Tool 345, 0, 0, 0
    BSubwayCmd_Tool 332, 0, 0, 32826
    BSubwayCmd_Tool 203, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_15BE
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 54, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1537
    WorkSetConst 0x8010, 2
    VMReturn

L_1537:
    BSubwayCmd_Tool 405, 7, 0x803a, 0
    BSubwayCmd_Tool 415, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1566
    WorkSetConst 0x8010, 2
    VMReturn

L_1566:
    BSubwayCmd_Tool 406, 7, 0, 32827
    BSubwayCmd_Tool 415, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1595
    WorkSetConst 0x8010, 2
    VMReturn

L_1595:
    VMStackPush 0x803a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x803b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_15BE
    WorkSetConst 0x803a, 1

L_15BE:
    WorkGet 0x8010, 0x803a
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803a, 0
    VMReturn

L_15D2:
    BSubwayCmd_Tool 401, 0, 0, 0
    Cmd_013C
    BSubwayCmd_Tool 330, 0, 0, 0
    VMReturn

L_15EA:
    BSubwayCmd_Tool 402, 52, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1611
    BSubwayCmd_Tool 413, 0, 0, 0

L_1611:
    VMCall L_15D2
    VMReturn

L_1619:
    BSubwayCmd_Tool 413, 0, 0, 0
    VMCall L_15D2
    VMReturn

L_162B:
    WorkSetConst 0x803c, 0
    WorkSetConst 0x803d, 0
    BSubwayCmd_Tool 310, 0, 0, 32829
    VMStackPush 0x803d
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x803d
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_168B
    BSubwayCmd_Tool 416, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_168B
    BSubwayCmd_Tool 354, 1, 0, 0

L_168B:
    BSubwayCmd_Tool 355, 0, 0, 32828
    VMStackPush 0x803c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_16CD
    // "Would you like to save the last battle as\nyour Battle Video?"
    SystemMsg BattleSubway_Text_WouldLikeSaveLast, 2
    YesNoWin 0x803c
    InfoMsgClose
    VMStackPush 0x803c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_16CD
    VMCall L_1A00

L_16CD:
    VMCall L_1890
    WorkSetConst 0x803d, 0
    WorkSetConst 0x803c, 0
    VMReturn

L_16E1:
    BSubwayCmd_Tool 350, 0, 0, 0
    FadeOutBlackQ
    FadeWait
    BSubwayCmd_Tool 347, 0, 0, 0
    BSubwayCmd_Tool 202, 1, 0, 0
    VMCall L_170B
    VMReturn

L_170B:
    WorkSetConst 0x803e, 0
    BSubwayCmd_Tool 202, 99, 0, 0
    BSubwayCmd_Tool 21, 0, 0, 32830
    WorkCmpConst 0x803e, 0
    VMJumpIf CMP_EQ, L_1738
    VMJump L_174A

L_1738:
    MapChangeCore ZONE_GEAR_STATION_2, 11, 0, 15, 3
    VMJump L_187E

L_174A:
    WorkCmpConst 0x803e, 5
    VMJumpIf CMP_EQ, L_175D
    VMJump L_176F

L_175D:
    MapChangeCore ZONE_GEAR_STATION_3, 11, 0, 15, 3
    VMJump L_187E

L_176F:
    WorkCmpConst 0x803e, 1
    VMJumpIf CMP_EQ, L_1782
    VMJump L_1794

L_1782:
    MapChangeCore ZONE_GEAR_STATION_4, 11, 0, 15, 3
    VMJump L_187E

L_1794:
    WorkCmpConst 0x803e, 6
    VMJumpIf CMP_EQ, L_17A7
    VMJump L_17B9

L_17A7:
    MapChangeCore ZONE_GEAR_STATION_5, 11, 0, 15, 3
    VMJump L_187E

L_17B9:
    WorkCmpConst 0x803e, 2
    VMJumpIf CMP_EQ, L_17CC
    VMJump L_17DE

L_17CC:
    MapChangeCore ZONE_GEAR_STATION_6, 11, 0, 15, 3
    VMJump L_187E

L_17DE:
    WorkCmpConst 0x803e, 3
    VMJumpIf CMP_EQ, L_17F1
    VMJump L_1803

L_17F1:
    MapChangeCore ZONE_GEAR_STATION_6, 11, 0, 15, 3
    VMJump L_187E

L_1803:
    WorkCmpConst 0x803e, 7
    VMJumpIf CMP_EQ, L_1816
    VMJump L_1828

L_1816:
    MapChangeCore ZONE_GEAR_STATION_7, 11, 0, 15, 3
    VMJump L_187E

L_1828:
    WorkCmpConst 0x803e, 8
    VMJumpIf CMP_EQ, L_183B
    VMJump L_184D

L_183B:
    MapChangeCore ZONE_GEAR_STATION_7, 11, 0, 15, 3
    VMJump L_187E

L_184D:
    WorkCmpConst 0x803e, 4
    VMJumpIf CMP_EQ, L_1860
    VMJump L_1872

L_1860:
    MapChangeCore ZONE_GEAR_STATION_8, 11, 0, 15, 3
    VMJump L_187E

L_1872:
    MapChangeCore ZONE_GEAR_STATION_2, 11, 0, 15, 3

L_187E:
    BSubwayCmd_Tool 202, 100, 0, 0
    WorkSetConst 0x803e, 0
    VMReturn

L_1890:
    WorkSetConst 0x803f, 0
    WorkSetConst EVENT_WORK_0x4176, 3
    BSubwayCmd_Tool 310, 0, 0, 32831
    VMStackPush 0x803f
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x803f
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_1904
    // "Communicating. Please stand by..."
    SystemMsg BattleSubway_Text_CommunicatingPleaseStandBy, 2
    VMSleep 30
    BSubwayCmd_Tool 402, 53, 0, 32800
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_18FE
    VMCall L_15EA
    VMJump L_1904

L_18FE:
    VMCall L_1619

L_1904:
    VMSleep 30
    VMCall L_16E1
    WorkSetConst 0x803f, 0
    VMReturn

L_1916:
    VMCall L_1619
    WorkSetConst EVENT_WORK_0x4176, 3
    VMCall L_16E1
    VMReturn

L_192A:
    WorkSetConst 0x8040, 0
    WorkSetConst 0x8041, 0
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8041, 1
    WorkGet 0x8042, 0x8010
    BSubwayCmd_Tool 344, 0, 0, 32832
    VMStackPush 0x8040
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1990
    // "Delete your existing Battle Video and\nsave the last battle?"
    SystemMsg BattleSubway_Text_DeleteExistingBattleVideo, 2
    BSubwayCmd_Tool 44, 1, 0, 32832
    InfoMsgClose
    VMStackPush 0x8040
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1990
    WorkSetConst 0x8041, 0

L_1990:
    VMStackPush 0x8041
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_19D4
    // "Saving your Battle Video...\nDon't turn off the power."
    SystemMsg BattleSubway_Text_SavingBattleVideoDont, 2
    VMSleep 1
    MsgSetLoadingSpinner 0
    BSubwayCmd_Tool 346, 0x8042, 0, 0
    InfoMsgClose
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000's battle has been saved as a\nBattle Video."
    SystemMsg BattleSubway_Text_SBattleHasBeen, 2
    ABKeyWait
    InfoMsgClose
    BSubwayCmd_Tool 354, 1, 0, 0

L_19D4:
    WorkGet 0x8010, 0x8041
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8041, 0
    WorkSetConst 0x8040, 0
    VMReturn

L_19EE:
    BSubwayCmd_Tool 358, 0, 0, 32784
    VMCall L_192A
    VMReturn

L_1A00:
    BSubwayCmd_Tool 358, 0, 0, 32784
    WorkAdd 0x8010, 1
    VMCall L_192A
    VMReturn

L_1A18:
    WorkSetConst 0x8043, 0
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8045, 0
    BSubwayCmd_Tool 310, 0, 0, 32836
    BSubwayCmd_Tool 30, 0x8044, 0, 32835
    WorkSetConst 0x8045, 255
    WorkCmpConst 0x8044, 0
    VMJumpIf CMP_EQ, L_1A57
    VMJump L_1A63

L_1A57:
    WorkSetConst 0x8045, 2
    VMJump L_1B3C

L_1A63:
    WorkCmpConst 0x8044, 1
    VMJumpIf CMP_EQ, L_1A76
    VMJump L_1A82

L_1A76:
    WorkSetConst 0x8045, 2
    VMJump L_1B3C

L_1A82:
    WorkCmpConst 0x8044, 2
    VMJumpIf CMP_EQ, L_1A95
    VMJump L_1AA1

L_1A95:
    WorkSetConst 0x8045, 2
    VMJump L_1B3C

L_1AA1:
    WorkCmpConst 0x8044, 3
    VMJumpIf CMP_EQ, L_1AB4
    VMJump L_1AC0

L_1AB4:
    WorkSetConst 0x8045, 2
    VMJump L_1B3C

L_1AC0:
    WorkCmpConst 0x8044, 5
    VMJumpIf CMP_EQ, L_1AD3
    VMJump L_1ADF

L_1AD3:
    WorkSetConst 0x8045, 6
    VMJump L_1B3C

L_1ADF:
    WorkCmpConst 0x8044, 6
    VMJumpIf CMP_EQ, L_1AF2
    VMJump L_1AFE

L_1AF2:
    WorkSetConst 0x8045, 6
    VMJump L_1B3C

L_1AFE:
    WorkCmpConst 0x8044, 7
    VMJumpIf CMP_EQ, L_1B11
    VMJump L_1B1D

L_1B11:
    WorkSetConst 0x8045, 6
    VMJump L_1B3C

L_1B1D:
    WorkCmpConst 0x8044, 8
    VMJumpIf CMP_EQ, L_1B30
    VMJump L_1B3C

L_1B30:
    WorkSetConst 0x8045, 6
    VMJump L_1B3C

L_1B3C:
    WorkSetConst 0x8010, 0
    VMStackPush 0x8043
    VMStackPush 0x8045
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1B5B
    WorkSetConst 0x8010, 1

L_1B5B:
    WorkSetConst 0x8045, 0
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8043, 0
    VMReturn

L_1B6F:
    ActorCmdExec 224, Movement_1BA8
    ActorCmdExec 226, Movement_1BB0
    ActorCmdWait
    VMReturn

L_1B83:
    ActorCmdExec 224, Movement_1BA8
    ActorCmdExec 225, Movement_1BA8
    ActorCmdExec 226, Movement_1BB0
    ActorCmdExec 227, Movement_1BB0
    ActorCmdWait
    VMReturn
    .balign 4, 0

Movement_1BA8:
    Move 15, 1
    MoveEnd

Movement_1BB0:
    Move 14, 1
    MoveEnd
