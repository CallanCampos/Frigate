#include "asm/field_script.inc"
#include "text/script/151.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Please take your designated position\nand start the battle."
    ParentActorMsg MSGFILE_SCRIPT, Bank151_Text_PleaseTakeDesignatedPosition, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
