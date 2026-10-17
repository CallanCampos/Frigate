#include "asm/field_script.inc"
#include "text/script/global_10530.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8023, 1
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8022, 0

L_003C:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0143
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_0062
    VMJump L_0074

L_0062:
    VMCall L_0155
    WorkGet 0x8024, 0x8020
    VMJump L_013D

L_0074:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_0087
    VMJump L_009F

L_0087:
    WorkSetConst 0x8021, 0
    VMCall L_01EE
    WorkSetConst 0x8024, 4
    VMJump L_013D

L_009F:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_00B2
    VMJump L_00CA

L_00B2:
    WorkSetConst 0x8021, 1
    VMCall L_01EE
    WorkSetConst 0x8024, 4
    VMJump L_013D

L_00CA:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_00DD
    VMJump L_00F5

L_00DD:
    WorkSetConst 0x8021, 2
    VMCall L_01EE
    WorkSetConst 0x8024, 4
    VMJump L_013D

L_00F5:
    WorkCmpConst 0x8024, 4
    VMJumpIf CMP_EQ, L_0108
    VMJump L_011E

L_0108:
    Cmd_02C5 10
    VMCall L_01BB
    WorkGet 0x8024, 0x8020
    VMJump L_013D

L_011E:
    WorkCmpConst 0x8024, 255
    VMJumpIf CMP_EQ, L_0131
    VMJump L_013D

L_0131:
    WorkSetConst 0x8023, 0
    VMJump L_013D

L_013D:
    VMJump L_003C

L_0143:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0155:
    WorkSetConst 0x8025, 0
    // "Which channel will you watch?"
    SystemMsg Global10530_Text_WhichChannelWillWatch, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32805
    ListMenuAdd 189, 65535, 1
    ListMenuAdd 190, 65535, 2
    ListMenuAdd 191, 65535, 3
    ListMenuAdd 192, 65535, 255
    ListMenuShow
    InfoMsgClose
    VMStackPush 0x8025
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AD
    WorkSetConst 0x8020, 255
    VMJump L_01B3

L_01AD:
    WorkGet 0x8020, 0x8025

L_01B3:
    WorkSetConst 0x8025, 0
    VMReturn

L_01BB:
    // "Keep watching?"
    SystemMsg Global10530_Text_KeepWatching, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E4
    WorkSetConst 0x8020, 0
    VMJump L_01EA

L_01E4:
    WorkSetConst 0x8020, 255

L_01EA:
    InfoMsgClose
    VMReturn

L_01EE:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    RecordAdd 23, 1
    HollowRivalCmd_022A 0x8021, 0x8026
    SystemMsg 0x8026, 2
    LastKeyWait
    TVCheckCommercial 0x8022, 0x8027
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0239
    WorkSetConst 0x8022, 1
    TVGenCommercialMsgID 0x8026
    SystemMsg 0x8026, 2
    LastKeyWait

L_0239:
    InfoMsgClose
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    VMReturn
    .balign 4, 0
