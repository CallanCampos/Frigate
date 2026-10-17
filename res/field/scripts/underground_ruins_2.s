#include "asm/field_script.inc"
#include "text/script/underground_ruins_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0044
    VMSleep 4
    FadeOutBlack
    ActorCmdWait
    FadeWait
    RTReserveScript 5
    MapChangeCore ZONE_UNDERGROUND_RUINS, 15, 0, 0, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0044:
    Move 9, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    FadeInBlack
    ActorCmdExec 255, Movement_0064
    ActorCmdWait
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0064:
    Move 8, 2
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Chamber of the one that joins\nthe sun in protecting this place."
    InfoMsg UndergroundRuins2_Text_ChamberOneJoinsSun, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Two answers are needed to\nfind the true path.[f000]븁\u0000\nCourageous one,\nlet me tell you the answer I know.[f000]븁\u0000\nCheck the ground six steps down and\n● steps right of the eyeball.[f000]븁\u0000\nThe other answer can only be found\nwhen the moon is in the sky."
    InfoMsg UndergroundRuins2_Text_TwoAnswersNeededFind, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The world is split into two:\nday and night...black and white..."
    InfoMsg UndergroundRuins2_Text_WorldSplitIntoTwo, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The protectors were born\nout of rock, ice, and magma..."
    InfoMsg UndergroundRuins2_Text_ProtectorsWereBornOut, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
