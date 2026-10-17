#include "asm/field_script.inc"
#include "text/script/global_10325.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0084
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0059
    // "Pokémon Trades help you connect with\nthe world![f000]븀\u0000\nThe world will be bigger![f000]븁\u0000\nIf you trade Pokémon with many people,\nyou will be happy![f000]븁\u0000\nBecause I will give you something good![f000]븁\u0000\nSo, Trainer, have you traded Pokémon\nwith many people?"
    ParentActorMsg MSGFILE_SCRIPT, Global10325_Text_PokemonTradesHelpConnect, 0, 0
    FlagSet EVENT_FLAG_0x0084
    VMJump L_0063

L_0059:
    // "Hello, Trainer![f000]븁\u0000\nHave you traded Pokémon with\nmany people?"
    ParentActorMsg MSGFILE_SCRIPT, Global10325_Text_HelloTrainerHaveTraded, 0, 0

L_0063:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0086
    VMCall L_00A8
    VMJump L_0092

L_0086:
    WorkSetConst 0x8020, 4
    VMCall L_0098

L_0092:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0098:
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_00A8:
    WorkSetConst 0x8026, 0
    // "Oh, wonderful.[f000]븁\u0000\nThen, let me check how many people you\nhave traded Pokémon with.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10325_Text_OhWonderfulThenLet, 0, 0
    // "...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10325_Text_Empty, 0, 0
    Cmd_0226 0x8021
    WordSetNumber 0, 0x8021, 2
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F2
    WorkSetConst 0x8020, 6
    VMCall L_0098
    VMJump L_01D4

L_00F2:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0117
    WorkSetConst 0x8020, 5
    VMCall L_0098
    VMJump L_01D4

L_0117:
    VMStackPush 0x8021
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0136
    WorkSetConst 0x8026, 12
    VMJump L_01B8

L_0136:
    VMStackPush 0x8021
    VMStackPushConst 40
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0155
    WorkSetConst 0x8026, 11
    VMJump L_01B8

L_0155:
    VMStackPush 0x8021
    VMStackPushConst 30
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0174
    WorkSetConst 0x8026, 10
    VMJump L_01B8

L_0174:
    VMStackPush 0x8021
    VMStackPushConst 20
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0193
    WorkSetConst 0x8026, 9
    VMJump L_01B8

L_0193:
    VMStackPush 0x8021
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_01B2
    WorkSetConst 0x8026, 8
    VMJump L_01B8

L_01B2:
    WorkSetConst 0x8026, 7

L_01B8:
    ParentActorMsg MSGFILE_SCRIPT, 0x8026, 0, 0
    VMCall L_01D6
    WorkSetConst 0x8020, 15
    VMCall L_0098

L_01D4:
    VMReturn

L_01D6:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 5
    WorkSetConst 0x8022, 126
    WorkSetConst 0x8025, 265
    VMCall L_026E
    WorkSetConst 0x8023, 10
    WorkSetConst 0x8022, 127
    WorkSetConst 0x8025, 229
    VMCall L_026E
    WorkSetConst 0x8023, 20
    WorkSetConst 0x8022, 128
    WorkSetConst 0x8025, 276
    VMCall L_026E
    WorkSetConst 0x8023, 30
    WorkSetConst 0x8022, 129
    WorkSetConst 0x8025, 287
    VMCall L_026E
    WorkSetConst 0x8023, 40
    WorkSetConst 0x8022, 130
    WorkSetConst 0x8025, 53
    VMCall L_026E
    WorkSetConst 0x8023, 50
    WorkSetConst 0x8022, 131
    WorkSetConst 0x8025, 1
    VMCall L_026E
    VMReturn

L_026E:
    WorkSetConst 0x8027, 0
    VMStackPush 0x8021
    VMStackPush 0x8023
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_02F5
    VMStackPushFlag 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F5
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    WorkSetConst 0x8027, 13
    VMJump L_02BF

L_02B9:
    WorkSetConst 0x8027, 14

L_02BF:
    ParentActorMsg MSGFILE_SCRIPT, 0x8027, 0, 0
    ActorMsgClose
    WorkSetConst 0x8024, 1
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8025
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet 0x8022

L_02F5:
    VMReturn
    .balign 4, 0
