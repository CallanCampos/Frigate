#include "asm/field_script.inc"
#include "text/script/global_10685.h"

// Script plugin 8, from the only plugin whose commands it decodes with

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
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8038, 0

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 2
    Plugin8_Cmd1003 50, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1007 8, 255, 0, 0
    Plugin8_Cmd1003 51, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1002 3, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0187
    Plugin8_Cmd1003 57, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8026
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    Plugin8_Cmd1026
    RecordAdd 130, 1
    VMJump L_0189

L_0187:
    ActorMsgClose

L_0189:
    WorkSetConst 0x8021, 1

L_018F:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024F
    VMCall L_0257
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_01BB
    VMJump L_01C7

L_01BB:
    VMCall L_034B
    VMJump L_0249

L_01C7:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_01DA
    VMJump L_01E6

L_01DA:
    VMCall L_05B6
    VMJump L_0249

L_01E6:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_01F9
    VMJump L_0205

L_01F9:
    VMCall L_06E6
    VMJump L_0249

L_0205:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0218
    VMJump L_0224

L_0218:
    VMCall L_0979
    VMJump L_0249

L_0224:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_0237
    VMJump L_0243

L_0237:
    VMCall L_17EE
    VMJump L_0249

L_0243:
    WorkSetConst 0x8021, 0

L_0249:
    VMJump L_018F

L_024F:
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0257:
    WorkSetConst 0x8039, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x803b, 0
    // ""
    SystemMsg Global10685_Text_Empty_253, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 174, 235, 0
    Plugin8_Cmd1002 6, 0x803a
    VMStackPush 0x803a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A1
    ListMenuAdd 176, 237, 2

L_02A1:
    Plugin8_Cmd1002 1, 0x8039
    VMStackPush 0x8039
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C2
    ListMenuAdd 175, 236, 1

L_02C2:
    Plugin8_Cmd1002 41, 0x803b
    VMStackPush 0x803b
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0312
    Plugin8_Cmd1002 15, 0x8026
    Plugin8_Cmd1002 13, 0x8027
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0312
    ListMenuAdd 177, 238, 3

L_0312:
    ListMenuAdd 183, 244, 7
    ListMenuShow
    InfoMsgClose
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0337
    WorkSetConst 0x8020, 7

L_0337:
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x8039, 0
    VMReturn

L_034B:
    WorkSetConst 0x803c, 0
    WorkSetConst 0x8030, 0
    Plugin8_Cmd1002 10, 0x803c
    Plugin8_Cmd1003 5, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 207, 65535, 0
    ListMenuAdd 208, 65535, 1
    ListMenuShow
    ActorMsgClose
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_03B1
    VMReturn

L_03B1:
    VMStackPush 0x803c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F7
    // "There is no room for a new building.[f000]븁\u0000\nPlease choose a building\nto be replaced.[f000]븁\u0000"
    SystemMsg Global10685_Text_ThereNoRoomNew_2, 2
    InfoMsgClose
    JoinAvenueStoreStart
    Plugin8Ov60_Cmd1011 1, 3, 0, 0x8029, 0x802a, 0x802b, 0x802c, 0x802d
    JoinAvenueStoreEnd
    VMStackPush 0x8029
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F7
    VMReturn

L_03F7:
    RecordAdd 128, 1
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    Plugin8_Cmd1007 18, 254, 0, 2
    Plugin8_Cmd1003 59, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    Plugin8_Cmd1007 15, 254, 0, 0
    Plugin8_Cmd1007 14, 254, 0, 1
    FadeOutBlack
    FadeWait
    VMStackPush 0x803c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0462
    Plugin8_Cmd1008 0, 0, 0x802c

L_0462:
    Plugin8_Cmd1005 0, 0x802e
    Plugin8_Cmd1008 0, 255, 0
    FieldClose
    FieldOpen
    EvCameraInit
    EvCameraUnbind
    WorkGet 0x8032, 0x802e
    VMCall L_1EFA
    FadeInBlack
    FadeWait
    // "...!![f000]븁\u0000"
    SystemMsg Global10685_Text_Empty_41, 2
    MEPlay SEQ_ME_AVENUE_01
    // "A nice [f000]Ł\u0001\u0000 called\n[f000]ĸ\u0001\u0001[f000]븀\u0000\nwas built!"
    SystemMsg Global10685_Text_NiceCalledBuilt, 2
    MEWait
    MsgWaitAdvance
    // "Let's visit\n[f000]ĸ\u0001\u0001![f000]븁\u0000"
    SystemMsg Global10685_Text_LetsVisit, 2
    InfoMsgClose
    VMCall L_17BC
    WorkCmpConst 0x802e, 0
    VMJumpIf CMP_EQ, L_04BD
    VMJump L_04C9

L_04BD:
    WorkSetConst EVENT_WORK_0x4000, 0
    VMJump L_05A2

L_04C9:
    WorkCmpConst 0x802e, 1
    VMJumpIf CMP_EQ, L_04DC
    VMJump L_04E8

L_04DC:
    WorkSetConst EVENT_WORK_0x4001, 0
    VMJump L_05A2

L_04E8:
    WorkCmpConst 0x802e, 2
    VMJumpIf CMP_EQ, L_04FB
    VMJump L_0507

L_04FB:
    WorkSetConst EVENT_WORK_0x4002, 0
    VMJump L_05A2

L_0507:
    WorkCmpConst 0x802e, 3
    VMJumpIf CMP_EQ, L_051A
    VMJump L_0526

L_051A:
    WorkSetConst EVENT_WORK_0x4003, 0
    VMJump L_05A2

L_0526:
    WorkCmpConst 0x802e, 4
    VMJumpIf CMP_EQ, L_0539
    VMJump L_0545

L_0539:
    WorkSetConst EVENT_WORK_0x4004, 0
    VMJump L_05A2

L_0545:
    WorkCmpConst 0x802e, 5
    VMJumpIf CMP_EQ, L_0558
    VMJump L_0564

L_0558:
    WorkSetConst EVENT_WORK_0x4005, 0
    VMJump L_05A2

L_0564:
    WorkCmpConst 0x802e, 6
    VMJumpIf CMP_EQ, L_0577
    VMJump L_0583

L_0577:
    WorkSetConst EVENT_WORK_0x4006, 0
    VMJump L_05A2

L_0583:
    WorkCmpConst 0x802e, 7
    VMJumpIf CMP_EQ, L_0596
    VMJump L_05A2

L_0596:
    WorkSetConst EVENT_WORK_0x4007, 0
    VMJump L_05A2

L_05A2:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8030, 1
    WorkSetConst 0x803c, 0
    VMReturn

L_05B6:
    Plugin8_Cmd1002 16, 0x8026
    WorkCmpConst 0x8026, 0
    VMJumpIf CMP_EQ, L_05CF
    VMJump L_068A

L_05CF:
    Plugin8_Cmd1007 18, 254, 0, 0
    Plugin8_Cmd1003 52, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    // "Please choose an assistant\nto be replaced.[f000]븁\u0000"
    SystemMsg Global10685_Text_PleaseChooseAssistantReplaced, 2
    InfoMsgClose
    JoinAvenueStoreStart
    Plugin8Ov60_Cmd1011 1, 1, 0, 0x8029, 0x802a, 0x802b, 0x802c, 0x802d
    JoinAvenueStoreEnd
    VMStackPush 0x8029
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0684
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    Plugin8_Cmd1007 18, 254, 0, 2
    Plugin8_Cmd1003 53, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    Plugin8_Cmd1008 0, 3, 0x802c
    Plugin8_Cmd1005 1, 0x802e
    Plugin8_Cmd1008 0, 255, 0
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkSetConst 0x8021, 0

L_0684:
    VMJump L_06E4

L_068A:
    WorkCmpConst 0x8026, 1
    VMJumpIf CMP_EQ, L_069D
    VMJump L_06B7

L_069D:
    Plugin8_Cmd1003 54, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    VMJump L_06E4

L_06B7:
    WorkCmpConst 0x8026, 2
    VMJumpIf CMP_EQ, L_06CA
    VMJump L_06E4

L_06CA:
    Plugin8_Cmd1003 55, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    VMJump L_06E4

L_06E4:
    VMReturn

L_06E6:
    WorkSetConst 0x803d, 0
    WorkSetConst 0x803e, 0
    WorkSetConst 0x803f, 0
    WorkSetConst 0x8022, 1

L_06FE:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0965
    // "What do you want to hear?"
    SystemMsg Global10685_Text_WhatWantHear, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    Plugin8_Cmd1002 15, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0741
    ListMenuAdd 210, 65535, 0

L_0741:
    Plugin8_Cmd1002 13, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0762
    ListMenuAdd 211, 65535, 1

L_0762:
    ListMenuAdd 222, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0785
    WorkSetConst 0x8020, 2

L_0785:
    InfoMsgClose
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_079A
    VMJump L_08D8

L_079A:
    Plugin8_Cmd1002 15, 0x803f
    WorkSetConst 0x803e, 0
    VMStackPush 0x803f
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_08AA

L_07B9:
    VMStackPush 0x803e
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_08A4
    WorkGet 0x803d, 0x803e
    WorkAdd 0x803d, 0
    Plugin8_Cmd1007 6, 254, 0, 0
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1003 0x803d, 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0898
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    WorkAdd 0x803e, 1
    WorkSetConst 0x8026, 0
    WorkGet 0x802e, 0x803e

L_0823:
    VMStackPush 0x802e
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_086D
    WorkGet 0x803d, 0x802e
    WorkAdd 0x803d, 0
    Plugin8_Cmd1003 0x803d, 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0861
    WorkSetConst 0x8026, 1

L_0861:
    WorkAdd 0x802e, 1
    VMJump L_0823

L_086D:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0892
    Plugin8_Cmd1003 10, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_0892:
    VMJump L_089E

L_0898:
    WorkAdd 0x803e, 1

L_089E:
    VMJump L_07B9

L_08A4:
    VMJump L_08D0

L_08AA:
    Plugin8_Cmd1007 6, 254, 0, 0
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1003 11, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_08D0:
    ActorMsgClose
    VMJump L_095F

L_08D8:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_08EB
    VMJump L_093A

L_08EB:
    Plugin8_Cmd1002 13, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0920
    Plugin8_Cmd1003 8, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    Plugin8_Cmd1036
    VMJump L_0934

L_0920:
    Plugin8_Cmd1003 9, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose

L_0934:
    VMJump L_095F

L_093A:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_094D
    VMJump L_0959

L_094D:
    WorkSetConst 0x8022, 0
    VMJump L_095F

L_0959:
    WorkSetConst 0x8022, 0

L_095F:
    VMJump L_06FE

L_0965:
    WorkSetConst 0x803f, 0
    WorkSetConst 0x803e, 0
    WorkSetConst 0x803d, 0
    VMReturn

L_0979:
    WorkSetConst 0x8040, 0
    WorkSetConst 0x8041, 0
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8038, 0
    Plugin8_Cmd1003 4, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    JoinAvenueStoreStart
    Plugin8Ov60_Cmd1011 1, 2, 0, 0x8029, 0x802a, 0x802b, 0x802c, 0x802d
    JoinAvenueStoreEnd
    VMStackPush 0x8029
    VMStackPushConst 255
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0B72
    RecordAdd 127, 1
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 18, 254, 0, 1
    Plugin8_Cmd1003 56, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    Plugin8_Cmd1022 0x802c, 0x8035, 0x8024, 0x8025, 0x8040
    WorkGet 0x8034, 0x8040
    WorkGet 0x8041, 0x8040
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    Plugin8_Cmd1024 0x802c, 4, 0x8026, 0x8026
    VMCall L_0B86
    VMStackPush 0x8033
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0A59
    WorkSetConst 0x8042, 1
    VMJump L_0A5F

