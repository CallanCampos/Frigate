#include "asm/field_script.inc"
#include "text/script/global_10270.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    SEPlay SEQ_SE_MESSAGE
    PokePartyGetCountBySpecies 386, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0049
    WorkSetConst 0x8021, 8
    VMCall L_021B
    VMJump L_00D9

L_0049:
    // "Deoxys is reacting to the meteor...[f000]븁\u0000\nWould you like to bring the Deoxys\nin your party closer to the meteor?"
    SystemMsg Global10270_Text_DeoxysReactingMeteorWould, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D7
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A6
    PokePartyFindBySpecies 386, 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A0
    VMCall L_0149
    VMJump L_00A0

L_00A0:
    VMJump L_00D1

L_00A6:
    // "Which Deoxys would you like to\nbring closer to the meteor?[f000]븁\u0000"
    SystemMsg Global10270_Text_WhichDeoxysWouldLike, 2
    InfoMsgClose
    CallPokeSelect 0, 0x8010, 0x8020, 0
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D1
    VMCall L_00DF

L_00D1:
    VMJump L_00D9

L_00D7:
    InfoMsgClose

L_00D9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00DF:
    WorkSetConst 0x8023, 0
    PokePartyIsEgg 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0110
    WorkSetConst 0x8021, 3
    VMCall L_021B
    VMJump L_0141

L_0110:
    PokePartyGetSpecies 0x8023, 0x8020
    VMStackPush 0x8023
    VMStackPushConst 386
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0135
    VMCall L_0149
    VMJump L_0141

L_0135:
    WorkSetConst 0x8021, 2
    VMCall L_021B

L_0141:
    WorkSetConst 0x8023, 0
    VMReturn

L_0149:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    PokePartyGetForme 0x8024, 0x8020
    WorkAdd 0x8024, 1
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_017A
    WorkSetConst 0x8024, 0

L_017A:
    PokePartySetForme 0x8020, 0x8024
    WordSetPartyPokeName 0, 0x8020
    PVPlay 386, 0
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AA
    WorkSetConst 0x8025, 4
    VMJump L_020D

L_01AA:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C9
    WorkSetConst 0x8025, 5
    VMJump L_020D

L_01C9:
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E8
    WorkSetConst 0x8025, 6
    VMJump L_020D

L_01E8:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0207
    WorkSetConst 0x8025, 7
    VMJump L_020D

L_0207:
    WorkSetConst 0x8025, 4

L_020D:
    SystemMsg 0x8025, 2
    PVWait
    LastKeyWait
    InfoMsgClose
    VMReturn

L_021B:
    SystemMsg 0x8021, 2
    LastKeyWait
    InfoMsgClose
    VMReturn
    .balign 4, 0
