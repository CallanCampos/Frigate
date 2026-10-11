#include "asm/field_script.inc"
#include "text/script/global_10330.h"

// Script plugin 1, from the zones that start its scripts

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

Script_1:
    BSubwayCmd_Tool 0, 0, 0, 32800
    BSubwayCmd_Tool 18, 0x8020, 0, 0
    WorkCmpConst EVENT_WORK_0x4176, 1
    VMJumpIf CMP_EQ, L_009F
    VMJump L_01FE

L_009F:
    BSubwayCmd_Tool 201, 0, 0, 0
    BSubwayCmd_Tool 21, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_01F8
    BSubwayCmd_Tool 201, 1, 0, 0
    BSubwayCmd_Tool 3, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DC
    BSubwayCmd_Tool 201, 2, 0, 0
    VMCall L_2EE0
    VMCall L_2E47
    WorkGet 0x8021, 0x8010
    VMCall L_317D
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_0132
    VMJump L_0150

L_0132:
    WorkSetConst 0x8008, 11
    WorkSetConst 0x8009, 14
    WorkSetConst 0x800a, 3
    WorkSetConst 0x800b, 0
    VMJump L_01CA

L_0150:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_0163
    VMJump L_0181

L_0163:
    WorkSetConst 0x8008, 11
    WorkSetConst 0x8009, 16
    WorkSetConst 0x800a, 3
    WorkSetConst 0x800b, 1
    VMJump L_01CA

L_0181:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_0194
    VMJump L_01B2

L_0194:
    WorkSetConst 0x8008, 10
    WorkSetConst 0x8009, 15
    WorkSetConst 0x800a, 3
    WorkSetConst 0x800b, 2
    VMJump L_01CA

L_01B2:
    WorkSetConst 0x8008, 13
    WorkSetConst 0x8009, 16
    WorkSetConst 0x800a, 0
    WorkSetConst 0x800b, 2

L_01CA:
    ActorSetGPos 0x8021, 0x8008, 0, 0x8009, 0x800a
    VMJump L_01F2

L_01DC:
    WorkSetConst EVENT_WORK_0x4176, 4
    VMCall L_2F21
    BSubwayCmd_Tool 201, 3, 0, 0

L_01F2:
    VMCall L_32B0

L_01F8:
    VMJump L_0229

L_01FE:
    WorkCmpConst EVENT_WORK_0x4176, 4
    VMJumpIf CMP_EQ, L_0211
    VMJump L_0229

L_0211:
    WorkSetConst EVENT_WORK_0x4176, 4
    VMCall L_2F21
    VMCall L_32B0
    VMJump L_0229

L_0229:
    VMHalt

Script_2:
    BSubwayCmd_Tool 0, 0, 0, 32800
    BSubwayCmd_Tool 18, 0x8020, 0, 0
    WorkCmpConst EVENT_WORK_0x4176, 1
    VMJumpIf CMP_EQ, L_0252
    VMJump L_0399

L_0252:
    BSubwayCmd_Tool 201, 0, 0, 0
    BSubwayCmd_Tool 21, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0393
    BSubwayCmd_Tool 201, 1, 0, 0
    BSubwayCmd_Tool 3, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0377
    BSubwayCmd_Tool 201, 2, 0, 0
    VMCall L_2EE0
    VMCall L_2E47
    WorkGet 0x8021, 0x8010
    VMCall L_317D
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_02E5
    VMJump L_02FD

L_02E5:
    WorkSetConst 0x8008, 11
    WorkSetConst 0x8009, 14
    WorkSetConst 0x800a, 3
    VMJump L_0365

L_02FD:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_0310
    VMJump L_0328

L_0310:
    WorkSetConst 0x8008, 11
    WorkSetConst 0x8009, 16
    WorkSetConst 0x800a, 3
    VMJump L_0365

L_0328:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_033B
    VMJump L_0353

L_033B:
    WorkSetConst 0x8008, 10
    WorkSetConst 0x8009, 15
    WorkSetConst 0x800a, 3
    VMJump L_0365

L_0353:
    WorkSetConst 0x8008, 13
    WorkSetConst 0x8009, 16
    WorkSetConst 0x800a, 0

L_0365:
    ActorSetGPos 0x8021, 0x8008, 0, 0x8009, 0x800a
    VMJump L_038D

L_0377:
    WorkSetConst EVENT_WORK_0x4176, 4
    VMCall L_2F21
    BSubwayCmd_Tool 201, 3, 0, 0

L_038D:
    VMCall L_32B0

L_0393:
    VMJump L_03C4

L_0399:
    WorkCmpConst EVENT_WORK_0x4176, 4
    VMJumpIf CMP_EQ, L_03AC
    VMJump L_03C4

L_03AC:
    WorkSetConst EVENT_WORK_0x4176, 4
    VMCall L_2F21
    VMCall L_32B0
    VMJump L_03C4

L_03C4:
    VMHalt

Script_3:
    ActorsPauseAll
    VMCall L_1F5C
    VMCall L_03EE
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_03EE
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_03EE:
    Plugin1_Cmd1000
    BSubwayCmd_Tool 0, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0419
    VMCall L_0475
    VMJump L_041F

L_0419:
    VMCall L_0421

L_041F:
    VMReturn

L_0421:
    VMCall L_1919
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0442
    VMCall L_28A4
    VMReturn

L_0442:
    VMCall L_2A3C
    VMCall L_069D
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_046D
    VMCall L_07E4
    VMJump L_0473

L_046D:
    VMCall L_28A4

L_0473:
    VMReturn

L_0475:
    BSubwayCmd_Tool 101, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C5
    WorkSetConst 0x8008, 58
    VMCall L_3151
    YesNoWin 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04BF
    MsgWinCloseAll
    VMCall L_0585
    VMReturn

L_04BF:
    VMJump L_04E6

L_04C5:
    VMCall L_1919
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E6
    VMCall L_28A4
    VMReturn

L_04E6:
    WordSetPlayerName 0
    BSubwayCmd_Tool 27, 0, 0, 32802
    WordSetNumber 1, 0x8022, 2
    WorkSetConst 0x8008, 57
    VMCall L_3151
    VMCall L_0708
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_051F
    VMJump L_0535

L_051F:
    BSubwayCmd_Tool 108, 1, 0, 0
    VMCall L_07E4
    VMJump L_0583

L_0535:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0548
    VMJump L_0554

L_0548:
    VMCall L_05DF
    VMJump L_0583

L_0554:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0567
    VMJump L_057D

L_0567:
    BSubwayCmd_Tool 108, 0, 0, 0
    VMCall L_07E4
    VMJump L_0583

L_057D:
    VMCall L_28A4

L_0583:
    VMReturn

L_0585:
    BSubwayCmd_Tool 45, 0, 0, 0
    VMCall L_2CC2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05AA
    VMReturn

L_05AA:
    VMCall L_3223
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05D7
    WorkSetConst 0x8008, 59
    VMCall L_3151
    MsgWinCloseAll
    VMCall L_2D3B

L_05D7:
    VMCall L_28A4
    VMReturn

L_05DF:
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0602
    RTCallGlobal 2005
    VMCall L_28A4
    VMReturn

L_0602:
    BSubwayCmd_Tool 45, 0, 0, 0
    VMCall L_2CC2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0627
    VMReturn

L_0627:
    VMCall L_31EA
    FunfestBGMReturn
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0648
    VMJump L_0695

L_0648:
    BSubwayCmd_Tool 105, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0693
    BSubwayCmd_Tool 112, 0, 0, 32802
    WordSetNumber 0, 0x8022, 2
    BSubwayCmd_Tool 113, 0, 0, 32802
    WordSetNumber 1, 0x8022, 3
    WorkSetConst 0x8008, 60
    VMCall L_3151

L_0693:
    Cmd_013C

L_0695:
    VMCall L_28A4
    VMReturn

L_069D:
    WorkSetConst 0x8010, 1

L_06A3:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0704
    VMCall L_1B64
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 47, 65535, 0
    ListMenuAdd 48, 65535, 1
    ListMenuAdd 49, 65535, 2
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06FE
    VMCall L_1C4F
    WorkSetConst 0x8010, 1

L_06FE:
    VMJump L_06A3

L_0704:
    MsgWinCloseAll
    VMReturn

L_0708:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    BSubwayCmd_Tool 107, 0, 0, 32808
    WorkSetConst 0x8027, 1

L_0724:
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D4
    VMCall L_1B64
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0787
    ListMenuAdd 47, 65535, 0
    ListMenuAdd 50, 65535, 1
    ListMenuAdd 51, 65535, 2
    ListMenuAdd 48, 65535, 3
    ListMenuAdd 49, 65535, 4
    VMJump L_07A7

