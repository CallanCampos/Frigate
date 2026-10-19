#include "asm/field_script.inc"
#include "text/script/accumula_town_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you just can't find common ground,\nit's time for a Pokémon battle![f000]븁\u0000\nBut I think my husband might be\ntaking it easy on me..."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown3_Text_WhenJustCantFind, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Look carefully and you'll see that\neven if it's the same kind of Pokémon,[f000]븀\u0000\neach individual has its own[f000]븀\u0000\nNature and strengths."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown3_Text_LookCarefullyYoullSee, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes you fight with others\nbecause they're different from you.[f000]븁\u0000\nBut sometimes you like them\nbecause they're different![f000]븁\u0000\nSometimes...I don't get it!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown3_Text_SometimesFightOthersBecause, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
