#include "asm/field_script.inc"
#include "text/script/castelia_city_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_2:
    CasteliaRushInit
    VMHalt

Script_3:
    CasteliaRushInit
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "GAME FREAK"
    MsgPlaceSign CasteliaCity6_Text_GameFreak, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