L_0787:
    ListMenuAdd 47, 65535, 0
    ListMenuAdd 50, 65535, 1
    ListMenuAdd 48, 65535, 3
    ListMenuAdd 49, 65535, 4

L_07A7:
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C8
    VMCall L_1C4F
    VMJump L_07CE

L_07C8:
    WorkSetConst 0x8027, 0

L_07CE:
    VMJump L_0724

L_07D4:
    MsgWinCloseAll
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    VMReturn

L_07E4:
    BSubwayCmd_Tool 0, 0, 0, 32800
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_0801
    VMJump L_080D

L_0801:
    VMCall L_08C9
    VMJump L_08C7

L_080D:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0820
    VMJump L_082C

L_0820:
    VMCall L_08C9
    VMJump L_08C7

L_082C:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_083F
    VMJump L_084B

L_083F:
    VMCall L_09A5
    VMJump L_08C7

L_084B:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_085E
    VMJump L_086A

L_085E:
    VMCall L_143B
    VMJump L_08C7

L_086A:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_087D
    VMJump L_0889

L_087D:
    VMCall L_08C9
    VMJump L_08C7

L_0889:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_089C
    VMJump L_08A8

L_089C:
    VMCall L_08C9
    VMJump L_08C7

L_08A8:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_08BB
    VMJump L_08C7

L_08BB:
    VMCall L_09A5
    VMJump L_08C7

L_08C7:
    VMReturn

L_08C9:
    BSubwayCmd_Tool 0, 0, 0, 32800
    Plugin1_Cmd1001 0, 0x8020
    VMCall L_2F54
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08FA
    VMCall L_28A4
    VMReturn

L_08FA:
    VMCall L_2A28
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0948
    BSubwayCmd_Tool 200, 1, 0, 32805
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_093E
    VMCall L_28A4
    VMReturn
    VMJump L_0948

L_093E:
    BSubwayCmd_Tool 501, 0, 0, 0

L_0948:
    VMCall L_1F07
    VMCall L_2D29
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0969
    VMReturn

L_0969:
    BSubwayCmd_Tool 359, 0, 0, 0
    BSubwayCmd_Tool 316, 0, 0, 0
    VMCall L_1DB2
    Cmd_01DD 6, 0, 0
    RecordAdd 48, 1
    Cmd_02C5 2
    FunfestBGMReturn
    VMCall L_23B4
    VMCall L_29D1
    VMReturn

L_09A5:
    BSubwayCmd_Tool 0, 0, 0, 32800
    Plugin1_Cmd1001 0, 0x8020
    VMCall L_2F54
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09D6
    VMCall L_28A4
    VMReturn

L_09D6:
    WorkSetConst 0x8008, 56
    VMCall L_3151
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A4D
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A1E
    MsgWinCloseAll
    RTCallGlobal 2005
    VMCall L_28A4
    VMReturn

L_0A1E:
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A3D
    WorkSetConst 0x8020, 8
    VMJump L_0A43

L_0A3D:
    WorkSetConst 0x8020, 3

L_0A43:
    BSubwayCmd_Tool 325, 0, 0, 0

L_0A4D:
    VMCall L_2A28
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A9B
    BSubwayCmd_Tool 200, 1, 0, 32805
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A91
    VMCall L_28A4
    VMReturn
    VMJump L_0A9B

L_0A91:
    BSubwayCmd_Tool 501, 0, 0, 0

L_0A9B:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0AEB
    VMCall L_1F07
    VMCall L_2D29
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ADF
    VMReturn

L_0ADF:
    VMCall L_0B2E
    VMJump L_0B2C

L_0AEB:
    BSubwayCmd_Tool 337, 0, 0, 0
    WorkSetConst 0x8008, 88
    VMCall L_3151
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    ListMenuAdd 89, 65535, 1
    ListMenuAdd 90, 65535, 0
    ListMenuShow
    BSubwayCmd_Tool 337, 0x8010, 0, 0
    VMCall L_0C31

L_0B2C:
    VMReturn

L_0B2E:
    VMCall L_2050
    WorkSetConst 0x8008, 69
    WorkSetConst 0x8009, 70
    VMCall L_2E8B
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    ListMenuAdd 71, 65535, 0
    ListMenuAdd 72, 65535, 1
    ListMenuAdd 73, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0B7C
    VMJump L_0B98

L_0B7C:
    BSubwayCmd_Tool 314, 0, 0, 0
    WorkSetConst 0x8008, 74
    WorkSetConst 0x8009, 77
    VMJump L_0BDD

L_0B98:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0BAB
    VMJump L_0BC7

L_0BAB:
    BSubwayCmd_Tool 314, 1, 0, 0
    WorkSetConst 0x8008, 75
    WorkSetConst 0x8009, 78
    VMJump L_0BDD

L_0BC7:
    BSubwayCmd_Tool 314, 2, 0, 0
    WorkSetConst 0x8008, 76
    WorkSetConst 0x8009, 79

L_0BDD:
    VMCall L_2E8B
    MsgWinCloseAll
    BSubwayCmd_Tool 335, 0, 0, 0
    BSubwayCmd_Tool 359, 0, 0, 0
    BSubwayCmd_Tool 316, 0, 0, 0
    VMCall L_21E0
    VMCall L_1DB2
    Cmd_01DD 6, 0, 0
    RecordAdd 48, 1
    Cmd_02C5 2
    FunfestBGMReturn
    VMCall L_23B4
    VMCall L_29D1
    VMReturn

L_0C31:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    VMCall L_1EDD
    BSubwayCmd_Tool 338, 0, 0, 32809
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C72
    WorkSetConst 0x8008, 91
    VMJump L_0C78

L_0C72:
    WorkSetConst 0x8008, 84

L_0C78:
    VMCall L_3151
    YesNoWin 0x8022
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C9D
    VMCall L_28A4
    VMReturn

L_0C9D:
    MsgWinCloseAll
    VMCall L_31EA
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CC0
    VMCall L_28A4
    VMReturn

L_0CC0:
    WorkSetConst 0x802a, 1
    VMStackPush 0x802a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D0E
    BSubwayCmd_Tool 45, 0, 0, 0
    VMCall L_2CC2
    WorkSetConst 0x8022, 0
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D08
    WorkSetConst 0x8022, 1

L_0D08:
    VMJump L_0D14

L_0D0E:
    WorkSetConst 0x8022, 0

L_0D14:
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0D29
    VMReturn

L_0D29:
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D7D
    BSubwayCmd_Tool 408, 0, 0, 0
    BSubwayCmd_Tool 409, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0D6D
    Cmd_013C
    VMCall L_28A4
    VMReturn

L_0D6D:
    BSubwayCmd_Tool 400, 0, 0, 0
    VMJump L_0E58

L_0D7D:
    WorkSetConst 0x8008, 61
    VMCall L_3151
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    ListMenuAdd 83, 65535, 1
    ListMenuAdd 82, 65535, 0
    ListMenuAdd 49, 65535, 2
    ListMenuShow
    VMStackPush 0x8022
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0DD7
    VMCall L_28A4
    VMReturn

L_0DD7:
    MsgWinCloseAll
    BSubwayCmd_Tool 400, 0, 0, 0
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E02
    VMCall L_137C
    VMJump L_0E08

L_0E02:
    VMCall L_13EB

L_0E08:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E58
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E4A
    VMCall L_291B
    VMCall L_28C4
    VMJump L_0E56

L_0E4A:
    VMCall L_28D4
    VMCall L_28A4

L_0E56:
    VMReturn

L_0E58:
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E83
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_0E83:
    // "Communicating. Please stand by..."
    SystemMsgAsync Global10330_Text_CommunicatingPleaseStandBy, 2
    BSubwayCmd_Tool 330, 1, 0, 0
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 9, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ECA
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_0ECA:
    BSubwayCmd_Tool 405, 4, 0, 0
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F01
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_0F01:
    BSubwayCmd_Tool 406, 4, 0, 32802
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F38
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_0F38:
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FA4
    InfoMsgClose
    WorkSetConst 0x8008, 64
    VMCall L_3151
    MsgWinCloseAll
    // "Communicating. Please stand by..."
    SystemMsgAsync Global10330_Text_CommunicatingPleaseStandBy, 2
    VMSleep 15
    BSubwayCmd_Tool 402, 10, 0, 32806
    MsgWinCloseAll
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F96
    VMCall L_291B
    VMCall L_28C4
    VMJump L_0FA2

L_0F96:
    VMCall L_28D4
    VMCall L_28A4

L_0FA2:
    VMReturn