L_0A59:
    WorkSetConst 0x8042, 0

L_0A5F:
    VMStackPush 0x8033
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ACB
    WorkGet 0x802c, 0x8037
    Plugin8_Cmd1024 0x8037, 2, 0x8035, 0x8026
    Plugin8_Cmd1024 0x8035, 3, 0x8024, 0x8026
    Plugin8_Cmd1003 62, 0x8025
    Plugin8_Cmd1024 0x8040, 1, 0x8034, 0x8026
    VMStackPush 0x8035
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AB5
    WorkGet 0x8041, 0x8034

L_0AB5:
    Plugin8_Cmd1024 0x8037, 5, 0x8026, 0x8026
    VMCall L_0B86
    VMJump L_0A5F

L_0ACB:
    VMStackPush 0x8042
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AEC
    WorkGet 0x8034, 0x8041
    VMCall L_10B2
    InfoMsgClose

L_0AEC:
    VMStackPush 0x8031
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B0B
    ActorSetGPos 255, 15, 0, 71, 0

L_0B0B:
    VMCall L_20B4
    Plugin8_Cmd1008 0, 255, 0
    Plugin8_Cmd1039 0
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin8_Cmd1031 28, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8038
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0B66
    // "To celebrate the building of so many\nwonderful shops, all the shops decided[f000]븀\u0000\nto have a promotion![f000]븁\u0000\nIt lasts for 7 days!\nCheck out all the shops!"
    SystemMsg Global10685_Text_CelebrateBuildingManyWonderful, 2
    LastKeyWait
    InfoMsgClose

L_0B66:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8030, 1

L_0B72:
    WorkSetConst 0x8042, 0
    WorkSetConst 0x8041, 0
    WorkSetConst 0x8040, 0
    VMReturn

L_0B86:
    WorkSetConst 0x8043, 0
    WorkSetConst 0x8044, 0
    WorkGet 0x8032, 0x802c
    VMCall L_120C
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 1, 0, 0x802c, 1
    // "[f000]Ā\u0001\u0000 has come to\n[f000]ĸ\u0001\u0001![f000]븁\u0000"
    SystemMsg Global10685_Text_HasCome, 2
    Plugin8_Cmd1024 0x802c, 0, 0x8036, 0x8037
    Plugin8_Cmd1002 60, 0x8026
    Plugin8_Cmd1031 27, 0x8027
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0D9C
    WorkCmpConst 0x8036, 0
    VMJumpIf CMP_EQ, L_0C10
    VMJump L_0C9C

L_0C10:
    Random 0x8026, 20
    WorkSetConst 0x8043, 0
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0C3B
    Plugin8_Cmd1003 93, 0x8043
    VMJump L_0C73

L_0C3B:
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0C5A
    Plugin8_Cmd1003 94, 0x8043
    VMJump L_0C73

L_0C5A:
    VMStackPush 0x8026
    VMStackPushConst 14
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0C73
    Plugin8_Cmd1003 95, 0x8043

L_0C73:
    VMStackPush 0x8043
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0C96
    Plugin8_Cmd1007 0, 0, 0x802c, 0
    SystemMsg 0x8043, 2

L_0C96:
    VMJump L_0D9C

L_0C9C:
    WorkCmpConst 0x8036, 1
    VMJumpIf CMP_EQ, L_0CAF
    VMJump L_0D1C

L_0CAF:
    Random 0x8026, 8
    WorkSetConst 0x8043, 0
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0CDA
    Plugin8_Cmd1003 94, 0x8043
    VMJump L_0CF3

L_0CDA:
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0CF3
    Plugin8_Cmd1003 95, 0x8043

L_0CF3:
    VMStackPush 0x8043
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0D16
    Plugin8_Cmd1007 0, 0, 0x802c, 0
    SystemMsg 0x8043, 2

L_0D16:
    VMJump L_0D9C

L_0D1C:
    WorkCmpConst 0x8036, 2
    VMJumpIf CMP_EQ, L_0D2F
    VMJump L_0D9C

L_0D2F:
    Random 0x8026, 12
    WorkSetConst 0x8043, 0
    VMStackPush 0x8026
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0D5A
    Plugin8_Cmd1003 94, 0x8043
    VMJump L_0D73

L_0D5A:
    VMStackPush 0x8026
    VMStackPushConst 7
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0D73
    Plugin8_Cmd1003 95, 0x8043

L_0D73:
    VMStackPush 0x8043
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0D96
    Plugin8_Cmd1007 0, 0, 0x802c, 0
    SystemMsg 0x8043, 2

L_0D96:
    VMJump L_0D9C

L_0D9C:
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 0, 0, 0x802c, 2
    Plugin8_Cmd1007 6, 0, 0x802c, 3
    // "[f000]Ā\u0001\u0002: [f000]ķ\u0001\u0003[f000]븁\u0000"
    SystemMsg Global10685_Text_Empty_48, 2
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 4
    Plugin8_Cmd1003 60, 0x8043
    SystemMsg 0x8043, 2
    VMStackPush 0x8035
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E19
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 1, 0, 0x802c, 1
    SystemMsg 0x8024, 2
    WorkSetConst 0x8033, 0
    VMJump L_1090

L_0E19:
    WorkSetConst 0x8033, 1
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 1, 0, 0x802c, 1
    Plugin8_Cmd1007 0, 0, 0x802c, 2
    Plugin8_Cmd1002 60, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E5C
    SystemMsg 0x8025, 2

L_0E5C:
    Plugin8_Cmd1007 0, 254, 0, 0
    SystemMsg 0x8024, 2
    Plugin8_Cmd1009 0x802c, 0x8034, 0x8026, 0x8044
    WorkAdd 0x8044, 1
    VMStackPush 0x8026
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0F61
    WorkSetConst 0x8038, 1
    WordSetNumber 5, 0x8034, 5
    // "[f000]ĸ\u0001\u0001's\npopularity went up by [f000]Ȃ\u0001\u0005 points![f000]븁\u0000"
    SystemMsg Global10685_Text_SPopularityWentUp, 2
    SEWait
    SEPlay SEQ_SE_SW_JA_EXP
    SEWait
    WordSetNumber 6, 0x8044, 2
    WorkCmpConst 0x8026, 1
    VMJumpIf CMP_EQ, L_0EC4
    VMJump L_0EE4

L_0EC4:
    // "...!![f000]븂\u0001<"
    SystemMsg Global10685_Text_Empty_45, 2
    MEPlay SEQ_ME_AVENUE_01
    // "[f000]ĸ\u0001\u0001 reached\nRank [f000]Ȃ\u0001\u0006!"
    SystemMsg Global10685_Text_ReachedRank, 2
    MEWait
    MsgWaitAdvance
    // "[f000]ĸ\u0001\u0001 has\nupgraded services![f000]븁\u0000"
    SystemMsg Global10685_Text_HasUpgradedServices, 2
    VMJump L_0F61

L_0EE4:
    WorkCmpConst 0x8026, 2
    VMJumpIf CMP_EQ, L_0EF7
    VMJump L_0F61

L_0EF7:
    // "...!![f000]븂\u0001<"
    SystemMsg Global10685_Text_Empty_45, 2
    MEPlay SEQ_ME_AVENUE_01
    // "[f000]ĸ\u0001\u0001 reached\nRank [f000]Ȃ\u0001\u0006!"
    SystemMsg Global10685_Text_ReachedRank, 2
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    FadeOutWhite
    FadeWait
    VMCall L_17DC
    FieldClose
    FieldOpen
    EvCameraInit
    EvCameraUnbind
    Plugin8_Cmd1006 0x802c
    WorkGet 0x8032, 0x802c
    VMCall L_1EFA
    FadeInWhite
    FadeWait
    Plugin8_Cmd1007 1, 0, 0x802c, 0
    MEPlay SEQ_ME_AVENUE_02
    // "[f000]ĸ\u0001\u0001\nis now [f000]ĸ\u0001\u0000!"
    SystemMsg Global10685_Text_Now, 2
    MEWait
    MsgWaitAdvance
    Plugin8_Cmd1007 1, 0, 0x802c, 1
    // "[f000]ĸ\u0001\u0001 has\nupgraded services![f000]븁\u0000"
    SystemMsg Global10685_Text_HasUpgradedServices, 2
    VMJump L_0F61

L_0F61:
    WorkCmpConst 0x8036, 0
    VMJumpIf CMP_EQ, L_0F74
    VMJump L_1012

L_0F74:
    Plugin8_Cmd1007 0, 0, 0x802c, 0
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1007 6, 0, 0x802c, 2
    Plugin8_Cmd1003 89, 0x8024
    SystemMsg 0x8024, 2
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1007 18, 254, 0, 2
    Plugin8_Cmd1007 6, 254, 0, 3
    Plugin8_Cmd1003 91, 0x8024
    SystemMsg 0x8024, 2
    Plugin8_Cmd1007 0, 0, 0x802c, 0
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1007 1, 0, 0x8037, 2
    Plugin8_Cmd1007 6, 254, 0, 3
    Plugin8_Cmd1007 18, 0, 0x802c, 4
    Plugin8_Cmd1003 90, 0x8024
    SystemMsg 0x8024, 2
    WorkSetConst 0x8033, 2
    VMJump L_1090

L_1012:
    WorkCmpConst 0x8036, 1
    VMJumpIf CMP_EQ, L_1025
    VMJump L_1031

L_1025:
    WorkSetConst 0x8033, 1
    VMJump L_1090

L_1031:
    WorkCmpConst 0x8036, 2
    VMJumpIf CMP_EQ, L_1044
    VMJump L_1090

L_1044:
    Plugin8_Cmd1007 0, 0, 0x802c, 0
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1007 6, 0, 0x802c, 2
    Plugin8_Cmd1003 89, 0x8024
    SystemMsg 0x8024, 2
    Plugin8_Cmd1007 18, 254, 0, 3
    Plugin8_Cmd1003 92, 0x8024
    SystemMsg 0x8024, 2
    WorkSetConst 0x8033, 1
    VMJump L_1090

L_1090:
    InfoMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    VMCall L_20B4
    WorkSetConst 0x8044, 0
    WorkSetConst 0x8043, 0
    VMReturn

L_10B2:
    WorkSetConst 0x8045, 0
    WorkSetConst 0x8046, 0
    Plugin8_Cmd1010 0x8034, 0x8045, 0x8046
    VMStackPush 0x8045
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_11F4
    VMCall L_20B4
    ActorCmdExec 0x8023, Movement_1204
    ActorCmdWait
    VMCall L_179C
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin8_Cmd1007 8, 255, 0, 0
    WordSetNumber 7, 0x8034, 5
    // "[f000]Ĺ\u0001\u0000's\npopularity went up by [f000]ȃ\u0001\u0007 points![f000]븁\u0000"
    SystemMsg Global10685_Text_SPopularityWentUp_2, 2
    SEWait
    SEPlay SEQ_SE_SW_JA_EXP
    SEWait
    VMStackPush 0x8045
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8045
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8045
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPush 0x8045
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_11E8
    Plugin8_Cmd1037 0
    WordSetNumber 8, 0x8046, 4
    // "...!![f000]븂\u0001<"
    SystemMsg Global10685_Text_Empty_50, 2
    MEPlay SEQ_ME_AVENUE_02
    // "[f000]Ĺ\u0001\u0000 reached\nRank [f000]ȃ\u0001\b!"
    SystemMsg Global10685_Text_ReachedRank_2, 2
    MEWait
    MsgWaitAdvance
    VMStackPush 0x8045
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPush 0x8045
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_11AF
    Plugin8_Cmd1007 12, 255, 0, 2
    // "This place's reputation as\n“[f000]ł\u0001\u0002\"[f000]븀\u0000\nis spreading![f000]븁\u0000"
    SystemMsg Global10685_Text_PlacesReputationSpreading, 2

