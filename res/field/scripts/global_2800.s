#include "asm/field_script.inc"
#include "text/script/global_7000.h"

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
    ScriptEntriesEnd

Script_1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag 0x8002
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0077
    ParentActorMsg 0x8006, 0x8005, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00DA

L_0077:
    ParentActorMsg 0x8006, 0x8003, 0, 0
    ActorMsgClose
    ItemCheckSpace 0x8000, 0x8001, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B6
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_02DF
    VMJump L_00C8

L_00B6:
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_0369

L_00C8:
    ParentActorMsg 0x8006, 0x8004, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet 0x8002

L_00DA:
    RTEndGlobal

Script_2:
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_0369
    RTEndGlobal
    VMHalt

Script_3:
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_0389
    RTEndGlobal
    VMHalt

Script_6:
    ItemCheckSpace 0x8000, 0x8001, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013B
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_02DF
    VMJump L_014D

L_013B:
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_0369

L_014D:
    RTEndGlobal
    VMHalt

L_0151:
    ItemCheckSpace 0x8000, 0x8001, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0184
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_02FB
    VMJump L_0196

L_0184:
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_0389

L_0196:
    VMReturn

Script_7:
    VMCall L_0151
    RTEndGlobal
    VMHalt

Script_14:
    ActorsPauseAll
    VMCall L_0151
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    VMCall L_01E5
    RTEndGlobal
    VMHalt

Script_12:
    VMCall L_01E5
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E1
    FlagSet 0x8002
    RecordAdd 44, 1
    Cmd_02C5 14

L_01E1:
    RTEndGlobal
    VMHalt

L_01E5:
    WorkSetConst 0x8020, 0
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    ItemCheckSpace 0x8000, 0x8001, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_022E
    VMCall L_04C9
    VMCall L_0419
    MEWait
    MsgWaitAdvance
    VMCall L_02FB
    VMJump L_024A

L_022E:
    VMCall L_030F
    ItemAdd 0x8000, 0x8001, 0x8010
    Cmd_01DD 0, 0x8000, 0
    VMCall L_0489

L_024A:
    WorkGet 0x8010, 0x8020
    VMReturn
    WorkSetConst 0x8020, 0

Script_15:
    VMCall L_0262
    RTEndGlobal
    VMHalt

L_0262:
    WorkSetConst 0x8021, 0
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    ItemCheckSpace 0x8000, 0x8001, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02AB
    VMCall L_04C9
    VMCall L_0419
    MEWait
    MsgWaitAdvance
    VMCall L_02DF
    VMJump L_02C7

L_02AB:
    VMCall L_030F
    ItemAdd 0x8000, 0x8001, 0x8010
    Cmd_01DD 0, 0x8000, 0
    VMCall L_04A5

L_02C7:
    WorkGet 0x8010, 0x8021
    VMReturn
    WorkSetConst 0x8021, 0

Script_4:
    VMCall L_02DF
    RTEndGlobal
    VMHalt

L_02DF:
    WordSetItemNameEx 0, 0x8008, 2, 0
    // "Too bad! There is no more room for\n[f000]ĉ\u0001\u0000![f000]븁\u0000"
    SystemMsg Global7000_Text_TooBadThereNo, 0
    InfoMsgClose
    VMReturn

Script_5:
    VMCall L_02FB
    RTEndGlobal
    VMHalt

L_02FB:
    WordSetItemNameEx 0, 0x8008, 2, 0
    // "Too bad! There is no more room for\n[f000]ĉ\u0001\u0000!"
    SystemMsg Global7000_Text_TooBadThereNo_2, 0
    LastKeyWait
    InfoMsgClose
    VMReturn

L_030F:
    WorkSetConst 0x8022, 0
    VMCall L_04C9
    VMSleep 4
    Cmd_0239 0x8022
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033C
    VMCall L_05CF

L_033C:
    VMCall L_0419
    MEWait
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035F
    ActorCmdWait
    VMCall L_05DD

L_035F:
    MsgWaitAdvance
    WorkSetConst 0x8022, 0
    VMReturn

L_0369:
    ItemAdd 0x8008, 0x8009, 0x8010
    VMCall L_04C9
    VMCall L_03A9
    MEWait
    MsgWaitAdvance
    VMCall L_04A5
    VMReturn

L_0389:
    ItemAdd 0x8008, 0x8009, 0x8010
    VMCall L_04C9
    VMCall L_03A9
    MEWait
    MsgWaitAdvance
    VMCall L_0489
    VMReturn

L_03A9:
    ItemGetPocket 0x8008, 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_03C2
    VMJump L_03DB