L_0FA4:
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 1, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FDB
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_0FDB:
    BSubwayCmd_Tool 405, 0, 0, 0
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1012
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_1012:
    BSubwayCmd_Tool 406, 0, 0, 32802
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1049
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_1049:
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_1124
    InfoMsgClose
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_10AF
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1094
    BSubwayCmd_Tool 336, 0, 0, 32802
    VMJump L_109E

L_1094:
    BSubwayCmd_Tool 336, 1, 0, 32802

L_109E:
    WordSetPokeSpecies 0, 0x8022
    WorkSetConst 0x8008, 62
    VMJump L_10D3

L_10AF:
    BSubwayCmd_Tool 336, 0, 0, 32802
    WordSetPokeSpecies 0, 0x8022
    BSubwayCmd_Tool 336, 1, 0, 32802
    WordSetPokeSpecies 1, 0x8022
    WorkSetConst 0x8008, 63

L_10D3:
    VMCall L_3151
    MsgWinCloseAll
    // "Communicating. Please stand by..."
    SystemMsgAsync Global10330_Text_CommunicatingPleaseStandBy, 2
    VMSleep 15
    BSubwayCmd_Tool 402, 8, 0, 32806
    MsgWinCloseAll
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1116
    VMCall L_291B
    VMCall L_28C4
    VMJump L_1122

L_1116:
    VMCall L_28EC
    VMCall L_28A4

L_1122:
    VMReturn

L_1124:
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 5, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_115B
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_115B:
    BSubwayCmd_Tool 405, 3, 0, 0
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1192
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_1192:
    BSubwayCmd_Tool 406, 3, 0, 0
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11C9
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_11C9:
    BSubwayCmd_Tool 319, 0, 0, 0
    BSubwayCmd_Tool 402, 2, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1200
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_1200:
    BSubwayCmd_Tool 410, 0, 0, 0
    BSubwayCmd_Tool 407, 0, 0, 32811
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1231
    BSubwayCmd_Tool 316, 0, 0, 0

L_1231:
    BSubwayCmd_Tool 402, 6, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_125E
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_125E:
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1281
    BSubwayCmd_Tool 405, 1, 0, 32802
    VMJump L_128B

L_1281:
    BSubwayCmd_Tool 406, 1, 0, 16384

L_128B:
    BSubwayCmd_Tool 415, 0, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12B8
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_12B8:
    BSubwayCmd_Tool 402, 7, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12E5
    InfoMsgClose
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_12E5:
    InfoMsgClose
    VMSleep 30
    BSubwayCmd_Tool 4, 0, 0, 0
    VMCall L_1F07
    VMCall L_2D3B
    VMCall L_1DB2
    // "Communicating. Please stand by..."
    SystemMsgAsync Global10330_Text_CommunicatingPleaseStandBy, 2
    VMSleep 15
    BSubwayCmd_Tool 402, 3, 0, 32806
    MsgWinCloseAll
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_133E
    VMCall L_291B
    VMCall L_28C4
    VMReturn

L_133E:
    BSubwayCmd_Tool 359, 0, 0, 0
    Cmd_01DD 6, 0, 0
    RecordAdd 48, 1
    Cmd_02C5 2
    FunfestBGMReturn
    VMCall L_23B4
    VMCall L_29D1
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    VMReturn

L_137C:
    BSubwayCmd_Tool 403, 0, 0, 32784
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_1399
    VMJump L_13A5

L_1399:
    WorkSetConst 0x8010, 1
    VMJump L_13E9

L_13A5:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_13B8
    VMJump L_13C4

L_13B8:
    WorkSetConst 0x8010, 0
    VMJump L_13E9

L_13C4:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_13D7
    VMJump L_13E3

L_13D7:
    WorkSetConst 0x8010, 0
    VMJump L_13E9

L_13E3:
    WorkSetConst 0x8010, 0

L_13E9:
    VMReturn

L_13EB:
    BSubwayCmd_Tool 404, 0, 0, 32784
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_1408
    VMJump L_1414

L_1408:
    WorkSetConst 0x8010, 1
    VMJump L_1439

L_1414:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_1427
    VMJump L_1433

L_1427:
    WorkSetConst 0x8010, 0
    VMJump L_1439

L_1433:
    WorkSetConst 0x8010, 0

L_1439:
    VMReturn

L_143B:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    BSubwayCmd_Tool 109, 0, 0, 32812
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1487
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1487
    RTCallGlobal 2005
    VMCall L_28A4
    VMReturn

L_1487:
    Plugin1_Cmd1001 0, 4
    VMCall L_2F54
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14AE
    VMCall L_28A4
    VMReturn

L_14AE:
    VMCall L_2A28
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14CF
    VMCall L_28A4
    VMReturn

L_14CF:
    VMStackPush 0x802c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1513
    BSubwayCmd_Tool 359, 0, 0, 0
    VMCall L_1F07
    VMCall L_2D29
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_150D
    VMReturn

L_150D:
    VMJump L_15B2

L_1513:
    VMCall L_1EDD
    BSubwayCmd_Tool 108, 0, 0, 0
    BSubwayCmd_Tool 45, 0, 0, 0
    VMCall L_2CC2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1548
    VMReturn

L_1548:
    VMCall L_31EA
    FunfestBGMReturn
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_156B
    VMCall L_28A4
    VMReturn

L_156B:
    BSubwayCmd_Tool 104, 0, 0, 32802
    Cmd_013C
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1592
    VMCall L_28A4
    VMReturn

L_1592:
    BSubwayCmd_Tool 359, 0, 0, 0
    VMCall L_1F07
    BSubwayCmd_Tool 108, 1, 0, 0
    VMCall L_2D3B

L_15B2:
    WorkSetConst 0x802d, 1

L_15B8:
    VMStackPush 0x802d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1667
    WorkSetConst 0x8008, 35
    VMCall L_3151
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_15FA
    WorkSetConst 0x802d, 0
    VMJump L_1661

L_15FA:
    WorkSetConst 0x8008, 39
    VMCall L_3151
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1661
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4176, 1
    BSubwayCmd_Tool 322, 0, 0, 0
    BSubwayCmd_Tool 306, 0, 0, 0
    Plugin1_Cmd1002
    VMCall L_2D3B
    FadeEx 3, 0, 16, 2
    FadeExWait
    GameCommDisconnect 0x8022
    FieldSubscreenDisable
    VMSleep 8
    BSubwayCmd_Tool 1, 0, 0, 0

L_1661:
    VMJump L_15B8

L_1667:
    MsgWinCloseAll
    RecordAdd 48, 1
    Cmd_02C5 2
    FunfestBGMReturn
    VMCall L_23B4
    VMCall L_29D1
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    VMReturn

Script_5:
    VMCall L_1EDD
    Cmd_013C
    BSubwayCmd_Tool 3, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_1788
    WorkSetConst 0x8008, 40
    VMCall L_3151
    YesNoWin 0x8010
    MsgWinCloseAll
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_16FF
    WorkSetConst EVENT_WORK_0x4176, 1
    FadeEx 3, 0, 16, 2
    FadeExWait
    GameCommDisconnect 0x8022
    FieldSubscreenDisable
    VMSleep 8
    BSubwayCmd_Tool 1, 0, 0, 0

L_16FF:
    Plugin1_Cmd1001 1, 10
    BSubwayCmd_Tool 310, 0, 0, 32784
    BSubwayCmd_Tool 8, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_1738
    VMCall L_1F07
    VMJump L_173E

L_1738:
    VMCall L_1F21

L_173E:
    BSubwayCmd_Tool 4, 0, 0, 0
    // "Saving...\nDon't turn off the power."
    SystemMsg Global10330_Text_SavingDontTurnOff, 2
    SaveDataWrite 0x8010
    MsgWinCloseAll
    WorkSetConst 0x8008, 42
    VMCall L_3151
    MsgWinCloseAll
    Cmd_01DD 6, 0, 0
    RecordAdd 48, 1
    Cmd_02C5 2
    FunfestBGMReturn
    VMCall L_23B4
    VMCall L_29D1
    VMJump L_178E

L_1788:
    VMCall L_292D

L_178E:
    RTEndGlobal
    VMReturn

Script_6:
    VMCall L_1EDD
    VMCall L_292D
    RTEndGlobal
    VMHalt

Script_7:
    VMCall L_26D0
    VMCall L_1EDD
    BSubwayCmd_Tool 21, 0, 0, 32800
    BSubwayCmd_Tool 38, 0x8020, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_17E1
    WorkSetConst 0x8008, 43
    VMCall L_3151

L_17E1:
    VMCall L_28A4
    RTEndGlobal
    VMHalt