L_11AF:
    VMStackPush 0x8045
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8045
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_11E8
    Plugin8_Cmd1007 4, 255, 0, 0
    // "...Oh?[f000]븁\u0000\nThe owner is here![f000]븁\u0000\nGo to [f000]ĺ\u0001\u0000's office.[f000]븁\u0000"
    SystemMsg Global10685_Text_OhOwnerHereGo, 2
    WorkSetConst EVENT_WORK_0x4119, 1

L_11E8:
    FadeEx 3, 0, 16, 2
    FadeExWait

L_11F4:
    WorkSetConst 0x8046, 0
    WorkSetConst 0x8045, 0
    VMReturn
    .balign 4, 0

Movement_1204:
    Move 69, 1
    MoveEnd

L_120C:
    WorkSetConst 0x8047, 0
    WorkGet 0x8047, 0x8032
    WorkAdd 0x8047, 52
    Plugin8_Cmd1002 0x8047, 0x802f
    VMCall L_1232
    WorkSetConst 0x8047, 0
    VMReturn

L_1232:
    VMCall L_20A4
    WorkCmpConst 0x8032, 0
    VMJumpIf CMP_EQ, L_124B
    VMJump L_12E4

L_124B:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_125E
    VMJump L_127C

L_125E:
    EvCameraMoveTo 6821, 53376, 0x51000, 0x138000, 0x1a000, 0x408000, 1
    VMJump L_12DE

L_127C:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_128F
    VMJump L_12AD

L_128F:
    EvCameraMoveTo 5797, 54016, 0x69000, 0x138000, 0x21000, 0x40a000, 1
    VMJump L_12DE

L_12AD:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_12C0
    VMJump L_12DE

L_12C0:
    EvCameraMoveTo 4517, 52480, 0x89000, 0x138000, 0x2a000, 0x402000, 1
    VMJump L_12DE

L_12DE:
    VMJump L_1798

L_12E4:
    WorkCmpConst 0x8032, 1
    VMJumpIf CMP_EQ, L_12F7
    VMJump L_1390

L_12F7:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_130A
    VMJump L_1328

L_130A:
    EvCameraMoveTo 6693, 12288, 0x51000, 0xb8000, 0x17000, 0x397000, 1
    VMJump L_138A

L_1328:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_133B
    VMJump L_1359

L_133B:
    EvCameraMoveTo 6693, 11776, 0x69000, 0xb8000, 0x26000, 0x39e000, 1
    VMJump L_138A

L_1359:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_136C
    VMJump L_138A

L_136C:
    EvCameraMoveTo 4773, 13696, 0x89000, 0xb8000, 0x2c000, 0x38e000, 1
    VMJump L_138A

L_138A:
    VMJump L_1798

L_1390:
    WorkCmpConst 0x8032, 2
    VMJumpIf CMP_EQ, L_13A3
    VMJump L_143C

L_13A3:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_13B6
    VMJump L_13D4

L_13B6:
    EvCameraMoveTo 6821, 53376, 0x51000, 0x138000, 0x1a000, 0x328000, 1
    VMJump L_1436

L_13D4:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_13E7
    VMJump L_1405

L_13E7:
    EvCameraMoveTo 5797, 54016, 0x69000, 0x138000, 0x21000, 0x32a000, 1
    VMJump L_1436

L_1405:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_1418
    VMJump L_1436

L_1418:
    EvCameraMoveTo 4517, 52480, 0x89000, 0x138000, 0x2a000, 0x322000, 1
    VMJump L_1436

L_1436:
    VMJump L_1798

L_143C:
    WorkCmpConst 0x8032, 3
    VMJumpIf CMP_EQ, L_144F
    VMJump L_14E8

L_144F:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_1462
    VMJump L_1480

L_1462:
    EvCameraMoveTo 6693, 12288, 0x51000, 0xb8000, 0x17000, 0x2b7000, 1
    VMJump L_14E2

L_1480:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_1493
    VMJump L_14B1

L_1493:
    EvCameraMoveTo 6693, 11776, 0x69000, 0xb8000, 0x26000, 0x2be000, 1
    VMJump L_14E2

L_14B1:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_14C4
    VMJump L_14E2

L_14C4:
    EvCameraMoveTo 4773, 13696, 0x89000, 0xb8000, 0x2c000, 0x2ae000, 1
    VMJump L_14E2

L_14E2:
    VMJump L_1798

L_14E8:
    WorkCmpConst 0x8032, 4
    VMJumpIf CMP_EQ, L_14FB
    VMJump L_1594

L_14FB:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_150E
    VMJump L_152C

L_150E:
    EvCameraMoveTo 6821, 53376, 0x51000, 0x138000, 0x1a000, 0x248000, 1
    VMJump L_158E

L_152C:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_153F
    VMJump L_155D

L_153F:
    EvCameraMoveTo 5797, 54016, 0x69000, 0x138000, 0x21000, 0x24a000, 1
    VMJump L_158E

L_155D:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_1570
    VMJump L_158E

L_1570:
    EvCameraMoveTo 4517, 52480, 0x89000, 0x138000, 0x2a000, 0x242000, 1
    VMJump L_158E

L_158E:
    VMJump L_1798

L_1594:
    WorkCmpConst 0x8032, 5
    VMJumpIf CMP_EQ, L_15A7
    VMJump L_1640

L_15A7:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_15BA
    VMJump L_15D8

L_15BA:
    EvCameraMoveTo 6693, 12288, 0x51000, 0xb8000, 0x17000, 0x1d7000, 1
    VMJump L_163A

L_15D8:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_15EB
    VMJump L_1609

L_15EB:
    EvCameraMoveTo 6693, 11776, 0x69000, 0xb8000, 0x26000, 0x1de000, 1
    VMJump L_163A

L_1609:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_161C
    VMJump L_163A

L_161C:
    EvCameraMoveTo 4773, 13696, 0x89000, 0xb8000, 0x2c000, 0x1ce000, 1
    VMJump L_163A

L_163A:
    VMJump L_1798

L_1640:
    WorkCmpConst 0x8032, 6
    VMJumpIf CMP_EQ, L_1653
    VMJump L_16EC

L_1653:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_1666
    VMJump L_1684

L_1666:
    EvCameraMoveTo 6821, 53376, 0x51000, 0x138000, 0x1a000, 0x168000, 1
    VMJump L_16E6

L_1684:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_1697
    VMJump L_16B5

L_1697:
    EvCameraMoveTo 5797, 54016, 0x69000, 0x138000, 0x21000, 0x16a000, 1
    VMJump L_16E6

L_16B5:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_16C8
    VMJump L_16E6

L_16C8:
    EvCameraMoveTo 4517, 52480, 0x89000, 0x138000, 0x2a000, 0x162000, 1
    VMJump L_16E6

L_16E6:
    VMJump L_1798

L_16EC:
    WorkCmpConst 0x8032, 7
    VMJumpIf CMP_EQ, L_16FF
    VMJump L_1798

L_16FF:
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_1712
    VMJump L_1730

L_1712:
    EvCameraMoveTo 6693, 12288, 0x51000, 0xb8000, 0x17000, 0xf7000, 1
    VMJump L_1792

L_1730:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_1743
    VMJump L_1761

L_1743:
    EvCameraMoveTo 6693, 11776, 0x69000, 0xb8000, 0x26000, 0xfe000, 1
    VMJump L_1792

L_1761:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_1774
    VMJump L_1792

L_1774:
    EvCameraMoveTo 4773, 13696, 0x89000, 0xb8000, 0x2c000, 0xee000, 1
    VMJump L_1792

L_1792:
    VMJump L_1798

L_1798:
    EvCameraWait
    VMReturn

L_179C:
    EvCameraMoveTo 64037, 0, 0xe1000, 0xf8000, 0x24000, 0x247000, 1
    EvCameraWait
    Plugin8_Cmd1014 4
    VMReturn

L_17BC:
    FadeEx 3, 0, 16, 2
    FadeExWait
    VMCall L_17DC
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMReturn

L_17DC:
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMCall L_20B4
    VMReturn

L_17EE:
    Plugin8_Cmd1003 58, 0x8024
    Plugin8_Cmd1007 6, 254, 0, 1
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    VMCall L_1834
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_1834:
    WorkSetConst 0x8048, 0
    Plugin8_Cmd1003 80, 0x8024
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 4, 255, 0, 1
    Plugin8_Cmd1007 6, 254, 0, 2
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1003 88, 0x8024
    Plugin8_Cmd1002 40, 0x802e
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_19E4
    WorkCmpConst 0x802e, 0
    VMJumpIf CMP_EQ, L_189C
    VMJump L_18AE

L_189C:
    WorkGet 0x8026, EVENT_WORK_0x4000
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_19B1

L_18AE:
    WorkCmpConst 0x802e, 1
    VMJumpIf CMP_EQ, L_18C1
    VMJump L_18D3

L_18C1:
    WorkGet 0x8026, EVENT_WORK_0x4001
    WorkSetConst EVENT_WORK_0x4001, 1
    VMJump L_19B1

L_18D3:
    WorkCmpConst 0x802e, 2
    VMJumpIf CMP_EQ, L_18E6
    VMJump L_18F8

L_18E6:
    WorkGet 0x8026, EVENT_WORK_0x4002
    WorkSetConst EVENT_WORK_0x4002, 1
    VMJump L_19B1

L_18F8:
    WorkCmpConst 0x802e, 3
    VMJumpIf CMP_EQ, L_190B
    VMJump L_191D

L_190B:
    WorkGet 0x8026, EVENT_WORK_0x4003
    WorkSetConst EVENT_WORK_0x4003, 1
    VMJump L_19B1

L_191D:
    WorkCmpConst 0x802e, 4
    VMJumpIf CMP_EQ, L_1930
    VMJump L_1942

L_1930:
    WorkGet 0x8026, EVENT_WORK_0x4004
    WorkSetConst EVENT_WORK_0x4004, 1
    VMJump L_19B1

L_1942:
    WorkCmpConst 0x802e, 5
    VMJumpIf CMP_EQ, L_1955
    VMJump L_1967

L_1955:
    WorkGet 0x8026, EVENT_WORK_0x4005
    WorkSetConst EVENT_WORK_0x4005, 1
    VMJump L_19B1

L_1967:
    WorkCmpConst 0x802e, 6
    VMJumpIf CMP_EQ, L_197A
    VMJump L_198C

L_197A:
    WorkGet 0x8026, EVENT_WORK_0x4006
    WorkSetConst EVENT_WORK_0x4006, 1
    VMJump L_19B1

L_198C:
    WorkCmpConst 0x802e, 7
    VMJumpIf CMP_EQ, L_199F
    VMJump L_19B1

L_199F:
    WorkGet 0x8026, EVENT_WORK_0x4007
    WorkSetConst EVENT_WORK_0x4007, 1
    VMJump L_19B1

L_19B1:
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_19E4
    Plugin8_Cmd1007 18, 254, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1007 0, 254, 0, 0

L_19E4:
    Plugin8_Cmd1031 19, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_1A39
    Plugin8_Cmd1007 8, 255, 0, 0
    Plugin8_Cmd1007 17, 255, 0, 1
    Plugin8_Cmd1003 87, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    WorkSetConst EVENT_WORK_0x4008, 1

L_1A39:
    WorkSetConst 0x8021, 1

L_1A3F:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1CCA
    VMCall L_1CD2
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_1A6B
    VMJump L_1BA9

