#include "asm/field_script.inc"
#include "text/script/nimbasa_gate_gate_2.h"

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
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 215
    WorkSet 0x8001, 1
    WorkSet 0x8002, 263
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

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Two years ago, a Pokémon\ncalled Zoroark was hiding it's lair[f000]븀\u0000\nin the Lostlorn Forest on Route 16."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaGateGate2_Text_TwoYearsAgoPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This way leads to Route 16\nand Marvelous Bridge!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaGateGate2_Text_WayLeadsRoute16, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