Script_8:
    VMCall L_26D0
    VMCall L_1EDD
    BSubwayCmd_Tool 21, 0, 0, 32800
    BSubwayCmd_Tool 304, 0, 0, 0
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1881
    BSubwayCmd_Tool 109, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1881
    BSubwayCmd_Tool 29, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1877
    BSubwayCmd_Tool 27, 0, 0, 32802
    WordSetNumber 0, 0x8022, 2
    WorkSetConst 0x8008, 87
    VMCall L_3151
    MsgWinCloseAll

L_1877:
    BSubwayCmd_Tool 100, 1, 0, 0

L_1881:
    // "Saving...\nDon't turn off the power."
    SystemMsg Global10330_Text_SavingDontTurnOff_2, 2
    SaveDataWrite 0x8010
    MsgWinCloseAll
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_190F
    BSubwayCmd_Tool 109, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_190F
    WorkSetConst 0x8008, 101
    VMCall L_3151
    YesNoWin 0x8022
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_190F
    VMCall L_3223
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_190F
    WorkSetConst 0x8008, 59
    VMCall L_3151
    MsgWinCloseAll
    VMCall L_2D3B

L_190F:
    VMCall L_28A4
    RTEndGlobal
    VMHalt

L_1919:
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    BSubwayCmd_Tool 0, 0, 0, 32800
    WorkSetConst 0x802e, 1
    FlagGet EVENT_FLAG_0x0960, 0x802f
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_194E
    VMJump L_195A

L_194E:
    WorkSetConst 0x8008, 1
    VMJump L_1B35

L_195A:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_196D
    VMJump L_1979

L_196D:
    WorkSetConst 0x8008, 2
    VMJump L_1B35

L_1979:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_198C
    VMJump L_1998

L_198C:
    WorkSetConst 0x8008, 3
    VMJump L_1B35

L_1998:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_19AB
    VMJump L_19DC

L_19AB:
    VMStackPush 0x802f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_19D0
    WorkSetConst 0x8008, 100
    WorkSetConst 0x802e, 0
    VMJump L_19D6

L_19D0:
    WorkSetConst 0x8008, 4

L_19D6:
    VMJump L_1B35

L_19DC:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_19EF
    VMJump L_1A4F

L_19EF:
    VMStackPush 0x802f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1A14
    WorkSetConst 0x8008, 97
    WorkSetConst 0x802e, 0
    VMJump L_1A49

L_1A14:
    BSubwayCmd_Tool 14, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1A3D
    WorkSetConst 0x8008, 5
    VMJump L_1A49

L_1A3D:
    WorkSetConst 0x8008, 92
    WorkSetConst 0x802e, 0

L_1A49:
    VMJump L_1B35

L_1A4F:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_1A62
    VMJump L_1AC2

L_1A62:
    VMStackPush 0x802f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1A87
    WorkSetConst 0x8008, 98
    WorkSetConst 0x802e, 0
    VMJump L_1ABC

L_1A87:
    BSubwayCmd_Tool 14, 1, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1AB0
    WorkSetConst 0x8008, 7
    VMJump L_1ABC

L_1AB0:
    WorkSetConst 0x8008, 93
    WorkSetConst 0x802e, 0

L_1ABC:
    VMJump L_1B35

L_1AC2:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_1AD5
    VMJump L_1B35

L_1AD5:
    VMStackPush 0x802f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1AFA
    WorkSetConst 0x8008, 99
    WorkSetConst 0x802e, 0
    VMJump L_1B2F

L_1AFA:
    BSubwayCmd_Tool 14, 2, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1B23
    WorkSetConst 0x8008, 9
    VMJump L_1B2F

L_1B23:
    WorkSetConst 0x8008, 94
    WorkSetConst 0x802e, 0

L_1B2F:
    VMJump L_1B35

L_1B35:
    VMCall L_3151
    WorkGet 0x8010, 0x802e
    VMStackPush 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1B56
    MsgWinCloseAll

L_1B56:
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    VMReturn

L_1B64:
    BSubwayCmd_Tool 0, 0, 0, 32800
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_1B81
    VMJump L_1B8D

L_1B81:
    WorkSetConst 0x8008, 21
    VMJump L_1C47

L_1B8D:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_1BA0
    VMJump L_1BAC

L_1BA0:
    WorkSetConst 0x8008, 22
    VMJump L_1C47

L_1BAC:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_1BBF
    VMJump L_1BCB

L_1BBF:
    WorkSetConst 0x8008, 23
    VMJump L_1C47

L_1BCB:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_1BDE
    VMJump L_1BEA

L_1BDE:
    WorkSetConst 0x8008, 24
    VMJump L_1C47

L_1BEA:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_1BFD
    VMJump L_1C09

L_1BFD:
    WorkSetConst 0x8008, 25
    VMJump L_1C47

L_1C09:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_1C1C
    VMJump L_1C28

L_1C1C:
    WorkSetConst 0x8008, 26
    VMJump L_1C47

L_1C28:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_1C3B
    VMJump L_1C47

L_1C3B:
    WorkSetConst 0x8008, 27
    VMJump L_1C47

L_1C47:
    VMCall L_3151
    VMReturn

L_1C4F:
    BSubwayCmd_Tool 0, 0, 0, 32800
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_1C6C
    VMJump L_1C8A

L_1C6C:
    WorkSetConst 0x8008, 13
    VMCall L_3151
    WorkSetConst 0x8008, 14
    VMCall L_3151
    VMJump L_1DB0

L_1C8A:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_1C9D
    VMJump L_1CBB

L_1C9D:
    WorkSetConst 0x8008, 15
    VMCall L_3151
    WorkSetConst 0x8008, 16
    VMCall L_3151
    VMJump L_1DB0

L_1CBB:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_1CCE
    VMJump L_1CEC

L_1CCE:
    WorkSetConst 0x8008, 17
    VMCall L_3151
    WorkSetConst 0x8008, 18
    VMCall L_3151
    VMJump L_1DB0

L_1CEC:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_1CFF
    VMJump L_1D1D

L_1CFF:
    WorkSetConst 0x8008, 19
    VMCall L_3151
    WorkSetConst 0x8008, 20
    VMCall L_3151
    VMJump L_1DB0

L_1D1D:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_1D30
    VMJump L_1D4E

L_1D30:
    WorkSetConst 0x8008, 13
    VMCall L_3151
    WorkSetConst 0x8008, 14
    VMCall L_3151
    VMJump L_1DB0

L_1D4E:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_1D61
    VMJump L_1D7F

L_1D61:
    WorkSetConst 0x8008, 15
    VMCall L_3151
    WorkSetConst 0x8008, 16
    VMCall L_3151
    VMJump L_1DB0

L_1D7F:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_1D92
    VMJump L_1DB0

L_1D92:
    WorkSetConst 0x8008, 17
    VMCall L_3151
    WorkSetConst 0x8008, 18
    VMCall L_3151
    VMJump L_1DB0

L_1DB0:
    VMReturn

L_1DB2:
    BSubwayCmd_Tool 21, 0, 0, 32800
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_1DCF
    VMJump L_1DDB

L_1DCF:
    WorkSetConst 0x8008, 32
    VMJump L_1ED3

L_1DDB:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_1DEE
    VMJump L_1DFA

L_1DEE:
    WorkSetConst 0x8008, 33
    VMJump L_1ED3

L_1DFA:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_1E0D
    VMJump L_1E19

L_1E0D:
    WorkSetConst 0x8008, 34
    VMJump L_1ED3

L_1E19:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_1E2C
    VMJump L_1E38

L_1E2C:
    WorkSetConst 0x8008, 34
    VMJump L_1ED3

L_1E38:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_1E4B
    VMJump L_1E57

L_1E4B:
    WorkSetConst 0x8008, 35
    VMJump L_1ED3

L_1E57:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_1E6A
    VMJump L_1E76

L_1E6A:
    WorkSetConst 0x8008, 36
    VMJump L_1ED3

L_1E76:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_1E89
    VMJump L_1E95

L_1E89:
    WorkSetConst 0x8008, 37
    VMJump L_1ED3

L_1E95:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_1EA8
    VMJump L_1EB4

L_1EA8:
    WorkSetConst 0x8008, 38
    VMJump L_1ED3

L_1EB4:
    WorkCmpConst 0x8020, 8
    VMJumpIf CMP_EQ, L_1EC7
    VMJump L_1ED3

L_1EC7:
    WorkSetConst 0x8008, 38
    VMJump L_1ED3

L_1ED3:
    VMCall L_3151
    MsgWinCloseAll
    VMReturn

L_1EDD:
    WorkSetConst EVENT_WORK_0x4176, 0
    WorkSetConst EVENT_WORK_0x4178, 0
    WorkSetConst EVENT_WORK_0x4179, 0
    BSubwayCmd_Tool 5, 0, 0, 0
    FlagSet EVENT_FLAG_0x0265
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    VMReturn

