#include "asm/field_script.inc"
#include "text/script/icirrus_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Reshiram and Zekrom\nhave lived for thousands of years.[f000]븁\u0000\nThey have likely met many heroes\nand bestowed their knowledge on them...[f000]븁\u0000\nBut the truth remains a mystery,\nand the world still isn't ideal.[f000]븁\u0000\nYet those two still believe in people.\nEven in heroes... How foolish."
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity3_Text_ReshiramZekromHaveLived, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey! Know what?\nBrycen is a popular actor again,[f000]븀\u0000\njust like he used to be!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity3_Text_HeyKnowWhatBrycen, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "After Brycen left, challengers\nstopped coming to the Gym...[f000]븁\u0000\nEven if you don't change,\nthe things around you sure do..."
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity3_Text_AfterBrycenLeftChallengers, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 613, 0
    // "Chooo!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity3_Text_Chooo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
