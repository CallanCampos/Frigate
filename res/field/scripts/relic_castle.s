#include "asm/field_script.inc"
#include "text/script/relic_castle.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    MapChangeQuicksand ZONE_RELIC_CASTLE_2, 15, 11
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    MapChangeQuicksand ZONE_RELIC_CASTLE_2, 10, 10
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are two things I've learned\nabout quicksand![f000]븁\u0000\nIf you try to walk through the middle,\nyou'll fall.[f000]븁\u0000\nAnd if you try to run through it,\nyou'll fall."
    ParentActorMsg MSGFILE_SCRIPT, RelicCastle_Text_ThereTwoThingsIve, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
