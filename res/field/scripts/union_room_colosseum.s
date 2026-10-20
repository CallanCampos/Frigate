#include "asm/field_script.inc"
#include "text/script/union_room_colosseum.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Please take your designated position\nand start the battle."
    ParentActorMsg MSGFILE_SCRIPT, UnionRoomColosseum_Text_PleaseTakeDesignatedPosition, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
