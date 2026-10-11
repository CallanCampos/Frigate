#include "asm/field_script.inc"
#include "text/script/mistralton_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Trading Sweet Hearts you receive\nthrough Feeling Checks is one way[f000]븀\u0000\nto get Heart Scales.[f000]븁\u0000\nIf you show off your Pokémon\nto a lady in Driftveil City, you[f000]븀\u0000\ncan get Heart Scales, too."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_TradingSweetHeartsReceive, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01c7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh! You're a TM Master![f000]븁\u0000\nThank you for showing me\na lot of TMs.[f000]븁\u0000\nUse lots of different moves, and\nbring out your Pokémon's power!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_OhYoureTmMaster, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02C7

L_005B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01c1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008A
    // "I'm very fussy about\nPokémon moves.[f000]븁\u0000\nI'm extra fussy about\nTechnical Machines! Yes, TMs![f000]븁\u0000\nHow many TMs have you collected\nso far?"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_ImVeryFussyAbout, 0, 0
    MsgWaitAdvance
    FlagSet EVENT_FLAG_0x01c1
    VMJump L_0096

L_008A:
    // "I'll give you something great\nwhen you collect a lot of TMs![f000]븀\u0000\nLet me see a lot of TMs!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_IllGiveSomethingGreat, 0, 0
    MsgWaitAdvance

L_0096:
    ItemGetTMCount 0x8020
    WordSetNumber 0, 0x8020, 2
    // "Hmm...[f000]븁\u0000\nYou have [f000]ȁ\u0001\u0000 TMs!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_HmmHaveTms, 0, 0
    MsgWaitAdvance
    VMStackPush 0x8020
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x01c2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00E0
    VMCall L_02CD
    FlagSet EVENT_FLAG_0x01c2
    VMJump L_02C7

L_00E0:
    VMStackPush 0x8020
    VMStackPushConst 20
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x01c3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0113
    VMCall L_02CD
    FlagSet EVENT_FLAG_0x01c3
    VMJump L_02C7

L_0113:
    VMStackPush 0x8020
    VMStackPushConst 35
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x01c4
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0146
    VMCall L_02CD
    FlagSet EVENT_FLAG_0x01c4
    VMJump L_02C7

L_0146:
    VMStackPush 0x8020
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x01c5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0179
    VMCall L_02CD
    FlagSet EVENT_FLAG_0x01c5
    VMJump L_02C7

L_0179:
    VMStackPush 0x8020
    VMStackPushConst 70
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x01c6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01AC
    VMCall L_02CD
    FlagSet EVENT_FLAG_0x01c6
    VMJump L_02C7

L_01AC:
    VMStackPush 0x8020
    VMStackPushConst 95
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0208
    WordSetItemName 0, 45
    // "Oh!\nI've never seen some of these before![f000]븁\u0000\nWow!\nI didn't even know such TMs existed.[f000]븁\u0000\nI can tell you've selected each one\nwith so much care.[f000]븁\u0000\nThank you! I was thrilled to see them![f000]븁\u0000\nAs a reward, please accept this\n[f000]ĉ\u0001\u0000 as a gift![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_OhIveNeverSeen, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 45
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Wow! Great job!\nYou've got great technical points![f000]븁\u0000\nI didn't know there were so many TMs![f000]븁\u0000\nThanks to you, I realized\nthe true depth of TMs![f000]븁\u0000\nYou are a TM Master!\nThank you very much!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_WowGreatJobYouve, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01c7
    VMJump L_02C7

L_0208:
    WorkSetConst 0x8021, 0
    Random 0x8021, 5
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_0227
    VMJump L_0237

L_0227:
    // "Some kind people will give you TMs.\nSo don't be shy! Talk to everyone!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_SomeKindPeopleWill, 0, 0
    VMJump L_02C3

L_0237:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_024A
    VMJump L_025A

L_024A:
    // "Gym Leaders will give you a TM for sure\nif you beat them![f000]븁\u0000\nBut you already know that, don't you?"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_GymLeadersWillGive, 0, 0
    VMJump L_02C3

L_025A:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_026D
    VMJump L_027D

L_026D:
    // "Do you know the Battle Subway\nin Nimbasa City?[f000]븁\u0000\nDefeat Trainers there and\ncollect a lot of BP,[f000]븀\u0000\nand you can get TMs!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_KnowBattleSubwayNimbasa, 0, 0
    VMJump L_02C3

L_027D:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_0290
    VMJump L_02A0

L_0290:
    // "I don't know who would be careless\nenough to drop them, but you can[f000]븀\u0000\nsometimes find TMs in places where[f000]븀\u0000\nyou'd least expect them."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_DontKnowWhoWould, 0, 0
    VMJump L_02C3

L_02A0:
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_02B3
    VMJump L_02C3

L_02B3:
    // "You can also buy TMs\nat Poké Marts![f000]븁\u0000\nThey're a bit expensive, though."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_CanAlsoBuyTms, 0, 0
    VMJump L_02C3

L_02C3:
    LastKeyWait
    MsgWinCloseAll

L_02C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_02CD:
    WordSetItemName 0, 45
    // "Oh!\nI've never seen some of these before![f000]븁\u0000\nWow!\nI didn't even know such TMs existed.[f000]븁\u0000\nI can tell you've selected each one\nwith so much care.[f000]븁\u0000\nThank you! I was thrilled to see them![f000]븁\u0000\nAs a reward, please accept this\n[f000]ĉ\u0001\u0000 as a gift![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_OhIveNeverSeen, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 45
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "There must be a lot more TMs![f000]븁\u0000\nIf you collect even more of them,\nI'll give you another fantastic gift!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity4_Text_ThereMustLotMore, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn
    .balign 4, 0