L_1F07:
    WorkSetConst EVENT_WORK_0x4176, 4
    WorkSetConst EVENT_WORK_0x4178, 1
    FlagSet EVENT_FLAG_0x0265
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    VMReturn

L_1F21:
    WorkSetConst EVENT_WORK_0x4176, 4
    WorkSetConst EVENT_WORK_0x4178, 2
    FlagSet EVENT_FLAG_0x0265
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    VMReturn
    .balign 4, 0

Movement_1F3C:
    Move 32, 1
    MoveEnd

Movement_1F44:
    Move 33, 1
    MoveEnd

Movement_1F4C:
    Move 34, 1
    MoveEnd

Movement_1F54:
    Move 35, 1
    MoveEnd

L_1F5C:
    VMCall L_2D49
    WorkGet 0x8021, 0x8010
    ActorGetGPos 255, 0x8008, 0x8009
    WorkCmpConst 0x8009, 14
    VMJumpIf CMP_EQ, L_1F83
    VMJump L_1F9B

L_1F83:
    ActorCmdExec 255, Movement_1F44
    ActorCmdExec 0x8021, Movement_1F3C
    ActorCmdWait
    VMJump L_1FF1

L_1F9B:
    WorkCmpConst 0x8009, 16
    VMJumpIf CMP_EQ, L_1FAE
    VMJump L_1FC6

L_1FAE:
    ActorCmdExec 255, Movement_1F3C
    ActorCmdExec 0x8021, Movement_1F44
    ActorCmdWait
    VMJump L_1FF1

L_1FC6:
    WorkCmpConst 0x8009, 17
    VMJumpIf CMP_EQ, L_1FD9
    VMJump L_1FF1

L_1FD9:
    ActorCmdExec 255, Movement_1FF4
    ActorCmdExec 0x8021, Movement_1F44
    ActorCmdWait
    VMJump L_1FF1

L_1FF1:
    VMReturn
    .balign 4, 0

Movement_1FF4:
    Move 12, 1
    MoveEnd

L_1FFC:
    VMCall L_2D49
    WorkGet 0x8021, 0x8010
    ActorGetGPos 255, 0x8008, 0x8009
    VMStackPush 0x8009
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMStackPush 0x8009
    VMStackPushConst 16
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_2045
    ActorCmdExec 255, Movement_2048
    ActorCmdExec 0x8021, Movement_1F4C
    ActorCmdWait

L_2045:
    VMReturn
    .balign 4, 0

Movement_2048:
    Move 14, 1
    MoveEnd

L_2050:
    VMCall L_2E47
    WorkGet 0x8021, 0x8010
    VMCall L_2EE0
    ActorSetGPos 0x8021, 2, 1, 12, 3
    VMCall L_317D
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_2087
    VMJump L_2095

L_2087:
    ActorCmdExec 0x8021, Movement_219C
    VMJump L_20DF

L_2095:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_20A8
    VMJump L_20B6

L_20A8:
    ActorCmdExec 0x8021, Movement_21AC
    VMJump L_20DF

L_20B6:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_20C9
    VMJump L_20D7

L_20C9:
    ActorCmdExec 0x8021, Movement_21BC
    VMJump L_20DF

L_20D7:
    ActorCmdExec 0x8021, Movement_21CC

L_20DF:
    ActorCmdWait
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_20F4
    VMJump L_2102

L_20F4:
    ActorCmdExec 255, Movement_1F4C
    VMJump L_214C

L_2102:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_2115
    VMJump L_2123

L_2115:
    ActorCmdExec 255, Movement_1F4C
    VMJump L_214C

L_2123:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_2136
    VMJump L_2144

L_2136:
    ActorCmdExec 255, Movement_1F4C
    VMJump L_214C

L_2144:
    ActorCmdExec 255, Movement_1F44

L_214C:
    ActorCmdWait
    BSubwayCmd_Tool 16, 0, 0, 32784
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2187
    WorkSetConst 0x8008, 65
    WorkSetConst 0x8009, 66
    BSubwayCmd_Tool 17, 0, 0, 0
    VMJump L_2193

L_2187:
    WorkSetConst 0x8008, 67
    WorkSetConst 0x8009, 68

L_2193:
    VMCall L_2E8B
    VMReturn
    .balign 4, 0

Movement_219C:
    Move 15, 6
    Move 13, 2
    Move 15, 3
    MoveEnd

Movement_21AC:
    Move 15, 6
    Move 13, 4
    Move 15, 3
    MoveEnd

Movement_21BC:
    Move 15, 6
    Move 13, 3
    Move 15, 2
    MoveEnd

Movement_21CC:
    Move 15, 6
    Move 13, 4
    Move 15, 5
    Move 32, 1
    MoveEnd

L_21E0:
    VMCall L_317D
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_21F9
    VMJump L_2207

L_21F9:
    ActorCmdExec 255, Movement_1F44
    VMJump L_2251

L_2207:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_221A
    VMJump L_2228

L_221A:
    ActorCmdExec 255, Movement_1F3C
    VMJump L_2251

L_2228:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_223B
    VMJump L_2249

L_223B:
    ActorCmdExec 255, Movement_1F54
    VMJump L_2251

L_2249:
    ActorCmdExec 255, Movement_1F4C

L_2251:
    ActorCmdWait
    VMReturn
    VMCall L_317D
    VMCall L_2D49
    WorkGet 0x8021, 0x8010
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_227A
    VMJump L_2288

L_227A:
    ActorCmdExec 255, Movement_1F44
    VMJump L_22D2

L_2288:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_229B
    VMJump L_22A9

L_229B:
    ActorCmdExec 255, Movement_1F3C
    VMJump L_22D2

L_22A9:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_22BC
    VMJump L_22CA

L_22BC:
    ActorCmdExec 255, Movement_1F54
    VMJump L_22D2

L_22CA:
    ActorCmdExec 255, Movement_1F4C

L_22D2:
    ActorCmdWait
    VMReturn
    VMCall L_2E47
    WorkGet 0x8021, 0x8010
    VMCall L_317D
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_22FB
    VMJump L_2309

L_22FB:
    ActorCmdExec 0x8021, Movement_2364
    VMJump L_2353

L_2309:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_231C
    VMJump L_232A

L_231C:
    ActorCmdExec 0x8021, Movement_2378
    VMJump L_2353

L_232A:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_233D
    VMJump L_234B

L_233D:
    ActorCmdExec 0x8021, Movement_238C
    VMJump L_2353

L_234B:
    ActorCmdExec 0x8021, Movement_23A0

L_2353:
    ActorCmdWait
    VMCall L_2F21
    VMCall L_21E0
    VMReturn
    .balign 4, 0

Movement_2364:
    Move 14, 3
    Move 12, 2
    Move 14, 6
    Move 69, 1
    MoveEnd

Movement_2378:
    Move 14, 3
    Move 12, 4
    Move 14, 6
    Move 69, 1
    MoveEnd

Movement_238C:
    Move 14, 2
    Move 12, 3
    Move 14, 6
    Move 69, 1
    MoveEnd

Movement_23A0:
    Move 14, 5
    Move 12, 4
    Move 14, 6
    Move 69, 1
    MoveEnd

L_23B4:
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    BSubwayCmd_Tool 310, 0, 0, 32800
    WorkSetConst 0x8024, 0
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_23F9
    WorkSetConst 0x8024, 1

L_23F9:
    VMCall L_2D49
    WorkGet 0x8030, 0x8010
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_242E
    VMCall L_2E47
    WorkGet 0x8031, 0x8010
    BSubwayCmd_Tool 23, 0x8031, 0, 0

L_242E:
    BSubwayCmd_Tool 23, 255, 0, 0
    VMCall L_317D
    WorkCmpConst 0x800a, 0
    VMJumpIf CMP_EQ, L_2451
    VMJump L_2482

L_2451:
    ActorCmdExec 0x8030, Movement_2578
    ActorCmdExec 255, Movement_25CC
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_247C
    ActorCmdExec 0x8031, Movement_2648

L_247C:
    VMJump L_254B

L_2482:
    WorkCmpConst 0x800a, 1
    VMJumpIf CMP_EQ, L_2495
    VMJump L_24C6

L_2495:
    ActorCmdExec 0x8030, Movement_2584
    ActorCmdExec 255, Movement_25E4
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_24C0
    ActorCmdExec 0x8031, Movement_266C

L_24C0:
    VMJump L_254B

L_24C6:
    WorkCmpConst 0x800a, 2
    VMJumpIf CMP_EQ, L_24D9
    VMJump L_250A