L_1A6B:
    Plugin8_Cmd1002 11, 0x8048
    WorkCmpConst 0x8048, 0
    VMJumpIf CMP_EQ, L_1A84
    VMJump L_1A90

L_1A84:
    VMCall L_20C4
    VMJump L_1B69

L_1A90:
    WorkCmpConst 0x8048, 1
    VMJumpIf CMP_EQ, L_1AA3
    VMJump L_1AAF

L_1AA3:
    VMCall L_2238
    VMJump L_1B69

L_1AAF:
    WorkCmpConst 0x8048, 2
    VMJumpIf CMP_EQ, L_1AC2
    VMJump L_1ACE

L_1AC2:
    VMCall L_2240
    VMJump L_1B69

L_1ACE:
    WorkCmpConst 0x8048, 3
    VMJumpIf CMP_EQ, L_1AE1
    VMJump L_1AED

L_1AE1:
    VMCall L_2248
    VMJump L_1B69

L_1AED:
    WorkCmpConst 0x8048, 4
    VMJumpIf CMP_EQ, L_1B00
    VMJump L_1B0C

L_1B00:
    VMCall L_2250
    VMJump L_1B69

L_1B0C:
    WorkCmpConst 0x8048, 5
    VMJumpIf CMP_EQ, L_1B1F
    VMJump L_1B2B

L_1B1F:
    VMCall L_2258
    VMJump L_1B69

L_1B2B:
    WorkCmpConst 0x8048, 6
    VMJumpIf CMP_EQ, L_1B3E
    VMJump L_1B4A

L_1B3E:
    VMCall L_2260
    VMJump L_1B69

L_1B4A:
    WorkCmpConst 0x8048, 7
    VMJumpIf CMP_EQ, L_1B5D
    VMJump L_1B69

L_1B5D:
    VMCall L_2268
    VMJump L_1B69

L_1B69:
    Plugin8_Cmd1007 4, 255, 0, 0
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1007 18, 254, 0, 2
    Plugin8_Cmd1003 81, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMJump L_1CC4

L_1BA9:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_1BBC
    VMJump L_1BCA

L_1BBC:
    ActorMsgClose
    VMCall L_06E6
    VMJump L_1CC4

L_1BCA:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_1BDD
    VMJump L_1C4C

L_1BDD:
    Plugin8_Cmd1003 96, 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_1C02
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_1C02:
    Plugin8_Cmd1007 9, 254, 0, 0
    Plugin8_Cmd1007 1, 254, 0, 1
    Plugin8_Cmd1007 10, 254, 0, 2
    Plugin8_Cmd1007 4, 255, 0, 3
    Plugin8_Cmd1007 18, 254, 0, 4
    Plugin8_Cmd1003 86, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMJump L_1CC4

L_1C4C:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_1C5F
    VMJump L_1C6B

L_1C5F:
    VMCall L_1D9A
    VMJump L_1CC4

L_1C6B:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_1C7E
    VMJump L_1CBE

L_1C7E:
    Plugin8_Cmd1007 4, 255, 0, 0
    Plugin8_Cmd1007 0, 254, 0, 1
    Plugin8_Cmd1007 18, 254, 0, 2
    Plugin8_Cmd1003 81, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMJump L_1CC4

L_1CBE:
    WorkSetConst 0x8021, 0

L_1CC4:
    VMJump L_1A3F

L_1CCA:
    WorkSetConst 0x8048, 0
    VMReturn

L_1CD2:
    WorkSetConst 0x8049, 0
    Plugin8_Cmd1003 6, 0x8024
    Plugin8_Cmd1007 1, 254, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1002 2, 0x8049
    Plugin8_Cmd1007 2, 254, 0, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 202, 65535, 0
    Plugin8_Cmd1002 15, 0x8026
    Plugin8_Cmd1002 13, 0x8027
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_1D4C
    ListMenuAdd 177, 65535, 1

L_1D4C:
    ListMenuAdd 182, 65535, 2
    VMStackPush 0x8049
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1D6F
    ListMenuAdd 209, 65535, 4

L_1D6F:
    ListMenuAdd 183, 65535, 3
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1D92
    WorkSetConst 0x8020, 3

L_1D92:
    WorkSetConst 0x8049, 0
    VMReturn

L_1D9A:
    WorkSetConst 0x804a, 0
    WorkSetConst 0x8022, 1

L_1DA6:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1E39
    Plugin8_Cmd1003 82, 0x8024
    Plugin8_Cmd1007 15, 254, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    YesNoWin 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1DFE
    WorkSetConst 0x8022, 0
    WorkSetConst 0x804a, 1
    VMJump L_1E33

L_1DFE:
    Plugin8_Cmd1003 83, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    YesNoWin 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1E33
    WorkSetConst 0x8022, 0
    WorkSetConst 0x804a, 0

L_1E33:
    VMJump L_1DA6

L_1E39:
    VMStackPush 0x804a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1ED2
    Plugin8_Cmd1007 6, 254, 0, 0
    Plugin8_Cmd1007 18, 254, 0, 1
    Plugin8_Cmd1003 84, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    Plugin8_Cmd1007 15, 254, 0, 0
    Plugin8_Cmd1007 14, 254, 0, 1
    FadeOutBlack
    FadeWait
    Plugin8_Cmd1005 2, 0x802e
    FieldClose
    FieldOpen
    EvCameraInit
    EvCameraUnbind
    WorkGet 0x8032, 0x802e
    VMCall L_1EFA
    FadeInBlack
    FadeWait
    MEPlay SEQ_ME_AVENUE_01
    // "Changed into\n[f000]ĸ\u0001\u0000!"
    SystemMsg Global10685_Text_ChangedInto, 2
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    RecordAdd 128, 1
    VMCall L_17BC
    WorkSetConst 0x8021, 0
    VMJump L_1EE4

L_1ED2:
    Plugin8_Cmd1003 85, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_1EE4:
    WorkSetConst 0x804a, 0
    VMReturn

L_1EEC:
    VMCall L_209E
    VMCall L_1F12
    VMReturn

L_1EFA:
    VMCall L_209E
    ActorCmdExec 255, Movement_4748
    ActorCmdWait
    VMCall L_1F12
    VMReturn

L_1F12:
    WorkCmpConst 0x8032, 0
    VMJumpIf CMP_EQ, L_1F25
    VMJump L_1F43

L_1F25:
    EvCameraMoveTo 3877, 49152, 0xb1000, 0x138000, 0x2c000, 0x3f8000, 1
    VMJump L_209A

L_1F43:
    WorkCmpConst 0x8032, 1
    VMJumpIf CMP_EQ, L_1F56
    VMJump L_1F74

L_1F56:
    EvCameraMoveTo 3877, 16384, 0xb1000, 0xb8000, 0x2d000, 0x388000, 1
    VMJump L_209A

L_1F74:
    WorkCmpConst 0x8032, 2
    VMJumpIf CMP_EQ, L_1F87
    VMJump L_1FA5

L_1F87:
    EvCameraMoveTo 3877, 49152, 0xb1000, 0x138000, 0x2c000, 0x318000, 1
    VMJump L_209A

L_1FA5:
    WorkCmpConst 0x8032, 3
    VMJumpIf CMP_EQ, L_1FB8
    VMJump L_1FD6

L_1FB8:
    EvCameraMoveTo 3877, 16384, 0xb1000, 0xb8000, 0x2d000, 0x2a8000, 1
    VMJump L_209A

L_1FD6:
    WorkCmpConst 0x8032, 4
    VMJumpIf CMP_EQ, L_1FE9
    VMJump L_2007

L_1FE9:
    EvCameraMoveTo 3877, 49152, 0xb1000, 0x138000, 0x2c000, 0x238000, 1
    VMJump L_209A

L_2007:
    WorkCmpConst 0x8032, 5
    VMJumpIf CMP_EQ, L_201A
    VMJump L_2038

L_201A:
    EvCameraMoveTo 3877, 16384, 0xb1000, 0xb8000, 0x2d000, 0x1c8000, 1
    VMJump L_209A

L_2038:
    WorkCmpConst 0x8032, 6
    VMJumpIf CMP_EQ, L_204B
    VMJump L_2069

L_204B:
    EvCameraMoveTo 3877, 49152, 0xb1000, 0x138000, 0x2c000, 0x158000, 1
    VMJump L_209A

L_2069:
    WorkCmpConst 0x8032, 7
    VMJumpIf CMP_EQ, L_207C
    VMJump L_209A

L_207C:
    EvCameraMoveTo 3877, 16384, 0xb1000, 0xb8000, 0x2d000, 0xe8000, 1
    VMJump L_209A

L_209A:
    EvCameraWait
    VMReturn

L_209E:
    Plugin8_Cmd1014 1
    VMReturn

L_20A4:
    Plugin8_Cmd1014 3
    ActorCmdExec 255, Movement_4748
    ActorCmdWait
    VMReturn

L_20B4:
    Plugin8_Cmd1014 2
    ActorCmdExec 255, Movement_4750
    ActorCmdWait
    VMReturn

L_20C4:
    WorkSetConst 0x804b, 0
    Plugin8_Cmd1002 9, 0x804b
    VMStackPush 0x804b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_215E
    ActorMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    Plugin8_Cmd1002 40, 0x8032
    VMCall L_1EEC
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkSetConst 0x802e, 0
    VMCall L_2178
    Plugin8_Cmd1031 19, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2150
    Plugin8_Cmd1003 145, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 1, 0
    WorkSetConst 0x802e, 1
    VMCall L_2178

L_2150:
    VMCall L_17BC
    Plugin8_Cmd1020
    VMJump L_2170

L_215E:
    Plugin8_Cmd1003 142, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_2170:
    WorkSetConst 0x804b, 0
    VMReturn

L_2178:
    WorkSetConst 0x804c, 0
    Plugin8_Cmd1015 0x802e, 0x804c, 0x8026
    Plugin8_Cmd1003 143, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 1, 0
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_21C3
    Plugin8_Cmd1003 141, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 1, 0
    VMJump L_2230

L_21C3:
    WordSetItemName 1, 0x804c
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_21ED
    Plugin8_Cmd1003 144, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 1, 0

L_21ED:
    WordSetNumber 0, 0x8026, 2
    Plugin8_Cmd1003 140, 0x8024
    MEPlay SEQ_ME_AVENUE_03
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 1, 0
    MEWait
    MsgWaitAdvance
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x804c
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_2230:
    WorkSetConst 0x804c, 0
    VMReturn

L_2238:
    VMCall L_2270
    VMReturn

L_2240:
    VMCall L_2270
    VMReturn

L_2248:
    VMCall L_2270
    VMReturn

L_2250:
    VMCall L_2270
    VMReturn

L_2258:
    VMCall L_2270
    VMReturn

L_2260:
    VMCall L_2270
    VMReturn

L_2268:
    VMCall L_2270
    VMReturn

L_2270:
    ActorMsgClose
    JoinAvenueStoreStart
    Plugin8Ov60_Cmd1011 0, 0, 1, 0x8029, 0x802a, 0x802b, 0x802c, 0x802d
    JoinAvenueStoreEnd
    WorkCmpConst 0x8029, 7
    VMJumpIf CMP_EQ, L_229B
    VMJump L_22A7

L_229B:
    VMCall L_248D
    VMJump L_248B

L_22A7:
    WorkCmpConst 0x8029, 6
    VMJumpIf CMP_EQ, L_22BA
    VMJump L_22C6

L_22BA:
    VMCall L_248D
    VMJump L_248B

L_22C6:
    WorkCmpConst 0x8029, 8
    VMJumpIf CMP_EQ, L_22D9
    VMJump L_22E5

L_22D9:
    VMCall L_248D
    VMJump L_248B

L_22E5:
    WorkCmpConst 0x8029, 5
    VMJumpIf CMP_EQ, L_22F8
    VMJump L_2304

