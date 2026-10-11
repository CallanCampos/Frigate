#include "asm/field_script.inc"
#include "text/script/black_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A town is something\nthat keeps changing..."
    ParentActorMsg MSGFILE_SCRIPT, BlackGate_Text_TownSomethingKeepsChanging, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Of course! When you compare\ndifferent things, it's natural to find[f000]븀\u0000\ngood points and bad points!"
    ParentActorMsg MSGFILE_SCRIPT, BlackGate_Text_CourseWhenCompareDifferent, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Today, we're dancing for no reason. ♪\nSomeday, we'll disappear for no reason."
    ParentActorMsg MSGFILE_SCRIPT, BlackGate_Text_TodayWereDancingNo_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
