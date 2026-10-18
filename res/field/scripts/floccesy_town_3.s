#include "asm/field_script.inc"
#include "text/script/floccesy_town_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The biggest city in Unova is Castelia![f000]븁\u0000\nI want to take the ship from Virbank\nand go play there!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown3_Text_BiggestCityUnovaCastelia, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "On a sunny day, my Patrat's fur\ngets all fluffy![f000]븀\u0000\nI can't help wanting to pet it!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown3_Text_SunnyDayPatratsFur, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Skuwaaa!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown3_Text_Skuwaaa, 0, 0
    PVWait
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
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    WorkSet 0x8002, 278
    WorkSet 0x8003, 0
    WorkSet 0x8004, 1
    WorkSet 0x8005, 1
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