L_22F8:
    VMCall L_248D
    VMJump L_248B

L_2304:
    WorkCmpConst 0x8029, 4
    VMJumpIf CMP_EQ, L_2317
    VMJump L_2323

L_2317:
    VMCall L_248D
    VMJump L_248B

L_2323:
    WorkCmpConst 0x8029, 3
    VMJumpIf CMP_EQ, L_2336
    VMJump L_2342

L_2336:
    VMCall L_248D
    VMJump L_248B

L_2342:
    WorkCmpConst 0x8029, 12
    VMJumpIf CMP_EQ, L_2355
    VMJump L_2361

L_2355:
    VMCall L_248D
    VMJump L_248B

L_2361:
    WorkCmpConst 0x8029, 13
    VMJumpIf CMP_EQ, L_2374
    VMJump L_2380

L_2374:
    VMCall L_248D
    VMJump L_248B

L_2380:
    WorkCmpConst 0x8029, 14
    VMJumpIf CMP_EQ, L_2393
    VMJump L_239F

L_2393:
    VMCall L_248D
    VMJump L_248B

L_239F:
    WorkCmpConst 0x8029, 15
    VMJumpIf CMP_EQ, L_23B2
    VMJump L_23BE

L_23B2:
    VMCall L_248D
    VMJump L_248B

L_23BE:
    WorkCmpConst 0x8029, 16
    VMJumpIf CMP_EQ, L_23D1
    VMJump L_23DD

L_23D1:
    VMCall L_248D
    VMJump L_248B

L_23DD:
    WorkCmpConst 0x8029, 17
    VMJumpIf CMP_EQ, L_23F0
    VMJump L_23FC

L_23F0:
    VMCall L_248D
    VMJump L_248B

L_23FC:
    WorkCmpConst 0x8029, 1
    VMJumpIf CMP_EQ, L_240F
    VMJump L_241B

L_240F:
    VMCall L_27FE
    VMJump L_248B

L_241B:
    WorkCmpConst 0x8029, 26
    VMJumpIf CMP_EQ, L_242E
    VMJump L_243A

L_242E:
    VMCall L_28C4
    VMJump L_248B

L_243A:
    WorkCmpConst 0x8029, 2
    VMJumpIf CMP_EQ, L_244D
    VMJump L_2459

L_244D:
    VMCall L_29FD
    VMJump L_248B

L_2459:
    WorkCmpConst 0x8029, 0
    VMJumpIf CMP_EQ, L_246C
    VMJump L_2472

L_246C:
    VMJump L_248B

L_2472:
    WorkCmpConst 0x8029, 255
    VMJumpIf CMP_EQ, L_2485
    VMJump L_248B

L_2485:
    VMJump L_248B

L_248B:
    VMReturn

L_248D:
    VMCall L_2B26
    CallPokeSelect 0, 0x8026, 0x8028, 0
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_27FC
    PokePartyIsEgg 0x8026, 0x8028
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_24DD
    Plugin8_Cmd1003 164, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMReturn

L_24DD:
    PokePartyGetParam 0x8026, 0x8028, 160
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_250C
    Plugin8_Cmd1003 176, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMReturn

L_250C:
    WorkSetConst 0x8027, 0
    WorkCmpConst 0x8029, 7
    VMJumpIf CMP_EQ, L_2525
    VMJump L_2535

L_2525:
    Plugin8_Cmd1018 0x8028, 5, 0x802a, 0x8026
    VMJump L_27A0

L_2535:
    WorkCmpConst 0x8029, 6
    VMJumpIf CMP_EQ, L_2548
    VMJump L_2558

L_2548:
    Plugin8_Cmd1018 0x8028, 4, 0x802a, 0x8026
    VMJump L_27A0

L_2558:
    WorkCmpConst 0x8029, 8
    VMJumpIf CMP_EQ, L_256B
    VMJump L_257B

L_256B:
    Plugin8_Cmd1018 0x8028, 3, 0x802a, 0x8026
    VMJump L_27A0

L_257B:
    WorkCmpConst 0x8029, 5
    VMJumpIf CMP_EQ, L_258E
    VMJump L_259E

L_258E:
    Plugin8_Cmd1018 0x8028, 2, 0x802a, 0x8026
    VMJump L_27A0

L_259E:
    WorkCmpConst 0x8029, 4
    VMJumpIf CMP_EQ, L_25B1
    VMJump L_25C1

L_25B1:
    Plugin8_Cmd1018 0x8028, 1, 0x802a, 0x8026
    VMJump L_27A0

L_25C1:
    WorkCmpConst 0x8029, 3
    VMJumpIf CMP_EQ, L_25D4
    VMJump L_25E4

L_25D4:
    Plugin8_Cmd1018 0x8028, 0, 0x802a, 0x8026
    VMJump L_27A0

L_25E4:
    WorkCmpConst 0x8029, 12
    VMJumpIf CMP_EQ, L_25F7
    VMJump L_262E

L_25F7:
    WorkSetConst 0x8027, 1
    Plugin8_Cmd1016 0x8028, 0x802b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2628
    Plugin8_Cmd1018 0x8028, 6, 0x802a, 0x8026
    WorkSetConst 0x8026, 1

L_2628:
    VMJump L_27A0

L_262E:
    WorkCmpConst 0x8029, 13
    VMJumpIf CMP_EQ, L_2641
    VMJump L_2678

L_2641:
    WorkSetConst 0x8027, 1
    Plugin8_Cmd1016 0x8028, 0x802b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2672
    Plugin8_Cmd1018 0x8028, 7, 0x802a, 0x8026
    WorkSetConst 0x8026, 1

L_2672:
    VMJump L_27A0

L_2678:
    WorkCmpConst 0x8029, 14
    VMJumpIf CMP_EQ, L_268B
    VMJump L_26C2

L_268B:
    WorkSetConst 0x8027, 1
    Plugin8_Cmd1016 0x8028, 0x802b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_26BC
    Plugin8_Cmd1018 0x8028, 8, 0x802a, 0x8026
    WorkSetConst 0x8026, 1

L_26BC:
    VMJump L_27A0

L_26C2:
    WorkCmpConst 0x8029, 15
    VMJumpIf CMP_EQ, L_26D5
    VMJump L_270C

L_26D5:
    WorkSetConst 0x8027, 1
    Plugin8_Cmd1016 0x8028, 0x802b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2706
    Plugin8_Cmd1018 0x8028, 10, 0x802a, 0x8026
    WorkSetConst 0x8026, 1

L_2706:
    VMJump L_27A0

L_270C:
    WorkCmpConst 0x8029, 16
    VMJumpIf CMP_EQ, L_271F
    VMJump L_2756

L_271F:
    WorkSetConst 0x8027, 1
    Plugin8_Cmd1016 0x8028, 0x802b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2750
    Plugin8_Cmd1018 0x8028, 11, 0x802a, 0x8026
    WorkSetConst 0x8026, 1

L_2750:
    VMJump L_27A0

L_2756:
    WorkCmpConst 0x8029, 17
    VMJumpIf CMP_EQ, L_2769
    VMJump L_27A0

L_2769:
    WorkSetConst 0x8027, 1
    Plugin8_Cmd1016 0x8028, 0x802b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_279A
    Plugin8_Cmd1018 0x8028, 9, 0x802a, 0x8026
    WorkSetConst 0x8026, 1

L_279A:
    VMJump L_27A0

L_27A0:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_27CB
    VMCall L_2AC3
    VMCall L_2BCA
    VMCall L_2C4D
    VMJump L_27FC

L_27CB:
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_27EA
    Plugin8_Cmd1003 167, 0x8024
    VMJump L_27F0

L_27EA:
    Plugin8_Cmd1003 166, 0x8024

L_27F0:
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_27FC:
    VMReturn

L_27FE:
    VMCall L_2B26
    CallPokeSelect 0, 0x8026, 0x8028, 0
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_28C2
    PokePartyIsEgg 0x8026, 0x8028
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_284E
    Plugin8_Cmd1003 164, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMReturn

L_284E:
    PokePartyGetParam 0x8026, 0x8028, 160
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_287D
    Plugin8_Cmd1003 176, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMReturn

L_287D:
    Plugin8_Cmd1016 0x8028, 0x802a, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_28B0
    VMCall L_2AC3
    VMCall L_2BCA
    VMCall L_2C4D
    VMJump L_28C2

L_28B0:
    Plugin8_Cmd1003 168, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_28C2:
    VMReturn

L_28C4:
    WorkSetConst 0x804d, 0
    VMCall L_2B26
    CallPokeSelect 1, 0x8026, 0x8028, 0
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_29F5
    PokePartyIsEgg 0x8026, 0x8028
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_29E3
    VMCall L_2AC3
    Plugin8_Cmd1019 0x802c, 0x8028, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2933
    Plugin8_Cmd1003 158, 0x8024
    VMJump L_2996

L_2933:
    VMStackPush 0x8026
    VMStackPushConst 5
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_2952
    Plugin8_Cmd1003 159, 0x8024
    VMJump L_2996

L_2952:
    VMStackPush 0x8026
    VMStackPushConst 10
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_2971
    Plugin8_Cmd1003 160, 0x8024
    VMJump L_2996

L_2971:
    VMStackPush 0x8026
    VMStackPushConst 40
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_2990
    Plugin8_Cmd1003 161, 0x8024
    VMJump L_2996

L_2990:
    Plugin8_Cmd1003 162, 0x8024

L_2996:
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_29D7
    RecordAdd 9, 1
    Cmd_02C5 8
    CallEggHatch 0x8026
    Plugin8_Cmd1003 156, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_29D7:
    VMCall L_2C4D
    VMJump L_29F5

L_29E3:
    Plugin8_Cmd1003 165, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_29F5:
    WorkSetConst 0x804d, 0
    VMReturn

L_29FD:
    VMCall L_2B26
    CallPokeSelect 0, 0x8026, 0x8028, 0
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2AC1
    PokePartyIsEgg 0x8026, 0x8028
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2A4D
    Plugin8_Cmd1003 164, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMReturn

L_2A4D:
    PokePartyGetParam 0x8026, 0x8028, 160
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2A7C
    Plugin8_Cmd1003 176, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMReturn

L_2A7C:
    Plugin8_Cmd1017 0x8028, 0x802a, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2AAF
    VMCall L_2AC3
    VMCall L_2BCA
    VMCall L_2C4D
    VMJump L_2AC1

L_2AAF:
    Plugin8_Cmd1003 169, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_2AC1:
    VMReturn

L_2AC3:
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    Plugin8_Cmd1002 40, 0x8032
    VMCall L_120C
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    WordSetPartyPokeName 2, 0x8028
    Plugin8_Cmd1007 18, 254, 0, 3
    Plugin8_Cmd1034 0x802c, 0, 0x8024
    SystemMsg 0x8024, 2
    InfoMsgClose
    VMCall L_17BC
    VMReturn

L_2B26:
    WorkSetConst 0x804e, 0
    Plugin8_Cmd1002 11, 0x804e
    WorkCmpConst 0x804e, 1
    VMJumpIf CMP_EQ, L_2B45
    VMJump L_2B51

L_2B45:
    Plugin8_Cmd1003 151, 0x8024
    VMJump L_2BB4

L_2B51:
    WorkCmpConst 0x804e, 4
    VMJumpIf CMP_EQ, L_2B64
    VMJump L_2B70

L_2B64:
    Plugin8_Cmd1003 155, 0x8024
    VMJump L_2BB4

L_2B70:
    WorkCmpConst 0x804e, 5
    VMJumpIf CMP_EQ, L_2B83
    VMJump L_2B8F

