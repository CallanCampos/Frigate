#include "asm/field_script.inc"
#include "text/script/castelia_city_20.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My friend worries about everyone's\nhealth when they set off on journeys,[f000]븀\u0000\nso he always nicknames Pokémon[f000]븀\u0000\nhe trades “Gesundheit.\"[f000]븁\u0000\nI wonder how he's doing?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity20_Text_FriendWorriesAboutEveryones, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 511, 0
    // "Gesundheit: ...Pan?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity20_Text_GesundheitPan, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Mr. Lock...\nHe was a mysterious man..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity20_Text_MrLockHeMysterious, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 223
    WorkSet 0x8001, 1
    WorkSet 0x8002, 382
    WorkSet 0x8003, 3
    WorkSet 0x8004, 4
    WorkSet 0x8005, 4
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
    .balign 4, 0
