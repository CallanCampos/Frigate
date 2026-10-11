#include "asm/field_script.inc"
#include "text/script/strange_house_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMSleep 16
    FlagReset EVENT_FLAG_0x0382
    ActorAdd 0
    ActorCmdExec 0, Movement_0084
    ActorCmdWait
    VMSleep 8
    // "An everlasting dark dream...\nAn endless dream of darkness...[f000]븁\u0000\nDad, Mom, Abra...\nWhere are you...?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StrangeHouse3_Text_EverlastingDarkDreamEndless, 0, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 0, Movement_00D8
    ActorCmdWait
    ActorDelete 0
    FlagSet EVENT_FLAG_0x0382
    WorkSetConst EVENT_WORK_0x4075, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The portrait has fallen..."
    InfoMsg StrangeHouse3_Text_PortraitHasFallen, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "No one saw it change, but now\nthe picture is hanging upside down..."
    InfoMsg StrangeHouse3_Text_NoOneSawChange, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0084:
    Move 73, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 2
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 3
    Move 70, 1
    Move 61, 3
    Move 69, 1
    Move 61, 4
    Move 70, 1
    MoveEnd

Movement_00D8:
    Move 69, 1
    Move 61, 4
    Move 70, 1
    Move 61, 3
    Move 69, 1
    Move 61, 3
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 2
    Move 70, 1
    Move 61, 2
    Move 69, 1
    Move 61, 1
    Move 70, 1
    Move 61, 1
    Move 69, 1
    Move 61, 1
    Move 70, 1
    MoveEnd
