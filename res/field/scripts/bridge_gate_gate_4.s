#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
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
    // "As its name suggests, Village Bridge\nis a bridge with a village on it![f000]븁\u0000\nThat's right! No matter what anyone\nelse says, that's what I think it is!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate4_Text_ItsNameSuggestsVillage, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Dark musicians of\nVillage Bridge aren't too bad..."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate4_Text_DarkMusiciansVillageBridge, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
