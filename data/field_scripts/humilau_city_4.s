#include "asm/field_script.inc"
#include "text/script/humilau_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0031
    FlagSet EVENT_FLAG_0x0334
    FlagReset EVENT_FLAG_0x0335
    FlagReset EVENT_FLAG_0x0336

L_0031:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Somehow, customers don't come\nto this place.[f000]븁\u0000\nOh! I have an idea!\nYou're a Trainer, aren't you?[f000]븁\u0000\nDo your best and become\nthe Champion![f000]븁\u0000\nThen, I can advertise this place\nas a room that the Champion visited!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_SomehowCustomersDontCome, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You're a wonderful person.\nYou can do things for others.[f000]븁\u0000\nIf such a person rings the bell,\nthe sound should reach here..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_YoureWonderfulPersonCan, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0092

L_007E:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "To choose this place for our honeymoon.\nThat's the man I chose!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_ChoosePlaceOurHoneymoon, 0, 0
    LastKeyWait
    ActorMsgClose

L_0092:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0112
    // "If the sound of the bell at the\nCelestial Tower on Route 7 reaches[f000]븀\u0000\nthis room, we can be happy![f000]븁\u0000\nI heard such a rumor.\nCan I ask you a favor?[f000]븀\u0000\nWill you ring the bell for my wife?"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_IfSoundBellCelestial, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FE
    // "You're so nice![f000]븁\u0000\nIt would be great if this world\nwas full of people like you!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_YoureNiceWouldGreat, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ee, 1
    FlagReset EVENT_FLAG_0x0337
    FlagReset EVENT_FLAG_0x0338
    FlagReset EVENT_FLAG_0x0339
    FlagReset EVENT_FLAG_0x033a
    VMJump L_010C

L_00FE:
    // "Oh, come on. Please ring the bell\nto make us happy!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_OhComePleaseRing, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_010C:
    VMJump L_01C5

L_0112:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 5
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0149
    // "I wonder if the sound of the bell on\nRoute 7 will reach here."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_WonderIfSoundBell, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01C5

L_0149:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A4
    // "Oh, you! As for the sound of the bell,\nI don't think I've heard it yet...[f000]븁\u0000\n...[f000]븁\u0000\nWhat? Did they say such things\nat the Celestial Tower...?[f000]븀\u0000\nWell, putting that aside...[f000]븁\u0000\nHere! This is a thank-you gift!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_OhSoundBellDont, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 28
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "I wish for their happiness..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_WishTheirHappiness, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ee, 7
    VMJump L_01C5

L_01A4:
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C5
    // "I wish for their happiness..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity4_Text_WishTheirHappiness, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01C5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
