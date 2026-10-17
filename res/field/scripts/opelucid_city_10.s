#include "asm/field_script.inc"
#include "text/script/opelucid_city_10.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0043
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pursuing ideals...\nWhat does that really mean?[f000]븁\u0000\nYou see, there was this guy called N,\nwho the legendary Pokémon Zekrom[f000]븀\u0000\nrecognized as the hero..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity10_Text_PursuingIdealsWhatDoes, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0057

L_0043:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pursuing truth...\nWhat does that really mean?[f000]븁\u0000\nYou see, there was this guy called N,\nwho the legendary Pokémon Reshiram[f000]븀\u0000\nrecognized as the hero..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity10_Text_PursuingTruthWhatDoes, 0, 0
    LastKeyWait
    ActorMsgClose

L_0057:
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
    WorkSet 0x8000, 228
    WorkSet 0x8001, 1
    WorkSet 0x8002, 236
    WorkSet 0x8003, 2
    WorkSet 0x8004, 3
    WorkSet 0x8005, 3
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
