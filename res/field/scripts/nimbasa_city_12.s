#include "asm/field_script.inc"
#include "text/script/nimbasa_city_12.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you battle with a friend via\nwireless communications, you can[f000]븀\u0000\nuse the Wonder Launcher rule[f000]븀\u0000\nto use items in battle!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity12_Text_WhenBattleFriendVia, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "As the Wonder Launcher's energy\ncharges up, you get more points.[f000]븁\u0000\nYou can spend those points\nto use various battle items."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity12_Text_WonderLaunchersEnergyCharges, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A few years ago, Nimbasa's\nglitz and glamor was pleasant,[f000]븀\u0000\nbut recently it's too much for me...[f000]븁\u0000\nMaybe I should go relax in the country.\nMy Pokémon might find that[f000]븀\u0000\nmore comfortable as well."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity12_Text_FewYearsAgoNimbasas, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0099
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So about those people whose Pokémon\nwere stolen...[f000]븁\u0000\nI hear former Team Plasma members\ncame around and returned the Pokémon[f000]븀\u0000\nthey took from a guy.[f000]븁\u0000\nBut is that enough?\nI'm not satisfied by that!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity12_Text_AboutThosePeopleWhose, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00AD

L_0099:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hear that some people's Pokémon\nwere stolen by Team Plasma,[f000]븀\u0000\nand they still haven't been reunited."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity12_Text_HearSomePeoplesPokemon, 0, 0
    LastKeyWait
    ActorMsgClose

L_00AD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 535, 0
    // "Tyyyym!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity12_Text_Tyyyym, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