L_24D9:
    ActorCmdExec 0x8030, Movement_2590
    ActorCmdExec 255, Movement_25FC
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2504
    ActorCmdExec 0x8031, Movement_2690

L_2504:
    VMJump L_254B

L_250A:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2533
    ActorCmdExec 0x8030, Movement_259C
    ActorCmdExec 255, Movement_2610
    VMJump L_254B

L_2533:
    ActorCmdExec 0x8030, Movement_25B4
    ActorCmdExec 255, Movement_2630
    ActorCmdExec 0x8031, Movement_26AC

L_254B:
    ActorCmdWait
    BSubwayCmd_Tool 19, 3, 0, 0
    SEPlay SEQ_SE_BDEMO_01
    VMSleep 20
    SEPlay SEQ_SE_BDEMO_02
    VMSleep 30
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8030, 0
    VMReturn
    .balign 4, 0

Movement_2578:
    Move 15, 5
    Move 32, 1
    MoveEnd

Movement_2584:
    Move 15, 5
    Move 32, 1
    MoveEnd

Movement_2590:
    Move 15, 5
    Move 32, 1
    MoveEnd

Movement_259C:
    Move 13, 1
    Move 15, 2
    Move 12, 1
    Move 15, 3
    Move 32, 1
    MoveEnd

Movement_25B4:
    Move 13, 2
    Move 15, 2
    Move 12, 2
    Move 15, 3
    Move 32, 1
    MoveEnd

Movement_25CC:
    Move 13, 1
    Move 15, 4
    Move 12, 2
    Move 60, 4
    Move 69, 1
    MoveEnd

Movement_25E4:
    Move 12, 1
    Move 15, 4
    Move 12, 2
    Move 60, 4
    Move 69, 1
    MoveEnd

Movement_25FC:
    Move 15, 5
    Move 12, 2
    Move 60, 4
    Move 69, 1
    MoveEnd

Movement_2610:
    Move 63, 4
    Move 15, 3
    Move 12, 2
    Move 60, 4
    Move 69, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_2630:
    Move 63, 6
    Move 15, 3
    Move 12, 2
    Move 60, 4
    Move 69, 1
    MoveEnd

Movement_2648:
    Move 15, 1
    Move 13, 1
    Move 15, 4
    Move 12, 1
    Move 63, 1
    Move 12, 1
    Move 60, 4
    Move 69, 1
    MoveEnd

Movement_266C:
    Move 15, 1
    Move 12, 1
    Move 15, 4
    Move 12, 1
    Move 60, 4
    Move 12, 1
    Move 60, 4
    Move 69, 1
    MoveEnd

Movement_2690:
    Move 15, 6
    Move 12, 1
    Move 60, 4
    Move 12, 1
    Move 60, 4
    Move 69, 1
    MoveEnd

Movement_26AC:
    Move 60, 6
    Move 12, 1
    Move 15, 3
    Move 12, 1
    Move 60, 4
    Move 12, 1
    Move 60, 4
    Move 69, 1
    MoveEnd

L_26D0:
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    BSubwayCmd_Tool 21, 0, 0, 32800
    WorkSetConst 0x8024, 0
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_2715
    WorkSetConst 0x8024, 1

L_2715:
    BSubwayCmd_Tool 23, 255, 0, 0
    ActorSetGPos 255, 16, 0, 13, 1
    BSubwayCmd_Tool 13, 255, 1, 0
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_277A
    VMCall L_2E47
    WorkGet 0x8033, 0x8010
    VMCall L_2EE0
    BSubwayCmd_Tool 13, 0x8033, 1, 0
    ActorSetGPos 0x8033, 16, 0, 13, 1
    BSubwayCmd_Tool 23, 0x8033, 0, 0

L_277A:
    VMCall L_2D49
    WorkGet 0x8032, 0x8010
    ActorCmdExec 0x8032, Movement_2834
    ActorCmdWait
    FadeInBlackQ
    FadeWait
    VMSleep 30
    ActorCmdExec 0x8032, Movement_283C
    ActorCmdExec 255, Movement_2854
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_27C3
    ActorCmdExec 0x8033, Movement_2870

L_27C3:
    ActorCmdWait
    BSubwayCmd_Tool 24, 255, 0, 0
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2824
    BSubwayCmd_Tool 24, 0x8033, 0, 0
    ActorCmdExec 255, Movement_1F4C
    ActorCmdWait
    WorkSetConst 0x8008, 95
    WorkSetConst 0x8009, 96
    VMCall L_2E8B
    MsgWinCloseAll
    ActorCmdExec 0x8033, Movement_2890
    ActorCmdWait
    VMCall L_2F21
    ActorCmdExec 255, Movement_1F54
    ActorCmdWait

L_2824:
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8032, 0
    VMReturn
    .balign 4, 0

Movement_2834:
    Move 3, 1
    MoveEnd

Movement_283C:
    Move 60, 5
    Move 63, 5
    Move 33, 1
    Move 63, 1
    Move 34, 1
    MoveEnd

Movement_2854:
    Move 70, 1
    Move 60, 4
    Move 13, 3
    Move 14, 5
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_2870:
    Move 60, 5
    Move 70, 1
    Move 60, 4
    Move 13, 3
    Move 14, 6
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_2890:
    Move 14, 2
    Move 12, 3
    Move 14, 5
    Move 69, 1
    MoveEnd

L_28A4:
    VMCall L_1EDD
    WorkSetConst 0x8008, 46
    VMCall L_3151
    LastKeyWait
    MsgWinCloseAll
    Plugin1_Cmd1002
    VMCall L_1FFC
    VMReturn

L_28C4:
    VMCall L_1EDD
    Plugin1_Cmd1002
    VMCall L_1FFC
    VMReturn

L_28D4:
    BSubwayCmd_Tool 401, 0, 0, 0
    Cmd_013C
    BSubwayCmd_Tool 330, 0, 0, 0
    VMReturn

L_28EC:
    BSubwayCmd_Tool 402, 4, 0, 32806
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2913
    BSubwayCmd_Tool 414, 0, 0, 0

L_2913:
    VMCall L_28D4
    VMReturn

L_291B:
    BSubwayCmd_Tool 414, 0, 0, 0
    VMCall L_28D4
    VMReturn

L_292D:
    BSubwayCmd_Tool 0, 0, 0, 32800
    WorkSetConst 0x8008, 45
    VMCall L_3151
    MsgWinCloseAll
    BSubwayCmd_Tool 7, 0, 0, 32784
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_29C9
    BSubwayCmd_Tool 109, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_29BD
    BSubwayCmd_Tool 29, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_29BD
    BSubwayCmd_Tool 27, 0, 0, 32802
    WordSetNumber 0, 0x8022, 2
    WorkSetConst 0x8008, 87
    VMCall L_3151
    ABKeyWait
    MsgWinCloseAll

L_29BD:
    VMCall L_1EDD
    VMCall L_2D3B

L_29C9:
    VMCall L_28A4
    VMReturn

L_29D1:
    FadeOutBlackQ
    FadeWait
    ActorCmdWait
    MapChangeCore ZONE_BATTLE_SUBWAY, 7, 0, 4, 0
    VMReturn

L_29E5:
    // "Please select the Pokémon you wish\nto enter.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10330_Text_PleaseSelectPokemonWish, 0x8008, 2, 0
    MsgWinCloseAll
    BSubwayCmd_Tool 356, 0, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2A12
    PokePartyRecoverAll

L_2A12:
    BSubwayCmd_Tool 300, 0, 0, 0
    BSubwayCmd_Tool 301, 0, 0, 32784
    VMReturn

L_2A28:
    VMCall L_2D49
    WorkGet 0x8008, 0x8010
    VMCall L_29E5
    VMReturn

L_2A3C:
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    BSubwayCmd_Tool 0, 0, 0, 32800
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_2C45
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2A9A
    WorkSetConst 0x8036, 3
    VMJump L_2AA0

L_2A9A:
    WorkSetConst 0x8036, 8

L_2AA0:
    BSubwayCmd_Tool 38, 0x8020, 0, 32802
    BSubwayCmd_Tool 38, 0x8036, 0, 32806
    BSubwayCmd_Tool 6, 0x8020, 0, 32820
    BSubwayCmd_Tool 6, 0x8036, 0, 32821
    VMStackPush 0x8034
    VMStackPushConst 9999
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_2AE1
    WorkSetConst 0x8034, 9999

L_2AE1:
    VMStackPush 0x8035
    VMStackPushConst 9999
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_2AFA
    WorkSetConst 0x8035, 9999

L_2AFA:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_2BC1
    VMStackPush 0x8034
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush 0x8035
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_2B63
    WordSetPlayerName 0
    WordSetNumber 1, 0x8034, 4
    WordSetNumber 2, 0x8035, 4
    WorkSetConst 0x8008, 107
    VMCall L_3151
    VMJump L_2BBB

