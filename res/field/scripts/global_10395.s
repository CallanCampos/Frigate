#include "asm/field_script.inc"
#include "text/script/global_10395.h"

// Script plugin 13, from the zones that start its scripts

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

Script_1:
    VMCall L_0059
    VMHalt

Script_2:
    FieldGetContinueFlag 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0057
    VMCall L_0059

L_0057:
    VMHalt

L_0059:
    Cmd_01DB 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009F
    FlagSet EVENT_FLAG_0x0266
    Cmd_01DB 6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0099
    Cmd_01DB 5, 0x8020
    ActorDelete 0x8020

L_0099:
    VMJump L_00DE

L_009F:
    FlagReset EVENT_FLAG_0x0266
    Cmd_01DB 6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CE
    Cmd_01DB 5, 0x8020
    VMCall L_00E0
    VMJump L_00DE

L_00CE:
    Cmd_01DB 5, 0x8020
    ActorAdd 0x8020
    VMCall L_00E0

L_00DE:
    VMReturn

L_00E0:
    ActorGetGPos 0x8020, 0x8023, 0x8024
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPush 0x8023
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0123
    WorkAdd 0x8023, 1
    ActorSetGPos 0x8020, 0x8023, 0, 0x8024, 1

L_0123:
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0139
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0139:
    Cmd_01DB 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0166
    DebugPrint 0x8010
    // "I haven't received any other gifts\nfor you.[f000]븁\u0000\nWe look forward to your next visit."
    ParentActorMsg MSGFILE_SCRIPT, Global10395_Text_HaventReceivedAnyOther, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_0166:
    WordSetPlayerName 0
    RTCGetDayPart 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0180
    VMJump L_0190

L_0180:
    // "Good morning. You must be [f000]Ā\u0001\u0000.[f000]븁\u0000\nI've received a Mystery Gift for you.\nHere you go![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10395_Text_GoodMorningMustIve, 0, 0
    VMJump L_01BD

L_0190:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_01A3
    VMJump L_01B3

L_01A3:
    // "Good day. You must be [f000]Ā\u0001\u0000.[f000]븁\u0000\nI've received a Mystery Gift for you.\nHere you go![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10395_Text_GoodDayMustIve, 0, 0
    VMJump L_01BD

L_01B3:
    // "Good evening. You must be [f000]Ā\u0001\u0000.[f000]븁\u0000\nI've received a Mystery Gift for you.\nHere you go![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10395_Text_GoodEveningMustIve, 0, 0

L_01BD:
    ActorMsgClose
    Cmd_01DB 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F6
    VMCall L_0208
    Cmd_02C5 29
    // "We look forward to your next visit."
    ParentActorMsg MSGFILE_SCRIPT, Global10395_Text_WeLookForwardNext, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0206

L_01F6:
    Cmd_01DB 3, 0x8025
    SystemMsg 0x8025, 2
    LastKeyWait
    InfoMsgClose

L_0206:
    VMReturn

L_0208:
    Cmd_01DB 2, 0x8025
    Cmd_01DB 7, 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0227
    VMJump L_0239

L_0227:
    MEPlay SEQ_ME_POKEGET
    SystemMsg 0x8025, 0
    MEWait
    VMJump L_02F6

L_0239:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_024C
    VMJump L_025E

L_024C:
    MEPlay SEQ_ME_TAMAGO_GET
    SystemMsg 0x8025, 0
    MEWait
    VMJump L_02F6

L_025E:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0271
    VMJump L_0283

L_0271:
    RTCallGlobal 2808
    SystemMsg 0x8025, 0
    MEWait
    VMJump L_02F6

L_0283:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_0296
    VMJump L_02AC

L_0296:
    MEPlay SEQ_ME_KEYITEM
    PlayFieldEffect 54
    SystemMsg 0x8025, 0
    MEWait
    VMJump L_02F6

L_02AC:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_02BF
    VMJump L_02D1

L_02BF:
    SEPlay SEQ_SE_FLD_133
    SystemMsg 0x8025, 0
    SEWait
    VMJump L_02F6

L_02D1:
    WorkCmpConst 0x8010, 6
    VMJumpIf CMP_EQ, L_02E4
    VMJump L_02F6

L_02E4:
    SEPlay SEQ_SE_FLD_133
    SystemMsg 0x8025, 0
    SEWait
    VMJump L_02F6

L_02F6:
    MsgWaitAdvance
    InfoMsgClose
    Cmd_01DB 4, 0x8010
    VMReturn
    .balign 4, 0
