#include "asm/field_script.inc"
#include "text/script/global_10390.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    DreamWorldCmd_01DC 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_002D
    FlagReset EVENT_FLAG_0x028e
    VMJump L_0031

L_002D:
    FlagSet EVENT_FLAG_0x028e

L_0031:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_004D
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    WorkSetConst 0x8020, 0

L_004D:
    DreamWorldCmd_01DC 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007C
    DebugPrint 0x8010
    // "I've given you all the Dream Remnants.[f000]븁\u0000\nNext time your Pokémon dreams, come\nback and see me again."
    ActorMsg MSGFILE_SCRIPT, Global10390_Text_IveGivenAllDream, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_007C:
    WordSetPlayerName 0
    // "This is a remnant of your\nPokémon's dream.[f000]븀\u0000\nHere, take it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10390_Text_RemnantPokemonsDreamHere, 0x8011, 2, 0
    ActorMsgClose
    DreamWorldCmd_01DC 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C2
    MEPlay SEQ_ME_ITEM
    // "[f000]Ā\u0001\u0000 received\nthe item(s)!"
    SystemMsg Global10390_Text_ReceivedItemS, 0
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    DreamWorldCmd_01DC 2, 0x8010
    DreamWorldCmd_01DC 3, 0x8010

L_00C2:
    DreamWorldCmd_01DC 4, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0112
    WorkSetConst 0x8010, 0

L_00E1:
    VMStackPush 0x8010
    VMStackPush 0x8020
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0112
    DreamWorldCmd_01DC 5, 0x8010
    // "You have no more room in your Bag\nfor [f000]ĉ\u0001\u0000. If you remove some,[f000]븀\u0000\nI'll give you the rest.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10390_Text_HaveNoMoreRoom, 0x8011, 2, 0
    WorkAdd 0x8010, 1
    VMJump L_00E1

L_0112:
    // "When your Pokémon has another dream,\ncome back and see me again."
    ActorMsg MSGFILE_SCRIPT, Global10390_Text_WhenPokemonHasAnother, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
