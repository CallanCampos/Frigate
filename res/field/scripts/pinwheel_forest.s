#include "asm/field_script.inc"
#include "text/script/pinwheel_forest.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Yo! Traveling Trainer![f000]븁\u0000\nBring a strong Pokémon\nto smash the challenge rock!"
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest_Text_YoTravelingTrainerBring, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Forest paths were created by\nthe Pokémon that often walk there.[f000]븁\u0000\nIf you walk the paths, sometimes\nyou can feel like a Pokémon yourself."
    ParentActorMsg MSGFILE_SCRIPT, PinwheelForest_Text_ForestPathsWereCreated, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006F
    VMCall L_007E
    VMJump L_0078

L_006F:
    // "It's a challenge rock."
    InfoMsg PinwheelForest_Text_ItsChallengeRock, 2
    LastKeyWait
    InfoMsgClose_0039

L_0078:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_007E:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8022, 0
    PokePartyGetCount 0x8024, 0

L_00AE:
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0194
    PokePartyIsEgg 0x8023, 0x8022
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0188
    PokePartyGetTypes 0x8020, 0x8021, 0x8022
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_017C
    WordSetPartyPokeName 0, 0x8022
    // "It's a challenge rock.[f000]븁\u0000\nWould you like to have [f000]Ă\u0001\u0000\nsmash the rock?"
    InfoMsg PinwheelForest_Text_ItsChallengeRockWould, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0168
    WordSetPartyPokeName 0, 0x8022
    // "[f000]Ă\u0001\u0000 tried to smash the\nchallenge rock.[f000]븁\u0000\nA piece of the rock broke away![f000]븁\u0000"
    InfoMsg PinwheelForest_Text_TriedSmashChallengeRock, 2
    InfoMsgClose_0039
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 90
    WorkSet 0x8001, 1
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_DAILY_0x0ac8
    WorkSetConst 0x8022, 6
    WorkSetConst 0x8025, 1
    VMJump L_0176

L_0168:
    InfoMsgClose_0039
    WorkSetConst 0x8022, 6
    WorkSetConst 0x8025, 1

L_0176:
    VMJump L_0182

L_017C:
    WorkAdd 0x8022, 1

L_0182:
    VMJump L_018E

L_0188:
    WorkAdd 0x8022, 1

L_018E:
    VMJump L_00AE

L_0194:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B0
    // "It's a challenge rock."
    InfoMsg PinwheelForest_Text_ItsChallengeRock, 2
    LastKeyWait
    InfoMsgClose_0039

L_01B0:
    VMReturn

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Pinwheel Forest\nDid you remember to pack an Antidote?"
    MsgPlaceSign PinwheelForest_Text_PinwheelForestDidRemember, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