L_03C2:
    WordSetPlayerName 0
    WordSetItemNameWithArticle 1, 0x8008
    WordSetTMMoveName 2, 0x8008
    // "[f000]Ā\u0001\u0000 obtained\n[f000]ĉ\u0001\u0001 [f000]ć\u0001\u0002!"
    SystemMsg Global7000_Text_Obtained_3, 0
    VMJump L_0417

L_03DB:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_03EE
    VMJump L_0406

L_03EE:
    FieldEffect 0x8008
    WordSetPlayerName 0
    WordSetItemName 1, 0x8008
    // "[f000]Ā\u0001\u0000 obtained the [f000][ff00]\u0001\u0002[f000]ĉ\u0001\u0001[f000][ff00]\u0001\u0000!"
    SystemMsg Global7000_Text_Obtained_2, 0
    VMJump L_0417

L_0406:
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8008, 0x8009, 1
    // "[f000]Ā\u0001\u0000 obtained [f000][ff00]\u0001\u0002[f000]ĉ\u0001\u0001[f000][ff00]\u0001\u0000!"
    SystemMsg Global7000_Text_Obtained, 0

L_0417:
    VMReturn

L_0419:
    ItemGetPocket 0x8008, 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0432
    VMJump L_044B

L_0432:
    WordSetPlayerName 0
    WordSetItemNameWithArticle 1, 0x8008
    WordSetTMMoveName 2, 0x8008
    // "[f000]Ā\u0001\u0000 found\n[f000]ĉ\u0001\u0001 [f000]ć\u0001\u0002!"
    SystemMsg Global7000_Text_Found_3, 0
    VMJump L_0487

L_044B:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_045E
    VMJump L_0476

L_045E:
    FieldEffect 0x8008
    WordSetPlayerName 0
    WordSetItemName 1, 0x8008
    // "[f000]Ā\u0001\u0000 found [f000][ff00]\u0001\u0002[f000]ĉ\u0001\u0001[f000][ff00]\u0001\u0000!"
    SystemMsg Global7000_Text_Found, 0
    VMJump L_0487

L_0476:
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8008, 0x8009, 1
    // "[f000]Ā\u0001\u0000 found [f000][ff00]\u0001\u0002[f000]ĉ\u0001\u0001[f000][ff00]\u0001\u0000!"
    SystemMsg Global7000_Text_Found_2, 0

L_0487:
    VMReturn

L_0489:
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8008, 0x8009, 0
    WordSetItemPocketName 2, 0x8008
    // "[f000]Ā\u0001\u0000 put the [f000]ĉ\u0001\u0001 in the\n[f000][ff00]\u0001\u0002[f000]Ē\u0001\u0002[f000][ff00]\u0001\u0000 Case."
    SystemMsg Global7000_Text_PutCase_2, 0
    LastKeyWait
    InfoMsgClose
    VMReturn

L_04A5:
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8008, 0x8009, 0
    WordSetItemPocketName 2, 0x8008
    // "[f000]Ā\u0001\u0000 put the [f000]ĉ\u0001\u0001 in the\n[f000][ff00]\u0001\u0002[f000]Ē\u0001\u0002[f000][ff00]\u0001\u0000 Case.[f000]븁\u0000"
    SystemMsg Global7000_Text_PutCase, 0
    InfoMsgClose
    VMReturn

Script_9:
    VMCall L_04C9
    RTEndGlobal
    VMHalt

L_04C9:
    VMStackPush 0x8008
    VMStackPushConst 616
    VMStackCmp CMP_EQ
    VMStackPush 0x8008
    VMStackPushConst 617
    VMStackCmp CMP_EQ
    VMStackPush 0x8008
    VMStackPushConst 622
    VMStackCmp CMP_EQ
    VMStackPush 0x8008
    VMStackPushConst 466
    VMStackCmp CMP_EQ
    VMStackPush 0x8008
    VMStackPushConst 628
    VMStackCmp CMP_EQ
    VMStackPush 0x8008
    VMStackPushConst 629
    VMStackCmp CMP_EQ
    VMStackPush 0x8008
    VMStackPushConst 638
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0542
    MEPlay SEQ_ME_ITEM_MYSTERY
    VMReturn

L_0542:
    ItemGetPocket 0x8008, 0x8010
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_055B
    VMJump L_0565

L_055B:
    MEPlay SEQ_ME_KEYITEM
    VMJump L_05B9

L_0565:
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0592
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0592
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0592
    VMJump L_059C

L_0592:
    MEPlay SEQ_ME_ITEM
    VMJump L_05B9

L_059C:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_05AF
    VMJump L_05B9

L_05AF:
    MEPlay SEQ_ME_WAZA
    VMJump L_05B9

L_05B9:
    VMReturn

Script_10:
    VMCall L_05CF
    RTEndGlobal
    VMHalt

