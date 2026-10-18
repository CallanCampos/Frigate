#include "asm/field_script.inc"
#include "text/script/nuvema_town_5.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "                                                                                                         "
    SystemMsg NuvemaTown5_Text_Empty, 2
    LastKeyWait
    InfoMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
