#include "asm/field_script.inc"
#include "text/script/route_4_9.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
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
    WorkSetConst 0x802a, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokePartyGetCount 0x8020, 1
    // "Hi! I'll mimic a Pokémon's sound!\nPlease listen!"
    ActorMsg MSGFILE_SCRIPT, Route49_Text_HiIllMimicPokemons, 0, 2, 0
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00AE
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0098
    VMCall L_00C6
    VMJump L_00A8

L_0098:
    // "It's just like the real Pokémon.\nWon't you listen to me?"
    ActorMsg MSGFILE_SCRIPT, Route49_Text_ItsJustLikeReal, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_00A8:
    VMJump L_00C0

L_00AE:
    MsgWaitAdvance
    // "...You have way too few Pokémon!\nI can't give you a quiz like this."
    ActorMsg MSGFILE_SCRIPT, Route49_Text_HaveWayTooFew, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_00C0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00C6:
    PokePartyGetCount 0x8020, 0

L_00CC:
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0120
    Random 0x8027, 0x8020
    PokePartyIsEgg 0x8028, 0x8027
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011A
    PokePartyGetParam 0x8022, 0x8027, 5
    PokePartyGetParam 0x8023, 0x8027, 111
    WorkSetConst 0x802a, 1
    WorkAdd 0x8026, 0x8027

L_011A:
    VMJump L_00CC

L_0120:
    // "OK. I'll mimic the sound of\nthe Pokémon you're with..."
    ActorMsg MSGFILE_SCRIPT, Route49_Text_OkIllMimicSound, 0, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMSleep 8
    PVPlay 0x8022, 0x8023
    PVWait
    DebugPrint 0x8026
    VMSleep 8
    // "I mimicked one of your party Pokémon.\nWhich Pokémon was it?"
    ActorMsg MSGFILE_SCRIPT, Route49_Text_MimickedOnePartyPokemon, 0, 2, 0
    MsgWaitAdvance
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32809
    PokePartyGetCount 0x8020, 0

L_0161:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_035C
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D5
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C7
    WordSetPartyPokeSpecies 0, 0x8021
    ListMenuAdd 8, 65535, 0
    PokePartyGetParam 0x8024, 0, 5
    PokePartyGetParam 0x8025, 0, 111
    DebugPrint 0x8024
    VMJump L_01CF

L_01C7:
    ListMenuAdd 14, 65535, 0

L_01CF:
    VMJump L_0350

L_01D5:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0222
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0214
    WordSetPartyPokeSpecies 1, 0x8021
    ListMenuAdd 9, 65535, 1
    VMJump L_021C

L_0214:
    ListMenuAdd 14, 65535, 1

L_021C:
    VMJump L_0350

L_0222:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026F
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0261
    WordSetPartyPokeSpecies 2, 0x8021
    ListMenuAdd 10, 65535, 2
    VMJump L_0269

L_0261:
    ListMenuAdd 14, 65535, 2

L_0269:
    VMJump L_0350

L_026F:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BC
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02AE
    WordSetPartyPokeSpecies 3, 0x8021
    ListMenuAdd 11, 65535, 3
    VMJump L_02B6

L_02AE:
    ListMenuAdd 14, 65535, 3

L_02B6:
    VMJump L_0350

L_02BC:
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0309
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02FB
    WordSetPartyPokeSpecies 4, 0x8021
    ListMenuAdd 12, 65535, 4
    VMJump L_0303

L_02FB:
    ListMenuAdd 14, 65535, 4

L_0303:
    VMJump L_0350

L_0309:
    VMStackPush 0x8021
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0350
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0348
    WordSetPartyPokeSpecies 5, 0x8021
    ListMenuAdd 13, 65535, 5
    VMJump L_0350

L_0348:
    ListMenuAdd 14, 65535, 5

L_0350:
    WorkAdd 0x8021, 1
    VMJump L_0161

L_035C:
    ListMenuShow
    PokePartyGetParam 0x8024, 0x8029, 5
    PokePartyGetParam 0x8025, 0x8029, 111
    PokePartyIsEgg 0x8028, 0x8029
    VMStackPush 0x8022
    VMStackPush 0x8024
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPush 0x8025
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_040A
    VMStackPushFlag EVENT_FLAG_0x016b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F4
    // "Correct![f000]븁\u0000\nYou can recognize the sound of\nPokémon you always hear, right?[f000]븁\u0000\nThank you for playing with me.\nThis is a small gift from me!"
    ActorMsg MSGFILE_SCRIPT, Route49_Text_CorrectCanRecognizeSound, 0, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 577
    WorkSet 0x8001, 1
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x016b
    VMJump L_0404

L_03F4:
    // "Correct![f000]븁\u0000\nOf course, you won't mistake\nthe sound of your partner Pokémon!"
    ActorMsg MSGFILE_SCRIPT, Route49_Text_CorrectCourseWontMistake, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0404:
    VMJump L_041F

L_040A:
    WordSetPartyPokeSpecies 0, 0x8026
    // "Close! Too bad! I'm sad!\nThat was [f000]ā\u0001\u0000!"
    ActorMsg MSGFILE_SCRIPT, Route49_Text_CloseTooBadIm, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_041F:
    VMReturn
    .balign 4, 0
