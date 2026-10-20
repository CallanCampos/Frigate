#include "asm/field_script.inc"
#include "text/script/bridge_gate_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello! If you cross the Skyarrow Bridge\nfrom here, you'll reach Castelia City."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGate2_Text_HelloIfCrossSkyarrow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "They say the Skyarrow Bridge was built\nas a result of the tireless pursuit[f000]븀\u0000\nof a safe and sturdy structure.[f000]븁\u0000\nI wonder what kind of Pokémon built it\nand how they felt while building it."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGate2_Text_TheySaySkyarrowBridge, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Use Fly, and you'll be there in no time![f000]븁\u0000\nBut some people prefer to cross\nthat bridge to go to Castelia City."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGate2_Text_UseFlyYoullThere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
