#include "asm/field_script.inc"
#include "text/script/strange_house.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's impossible to tell\nwhen this portrait was painted..."
    InfoMsg StrangeHouse_Text_ItsImpossibleTellWhen, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "If you look at it closely, you can see\nit's covered in scratches."
    InfoMsg StrangeHouse_Text_IfLookCloselyCan, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
