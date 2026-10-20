#include "asm/field_script.inc"
#include "text/script/castelia_city_18.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    MedalGetCount 3, 0x8020
    WordSetNumber 0, 0x8020, 3
    // "Hi there! You're participating\nin the Medal Rally, aren't you?[f000]븁\u0000\nLet me have a look here.\nThe number of Medals you have is...[f000]븀\u0000\n[f000]Ȃ\u0001\u0000![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_HiThereYoureParticipating, 0, 0
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 9
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0069
    // "I have 10!\nI'm ahead of you!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_Have10ImAhead, 0, 0
    VMJump L_0185

L_0069:
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 29
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_009C
    // "I have 30!\nYou've collected many,[f000]븀\u0000\nbut I'm not going to lose!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_Have30YouveCollected, 0, 0
    VMJump L_0185

L_009C:
    VMStackPush 0x8020
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 49
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00CF
    // "I have 50![f000]븁\u0000\nDid you know? If you collect\n50 Medals, something good happens."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_Have50DidKnow, 0, 0
    VMJump L_0185

L_00CF:
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 99
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0102
    // "Whoa! You reached the goal, too?[f000]븁\u0000\nMe?[f000]븁\u0000\nI have 100!\nHeh heh!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_WhoaReachedGoalToo, 0, 0
    VMJump L_0185

L_0102:
    VMStackPush 0x8020
    VMStackPushConst 100
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 199
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0135
    // "Amazing!\nYou've collected that many?[f000]븁\u0000\nYou have more than me![f000]븁\u0000\nI've been having trouble\ngetting more since I collected 100..."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_AmazingYouveCollectedMany, 0, 0
    VMJump L_0185

L_0135:
    VMStackPush 0x8020
    VMStackPushConst 200
    VMStackCmp CMP_GE
    VMStackPush 0x8020
    VMStackPushConst 254
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0168
    // "If anyone can collect every\nMedal, it's you![f000]븁\u0000\nI don't know how many\nthere are in all, but...[f000]븁\u0000\nMe? I only have 150...[f000]븁\u0000\nI'll do my best!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_IfAnyoneCanCollect, 0, 0
    VMJump L_0185

L_0168:
    VMStackPush 0x8020
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0185
    // "Wow![f000]븁\u0000\nYou collected every Medal?!\nA-amazing! That's too incredible![f000]븁\u0000\nI finally collected 200![f000]븁\u0000\nI'll use you as an inspiration,\nand I'll do my best to get every one!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_WowCollectedEveryMedal, 0, 0

L_0185:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The maniacs for items are sure odd![f000]븁\u0000\nThey'll buy ordinary items\nfor much more than normal![f000]븁\u0000\nIf you hold on to items, even ones\nyou have no use for, you might be able to[f000]븀\u0000\nsell them to a maniac!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_ManiacsItemsSureOdd, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Roggenrola's Ability is Sturdy![f000]븁\u0000\nIf an attack that would knock it out\nhits it when its HP is full,[f000]븀\u0000\nit will stay standing with one HP!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_RoggenrolasAbilitySturdyIf, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Sturrr!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity18_Text_Sturrr, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
