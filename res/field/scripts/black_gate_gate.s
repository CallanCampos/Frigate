#include "asm/field_script.inc"
#include "text/script/black_gate_gate.h"

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
    // "If you pass through this gate...\nWhere could it be connected to?"
    ParentActorMsg MSGFILE_SCRIPT, BlackGateGate_Text_IfPassThroughGate, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "People come and go from\nthe cities and the country while[f000]븀\u0000\nsearching for a place where they belong."
    ParentActorMsg MSGFILE_SCRIPT, BlackGateGate_Text_PeopleComeGoFrom, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Unova's Challenge...\nBlack Tower or White Treehollow...[f000]븀\u0000\nWhat in the world are they like?"
    ParentActorMsg MSGFILE_SCRIPT, BlackGateGate_Text_UnovasChallengeBlackTower, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
