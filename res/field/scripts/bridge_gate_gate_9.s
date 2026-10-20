#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "How was it?\nFor Pokémon under the ocean,[f000]븀\u0000\nthe world looks like that, doesn't it?"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate9_Text_HowPokemonUnderOcean, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The path is to see rather than to move."
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate9_Text_PathSeeRatherThan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 369
    WorkSet 0x8001, 1
    WorkSet 0x8002, 343
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

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a sign that explains\nthe Marine Tube.[f000]븁\u0000"
    InfoMsg BridgeGateGate9_Text_ItsSignExplainsMarine, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 3, 1
    FieldOpen
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
