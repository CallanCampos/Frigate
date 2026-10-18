#include "asm/field_script.inc"
#include "text/script/striaton_city_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003F
    WorkSetConst EVENT_WORK_0x4020, 209
    VMJump L_0045

L_003F:
    WorkSetConst EVENT_WORK_0x4020, 129

L_0045:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've trained only Fire-type Pokémon,\n'cause they're my favorites![f000]븁\u0000\nThey don't do well against Water-, Rock-,\nor Ground-type Pokémon and moves.[f000]븁\u0000\nBut thinking about how to compensate\nfor that is one of the fun things[f000]븀\u0000\nabout being a Trainer."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity8_Text_IveTrainedOnlyFire, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes I look through my PC Box and\npick out an interesting Pokémon to raise![f000]븁\u0000\nThere are so many things you never\nknow until you raise a certain Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity8_Text_SometimesLookThroughPc, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Skreee!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity8_Text_Skreee, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DE
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 546, 0
    // "Fwee-oosh!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity8_Text_FweeOosh, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_00FA

L_00DE:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 548, 0
    // "Fwee lee lee... ♪"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity8_Text_FweeLeeLee, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose

L_00FA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
