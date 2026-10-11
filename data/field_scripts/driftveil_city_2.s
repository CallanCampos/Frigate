#include "asm/field_script.inc"
#include "text/script/driftveil_city_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Driftveil Chateau Hotel.\nWe're currently all booked up,[f000]븀\u0000\nbut feel free to enjoy the ambiance."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity2_Text_WelcomeDriftveilChateauHotel, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
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
