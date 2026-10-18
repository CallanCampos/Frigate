#include "asm/field_script.inc"
#include "text/script/undella_town_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder why only the father\nfrom The Riches stayed here..."
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown2_Text_WonderWhyOnlyFather, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WordSetLoadJoinAvenueName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hear that the shops in\n[f000]Ĺ\u0001\u0000 are run by amazing[f000]븀\u0000\nPokémon Trainers.[f000]븁\u0000\nOK! I can't lose to them!\nI'm getting excited about this!"
    ParentActorMsg MSGFILE_SCRIPT, UndellaTown2_Text_HearShopsRunBy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