L_2B83:
    Plugin8_Cmd1003 157, 0x8024
    VMJump L_2BB4

L_2B8F:
    WorkCmpConst 0x804e, 7
    VMJumpIf CMP_EQ, L_2BA2
    VMJump L_2BAE

L_2BA2:
    Plugin8_Cmd1003 153, 0x8024
    VMJump L_2BB4

L_2BAE:
    Plugin8_Cmd1003 151, 0x8024

L_2BB4:
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    WorkSetConst 0x804e, 0
    VMReturn

L_2BCA:
    WorkSetConst 0x804f, 0
    Plugin8_Cmd1002 11, 0x804f
    WorkCmpConst 0x804f, 1
    VMJumpIf CMP_EQ, L_2BE9
    VMJump L_2BF5

L_2BE9:
    Plugin8_Cmd1003 150, 0x8024
    VMJump L_2C39

L_2BF5:
    WorkCmpConst 0x804f, 4
    VMJumpIf CMP_EQ, L_2C08
    VMJump L_2C14

L_2C08:
    Plugin8_Cmd1003 154, 0x8024
    VMJump L_2C39

L_2C14:
    WorkCmpConst 0x804f, 7
    VMJumpIf CMP_EQ, L_2C27
    VMJump L_2C33

L_2C27:
    Plugin8_Cmd1003 152, 0x8024
    VMJump L_2C39

L_2C33:
    Plugin8_Cmd1003 150, 0x8024

L_2C39:
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    WorkSetConst 0x804f, 0
    VMReturn

L_2C4D:
    MoneyWinDisp 31, 1
    Plugin8_Cmd1003 163, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    Plugin8_Cmd1021 0x802d, 0x802c
    SEPlay SEQ_SE_SYS_22
    MoneyWinUpdate
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 paid\n$[f000]ȅ\u0001\u0001![f000]븁\u0000"
    SystemMsg Global10685_Text_Paid, 2
    InfoMsgClose
    MoneyWinClose
    VMReturn

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    WorkSetConst 0x8021, 1

L_2C98:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2E66
    VMCall L_2E6E
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_2CC4
    VMJump L_2D39

L_2CC4:
    Plugin8_Cmd1007 11, 255, 0, 0
    Plugin8_Cmd1007 13, 255, 0, 1
    Plugin8_Cmd1007 12, 255, 0, 2
    Plugin8_Cmd1003 102, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1031 19, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2D33
    Plugin8_Cmd1007 8, 255, 0, 3
    Plugin8_Cmd1007 17, 255, 0, 4
    Plugin8_Cmd1003 108, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_2D33:
    VMJump L_2E60

L_2D39:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_2D4C
    VMJump L_2D5A

L_2D4C:
    ActorMsgClose
    Plugin8_Cmd1023 0, 1
    VMJump L_2E60

L_2D5A:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_2D6D
    VMJump L_2D7B

L_2D6D:
    ActorMsgClose
    Plugin8_Cmd1023 1, 1
    VMJump L_2E60

L_2D7B:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_2D8E
    VMJump L_2D9C

L_2D8E:
    ActorMsgClose
    Plugin8_Cmd1023 2, 1
    VMJump L_2E60

L_2D9C:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_2DAF
    VMJump L_2DBD

L_2DAF:
    ActorMsgClose
    Plugin8_Cmd1023 3, 1
    VMJump L_2E60

L_2DBD:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_2DD0
    VMJump L_2E1B

L_2DD0:
    Plugin8_Cmd1003 115, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    YesNoWin 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2E15
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x413a, 1
    FlagSet EVENT_FLAG_0x09f1
    MapChangeWarp ZONE_JOIN_AVENUE, 15, 70, 0
    WorkSetConst 0x8021, 0

L_2E15:
    VMJump L_2E60

L_2E1B:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_2E2E
    VMJump L_2E5A

L_2E2E:
    Plugin8_Cmd1003 101, 0x8024
    Plugin8_Cmd1007 4, 255, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMJump L_2E60

L_2E5A:
    WorkSetConst 0x8021, 0

L_2E60:
    VMJump L_2C98

L_2E66:
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_2E6E:
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    Plugin8_Cmd1007 4, 255, 0, 2
    Plugin8_Cmd1007 18, 254, 0, 3
    Plugin8_Cmd1003 100, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 185, 65535, 0
    Plugin8_Cmd1002 34, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2EDA
    ListMenuAdd 204, 65535, 1

L_2EDA:
    ListMenuAdd 205, 65535, 2
    ListMenuAdd 206, 65535, 3
    ListMenuAdd 212, 65535, 4
    Plugin8_Cmd1002 62, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2F13
    ListMenuAdd 225, 65535, 5

L_2F13:
    ListMenuAdd 183, 65535, 6
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2F36
    WorkSetConst 0x8020, 6

L_2F36:
    VMReturn

L_2F38:
    WorkSetConst 0x8050, 0
    WorkSetConst 0x8051, 0
    WorkSetConst 0x8052, 0
    Plugin8_Cmd1031 17, 0x8052
    WorkSetConst 0x8022, 1

L_2F56:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_30FC
    Plugin8_Cmd1003 106, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 215, 65535, 0
    ListMenuAdd 216, 65535, 1
    ListMenuAdd 217, 65535, 2
    ListMenuAdd 218, 65535, 3
    ListMenuAdd 219, 65535, 4
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2FC7
    WorkSetConst 0x8020, 4

L_2FC7:
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2FEC
    Plugin8_Cmd1030 17, 0x8052
    WorkSetConst 0x8022, 0
    VMJump L_30F6

L_2FEC:
    ActorMsgClose
    Plugin8_Cmd1030 17, 0x8020
    FadeOutBlackQ
    FadeWait
    PlayerGetGPos 0x8050, 0x8051
    MapChangeCore ZONE_JOIN_AVENUE, 0, 0, 0, 0
    EvCameraInit
    EvCameraUnbind
    VMCall L_179C
    FadeInBlackQ
    FadeWait
    WorkGet 0x8024, 0x8020
    WorkAdd 0x8024, 149
    SystemMsg 0x8024, 2
    YesNoWin 0x8027
    InfoMsgClose
    FadeOutBlackQ
    FadeWait
    EvCameraRebind
    EvCameraEnd
    VMStackPush 0x8051
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3082
    VMStackPush 0x8050
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3070
    MapChangeCore ZONE_JOIN_AVENUE_2, 10, 0, 11, 3
    VMJump L_307C

L_3070:
    MapChangeCore ZONE_JOIN_AVENUE_2, 12, 0, 11, 2

L_307C:
    VMJump L_30B3

L_3082:
    VMStackPush 0x8051
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_30A7
    MapChangeCore ZONE_JOIN_AVENUE_2, 11, 0, 10, 1
    VMJump L_30B3

L_30A7:
    MapChangeCore ZONE_JOIN_AVENUE_2, 11, 0, 12, 0

L_30B3:
    ActorFindByGPos 0x8023, 0x8026, 11, 0, 11
    Plugin8_Cmd1030 26, 0x8023
    ActorSetEyeToEye
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_30F6
    Plugin8_Cmd1003 105, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    WorkSetConst 0x8022, 0

L_30F6:
    VMJump L_2F56

L_30FC:
    WorkSetConst 0x8052, 0
    WorkSetConst 0x8051, 0
    WorkSetConst 0x8050, 0
    VMReturn

L_3110:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8022, 1

L_311C:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31E2
    Plugin8_Cmd1003 107, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 226, 65535, 0
    ListMenuAdd 227, 65535, 1
    ListMenuAdd 228, 65535, 2
    ListMenuAdd 229, 65535, 3
    ListMenuAdd 230, 65535, 4
    ListMenuAdd 231, 65535, 5
    ListMenuAdd 232, 65535, 6
    ListMenuAdd 233, 65535, 7
    ListMenuAdd 234, 65535, 8
    ListMenuAdd 219, 65535, 10
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31B5
    WorkSetConst 0x8020, 10

L_31B5:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_31D4
    WorkSetConst 0x8022, 0
    VMJump L_31DC

L_31D4:
    ActorMsgClose
    Plugin8_Cmd1004 0x8020, 0x8020

L_31DC:
    VMJump L_311C

L_31E2:
    VMReturn

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8053, 0
    WorkSetConst 0x8054, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    Plugin8_Cmd1007 4, 255, 0, 2
    Plugin8_Cmd1007 18, 254, 0, 3
    Plugin8_Cmd1003 100, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    // "            "
    SystemMsg Global10685_Text_Empty_31, 2
    WorkSetConst 0x8021, 1

L_3248:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_33F6
    VMCall L_3408
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_3274
    VMJump L_3359

L_3274:
    VMCall L_3450
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_328D
    VMJump L_32AD

L_328D:
    // "            "
    SystemMsg Global10685_Text_Empty_31, 2
    InfoMsgClose
    Plugin8_Cmd1001
    MapChangeWarp ZONE_JOIN_AVENUE, 42, 52, 1
    WorkSetConst 0x8021, 0
    VMJump L_3353

L_32AD:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_32C0
    VMJump L_32DA

L_32C0:
    InfoMsgClose
    Plugin8_Cmd1001
    MapChangeWarp ZONE_JOIN_AVENUE, 31, 39, 1
    WorkSetConst 0x8021, 0
    VMJump L_3353

L_32DA:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_32ED
    VMJump L_3307

L_32ED:
    InfoMsgClose
    Plugin8_Cmd1001
    MapChangeWarp ZONE_JOIN_AVENUE, 31, 27, 1
    WorkSetConst 0x8021, 0
    VMJump L_3353

L_3307:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_331A
    VMJump L_3334

L_331A:
    InfoMsgClose
    Plugin8_Cmd1001
    MapChangeWarp ZONE_JOIN_AVENUE, 31, 15, 1
    WorkSetConst 0x8021, 0
    VMJump L_3353

L_3334:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_3347
    VMJump L_3353

L_3347:
    WorkSetConst 0x8021, 0
    VMJump L_3353

L_3353:
    VMJump L_33F0

L_3359:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_336C
    VMJump L_33A3

L_336C:
    InfoMsgClose
    Plugin8_Cmd1001
    FadeOutBlackQ
    FadeWait
    CallPlaceSelect 0x8053, 0x8054
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8053
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3397
    MapChangeFlyWarp 32852, 0

L_3397:
    WorkSetConst 0x8021, 0
    VMJump L_33F0

L_33A3:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_33B6
    VMJump L_33E6

L_33B6:
    InfoMsgClose
    Plugin8_Cmd1003 101, 0x8024
    Plugin8_Cmd1007 4, 255, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    Plugin8_Cmd1001
    WorkSetConst 0x8021, 0
    VMJump L_33F0

L_33E6:
    InfoMsgClose
    Plugin8_Cmd1001
    WorkSetConst 0x8021, 0

L_33F0:
    VMJump L_3248

L_33F6:
    WorkSetConst 0x8054, 0
    WorkSetConst 0x8053, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_3408:
    Plugin8_Cmd1007 8, 255, 0, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 189, 65535, 0
    ListMenuAdd 190, 65535, 1
    ListMenuAdd 183, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_344E
    WorkSetConst 0x8020, 2

L_344E:
    VMReturn

L_3450:
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 191, 65535, 0
    ListMenuAdd 192, 65535, 1
    ListMenuAdd 193, 65535, 2
    ListMenuAdd 194, 65535, 3
    ListMenuAdd 183, 65535, 4
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_349C
    WorkSetConst 0x8020, 4

L_349C:
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    Plugin8_Cmd1007 4, 255, 0, 2
    Plugin8_Cmd1007 18, 254, 0, 3
    Plugin8_Cmd1003 100, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    WorkSetConst 0x8021, 1