L_2B63:
    VMStackPush 0x8034
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2B92
    WordSetPlayerName 0
    WordSetNumber 1, 0x8034, 4
    WorkSetConst 0x8008, 105
    VMCall L_3151
    VMJump L_2BBB

L_2B92:
    VMStackPush 0x8035
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2BBB
    WordSetPlayerName 0
    WordSetNumber 1, 0x8035, 4
    WorkSetConst 0x8008, 106
    VMCall L_3151

L_2BBB:
    VMJump L_2C3F

L_2BC1:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2C03
    VMStackPush 0x8034
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2BFD
    WordSetPlayerName 0
    WordSetNumber 1, 0x8034, 4
    WorkSetConst 0x8008, 105
    VMCall L_3151

L_2BFD:
    VMJump L_2C3F

L_2C03:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2C3F
    VMStackPush 0x8035
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2C3F
    WordSetPlayerName 0
    WordSetNumber 1, 0x8035, 4
    WorkSetConst 0x8008, 106
    VMCall L_3151

L_2C3F:
    VMJump L_2CAE

L_2C45:
    BSubwayCmd_Tool 38, 0x8020, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2CAE
    BSubwayCmd_Tool 6, 0x8020, 0, 32820
    VMStackPush 0x8034
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2CAE
    WordSetPlayerName 0
    VMStackPush 0x8034
    VMStackPushConst 9999
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_2C9B
    WorkSetConst 0x8034, 9999

L_2C9B:
    WordSetNumber 1, 0x8034, 4
    WorkSetConst 0x8008, 12
    VMCall L_3151

L_2CAE:
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8034, 0
    VMReturn

L_2CC2:
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
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2D21
    VMCall L_28A4
    WorkSetConst 0x8010, 1
    VMJump L_2D27

L_2D21:
    WorkSetConst 0x8010, 0

L_2D27:
    VMReturn

L_2D29:
    BSubwayCmd_Tool 4, 0, 0, 0
    VMCall L_2CC2
    VMReturn

L_2D3B:
    // "Saving...\nDon't turn off the power."
    SystemMsg Global10330_Text_SavingDontTurnOff_3, 2
    SaveDataWrite 0x8022
    MsgWinCloseAll
    VMReturn

L_2D49:
    RTGetZoneID 0x8010
    WorkCmpConst 0x8010, 67
    VMJumpIf CMP_EQ, L_2D60
    VMJump L_2D6C

L_2D60:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2D6C:
    WorkCmpConst 0x8010, 68
    VMJumpIf CMP_EQ, L_2D7F
    VMJump L_2D8B

L_2D7F:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2D8B:
    WorkCmpConst 0x8010, 69
    VMJumpIf CMP_EQ, L_2D9E
    VMJump L_2DAA

L_2D9E:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2DAA:
    WorkCmpConst 0x8010, 70
    VMJumpIf CMP_EQ, L_2DBD
    VMJump L_2DC9

L_2DBD:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2DC9:
    WorkCmpConst 0x8010, 71
    VMJumpIf CMP_EQ, L_2DDC
    VMJump L_2DE8

L_2DDC:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2DE8:
    WorkCmpConst 0x8010, 72
    VMJumpIf CMP_EQ, L_2DFB
    VMJump L_2E07

L_2DFB:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2E07:
    WorkCmpConst 0x8010, 73
    VMJumpIf CMP_EQ, L_2E1A
    VMJump L_2E26

L_2E1A:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2E26:
    WorkCmpConst 0x8010, 74
    VMJumpIf CMP_EQ, L_2E39
    VMJump L_2E45

L_2E39:
    WorkSetConst 0x8010, 0
    VMJump L_2E45

L_2E45:
    VMReturn

L_2E47:
    RTGetZoneID 0x8010
    WorkCmpConst 0x8010, 71
    VMJumpIf CMP_EQ, L_2E5E
    VMJump L_2E6A

L_2E5E:
    WorkSetConst 0x8010, 1
    VMJump L_2E89

L_2E6A:
    WorkCmpConst 0x8010, 72
    VMJumpIf CMP_EQ, L_2E7D
    VMJump L_2E89

L_2E7D:
    WorkSetConst 0x8010, 1
    VMJump L_2E89

L_2E89:
    VMReturn

L_2E8B:
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8038, 0
    WorkGet 0x8023, 0x8008
    TrainerCardGetSex 0x8037
    VMStackPush 0x8037
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2EBA
    WorkGet 0x8023, 0x8009

L_2EBA:
    VMCall L_2E47
    WorkGet 0x8038, 0x8010
    ActorMsg MSGFILE_SCRIPT, 0x8023, 0x8038, 2, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8037, 0
    VMReturn

L_2EE0:
    VMCall L_2E47
    WorkGet 0x8021, 0x8010
    FlagReset EVENT_FLAG_0x0265
    BSubwayCmd_Tool 25, 0x8021, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2F11
    ActorDelete 0x8021

L_2F11:
    BSubwayCmd_Tool 31, 0, 0, 16416
    ActorAdd 0x8021
    VMReturn

L_2F21:
    VMCall L_2E47
    WorkGet 0x8021, 0x8010
    FlagSet EVENT_FLAG_0x0265
    BSubwayCmd_Tool 25, 0x8021, 0, 32802
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2F52
    ActorDelete 0x8021

L_2F52:
    VMReturn

L_2F54:
    WorkSetConst 0x8039, 0
    WorkSetConst 0x8039, 20
    BSubwayCmd_Tool 21, 0, 0, 32800
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_2F7D
    VMJump L_2F89

L_2F7D:
    WorkSetConst 0x8039, 20
    VMJump L_3081

L_2F89:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_2F9C
    VMJump L_2FA8

L_2F9C:
    WorkSetConst 0x8039, 20
    VMJump L_3081

L_2FA8:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_2FBB
    VMJump L_2FC7

L_2FBB:
    WorkSetConst 0x8039, 21
    VMJump L_3081

L_2FC7:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_2FDA
    VMJump L_2FE6

L_2FDA:
    WorkSetConst 0x8039, 21
    VMJump L_3081

L_2FE6:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_2FF9
    VMJump L_3005

L_2FF9:
    WorkSetConst 0x8039, 22
    VMJump L_3081

L_3005:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_3018
    VMJump L_3024

L_3018:
    WorkSetConst 0x8039, 22
    VMJump L_3081

L_3024:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_3037
    VMJump L_3043

L_3037:
    WorkSetConst 0x8039, 22
    VMJump L_3081

L_3043:
    WorkCmpConst 0x8020, 8
    VMJumpIf CMP_EQ, L_3056
    VMJump L_3062

L_3056:
    WorkSetConst 0x8039, 22
    VMJump L_3081

L_3062:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_3075
    VMJump L_3081

L_3075:
    WorkSetConst 0x8039, 20
    VMJump L_3081

L_3081:
    Cmd_01B0 0x8039, 0x8022
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_309A
    VMJump L_30B0

L_309A:
    BSubwayCmd_Tool 324, 0, 0, 0
    WorkSetConst 0x8022, 1
    VMJump L_3143

L_30B0:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_30C3
    VMJump L_30E3

L_30C3:
    BSubwayCmd_Tool 324, 1, 0, 0
    BSubwayCmd_Tool 340, 0, 0, 0
    WorkSetConst 0x8022, 1
    VMJump L_3143

L_30E3:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_30F6
    VMJump L_3102

L_30F6:
    WorkSetConst 0x8022, 0
    VMJump L_3143

L_3102:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_3115
    VMJump L_313D

L_3115:
    BSubwayCmd_Tool 341, 0, 0, 32802
    VMStackPush 0x8000
    WorkSet 0x8000, 0x8022
    RTCallGlobal 10260
    VMStackPop 0x8000
    WorkSetConst 0x8022, 0
    VMJump L_3143

L_313D:
    WorkSetConst 0x8022, 0

L_3143:
    WorkGet 0x8010, 0x8022
    WorkSetConst 0x8039, 0
    VMReturn

L_3151:
    WorkSetConst 0x803a, 0
    WorkGet 0x8023, 0x8008
    VMCall L_2D49
    WorkGet 0x803a, 0x8010
    ActorMsg MSGFILE_SCRIPT, 0x8023, 0x803a, 2, 0
    WorkSetConst 0x803a, 0
    VMReturn

L_317D:
    ActorGetGPos 255, 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31C3
    VMStackPush 0x8009
    VMStackPushConst 14
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31B7
    WorkSetConst 0x800a, 0
    VMJump L_31BD

