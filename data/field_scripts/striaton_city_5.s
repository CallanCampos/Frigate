#include "asm/field_script.inc"
#include "text/script/striaton_city_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 3
    WorkSet 0x8001, 1
    WorkSet 0x8002, 104
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

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Whatever your age, you can have Pokémon\nbattles if you have Pokémon by your side.[f000]븁\u0000\nIf you use the Internet,\nthere are many people to battle!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity5_Text_WhateverAgeCanHave, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "After many battles, you start to see\nmore deeply and understand things.[f000]븁\u0000\nThat's why I know exactly what\nmy husband is thinking!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity5_Text_AfterManyBattlesStart, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
