#include "asm/field_script.inc"
#include "text/script/pokestar_studios_4.h"

// Script plugin 10, from the zones that use this file

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
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntry Script_25
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

Script_1:
    FlagSet EVENT_FLAG_0x03d4
    FlagSet EVENT_FLAG_0x03d5
    FlagSet EVENT_FLAG_0x03d6
    FlagSet EVENT_FLAG_0x03d7
    FlagSet EVENT_FLAG_0x03d8
    VMHalt

Script_24:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Movies are wonderful!\nThey get two thumbs up!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios4_Text_MoviesWonderfulTheyGet, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The records set by Pokéstar Studios\nmovies are left on this board!"
    ParentActorMsg MSGFILE_SCRIPT, PokestarStudios4_Text_RecordsSetByPokestar, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 2
    WorkSetConst 0x8024, 3
    WorkSetConst 0x8023, 4
    WorkSetConst 0x8022, 5
    WorkSetConst 0x8025, 6
    WorkSetConst 0x8026, 7
    WorkGet 0x8029, EVENT_WORK_0x4005
    VMCall L_0686
    WorkGet EVENT_WORK_0x4005, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8021, 1
    WorkSetConst 0x8020, 8
    WorkSetConst 0x8024, 9
    WorkSetConst 0x8023, 10
    WorkSetConst 0x8022, 11
    WorkSetConst 0x8025, 12
    WorkSetConst 0x8026, 13
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8021, 2
    WorkSetConst 0x8020, 14
    WorkSetConst 0x8024, 15
    WorkSetConst 0x8023, 16
    WorkSetConst 0x8022, 17
    WorkSetConst 0x8025, 18
    WorkSetConst 0x8026, 19
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8021, 3
    WorkSetConst 0x8020, 20
    WorkSetConst 0x8024, 21
    WorkSetConst 0x8023, 22
    WorkSetConst 0x8022, 23
    WorkSetConst 0x8025, 24
    WorkSetConst 0x8026, 25
    WorkGet 0x8029, EVENT_WORK_0x4006
    VMCall L_0686
    WorkGet EVENT_WORK_0x4006, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8021, 4
    WorkSetConst 0x8020, 26
    WorkSetConst 0x8024, 27
    WorkSetConst 0x8023, 28
    WorkSetConst 0x8022, 29
    WorkSetConst 0x8025, 30
    WorkSetConst 0x8026, 31
    WorkGet 0x8029, EVENT_WORK_0x4007
    VMCall L_0686
    WorkGet EVENT_WORK_0x4007, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8021, 5
    WorkSetConst 0x8020, 32
    WorkSetConst 0x8024, 33
    WorkSetConst 0x8023, 34
    WorkSetConst 0x8022, 35
    WorkSetConst 0x8025, 36
    WorkSetConst 0x8026, 37
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8021, 6
    WorkSetConst 0x8020, 38
    WorkSetConst 0x8024, 39
    WorkSetConst 0x8023, 40
    WorkSetConst 0x8022, 41
    WorkSetConst 0x8025, 42
    WorkSetConst 0x8026, 43
    WorkGet 0x8029, EVENT_WORK_0x4008
    VMCall L_0686
    WorkGet EVENT_WORK_0x4008, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x8021, 7
    WorkSetConst 0x8020, 44
    WorkSetConst 0x8024, 45
    WorkSetConst 0x8023, 46
    WorkSetConst 0x8022, 47
    WorkSetConst 0x8025, 48
    WorkSetConst 0x8026, 49
    WorkGet 0x8029, EVENT_WORK_0x4009
    VMCall L_0686
    WorkGet EVENT_WORK_0x4009, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WorkSetConst 0x8021, 8
    WorkSetConst 0x8020, 50
    WorkSetConst 0x8024, 51
    WorkSetConst 0x8023, 52
    WorkSetConst 0x8022, 53
    WorkSetConst 0x8025, 54
    WorkSetConst 0x8026, 55
    WorkGet 0x8029, EVENT_WORK_0x400a
    VMCall L_0686
    WorkGet EVENT_WORK_0x400a, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8021, 9
    WorkSetConst 0x8020, 56
    WorkSetConst 0x8024, 57
    WorkSetConst 0x8023, 58
    WorkSetConst 0x8022, 59
    WorkSetConst 0x8025, 60
    WorkSetConst 0x8026, 61
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8021, 10
    WorkSetConst 0x8020, 62
    WorkSetConst 0x8024, 63
    WorkSetConst 0x8023, 64
    WorkSetConst 0x8022, 65
    WorkSetConst 0x8025, 66
    WorkSetConst 0x8026, 67
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst 0x8021, 11
    WorkSetConst 0x8020, 68
    WorkSetConst 0x8024, 69
    WorkSetConst 0x8023, 70
    WorkSetConst 0x8022, 71
    WorkSetConst 0x8025, 72
    WorkSetConst 0x8026, 73
    WorkGet 0x8029, EVENT_WORK_0x400b
    VMCall L_0686
    WorkGet EVENT_WORK_0x400b, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8021, 12
    WorkSetConst 0x8020, 74
    WorkSetConst 0x8024, 75
    WorkSetConst 0x8023, 76
    WorkSetConst 0x8022, 77
    WorkSetConst 0x8025, 78
    WorkSetConst 0x8026, 79
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WorkSetConst 0x8021, 13
    WorkSetConst 0x8020, 80
    WorkSetConst 0x8024, 81
    WorkSetConst 0x8023, 82
    WorkSetConst 0x8022, 83
    WorkSetConst 0x8025, 84
    WorkSetConst 0x8026, 85
    WorkGet 0x8029, EVENT_WORK_0x400c
    VMCall L_0686
    WorkGet EVENT_WORK_0x400c, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8021, 14
    WorkSetConst 0x8020, 86
    WorkSetConst 0x8024, 87
    WorkSetConst 0x8023, 88
    WorkSetConst 0x8022, 89
    WorkSetConst 0x8025, 90
    WorkSetConst 0x8026, 91
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    WorkSetConst 0x8021, 15
    WorkSetConst 0x8020, 92
    WorkSetConst 0x8024, 93
    WorkSetConst 0x8023, 94
    WorkSetConst 0x8022, 95
    WorkSetConst 0x8025, 96
    WorkSetConst 0x8026, 97
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    WorkSetConst 0x8021, 16
    WorkSetConst 0x8020, 98
    WorkSetConst 0x8024, 99
    WorkSetConst 0x8023, 100
    WorkSetConst 0x8022, 101
    WorkSetConst 0x8025, 102
    WorkSetConst 0x8026, 103
    WorkGet 0x8029, EVENT_WORK_0x400d
    VMCall L_0686
    WorkGet EVENT_WORK_0x400d, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    WorkSetConst 0x8021, 17
    WorkSetConst 0x8020, 104
    WorkSetConst 0x8024, 105
    WorkSetConst 0x8023, 106
    WorkSetConst 0x8022, 107
    WorkSetConst 0x8025, 108
    WorkSetConst 0x8026, 109
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    WorkSetConst 0x8021, 18
    WorkSetConst 0x8020, 110
    WorkSetConst 0x8024, 111
    WorkSetConst 0x8023, 112
    WorkSetConst 0x8022, 113
    WorkSetConst 0x8025, 114
    WorkSetConst 0x8026, 115
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    WorkSetConst 0x8021, 19
    WorkSetConst 0x8020, 116
    WorkSetConst 0x8024, 117
    WorkSetConst 0x8023, 118
    WorkSetConst 0x8022, 119
    WorkSetConst 0x8025, 120
    WorkSetConst 0x8026, 121
    WorkSetConst 0x8029, 1
    VMCall L_0686
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    WorkSetConst 0x8021, 20
    WorkSetConst 0x8020, 122
    WorkSetConst 0x8024, 123
    WorkSetConst 0x8023, 124
    WorkSetConst 0x8022, 125
    WorkSetConst 0x8025, 126
    WorkSetConst 0x8026, 127
    WorkGet 0x8029, EVENT_WORK_0x400e
    VMCall L_0686
    WorkGet EVENT_WORK_0x400e, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    WorkSetConst 0x8021, 21
    WorkSetConst 0x8020, 128
    WorkSetConst 0x8024, 129
    WorkSetConst 0x8023, 130
    WorkSetConst 0x8022, 131
    WorkSetConst 0x8025, 132
    WorkSetConst 0x8026, 133
    WorkGet 0x8029, EVENT_WORK_0x400f
    VMCall L_0686
    WorkGet EVENT_WORK_0x400f, 0x8029
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0686:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    ActorMsg MSGFILE_SCRIPT, 0x8020, 0x8011, 2, 0
    MsgWaitAdvance
    Plugin10_Cmd1024 EVENT_WORK_0x4002, 0x8021, 0x8027, 0x8028
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06E6
    VMCall L_0769
    VMJump L_06EC

