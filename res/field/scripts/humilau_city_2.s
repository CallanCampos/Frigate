#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    Random EVENT_WORK_0x400b, 5
    WorkSetConst 0x8020, 1
    WorkAdd 0x8020, EVENT_WORK_0x400b
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
