#include "asm/field_script.inc"
#include "text/script/village_bridge_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    WorkSetConst 0x8020, 0
    Random 0x8020, 3
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac4
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_009E
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006A
    FlagSet EVENT_FLAG_DAILY_0x0ac2
    VMJump L_009E

L_006A:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0087
    FlagSet EVENT_FLAG_DAILY_0x0ac3
    VMJump L_009E

L_0087:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009E
    FlagSet EVENT_FLAG_DAILY_0x0ac4

L_009E:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CD
    // "A woman was living here with\na group of Patrat.[f000]븁\u0000\nI wonder where they went."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge6_Text_WomanLivingHereGroup, 0, 0
    LastKeyWait
    VMJump L_0111

L_00CD:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F2
    // "Why don't we move into this house?[f000]븁\u0000\nThose musicians are always nearby.\nIsn't that nice?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge6_Text_WhyDontWeMove, 0, 0
    LastKeyWait
    VMJump L_0111

L_00F2:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac4
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0111
    // "What do you think? We've been indecisive\nfor two years. Let's live here, shall we?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge6_Text_WhatThinkWeveBeen, 0, 0
    LastKeyWait

L_0111:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0146
    // "Seeing Watchy Watchog on TV\nreminds me of those Patrat."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge6_Text_SeeingWatchyWatchogTv, 0, 0
    LastKeyWait
    VMJump L_018A

L_0146:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016B
    // "I learned all the names\nof the musicians![f000]븁\u0000\nDerleth, Aickman,\nRusso, and Koontz."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge6_Text_LearnedAllNamesMusicians, 0, 0
    LastKeyWait
    VMJump L_018A

L_016B:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac4
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018A
    // "Maybe we should move in.[f000]븁\u0000\nWe don't even know when the villa\nin Undella Town will be finished..."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge6_Text_MaybeWeShouldMove, 0, 0
    LastKeyWait

L_018A:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
