#include "asm/field_script.inc"
#include "text/script/route_8.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 8"
    MsgPlaceSign Route8_Text_Route8, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Tubeline Bridge\nUnova's famous railway bridge"
    MsgPlaceSign Route8_Text_TubelineBridgeUnovasFamous, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips!\n[f000]븁\u0000\nPress SELECT to change the location\nof items in the Bag![f000]븁\u0000\nPoink!"
    MsgPlaceSign Route8_Text_TrainerTipsPressSelect, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a6f
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    RTCGetDayPart 0x8010
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0abd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B6
    // "That's right.[f000]븁\u0000\nI find rocks, and then\nI give them to people...[f000]븀\u0000\nThat's my simple life.[f000]븀\u0000\nYou rock...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_ThatsRightFindRocks, 0, 0
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E8
    // "Here, I'll give you the Damp Rock\nI found this morning.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_HereIllGiveDamp, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 285
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_019E

L_00E8:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012D
    // "Here, I'll give you the Heat Rock\nI found this afternoon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_HereIllGiveHeat, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 284
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_019E

L_012D:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0172
    // "Here, I'll give you the Smooth Rock\nI found this evening.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_HereIllGiveSmooth, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 283
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_019E

L_0172:
    // "Here, I'll give you the Icy Rock\nI found tonight.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_HereIllGiveIcy, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 282
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_019E:
    // "Yeah, yeah...[f000]븁\u0000\nIf you like rocks, come back tomorrow...\nRoll in at a different time, if possible.[f000]븀\u0000\nI'll be here, I pumice."
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_YeahYeahIfLike, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0abd
    VMJump L_01C4

L_01B6:
    // "Yeah, yeah...[f000]븁\u0000\nIf you like rocks, come back tomorrow...\nRoll in at a different time, if possible.[f000]븀\u0000\nI'll be here, I pumice."
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_YeahYeahIfLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01C4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    ItemCheckAmount ITEM_SUPER_ROD, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024A
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey there, Pokémon Trainer![f000]븁\u0000\nI'm a member of the Hip Waders![f000]븁\u0000\nJust as the name suggests,\nwe're a fishing team![f000]븁\u0000\nIf you want to learn more, come on\nover to my house on Village Bridge![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route8_Text_HeyTherePokemonTrainer, 1, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 183
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0228
    ActorWalkRoute 1, 243, 180, 1, 8, 1
    VMJump L_0236

L_0228:
    ActorWalkRoute 1, 243, 180, 1, 8, 0

L_0236:
    ActorCmdWait
    ActorDelete 1
    FlagSet EVENT_FLAG_0x0317
    FlagReset EVENT_FLAG_0x0318
    VMJump L_025E

L_024A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey there, Pokémon Trainer![f000]븁\u0000\nI'm a member of the Hip Waders![f000]븁\u0000\nOh, you don't have a fishing rod...[f000]븁\u0000\nMaybe I'll go invite Professor Juniper\nin Nuvema Town instead..."
    ParentActorMsg MSGFILE_SCRIPT, Route8_Text_HeyTherePokemonTrainer_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_025E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
