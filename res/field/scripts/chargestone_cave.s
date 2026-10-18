#include "asm/field_script.inc"
#include "text/script/chargestone_cave.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Chargestone Cave\nA shocking experience!"
    MsgPlaceSign ChargestoneCave_Text_ChargestoneCaveShockingExperience, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
