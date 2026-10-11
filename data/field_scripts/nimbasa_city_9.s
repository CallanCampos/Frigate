#include "asm/field_script.inc"
#include "text/script/nimbasa_city_9.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We love sports.\nWatching games is great, but we enjoy[f000]븀\u0000\nwatching practices, too!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_WeLoveSportsWatching, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "New styles of basketball and tennis\ncreated by people and Pokémon...[f000]븀\u0000\nThese may be advanced forms of sports."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_NewStylesBasketballTennis, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0085
    // "Hey! Now! There!\nAh! No! No, no![f000]븁\u0000\nYes! Yes, yes!\nThat's right! Ha![f000]븁\u0000\nWhy don't you do it like I said?!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_HeyNowThereAh, 0, 0
    VMJump L_008F

L_0085:
    // "I watch them practice quietly.\nBecause I trust the athletes!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_WatchThemPracticeQuietly, 0, 0

L_008F:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hm-hum! I can copy that play in my\nnext game."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_HmHumCanCopy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Did you see it?\nThat's a great muscle move![f000]븁\u0000\nGood muscle! Good hustle!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_DidSeeThatsGreat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh! This could be a once-in-a-lifetime\ngame! I might witness history!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_OhCouldOnceLifetime, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Woooow! Coooool!\nSomeday I want to be on that court!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_WoooowCooooolSomedayWant, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "I believe in big money!\n...No, I mean I will gain glory!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity9_Text_BelieveBigMoneyNo, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
