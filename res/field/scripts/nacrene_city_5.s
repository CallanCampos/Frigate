#include "asm/field_script.inc"
#include "text/script/nacrene_city_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_5:
    WorkSetConst 0x8020, 0
    Cmd_02B3 2, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0039
    FlagReset EVENT_FLAG_0x03e0

L_0039:
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm thinking about becoming\nLoblolly's apprentice!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity5_Text_ImThinkingAboutBecoming, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Loblolly's furniture is the finest around!\nIt's like furniture out of a dream."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity5_Text_LoblollysFurnitureFinestAround, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "She said, even if I'm clumsy,\nif I keep at it, I can make furniture!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity5_Text_SheSaidEvenIf, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm surrounded by furniture I admire!\nNow THIS is a dream world!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity5_Text_ImSurroundedByFurniture, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
