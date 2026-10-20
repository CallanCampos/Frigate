#include "asm/field_script.inc"
#include "text/script/aspertia_city_5.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPushFlag EVENT_FLAG_0x0961
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0045
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey there, [f000]Ā\u0001\u0000![f000]븁\u0000\nWhy, look at that! You've got a Pokémon\nwith you! That's great!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity5_Text_HeyThereWhyLook, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0059

L_0045:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey there, [f000]Ā\u0001\u0000![f000]븁\u0000\nGoing to have [f000]Ā\u0001\u0001 brag to you\nabout his Pokémon again today?"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity5_Text_HeyThereGoingHave, 0, 0
    LastKeyWait
    ActorMsgClose

L_0059:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0961
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0094
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Take good care of your Pokémon![f000]븁\u0000\nI'm sure that little one will show\nyou a whole new world!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity5_Text_TakeGoodCarePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00A8

L_0094:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you have a Pokémon with you,\nyou can even walk outside of town!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity5_Text_IfHavePokemonCan, 0, 0
    LastKeyWait
    ActorMsgClose

L_00A8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
