#include "asm/field_script.inc"
#include "text/script/global_2225.h"

    ScriptEntry Script_1
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello, hello!\nI am the official Name Rater![f000]븁\u0000\nWant me to rate the nicknames\nof your Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2225_Text_HelloHelloAmOfficial, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0047
    VMCall L_017C
    VMJump L_004D

L_0047:
    VMCall L_0053

L_004D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0053:
    // "Which Pokémon's nickname\nshould I critique?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2225_Text_WhichPokemonsNicknameShould, 0, 0
    ActorMsgClose
    CallPokeSelect 0, 0x8010, 0x8020, 0
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0088
    VMCall L_017C
    VMJump L_008E

L_0088:
    VMCall L_0090

L_008E:
    VMReturn

L_0090:
    PokePartyIsEgg 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BB
    WorkSetConst 0x8021, 8
    VMCall L_016C
    VMJump L_012A

L_00BB:
    WordSetPartyPokeName 0, 0x8020
    PokePartyIsOriginGame 0x8010, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EB
    WorkSetConst 0x8021, 7
    VMCall L_016C
    VMJump L_012A

L_00EB:
    // "Hmmm. [f000]Ă\u0001\u0000, is it?[f000]븁\u0000\nThat sounds good! It's a very “you\"\nsort of name.[f000]븁\u0000\nBut if you'd like to change it, I can help\nyou do that. How about it?"
    ParentActorMsg MSGFILE_SCRIPT, Global2225_Text_HmmmSoundsGoodIts, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0118
    VMCall L_017C
    VMJump L_012A

L_0118:
    // "Excellent! What would you like\nthe new nickname to be?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2225_Text_ExcellentWhatWouldLike, 0, 0
    ActorMsgClose
    VMCall L_012C

L_012A:
    VMReturn

L_012C:
    CallPokeNameInput 0x8010, 0x8020, 1
    WordSetPartyPokeName 0, 0x8020
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015E
    WorkSetConst 0x8021, 6
    VMCall L_016C
    VMJump L_016A

L_015E:
    WorkSetConst 0x8021, 4
    VMCall L_016C

L_016A:
    VMReturn

L_016C:
    ParentActorMsg MSGFILE_SCRIPT, 0x8021, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_017C:
    WorkSetConst 0x8021, 5
    VMCall L_016C
    VMReturn
    .balign 4, 0
