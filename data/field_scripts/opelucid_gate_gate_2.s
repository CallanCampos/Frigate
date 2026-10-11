#include "asm/field_script.inc"
#include "text/script/opelucid_gate_gate_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
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
    // "Trainer who came to Opelucid City! Hello![f000]븁\u0000\nTrainer who is going to Route 11!\nPlease come again!"
    // "Trainer who came to Opelucid City! Hello![f000]븁\u0000\nTrainer who is going to Route 11!\nPlease come again!"
    ActorMsgVersioned 1024, OpelucidGateGate2_Text_TrainerWhoCameOpelucid, OpelucidGateGate2_Text_TrainerWhoCameOpelucid_2, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
