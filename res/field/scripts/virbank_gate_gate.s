#include "asm/field_script.inc"
#include "text/script/virbank_gate_gate.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome! Pass through the gate,\nand you'll arrive at Pokéstar Studios!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankGateGate_Text_WelcomePassThroughGate, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokéstar Studios! That is a place\nwhere you can experience different lives!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankGateGate_Text_PokestarStudiosPlaceWhere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
