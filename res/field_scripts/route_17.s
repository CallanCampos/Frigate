#include "asm/field_script.inc"
#include "text/script/route_17.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 17\nBeware of rapidly flowing water!"
    MsgPlaceSign Route17_Text_Route17BewareRapidly, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
