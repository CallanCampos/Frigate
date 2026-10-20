#include "asm/field_script.inc"
#include "text/script/humilau_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0195
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0039
    // "Whoever trades will be my best friend\nafter trading Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_WhoeverTradesWillBest, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0106

L_0039:
    // "Hey, hey, hey!\nDo you know the Pokémon called Mantine?[f000]븁\u0000\nIf you have one, trade my Tangrowth\nfor your Mantine. OK?"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_HeyHeyHeyKnow, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F8
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E4
    WorkSetConst 0x8022, 0
    FieldTradeCheck 0x8022, TRADE_TANGROWTH, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D0
    // "Pretty good!\nOK. Let's trade Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_PrettyGoodOkLets, 0, 0
    MsgWinCloseAll
    FieldTradeStart 27, 0x8020
    // "Hey, hey, hey! The Tangrowth\nI gave you is awesome, isn't it?[f000]븁\u0000\nI got your Mantine,\nand I feel awesome, too!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_HeyHeyHeyTangrowth, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0195
    VMJump L_00DE

L_00D0:
    // "No, no, no. What I want is\na Mantine."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_NoNoNoWhat, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00DE:
    VMJump L_00F2

L_00E4:
    // "OK. That's fine. But if you change your\nmind, let's trade Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_OkThatsFineBut, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00F2:
    VMJump L_0106

L_00F8:
    // "OK. That's fine. But if you change your\nmind, let's trade Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_OkThatsFineBut, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0106:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've come to give fashion tips to\nmy boyfriend..."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity3_Text_IveComeGiveFashion, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
