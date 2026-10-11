#include "asm/field_script.inc"
#include "text/script/aspertia_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0039
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It looks like my son found\nsomething important.[f000]븁\u0000\nIt's all because Pokémon--\nand you--were by his side!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity3_Text_LooksLikeSonFound, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_007D

L_0039:
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0069
    WordSetLoadRivalName 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's too bad. If you had a Pokémon\nwith you as well, you could compete[f000]븀\u0000\nwith [f000]Ā\u0001\u0001 and see who is the[f000]븀\u0000\nbetter Trainer!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity3_Text_ItsTooBadIf, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_007D

L_0069:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "To be honest, I don't want\nmy son to go on a journey.[f000]븀\u0000\nI mean, his goal is...[f000]븁\u0000\nBut there is no parent who doesn't\nwish for his or her child to grow."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity3_Text_HonestDontWantSon, 0, 0
    LastKeyWait
    ActorMsgClose

L_007D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B8
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if she and Liepard will\ngo on a journey together as well..."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity3_Text_WonderIfSheLiepard, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00FD

L_00B8:
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Someday you will both go on a\njourney with your Pokémon, too!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity3_Text_SomedayWillBothGo, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00FD

L_00E5:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0000...[f000]븁\u0000\nIf [f000]Ā\u0001\u0001 loses his way\non the path, or in life really,[f000]븀\u0000\nplease help him, won't you?[f000]븁\u0000\nHe's the kind of person who, well,\nwho lets rage build inside him."
    // "[f000]Ā\u0001\u0000...[f000]븁\u0000\nIf [f000]Ā\u0001\u0001 loses his way\non the path, or in life really,[f000]븀\u0000\nplease help him, won't you?[f000]븁\u0000\nHe's the kind of person who, well,\nwho lets rage build inside him."
    ActorMsgGendered 1024, AspertiaCity3_Text_IfLosesHisWay, AspertiaCity3_Text_IfLosesHisWay_2, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_00FD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
