#include "asm/field_script.inc"
#include "text/script/global_10435.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkGet 0x8020, 0x8000
    WorkGet 0x8021, 0x8001
    WordSetPlayerName 0
    WordSetPassPowerName 2, 0x8020
    // "[f000]Ā\u0001\u0000 used the Pass Power\n“[f000]Ĝ\u0001\u0002!\"[f000]븁\u0000"
    SystemMsg Global10435_Text_UsedPassPower, 2
    InfoMsgClose
    Cmd_01DF
    WorkAdd 0x8021, 8
    SystemMsg 0x8021, 2
    LastKeyWait
    InfoMsgClose
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8025, 1

L_007E:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B4
    PassPowerPopDepletedID 0x8023, 0x8024
    WorkGet 0x8025, 0x8024
    WordSetPassPowerName 2, 0x8023
    WorkAdd 0x8024, 1
    SystemMsg 0x8024, 2
    VMJump L_007E

L_00B4:
    LastKeyWait
    InfoMsgClose
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