L_06E6:
    VMCall L_06F2

L_06EC:
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_06F2:
    Plugin10_Cmd1010 EVENT_WORK_0x4002, 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_070B
    VMJump L_071D

L_070B:
    ActorMsg MSGFILE_SCRIPT, 0x8022, 0x8011, 2, 0
    VMJump L_0767

L_071D:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0730
    VMJump L_0742

L_0730:
    ActorMsg MSGFILE_SCRIPT, 0x8023, 0x8011, 2, 0
    VMJump L_0767

L_0742:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0755
    VMJump L_0767

L_0755:
    ActorMsg MSGFILE_SCRIPT, 0x8024, 0x8011, 2, 0
    VMJump L_0767

L_0767:
    VMReturn

L_0769:
    WorkSetConst 0x802a, 0
    ActorMsg MSGFILE_SCRIPT, 0x8025, 0x8011, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ItemCheckSpace 0x8027, 0x8028, 0x802a
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8027
    WorkSet 0x8001, 0x8028
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07CC
    ActorMsg MSGFILE_SCRIPT, 0x8026, 0x8011, 2, 0
    VMJump L_07D8

L_07CC:
    WorkSetConst 0x8029, 1
    VMCall L_06F2

L_07D8:
    WorkSetConst 0x802a, 0
    VMReturn
