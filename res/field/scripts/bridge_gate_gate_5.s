#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
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
    // "...I'm sorry.\nI ended up losing my uniform."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate5_Text_ImSorryEndedUp, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I did my best to choose a Pokémon\nwho was both cute and strong!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate5_Text_DidBestChoosePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Minccino!\nMy grandpa caught it for me![f000]븀\u0000\nTail Slap is its specialty!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate5_Text_MinccinoGrandpaCaughtTail, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 572, 0
    // "Chichino! ♪"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate5_Text_Chichino, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
