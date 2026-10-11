#include "asm/field_script.inc"
#include "text/script/global_10795.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

L_0058:
    SEPlay SEQ_SE_MESSAGE
    // "Look!\nYou've found a narrow path![f000]븁\u0000\nWill you follow it?"
    SystemMsg Global10795_Text_LookYouveFoundNarrow, 2
    YesNoWin 0x8010
    InfoMsgClose
    SEWait
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0089
    Cmd_02C5 27
    SEPlay SEQ_SE_KAIDAN
    HiddenHollowCallWarpIn 0x8020

L_0089:
    VMReturn

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a50
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a51
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a52
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a53
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 4
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a54
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 5
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a55
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 6
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a56
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8020, 7
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a57
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x8020, 8
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a58
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WorkSetConst 0x8020, 9
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a59
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8020, 10
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a5a
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8020, 11
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a5b
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst 0x8020, 12
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a5c
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8020, 13
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a5d
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WorkSetConst 0x8020, 14
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a5e
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8020, 15
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a5f
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    WorkSetConst 0x8020, 16
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a60
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    WorkSetConst 0x8020, 17
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a61
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    WorkSetConst 0x8020, 18
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a62
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    WorkSetConst 0x8020, 19
    VMCall L_0058
    FlagSet EVENT_FLAG_0x0a63
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
