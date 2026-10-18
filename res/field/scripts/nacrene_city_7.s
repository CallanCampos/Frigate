#include "asm/field_script.inc"
#include "text/script/nacrene_city_7.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003B
    WorkSetConst EVENT_WORK_0x4020, 209
    VMJump L_0041

L_003B:
    WorkSetConst EVENT_WORK_0x4020, 129

L_0041:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokePartyGetMemberByType 0x8008, 2
    WordSetPartyPokeSpecies 0, 0x8008
    // "Oh my, what a lovely Trainer!\nWhat kind of Pokémon do you have?[f000]븁\u0000\nOh, your [f000]ā\u0001\u0000...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_OhWhatLovelyTrainer, 0, 0
    PokePartyGetHappiness 0x8009, 0x8008
    VMStackPush 0x8009
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0089
    // "It's very friendly toward you!\nIt must be happy with you."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_ItsVeryFriendlyToward, 0, 0
    VMJump L_01AF

L_0089:
    VMStackPush 0x8009
    VMStackPushConst 200
    VMStackCmp CMP_GE
    VMStackPush 0x8009
    VMStackPushConst 254
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00C1
    WordSetPartyPokeSpecies 0, 0x8008
    // "You must really like [f000]ā\u0001\u0000\nand always keep it by your side!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_MustReallyLikeAlways, 0, 0
    VMJump L_01AF

L_00C1:
    VMStackPush 0x8009
    VMStackPushConst 150
    VMStackCmp CMP_GE
    VMStackPush 0x8009
    VMStackPushConst 199
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00F9
    WordSetPartyPokeSpecies 0, 0x8008
    // "You and [f000]ā\u0001\u0000 can\nbecome an even more wonderful team!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_CanBecomeEvenMore, 0, 0
    VMJump L_01AF

L_00F9:
    VMStackPush 0x8009
    VMStackPushConst 100
    VMStackCmp CMP_GE
    VMStackPush 0x8009
    VMStackPushConst 149
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_012C
    // "It's a little bit friendly to you...\nSomething like that."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_ItsLittleBitFriendly, 0, 0
    VMJump L_01AF

L_012C:
    VMStackPush 0x8009
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMStackPush 0x8009
    VMStackPushConst 99
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_015F
    // "Hmmm...\nIt may still take some time."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_HmmmMayStillTake, 0, 0
    VMJump L_01AF

L_015F:
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush 0x8009
    VMStackPushConst 49
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0192
    // "Are you just letting it get\nknocked out in Pokémon battles?!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_JustLettingGetKnocked, 0, 0
    VMJump L_01AF

L_0192:
    VMStackPush 0x8009
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AF
    // "What's this? Are you a disciplinarian? Or\ndo you plan to use the move Frustration?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_WhatsDisciplinarianPlanUse, 0, 0

L_01AF:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 546, 0
    // "Pwoof..."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_Pwoof, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_0210

L_01F4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 548, 0
    // "Fwish fwish!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_FwishFwish, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose

L_0210:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Some Pokémon might think it's\nsafer to live with humans than try[f000]븀\u0000\nto survive in the harsh wilderness."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity7_Text_SomePokemonMightThink, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
