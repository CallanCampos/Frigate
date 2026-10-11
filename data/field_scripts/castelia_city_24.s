#include "asm/field_script.inc"
#include "text/script/castelia_city_24.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So, I'm a “curiosee\"!\nDo you know what a “curiosee\" is?[f000]븁\u0000\nIt's a person who is curious about\neverything, who checks places[f000]븀\u0000\nwith nothing there, and who talks[f000]븀\u0000\nto all sorts of people!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity24_Text_ImCurioseeKnowWhat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 371
    WorkSet 0x8001, 1
    WorkSet 0x8002, 209
    WorkSet 0x8003, 1
    WorkSet 0x8004, 2
    WorkSet 0x8005, 2
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Whooon!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity24_Text_Whooon, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