L_3528:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3696
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    Plugin8_Cmd1007 4, 255, 0, 2
    Plugin8_Cmd1007 18, 254, 0, 3
    Plugin8_Cmd1003 100, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMCall L_369E
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_358E
    VMJump L_35A4

L_358E:
    ActorMsgClose
    FadeOutBlackQ
    FadeWait
    CallPC 0x8026, 2
    FadeInBlackQ
    FadeWait
    VMJump L_3690

L_35A4:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_35B7
    VMJump L_35C3

L_35B7:
    VMCall L_3110
    VMJump L_3690

L_35C3:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_35D6
    VMJump L_35E2

L_35D6:
    VMCall L_370D
    VMJump L_3690

L_35E2:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_35F5
    VMJump L_3632

L_35F5:
    Plugin8_Cmd1002 64, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3626
    Plugin8_Cmd1003 116, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMJump L_362C

L_3626:
    VMCall L_3764

L_362C:
    VMJump L_3690

L_3632:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_3645
    VMJump L_3651

L_3645:
    VMCall L_2F38
    VMJump L_3690

L_3651:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_3664
    VMJump L_3690

L_3664:
    Plugin8_Cmd1003 101, 0x8024
    Plugin8_Cmd1007 4, 255, 0, 0
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8021, 0
    VMJump L_3690

L_3690:
    VMJump L_3528

L_3696:
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_369E:
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 195, 65535, 0
    ListMenuAdd 220, 65535, 1
    ListMenuAdd 213, 65535, 2
    ListMenuAdd 214, 65535, 3
    Plugin8_Cmd1002 33, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_36E8
    ListMenuAdd 186, 65535, 4

L_36E8:
    ListMenuAdd 183, 65535, 5
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_370B
    WorkSetConst 0x8020, 5

L_370B:
    VMReturn

L_370D:
    ActorMsgClose
    JoinAvenueStoreStart
    Plugin8Ov60_Cmd1011 2, 0, 2, 0x8029, 0x802a, 0x802b, 0x802c, 0x802d
    JoinAvenueStoreEnd
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3762
    Plugin8_Cmd1003 103, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    FadeOutBlack
    FadeWait
    FieldClose
    FieldOpen
    Plugin8_Cmd1006 8
    FadeInBlack
    FadeWait
    WorkSetConst 0x8021, 0

L_3762:
    VMReturn

L_3764:
    ActorMsgClose
    JoinAvenueStoreStart
    Plugin8Ov60_Cmd1011 2, 1, 2, 0x8029, 0x802a, 0x802b, 0x802c, 0x802d
    JoinAvenueStoreEnd
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_37B5
    Plugin8_Cmd1003 104, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    FadeOutBlack
    FadeWait
    FieldClose
    FieldOpen
    FadeInBlack
    FadeWait
    WorkSetConst 0x8021, 0

L_37B5:
    VMReturn

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8055, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 1
    Plugin8_Cmd1007 4, 255, 0, 2
    Plugin8_Cmd1007 18, 254, 0, 3
    Plugin8_Cmd1003 100, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1002 17, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3826
    WorkAdd 0x8055, 1

L_3826:
    Plugin8_Cmd1002 18, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3845
    WorkAdd 0x8055, 1

L_3845:
    Plugin8_Cmd1002 21, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3864
    WorkAdd 0x8055, 1

L_3864:
    Plugin8_Cmd1002 20, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3883
    WorkAdd 0x8055, 1

L_3883:
    Plugin8_Cmd1002 19, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_38A2
    WorkAdd 0x8055, 1

L_38A2:
    Plugin8_Cmd1002 22, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_38C1
    WorkAdd 0x8055, 1

L_38C1:
    Plugin8_Cmd1002 23, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_38E0
    WorkAdd 0x8055, 1

L_38E0:
    Plugin8_Cmd1002 24, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_38FF
    WorkAdd 0x8055, 1

L_38FF:
    VMStackPush 0x8055
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_393E
    Plugin8_Cmd1007 16, 255, 0, 0
    Plugin8_Cmd1007 8, 255, 0, 1
    Plugin8_Cmd1003 113, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMJump L_396A

L_393E:
    VMCall L_3998
    Plugin8_Cmd1007 16, 255, 0, 0
    Plugin8_Cmd1007 8, 255, 0, 1
    Plugin8_Cmd1003 114, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_396A:
    Plugin8_Cmd1007 4, 255, 0, 0
    Plugin8_Cmd1003 101, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    Plugin8_Cmd1001
    WorkSetConst 0x8055, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_3998:
    Plugin8_Cmd1003 112, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32800
    Plugin8_Cmd1002 17, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_39D4
    ListMenuAdd 280, 65535, 0

L_39D4:
    Plugin8_Cmd1002 18, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_39F5
    ListMenuAdd 281, 65535, 1

L_39F5:
    Plugin8_Cmd1002 21, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3A16
    ListMenuAdd 284, 65535, 4

L_3A16:
    Plugin8_Cmd1002 20, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3A37
    ListMenuAdd 282, 65535, 2

L_3A37:
    Plugin8_Cmd1002 19, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3A58
    ListMenuAdd 283, 65535, 3

L_3A58:
    Plugin8_Cmd1002 22, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3A79
    ListMenuAdd 285, 65535, 5

L_3A79:
    Plugin8_Cmd1002 23, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3A9A
    ListMenuAdd 286, 65535, 6

L_3A9A:
    Plugin8_Cmd1002 24, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3ABB
    ListMenuAdd 287, 65535, 7

L_3ABB:
    ListMenuShow
    Plugin8_Cmd1030 16, 0x8020
    VMReturn

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 4, 255, 0, 1
    Plugin8_Cmd1007 6, 254, 0, 2
    Plugin8_Cmd1003 120, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1002 3, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_3B60
    Plugin8_Cmd1003 57, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8026
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    Plugin8_Cmd1026
    RecordAdd 130, 1
    VMJump L_3B62

L_3B60:
    ActorMsgClose

L_3B62:
    WorkSetConst 0x8021, 1

L_3B68:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3BEA
    VMCall L_3BF2
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_3B94
    VMJump L_3BA0

L_3B94:
    VMCall L_034B
    VMJump L_3BE4

L_3BA0:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_3BB3
    VMJump L_3BBF

L_3BB3:
    VMCall L_0979
    VMJump L_3BE4

L_3BBF:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_3BD2
    VMJump L_3BDE

L_3BD2:
    VMCall L_17EE
    VMJump L_3BE4

L_3BDE:
    WorkSetConst 0x8021, 0

L_3BE4:
    VMJump L_3B68

L_3BEA:
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_3BF2:
    WorkSetConst 0x8056, 0
    // ""
    SystemMsg Global10685_Text_Empty_253, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 174, 235, 0
    Plugin8_Cmd1002 6, 0x8056
    VMStackPush 0x8056
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3C30
    ListMenuAdd 176, 237, 2

L_3C30:
    ListMenuAdd 183, 244, 7
    ListMenuShow
    InfoMsgClose
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3C55
    WorkSetConst 0x8020, 7

L_3C55:
    WorkSetConst 0x8056, 0
    VMReturn

Script_10:
    ActorsPauseAll
    WorkSetConst 0x8057, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1029 0x8057, 0, 0x8023
    Plugin8_Cmd1029 0x8023, 33, 0x802e
    WorkGet 0x8057, 0x802e
    WorkAdd 0x8057, 0
    Plugin8_Cmd1031 0x8057, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3CC0
    Plugin8_Cmd1029 0x8023, 28, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    VMCall L_3E50
    VMJump L_3D4E

L_3CC0:
    Plugin8_Cmd1029 0x8023, 29, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 1719, 65535, 0
    ListMenuAdd 1720, 65535, 1
    ListMenuAdd 1721, 65535, 2
    ListMenuShow
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3D10
    WorkSetConst 0x8020, 2

L_3D10:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_3D23
    VMJump L_3D2F

L_3D23:
    VMCall L_3D5C
    VMJump L_3D4E

L_3D2F:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_3D42
    VMJump L_3D4E

L_3D42:
    VMCall L_3E50
    VMJump L_3D4E

L_3D4E:
    ActorMsgClose
    WorkSetConst 0x8057, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_3D5C:
    WorkSetConst EVENT_WORK_0x400b, 0
    Plugin8_Cmd1032 0x802e, EVENT_WORK_0x400b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3D99
    Plugin8_Cmd1029 0x8023, 32, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    VMJump L_3D9F

L_3D99:
    VMCall L_3DA1

L_3D9F:
    VMReturn

L_3DA1:
    WorkSetConst 0x8021, 1

L_3DA7:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3E4E
    Plugin8_Cmd1032 0x802e, EVENT_WORK_0x400b, 0x8026
    WorkAdd EVENT_WORK_0x400b, 1
    DebugPrint 0x8026
    WorkAdd 0x8026, 16
    WorkSub 0x8026, 1
    DebugPrint 0x8026
    Plugin8_Cmd1029 0x8023, 0x8026, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1032 0x802e, EVENT_WORK_0x400b, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3E17
    WorkSetConst 0x8021, 0
    VMJump L_3E48

L_3E17:
    Plugin8_Cmd1029 0x8023, 31, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    YesNoWin 0x8027
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_3E48
    WorkSetConst 0x8021, 0

L_3E48:
    VMJump L_3DA7

L_3E4E:
    VMReturn

L_3E50:
    WorkSetConst 0x8058, 0
    WorkSetConst 0x8059, 0
    Plugin8_Cmd1029 0x8023, 1, 0x802e
    Plugin8_Cmd1029 0x8023, 3, 0x8058
    Plugin8_Cmd1007 4, 255, 0, 0
    Plugin8_Cmd1007 19, 255, 0, 1
    Plugin8_Cmd1007 20, 255, 0, 2
    Plugin8_Cmd1029 0x8023, 2, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32800

L_3EA7:
    VMStackPush 0x8059
    VMStackPush 0x8058
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_3EDA
    DebugPrint 0x8059
    Plugin8_Cmd1033 0x802e, 0x8059, 0
    ListMenuAdd 1682, 65535, 0x8059
    WorkAdd 0x8059, 1
    VMJump L_3EA7

L_3EDA:
    ListMenuShow
    Plugin8_Cmd1029 0x8023, 30, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    LastKeyWait
    Plugin8_Cmd1029 0x8023, 33, 0x802e
    WorkGet 0x8058, 0x802e
    WorkAdd 0x8058, 0
    WorkAdd 0x8020, 1
    Plugin8_Cmd1030 0x8058, 0x8020
    WorkSetConst 0x8059, 0
    WorkSetConst 0x8058, 0
    VMReturn

Script_11:
    ActorsPauseAll
    WorkSetConst 0x805a, 0
    WorkSetConst 0x805b, 0
    Plugin8_Cmd1013 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1029 0x8023, 0, 0x8023
    Plugin8_Cmd1007 8, 255, 0, 0
    // "I love [f000]Ĺ\u0001\u0000![f000]븁\u0000\nI love people in this avenue, too![f000]븁\u0000\nThat's why I'm checking\neveryone's history.[f000]븁\u0000\nWhose history do you want to know?"
    ActorMsg MSGFILE_SCRIPT, Global10685_Text_LoveLovePeopleAvenue, 0x8023, 2, 0
    VMCall L_412C
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_40FC
    WorkGet 0x802e, 0x8020
    Plugin8_Cmd1007 0, 0, 0x802e, 0
    // "Then, I'll tell\nas much of [f000]Ā\u0001\u0000's history[f000]븀\u0000\nas I know.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10685_Text_ThenIllTellMuch, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 0, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 1, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 2, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 3, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 5, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    // "That is [f000]Ā\u0001\u0000's history.[f000]븁\u0000\nDo you want to know more\nabout [f000]Ā\u0001\u0000?"
    ActorMsg MSGFILE_SCRIPT, Global10685_Text_SHistoryWantKnow, 0x8023, 2, 0
    YesNoWin 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_40FC
    Plugin8_Cmd1035 0x802e, 6, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 7, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 8, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 9, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    // "On [f000]Ā\u0001\u0000's\nJoin Avenue...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10685_Text_SJoinAvenue, 0x8023, 2, 0
    Plugin8_Cmd1035 0x802e, 10, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_409B
    // "I don't know what kind of shops\nthey are...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10685_Text_DontKnowWhatKind, 0x8023, 2, 0
    VMJump L_40FC

