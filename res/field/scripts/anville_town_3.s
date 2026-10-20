#include "asm/field_script.inc"
#include "text/script/anville_town_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Anville Town is packed on the weekends.\nEveryone sure loves trains!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown3_Text_AnvilleTownPackedWeekends, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 91
    WordSetItemName 0, 0x8020
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "On the weekend, many [f000]ĉ\u0001\u0000\ncollectors gather here.[f000]븁\u0000\nThey'll trade them for different items."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown3_Text_WeekendManyCollectorsGather, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "One day, I'm going to take the\nSubway Bosses Ingo and Emmet down!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown3_Text_OneDayImGoing, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
