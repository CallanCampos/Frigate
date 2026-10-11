#include "asm/field_script.inc"
#include "text/script/opelucid_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 539
    WorkSet 0x8001, 1
    WorkSet 0x8002, 240
    WorkSet 0x8003, 3
    WorkSet 0x8004, 4
    WorkSet 0x8005, 4
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wh-what's going\nto happen to Opelucid City?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_WhWhatsGoingHappen, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01bc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0163
    VMStackPushFlag EVENT_FLAG_0x00f1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0149
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I have an item that I don't know how to\nuse. Would you give it a try and see if[f000]븀\u0000\nyou can make it work?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_HaveItemDontKnow, 1, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0133
    // "You may be able to master it. Here it is![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_MayAbleMasterHere, 1, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 543
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "If a Pokémon holds a Ring Target, it can\nbe hit even by a move that would usually[f000]븀\u0000\nhave no effect.[f000]븁\u0000\nFor example, a Normal-type move would\nhit a Ghost-type Pokémon.[f000]븁\u0000\nMastering this item is a bit tough...\nActually, it's very tough, but think[f000]븀\u0000\nhow useful it could be!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_IfPokemonHoldsRing, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x00f1
    VMJump L_0143

L_0133:
    // "I know...\nYou'll also have trouble figuring it out."
    ActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_KnowYoullAlsoHave, 1, 0, 0
    LastKeyWait
    ActorMsgClose

L_0143:
    VMJump L_015D

L_0149:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If a Pokémon holds a Ring Target, it can\nbe hit even by a move that would usually[f000]븀\u0000\nhave no effect.[f000]븁\u0000\nFor example, a Normal-type move would\nhit a Ghost-type Pokémon.[f000]븁\u0000\nMastering this item is a bit tough...\nActually, it's very tough, but think[f000]븀\u0000\nhow useful it could be!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_IfPokemonHoldsRing, 0, 0
    LastKeyWait
    ActorMsgClose

L_015D:
    VMJump L_0177

L_0163:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's cold...[f000]븁\u0000\nAnd Dragon types really don't like cold!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_ItsColdDragonTypes, 0, 0
    LastKeyWait
    ActorMsgClose

L_0177:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01bc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CB
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BB
    // "I was Iris's coach![f000]븁\u0000\nEven from the day Iris came here,\nshe was so much stronger than me![f000]븁\u0000\nNow, I wouldn't stand a chance!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_IrissCoachEvenFrom, 0, 0
    VMJump L_01C5

L_01BB:
    // "Drayden teaches the move Draco Meteor.\nIt's the strongest Dragon-type move.[f000]븁\u0000\nBut the Special Attack of the Pokémon\nthat uses it drops sharply."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_DraydenTeachesMoveDraco, 0, 0

L_01C5:
    VMJump L_01D5

L_01CB:
    // "Even Drayden can't handle a\nstrange situation like this alone..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity3_Text_EvenDraydenCantHandle, 0, 0

L_01D5:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
