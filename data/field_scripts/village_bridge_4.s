#include "asm/field_script.inc"
#include "text/script/village_bridge_4.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x012b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0079
    VMCall L_0144
    VMJump L_013E

L_0079:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4182
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00BB
    WordSetPokeSpecies 0, EVENT_WORK_0x4182
    // "The Official Hip Waders Catch of the Day\nis [f000]ā\u0001\u0000![f000]븀\u0000\nHave you caught one already?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_OfficialHipWadersCatch, 0, 0
    YesNoWin 0x8010
    VMCall L_01CB
    VMJump L_013E

L_00BB:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x012b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_010D
    // "Yo! Member No. 2!\nTimes have changed...[f000]븁\u0000\nThat's right! Because today is a new day,\nwe've got a new Official Hip Waders[f000]븀\u0000\nCatch of the Day! Isn't it exciting?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_YoMemberNo2, 0, 0
    FishingChallengeGetRandomPkm EVENT_WORK_0x4182
    WordSetPokeSpecies 0, EVENT_WORK_0x4182
    // "Here we go! I'll announce the\nOfficial Hip Waders Catch of the Day![f000]븁\u0000\nThe Pokémon we're fishing for\ntoday is [f000]ā\u0001\u0000![f000]븀\u0000\nIf you catch one, please show it to me!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_HereWeGoIll, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ac1
    FlagSet EVENT_FLAG_0x012b
    VMJump L_013E

L_010D:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ac1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4182
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_013E
    // "The appeal of fishing...[f000]븁\u0000\nThat's something you have to discover\nthrough experience, rather than[f000]븀\u0000\nlistening to me explaining about it![f000]븁\u0000\nRight, Member No. 2?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_AppealFishingThatsSomething, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_013E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0144:
    VMStackPushFlag EVENT_FLAG_0x012a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0167
    // "Hey, Trainer![f000]븁\u0000\nWhy don't you join the Hip Waders\nand do some fishing?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_HeyTrainerWhyDont, 0, 0
    VMJump L_0171

L_0167:
    // "Oh! A Trainer!\nYou got here without any delay![f000]븁\u0000\nAs I told you on Route 8,\nI'm a member of the Hip Waders.[f000]븁\u0000\nAs its name suggests,\nit's a fishing club![f000]븁\u0000\nWhy don't you join up and enjoy\nsome fishing?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_OhTrainerGotHere, 0, 0

L_0171:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B7
    FishingChallengeGetRandomPkm EVENT_WORK_0x4182
    WordSetPokeSpecies 0, EVENT_WORK_0x4182
    // "Awesome! Now you are the second member\nof the Hip Waders![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_AwesomeNowSecondMember, 0, 0
    // "Here we go! I'll announce the\nOfficial Hip Waders Catch of the Day![f000]븁\u0000\nThe Pokémon we're fishing for\ntoday is [f000]ā\u0001\u0000![f000]븀\u0000\nIf you catch one, please show it to me!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_HereWeGoIll, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ac1
    FlagSet EVENT_FLAG_0x012b
    VMJump L_01C9

L_01B7:
    // "That's the way the tide turns, huh?[f000]븁\u0000\nBut, I won't give up. If you change your\nmind, come see me again!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_ThatsWayTideTurns, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x012a

L_01C9:
    VMReturn

L_01CB:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02FF
    // "That's great! Show it to me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_ThatsGreatShow, 0, 0
    MsgWinCloseAll
    RTCGetDate 0x8026, 0x8024
    CallPokeSelect 0, 0x8022, 0x8021, 0
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EB
    PokePartyGetSpecies 0x8023, 0x8021
    WordSetPokeSpecies 0, 0x8023
    PokePartyGetMetDate 0x8029, 0x8027, 0x8025, 0x8021
    VMStackPush EVENT_WORK_0x4182
    VMStackPush 0x8023
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPush 0x8027
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPush 0x8025
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_028D
    // "Oh! This is a great [f000]ā\u0001\u0000![f000]븁\u0000\nThat's why you're the second member of\nthe Hip Waders![f000]븁\u0000\nCool! Take these with you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_OhGreatThatsWhy, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 7
    WorkSet 0x8001, 5
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst EVENT_WORK_0x4182, 0
    VMJump L_02E5

L_028D:
    VMStackPush EVENT_WORK_0x4182
    VMStackPush 0x8023
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02B4
    // "Oh! This is a great [f000]ā\u0001\u0000![f000]븁\u0000\nBut it's not the Official Hip Waders\nCatch of the Day. Sorry!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_OhGreatButIts, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02E5

L_02B4:
    VMStackPush 0x8026
    VMStackPush 0x8027
    VMStackCmp CMP_NE
    VMStackPush 0x8024
    VMStackPush 0x8025
    VMStackCmp CMP_NE
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02E5
    // "Oh! This is a great [f000]ā\u0001\u0000![f000]븁\u0000\n...But when it comes to the\nOfficial Hip Waders Catch of the Day,[f000]븀\u0000\na Pokémon you caught before today[f000]븀\u0000\ndoesn't count. Sorry!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_OhGreatButWhen, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02E5:
    VMJump L_02F9

L_02EB:
    // "The Official Hip Waders Catch of the Day\nis [f000]ā\u0001\u0000![f000]븀\u0000\nI'm counting on you, Member No. 2!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_OfficialHipWadersCatch_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02F9:
    VMJump L_030D

L_02FF:
    // "The Official Hip Waders Catch of the Day\nis [f000]ā\u0001\u0000![f000]븀\u0000\nI'm counting on you, Member No. 2!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge4_Text_OfficialHipWadersCatch_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_030D:
    VMReturn
    .balign 4, 0
