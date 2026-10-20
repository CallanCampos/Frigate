#include "asm/field_script.inc"
#include "text/script/striaton_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It must have taken a lot of resolve\nfor Cilan, Chili, and Cress to[f000]븀\u0000\nresign as Gym Leaders and leave[f000]븀\u0000\nto go retrain themselves."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity3_Text_MustHaveTakenLot, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0186
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0059
    // "I hear some people have felt\nthe presence of a mysterious Pokémon[f000]븀\u0000\nin the Dreamyard lately!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity3_Text_HearSomePeopleHave, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_006B

L_0059:
    // "Latias?\nIt was in the Dreamyard?![f000]븁\u0000\nIt must have appeared there because\nit sensed the dreams lingering there."
    // "Latios?\nIt was in the Dreamyard?![f000]븁\u0000\nIt must have appeared there because\nit sensed the dreams lingering there."
    ActorMsgVersioned 1024, StriatonCity3_Text_LatiasDreamyardMustHave, StriatonCity3_Text_LatiosDreamyardMustHave, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_006B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Fennel left for Castelia City.\nWhat's so great about the city, anyway?"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity3_Text_FennelLeftCasteliaCity, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
