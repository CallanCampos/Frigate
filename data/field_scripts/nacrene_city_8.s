#include "asm/field_script.inc"
#include "text/script/nacrene_city_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
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

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Dire Hit? In Unova, it's called Dire Hit.\nHuh? No difference?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity8_Text_DireHitUnovaIts, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's been two years since I opened for\nbusiness, and thanks to everyone,[f000]븀\u0000\nI'm doing great![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity8_Text_ItsBeenTwoYears, 0x8011, 2, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 14
    WorkSet 0x8001, 0
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
