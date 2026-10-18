#include "asm/field_script.inc"
#include "text/script/bridge_gate_gate_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Watch it! People use “railway fan\" as\na catchall term, but there are many[f000]븀\u0000\ntypes of railway fans![f000]븁\u0000\nThere are riding fans, detraining fans,\nstation fans, train-car fans,[f000]븀\u0000\nschedule-table fans, picture-taking fans,[f000]븀\u0000\nrecording fans, and more![f000]븁\u0000\nDon't go thinking they're all the same!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate2_Text_WatchPeopleUseRailway, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "For someone like me, who is checking\nout rail lines all over the world,[f000]븀\u0000\nAnville Town, where you can look at[f000]븀\u0000\nvarious trains, gets pretty high marks!"
    ParentActorMsg MSGFILE_SCRIPT, BridgeGateGate2_Text_SomeoneLikeWhoChecking, 0, 0
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
    WorkSet 0x8000, 34
    WorkSet 0x8001, 1
    WorkSet 0x8002, 346
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
