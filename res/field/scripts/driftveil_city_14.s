#include "asm/field_script.inc"
#include "text/script/driftveil_city_14.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey! Stunfisk! Panpour!\nMy dear precious Pokémon![f000]븀\u0000\nKick back and relax today!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity14_Text_HeyStunfiskPanpourDear, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 515, 0
    // "Papur-pur!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity14_Text_PapurPur, 0, 0
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
    PVPlay 618, 0
    // "Stun!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity14_Text_Stun, 0, 0
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
    WorkSet 0x8000, 229
    WorkSet 0x8001, 1
    WorkSet 0x8002, 322
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
