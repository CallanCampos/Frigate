#include "asm/field_script.inc"
#include "text/script/twist_mountain_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a rock covered with ice.\nTouching it could make you freeze."
    InfoMsg TwistMountain6_Text_ItsRockCoveredIce, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    FlagSet EVENT_FLAG_0x03b6
    FlagSet EVENT_FLAG_0x03b7
    FlagSet EVENT_FLAG_0x03b9
    FlagSet EVENT_FLAG_0x03bb
    FlagSet EVENT_FLAG_0x03bd
    FlagReset EVENT_FLAG_0x03b4
    FlagReset EVENT_FLAG_0x03b5
    FlagReset EVENT_FLAG_0x03b8
    FlagReset EVENT_FLAG_0x03ba
    FlagReset EVENT_FLAG_0x03bc
    VMHalt
    .balign 4, 0
