#include "asm/field_script.inc"
#include "text/script/accumula_gate.h"

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
    // "Good afternoon, Trainer![f000]븁\u0000\nThe electric bulletin board also\ndisplays information about mass[f000]븀\u0000\noutbreaks! Be sure to check it!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaGate_Text_GoodAfternoonTrainerElectric, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I love imagining what letter will display\nnext on the electric bulletin board...[f000]븀\u0000\nIt makes me so happy!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaGate_Text_LoveImaginingWhatLetter, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