L_31B7:
    WorkSetConst 0x800a, 1

L_31BD:
    VMJump L_31E8

L_31C3:
    VMStackPush 0x8008
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31E2
    WorkSetConst 0x800a, 2
    VMJump L_31E8

L_31E2:
    WorkSetConst 0x800a, 3

L_31E8:
    VMReturn

L_31EA:
    VMStackPush 0x8000
    RTCallGlobal 2004
    WorkSet 0x8010, 0x8000
    VMStackPop 0x8000
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_321B
    WorkSetConst 0x8010, 1
    VMJump L_3221

L_321B:
    WorkSetConst 0x8010, 0

L_3221:
    VMReturn

L_3223:
    BSubwayCmd_Tool 100, 1, 0, 0
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3250
    RTCallGlobal 2005
    WorkSetConst 0x8010, 0
    VMReturn

L_3250:
    VMCall L_31EA
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3271
    WorkSetConst 0x8010, 0
    VMReturn

L_3271:
    FunfestBGMReturn
    BSubwayCmd_Tool 103, 0, 0, 32802
    Cmd_013C
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_32A8
    BSubwayCmd_Tool 100, 0, 0, 0
    WorkSetConst 0x8010, 1
    VMJump L_32AE

L_32A8:
    WorkSetConst 0x8010, 0

L_32AE:
    VMReturn

L_32B0:
    VMCall L_2D49
    WorkGet 0x8021, 0x8010
    VMCall L_317D
    BSubwayCmd_Tool 35, 0x8021, 0x800a, 0
    VMReturn

Script_9:
    VMReturn

Script_15:
    VMCall L_3223
    RTEndGlobal
    VMReturn

Script_16:
    VMCall L_2F54
    RTEndGlobal
    VMReturn

Script_17:
    VMCall L_29E5
    RTEndGlobal
    VMReturn

Script_18:
    WorkSetConst 0x803b, 0
    WorkGet 0x803b, 0x8008
    // "What kinds of Pokémon\nshould I enter?"
    // "What kinds of Pokémon\nshould I enter?"
    ActorMsgGendered 1024, Global10330_Text_WhatKindsPokemonShould_2, Global10330_Text_WhatKindsPokemonShould, 0x803b, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    ListMenuAdd 71, 65535, 0
    ListMenuAdd 72, 65535, 1
    ListMenuAdd 73, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_333E
    VMJump L_3362

L_333E:
    BSubwayCmd_Tool 314, 0, 0, 0
    // "OK. I'll focus on Attack!\nLet's show them we are the best pair![f000]븁\u0000"
    // "OK.\nI'll focus on Attack![f000]븁\u0000"
    ActorMsgGendered 1024, Global10330_Text_OkIllFocusAttack_2, Global10330_Text_OkIllFocusAttack, 0x803b, 2, 0
    WorkSetConst 0x8010, 1
    VMJump L_33B7

L_3362:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_3375
    VMJump L_3399

L_3375:
    BSubwayCmd_Tool 314, 1, 0, 0
    // "OK. I'll focus on Defense!\nLet's show them we are the best pair![f000]븁\u0000"
    // "OK.\nI'll focus on Defense![f000]븁\u0000"
    ActorMsgGendered 1024, Global10330_Text_OkIllFocusDefense_2, Global10330_Text_OkIllFocusDefense, 0x803b, 2, 0
    WorkSetConst 0x8010, 1
    VMJump L_33B7

L_3399:
    BSubwayCmd_Tool 314, 2, 0, 0
    // "OK. I'll focus on a balance between\nAttack and Defense.[f000]븀\u0000\nLet's show them we are the best pair![f000]븁\u0000"
    // "OK. I'll focus on a balance between\nAttack and Defense![f000]븁\u0000"
    ActorMsgGendered 1024, Global10330_Text_OkIllFocusBalance_2, Global10330_Text_OkIllFocusBalance, 0x803b, 2, 0
    WorkSetConst 0x8010, 1

L_33B7:
    MsgWinCloseAll
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_33D6
    BSubwayCmd_Tool 335, 0, 0, 0

L_33D6:
    RTEndGlobal
    WorkSetConst 0x803b, 0
    VMReturn

Script_19:
    VMStackPush 0x8009
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_3437
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3416
    BSubwayCmd_Tool 336, 0, 0, 32802
    VMJump L_3420

L_3416:
    BSubwayCmd_Tool 336, 1, 0, 32802

L_3420:
    WordSetPokeSpecies 0, 0x8022
    // "Both Trainers have chosen the\nPokémon [f000]ā\u0001\u0000.[f000]븁\u0000\nPlease confer with the other Trainer\nand choose different Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10330_Text_BothTrainersHaveChosen, 0x8008, 2, 0
    VMJump L_3461

L_3437:
    BSubwayCmd_Tool 336, 0, 0, 32802
    WordSetPokeSpecies 0, 0x8022
    BSubwayCmd_Tool 336, 1, 0, 32802
    WordSetPokeSpecies 1, 0x8022
    // "Both Trainers have chosen the Pokémon\n[f000]ā\u0001\u0000 and [f000]ā\u0001\u0001.[f000]븁\u0000\nPlease confer with the other Trainer\nand choose different Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10330_Text_BothTrainersHaveChosen_2, 0x8008, 2, 0

L_3461:
    MsgWinCloseAll
    RTEndGlobal
    VMReturn

Script_10:
    BSubwayCmd_Tool 34, 0, 0, 0
    VMHalt

Script_12:
    ActorsPauseAll
    VMCall L_1F5C
    WorkSetConst 0x8008, 1
    VMCall L_34A7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8008, 0
    VMCall L_34A7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_34A7:
    WorkSetConst 0x803c, 0
    WorkGet 0x803c, 0x8008
    WorkSetConst 0x8023, 109
    ActorMsg MSGFILE_SCRIPT, 0x8023, 0, 2, 0
    YesNoWin 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_34E8
    WorkSetConst 0x8023, 110
    VMJump L_3507

L_34E8:
    WorkSetConst 0x8023, 111
    VMStackPush 0x803c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3507
    WorkSetConst 0x8023, 112

L_3507:
    ActorMsg MSGFILE_SCRIPT, 0x8023, 0, 2, 0
    VMStackPush 0x8023
    VMStackPushConst 111
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3528
    LastKeyWait

L_3528:
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3563
    Cmd_0222 EVENT_WORK_0x4162
    VMCall L_3571
    FadeOutBlackQ
    FadeWait
    MapChangeCore ZONE_ANVILLE_TOWN, 22, 0, 54, 2
    FadeInBlackQ
    FadeWait
    CallPlaceNameDisp
    VMJump L_3569

L_3563:
    VMCall L_1FFC

L_3569:
    WorkSetConst 0x803c, 0
    VMReturn

L_3571:
    BSubwayCmd_Tool 23, 255, 0, 0
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_3592
    VMJump L_35A8

L_3592:
    ActorCmdExec 0, Movement_2584
    ActorCmdExec 255, Movement_25E4
    VMJump L_360A

L_35A8:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_35BB
    VMJump L_35D1

L_35BB:
    ActorCmdExec 0, Movement_2578
    ActorCmdExec 255, Movement_25CC
    VMJump L_360A

L_35D1:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_35E4
    VMJump L_35FA

L_35E4:
    ActorCmdExec 0, Movement_259C
    ActorCmdExec 255, Movement_2610
    VMJump L_360A

L_35FA:
    ActorCmdExec 0, Movement_2590
    ActorCmdExec 255, Movement_25FC

L_360A:
    ActorCmdWait
    BSubwayCmd_Tool 19, 3, 0, 0
    SEPlay SEQ_SE_BDEMO_01
    VMSleep 20
    SEPlay SEQ_SE_BDEMO_02
    VMSleep 30
    VMReturn

Script_13:
    WorkSetConst 0x803d, 0
    WorkSetConst 0x803d, 0
    BSubwayCmd_Tool 23, 255, 0, 0
    ActorSetGPos 255, 16, 0, 13, 1
    BSubwayCmd_Tool 13, 255, 1, 0
    ActorCmdExec 0x803d, Movement_2834
    ActorCmdWait
    FadeInBlackQ
    FadeWait
    VMSleep 30
    ActorCmdExec 0x803d, Movement_283C
    ActorCmdExec 255, Movement_2854
    ActorCmdWait
    BSubwayCmd_Tool 24, 255, 0, 0
    RTEndGlobal
    WorkSetConst 0x803d, 0
    VMReturn

Script_14:
    FadeOutBlackQ
    FadeWait
    MapChangeCore ZONE_BATTLE_SUBWAY, 7, 0, 4, 0
    VMHalt
    .balign 4, 0