Script_11:
    VMCall L_05DD
    RTEndGlobal
    VMHalt

L_05CF:
    PlayerSetSpecialSequence 16
    ActorCmdExec 255, Movement_05E4
    VMReturn

L_05DD:
    PlayerSetSpecialSequence 8
    VMReturn
    .balign 4, 0

Movement_05E4:
    Move 154, 1
    MoveEnd

Script_13:
    ActorsPauseAll
    WorkSetConst 0x8023, 0
    WorkGet 0x8008, 0x8000
    WorkSetConst 0x8009, 1
    ItemCheckSpace 0x8000, 1, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_063B
    VMCall L_04C9
    VMCall L_0419
    MEWait
    MsgWaitAdvance
    // "Too bad! The Bag is full!\n[f000]Ā\u0001\u0000 gave up the hidden item."
    SystemMsg Global7000_Text_TooBadBagFull, 0
    LastKeyWait
    InfoMsgClose
    VMJump L_066C

L_063B:
    VMCall L_030F
    ItemAdd 0x8000, 1, 0x8010
    Cmd_01DD 0, 0x8000, 0
    WordSetPlayerName 0
    WordSetItemName 1, 0x8000
    WordSetItemPocketName 2, 0x8008
    // "[f000]Ā\u0001\u0000 put the [f000]ĉ\u0001\u0001 in the\n[f000][ff00]\u0001\u0002[f000]Ē\u0001\u0002[f000][ff00]\u0001\u0000 Case.[f000]븁\u0000"
    SystemMsg Global7000_Text_PutCase, 0
    // "You find a note on the item.[f000]븁\u0000\n“Please make good use of this item on\nyour adventure! Sincerely, [f000]Ā\u0001\u0003\"[f000]븁\u0000\nIt looks like [f000]Ā\u0001\u0003 hid the item."
    SystemMsg Global7000_Text_FindNoteItemPlease, 0
    LastKeyWait

L_066C:
    InfoMsgClose
    WorkGet 0x8010, 0x8023
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkGet 0x8024, 0x8000
    WorkGet 0x8025, 0x8001
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06CB
    // "Too bad! There's no more\nroom for the [f000]ĉ\u0001\u0001![f000]븀\u0000\nGave up on taking the [f000]ĉ\u0001\u0001!"
    SystemMsg Global7000_Text_TooBadTheresNo, 0
    VMJump L_072D

L_06CB:
    MEPlay SEQ_ME_ITEM
    VMSleep 4
    Cmd_0239 0x8028
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06F0
    VMCall L_05CF

L_06F0:
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8024, 0x8025, 0
    WordSetNumber 2, 0x8025, 3
    // "[f000]Ā\u0001\u0000 obtained\n[f000]Ȃ\u0001\u0002 [f000][ff00]\u0001\u0002[f000]ĉ\u0001\u0001[f000][ff00]\u0001\u0000!"
    SystemMsg Global7000_Text_Obtained_4, 0
    MEWait
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0725
    ActorCmdWait
    VMCall L_05DD

L_0725:
    ItemAdd 0x8024, 0x8025, 0x8010

L_072D:
    LastKeyWait
    InfoMsgClose
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkGet 0x8029, 0x8000
    ItemCheckSpace 0x8029, 1, 0x802a
    WorkGet 0x8008, 0x8029
    FunfestMissionBroadcast 23, 0x8029
    ItemAdd 0x8029, 1, 0x8010
    VMCall L_030F
    // "The Funfest Mission score\nwent up by one![f000]븁\u0000"
    SystemMsg Global7000_Text_FunfestMissionScoreWent, 0
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C0
    VMCall L_02FB
    VMJump L_07C6

L_07C0:
    VMCall L_0489

L_07C6:
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    RTEndGlobal
    VMHalt

Script_18:
    ItemCheckSpace 0x8000, 0x8001, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_081B
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_02DF
    VMJump L_082D

L_081B:
    WorkGet 0x8008, 0x8000
    WorkGet 0x8009, 0x8001
    VMCall L_0831

L_082D:
    RTEndGlobal
    VMHalt

L_0831:
    ItemAdd 0x8008, 0x8009, 0x8010
    VMCall L_04C9
    WordSetPlayerName 0
    WordSetItemNameEx 1, 0x8008, 0x8009, 0
    WordSetNumber 2, 0x8009, 1
    // "[f000]Ā\u0001\u0000 obtained\n[f000]Ȁ\u0001\u0002 [f000]ĉ\u0001\u0001!"
    SystemMsg Global7000_Text_Obtained_5, 0
    MEWait
    MsgWaitAdvance
    VMCall L_04A5
    VMReturn
    .balign 4, 0
