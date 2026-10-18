#include "asm/field_script.inc"
#include "text/script/shopping_mall_2.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This mountain of cardboard boxes...[f000]븁\u0000\nThey're all full of electrical appliances\nwe can't sell anymore.[f000]븁\u0000\nThey're just going to waste. I wonder if\nsomeone could put them to good use?"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall2_Text_MountainCardboardBoxesTheyre, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
