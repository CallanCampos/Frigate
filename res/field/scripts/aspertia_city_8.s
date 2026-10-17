#include "asm/field_script.inc"
#include "text/script/aspertia_city_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0039
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My flaky fortune-telling says that\nyou'll meet a Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity8_Text_FlakyFortuneTellingSays, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00CB

L_0039:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0115
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BD
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 3
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00A9
    // "Oh! You already have a Gym Badge![f000]븁\u0000\nThis is a present from me!\nI hope it helps you out![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity8_Text_OhAlreadyHaveGym, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 2
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "The Ultra Ball is really good.\nIt performs much better[f000]븀\u0000\nthan a regular Poké Ball!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity8_Text_UltraBallReallyGood, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0115
    VMJump L_00B7

L_00A9:
    // "Going to the next town was a big\nadventure when I was a kid![f000]븁\u0000\nOh yeah! Here, I'll give you something\nI always used to take with me back then![f000]븁\u0000\nUm... Now, where is it?\nI'll look for it! Sorry!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity8_Text_GoingNextTownBig, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00B7:
    VMJump L_00CB

L_00BD:
    // "The Ultra Ball is really good.\nIt performs much better[f000]븀\u0000\nthan a regular Poké Ball!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity8_Text_UltraBallReallyGood, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00CB:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0100
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if there's a Pokémon that was\ndropped on the ground somewhere..."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity8_Text_WonderIfTheresPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0114

L_0100:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's the worst when you find the Pokémon\nyou were looking for and you don't[f000]븀\u0000\nhave any Poké Balls."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity8_Text_ItsWorstWhenFind, 0, 0
    LastKeyWait
    ActorMsgClose

L_0114:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