L_409B:
    Plugin8_Cmd1035 0x802e, 11, 0x8026
    WorkSetConst 0x805a, 0

L_40A9:
    VMStackPush 0x805a
    VMStackPush 0x8026
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_40E8
    WorkGet 0x805b, 0x805a
    WorkAdd 0x805b, 13
    Plugin8_Cmd1035 0x802e, 0x805b, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    WorkAdd 0x805a, 1
    VMJump L_40A9

L_40E8:
    Plugin8_Cmd1035 0x802e, 12, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0

L_40FC:
    Plugin8_Cmd1013 0
    Plugin8_Cmd1007 8, 255, 0, 0
    // "I love [f000]Ĺ\u0001\u0000![f000]븁\u0000\nI love people in this avenue, too![f000]븁\u0000\nI'll keep watching this avenue forever.[f000]븁\u0000\nPlease speak to me again!"
    ActorMsg MSGFILE_SCRIPT, Global10685_Text_LoveLovePeopleAvenue_2, 0x8023, 2, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x805b, 0
    WorkSetConst 0x805a, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_412C:
    ListMenu_AnchorTopRight 31, 1, 0, 129, 32800
    Plugin8_Cmd1002 32, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_4160
    Plugin8_Cmd1007 0, 0, 7, 0
    ListMenuAdd 1752, 65535, 7

L_4160:
    Plugin8_Cmd1002 31, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_418B
    Plugin8_Cmd1007 0, 0, 6, 0
    ListMenuAdd 1752, 65535, 6

L_418B:
    Plugin8_Cmd1002 30, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_41B6
    Plugin8_Cmd1007 0, 0, 5, 0
    ListMenuAdd 1752, 65535, 5

L_41B6:
    Plugin8_Cmd1002 29, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_41E1
    Plugin8_Cmd1007 0, 0, 4, 0
    ListMenuAdd 1752, 65535, 4

L_41E1:
    Plugin8_Cmd1002 28, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_420C
    Plugin8_Cmd1007 0, 0, 3, 0
    ListMenuAdd 1752, 65535, 3

L_420C:
    Plugin8_Cmd1002 27, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_4237
    Plugin8_Cmd1007 0, 0, 2, 0
    ListMenuAdd 1752, 65535, 2

L_4237:
    Plugin8_Cmd1002 26, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_4262
    Plugin8_Cmd1007 0, 0, 1, 0
    ListMenuAdd 1752, 65535, 1

L_4262:
    Plugin8_Cmd1002 25, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_428D
    Plugin8_Cmd1007 0, 0, 0, 0
    ListMenuAdd 1752, 65535, 0

L_428D:
    ListMenuAdd 1751, 65535, 65534
    ListMenuShow
    VMReturn

Script_12:
    ActorsPauseAll
    WorkSetConst 0x805c, 0
    WorkSetConst 0x805d, 0
    PlayerGetGPos 0x805c, 0x805d
    WorkSetConst 0x802e, 0
    VMStackPush 0x805d
    VMStackPushConst 60
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 66
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_42E2
    WorkSetConst 0x802e, 0
    VMJump L_4425

L_42E2:
    VMStackPush 0x805d
    VMStackPushConst 53
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 59
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_4311
    WorkSetConst 0x802e, 1
    VMJump L_4425

L_4311:
    VMStackPush 0x805d
    VMStackPushConst 46
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 52
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_4340
    WorkSetConst 0x802e, 2
    VMJump L_4425

L_4340:
    VMStackPush 0x805d
    VMStackPushConst 39
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 45
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_436F
    WorkSetConst 0x802e, 3
    VMJump L_4425

L_436F:
    VMStackPush 0x805d
    VMStackPushConst 32
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 38
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_439E
    WorkSetConst 0x802e, 4
    VMJump L_4425

L_439E:
    VMStackPush 0x805d
    VMStackPushConst 25
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 31
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_43CD
    WorkSetConst 0x802e, 5
    VMJump L_4425

L_43CD:
    VMStackPush 0x805d
    VMStackPushConst 18
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 24
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_43FC
    WorkSetConst 0x802e, 6
    VMJump L_4425

L_43FC:
    VMStackPush 0x805d
    VMStackPushConst 11
    VMStackCmp CMP_GE
    VMStackPush 0x805d
    VMStackPushConst 17
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_4425
    WorkSetConst 0x802e, 7

L_4425:
    WorkAdd 0x802e, 12
    Plugin8_Cmd1003 0x802e, 0x8024
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    MsgPlaceSign 0x8024, 2
    MsgPlaceSignClose
    WorkSetConst 0x805d, 0
    WorkSetConst 0x805c, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 2
    Plugin8_Cmd1003 125, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    WorkSetConst 0x8021, 1

L_4493:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_450F
    VMCall L_4517
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_44BF
    VMJump L_44E4

L_44BF:
    VMCall L_034B
    VMStackPush 0x8030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_44DE
    WorkSetConst EVENT_WORK_0x4110, 3

L_44DE:
    VMJump L_4509

L_44E4:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_44F7
    VMJump L_4503

L_44F7:
    VMCall L_17EE
    VMJump L_4509

L_4503:
    WorkSetConst 0x8021, 0

L_4509:
    VMJump L_4493

L_450F:
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_4517:
    // ""
    SystemMsg Global10685_Text_Empty_253, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 174, 235, 0
    ListMenuAdd 183, 244, 7
    ListMenuShow
    InfoMsgClose
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_4553
    WorkSetConst 0x8020, 7

L_4553:
    VMReturn

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8031, 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 0, 254, 0, 0
    Plugin8_Cmd1007 6, 254, 0, 2
    Plugin8_Cmd1003 130, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    WorkSetConst 0x8021, 1

L_4599:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_4619
    VMCall L_4621
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_45C5
    VMJump L_45EE

L_45C5:
    VMCall L_0979
    VMStackPush 0x8030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_45E8
    WorkSetConst EVENT_WORK_0x4110, 5
    RTReserveScript 6

L_45E8:
    VMJump L_4613

L_45EE:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_4601
    VMJump L_460D

L_4601:
    VMCall L_17EE
    VMJump L_4613

L_460D:
    WorkSetConst 0x8021, 0

L_4613:
    VMJump L_4599

L_4619:
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_4621:
    // ""
    SystemMsg Global10685_Text_Empty_253, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 176, 237, 2
    ListMenuAdd 183, 244, 7
    ListMenuShow
    InfoMsgClose
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_465D
    WorkSetConst 0x8020, 7

L_465D:
    VMReturn

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1000
    Plugin8_Cmd1002 0, 0x8023
    Plugin8_Cmd1007 8, 255, 0, 1
    WordSetPlayerName 0
    Plugin8_Cmd1003 170, 0x8024
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8023, 2, 0
    ActorMsgClose
    WorkSetConst 0x8021, 1

L_4696:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_46F9
    VMCall L_4701
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_46C2
    VMJump L_46CE

L_46C2:
    VMCall L_0979
    VMJump L_46F3

L_46CE:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_46E1
    VMJump L_46ED

L_46E1:
    VMCall L_17EE
    VMJump L_46F3

L_46ED:
    WorkSetConst 0x8021, 0

L_46F3:
    VMJump L_4696

L_46F9:
    Plugin8_Cmd1001
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_4701:
    // ""
    SystemMsg Global10685_Text_Empty_253, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32800
    ListMenuAdd 176, 237, 0
    Plugin8_Cmd1031 22, 0x8026
    ListMenuAdd 183, 244, 1
    ListMenuShow
    InfoMsgClose
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_4743
    WorkSetConst 0x8020, 1

L_4743:
    VMReturn
    .balign 4, 0

Movement_4748:
    Move 69, 1
    MoveEnd

Movement_4750:
    Move 70, 1
    MoveEnd

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8021, 1

L_4760:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_495C
    ListMenu_AnchorTopLeft 1, 1, 0, 1, 32800
    ListMenuAdd 2190, 65535, 3
    ListMenuAdd 2187, 65535, 0
    ListMenuAdd 2188, 65535, 1
    ListMenuAdd 2189, 65535, 2
    ListMenuAdd 2199, 65535, 4
    ListMenuShow
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_47B9
    VMJump L_4808

L_47B9:
    VMCall L_4962
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_4802
    WorkGet 0x8032, 0x8020
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    VMCall L_1EFA
    FadeEx 3, 16, 0, 2
    FadeExWait
    ABKeyWait
    VMCall L_17BC

L_4802:
    VMJump L_4956

L_4808:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_481B
    VMJump L_4880

L_481B:
    VMCall L_4962
    VMCall L_49AF
    VMStackPush 0x8020
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMStackPush 0x802f
    VMStackPushConst 65534
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_487A
    WorkGet 0x8032, 0x8020
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    VMCall L_1232
    FadeEx 3, 16, 0, 2
    FadeExWait
    ABKeyWait
    VMCall L_17BC

L_487A:
    VMJump L_4956

L_4880:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_4893
    VMJump L_48C3

L_4893:
    FadeEx 3, 0, 16, 2
    FadeExWait
    EvCameraInit
    EvCameraUnbind
    VMCall L_179C
    FadeEx 3, 16, 0, 2
    FadeExWait
    ABKeyWait
    VMCall L_17BC
    VMJump L_4956

L_48C3:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_48D6
    VMJump L_491D

L_48D6:
    VMStackPush EVENT_WORK_0x4110
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_4917
    WorkSetConst EVENT_WORK_0x4110, 6
    WorkSetConst EVENT_WORK_0x410b, 1
    Plugin8_Cmd1030 18, 0
    Plugin8_Cmd1028 3, 4
    Plugin8_Cmd1028 3, 5
    MapChangeWarp ZONE_ROUTE_4, 430, 490, 0
    WorkSetConst 0x8021, 0

L_4917:
    VMJump L_4956

L_491D:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_4930
    VMJump L_4950

L_4930:
    WorkSetConst EVENT_WORK_0x413a, 1
    FlagSet EVENT_FLAG_0x09f1
    MapChangeWarp ZONE_JOIN_AVENUE, 15, 70, 0
    WorkSetConst 0x8021, 0
    VMJump L_4956

L_4950:
    WorkSetConst 0x8021, 0

L_4956:
    VMJump L_4760

L_495C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_4962:
    ListMenu_AnchorTopLeft 1, 1, 0, 1, 32800
    ListMenuAdd 2191, 65535, 0
    ListMenuAdd 2192, 65535, 1
    ListMenuAdd 2193, 65535, 2
    ListMenuAdd 2194, 65535, 3
    ListMenuAdd 2195, 65535, 4
    ListMenuAdd 2196, 65535, 5
    ListMenuAdd 2197, 65535, 6
    ListMenuAdd 2198, 65535, 7
    ListMenuShow
    VMReturn

L_49AF:
    ListMenu_AnchorTopLeft 1, 1, 0, 1, 32815
    ListMenuAdd 2191, 65535, 0
    ListMenuAdd 2192, 65535, 1
    ListMenuAdd 2193, 65535, 2
    ListMenuShow
    VMReturn
