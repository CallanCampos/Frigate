#include "asm/field_script.inc"
#include "text/script/driftveil_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Driftveil Luxury Suites.\nI'm so sorry, but we're full.[f000]븁\u0000\nPlease enjoy our lobby."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_WelcomeDriftveilLuxurySuites, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Different kinds of Pokémon can learn\ndifferent kinds of moves."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_DifferentKindsPokemonCan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0141
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B0
    WorkSetConst 0x8020, 0
    // "I am grateful that you came all this way!\nAre you a Trainer?[f000]븁\u0000\nOoh, you have a Pokédex!\nIt's so marvelous![f000]븁\u0000\nI wonder if you'd please do me a favor."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_AmGratefulCameAll, 0, 0, 0
    YesNoWin 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0096
    VMCall L_0148
    VMJump L_00A6

L_0096:
    // "I'm touched that you came all the way\nhere, yet you are being so unkind."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_ImTouchedCameAll, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_00A6:
    FlagSet EVENT_FLAG_0x0141
    VMJump L_0142

L_00B0:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012D
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0121
    WorkSetConst 0x8021, 0
    // "Oh, Trainer. I'm so pleased that you\ncame all the way here![f000]븁\u0000\nI'm wondering if you would do me a favor\ntoday, too."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_OhTrainerImPleased, 0, 0, 0
    YesNoWin 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010B
    VMCall L_0148
    VMJump L_011B

L_010B:
    // "I'm touched that you came all the way\nhere, yet you are being so unkind."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_ImTouchedCameAll, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_011B:
    VMJump L_0127

L_0121:
    VMCall L_0148

L_0127:
    VMJump L_0142

L_012D:
    WordSetMoveName 0, EVENT_WORK_0x4183
    // "I wonder what [f000]ć\u0001\u0000 looks like\nwhen it is actually used.[f000]븁\u0000\nI am sure you can use it skillfully!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_WonderWhatLooksLike, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0142:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0148:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FD
    ItemGetRandomOwnedTMMove EVENT_WORK_0x4183
    FlagSet EVENT_FLAG_DAILY_0x0ac6
    WordSetMoveName 0, EVENT_WORK_0x4183
    WorkSetConst 0x8022, 0
    PokePartyHasMoveAny 0x8022, EVENT_WORK_0x4183
    DebugPrint 0x8022
    DebugPrint EVENT_WORK_0x4183
    VMStackPush 0x8022
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A5
    // "To be clear, I would like to\nsee a Pokémon that knows [f000]ć\u0001\u0000.[f000]븀\u0000\nThank you."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_ClearWouldLikeSee, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01F1

L_01A5:
    WordSetPartyPokeSpecies 1, 0x8022
    // "I would like to see a Pokémon that\nlearned a move called [f000]ć\u0001\u0000.[f000]븁\u0000\nTo be clear, that's [f000]ć\u0001\u0000.\nPlease show it to me today.[f000]븁\u0000\nOh, my, my, my![f000]븁\u0000\nYour [f000]ā\u0001\u0001 can\nuse [f000]ć\u0001\u0000![f000]븁\u0000\nDid you go to the trouble of teaching it\nto your Pokémon? Marvelous![f000]븁\u0000\nThis is a small token of my appreciation.\nPlease don't hesitate to accept this.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_WouldLikeSeePokemon, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 93
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    WordSetMoveName 0, EVENT_WORK_0x4183
    // "I wonder what [f000]ć\u0001\u0000 looks like\nwhen it is actually used.[f000]븁\u0000\nI am sure you can use it skillfully!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_WonderWhatLooksLike, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_DAILY_0x0ac7

L_01F1:
    WorkSetConst 0x8022, 0
    VMJump L_0291

L_01FD:
    WordSetMoveName 0, EVENT_WORK_0x4183
    WorkSetConst 0x8023, 0
    PokePartyHasMoveAny 0x8023, EVENT_WORK_0x4183
    DebugPrint 0x8023
    DebugPrint EVENT_WORK_0x4183
    VMStackPush 0x8023
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023F
    // "To be clear, I would like to\nsee a Pokémon that knows [f000]ć\u0001\u0000.[f000]븀\u0000\nThank you."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_ClearWouldLikeSee, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_028B

L_023F:
    WordSetPartyPokeSpecies 1, 0x8023
    // "Oh, my goodness. Your [f000]ā\u0001\u0001 can\nuse [f000]ć\u0001\u0000![f000]븁\u0000\nYou went to all the trouble of teaching\nit to your Pokémon? That's so touching![f000]븁\u0000\nThis is a small token of my appreciation.\nPlease don't hesitate to accept this.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_OhGoodnessCanUse, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 93
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    WordSetMoveName 0, EVENT_WORK_0x4183
    // "I wonder what [f000]ć\u0001\u0000 looks like\nwhen it is actually used.[f000]븁\u0000\nI am sure you can use it skillfully!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity4_Text_WonderWhatLooksLike, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_DAILY_0x0ac7

L_028B:
    WorkSetConst 0x8023, 0

L_0291:
    VMReturn
    .balign 4, 0
