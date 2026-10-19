#include "asm/field_script.inc"
#include "text/script/aspertia_city_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0045
    FlagSet EVENT_FLAG_0x0323
    VMJump L_0049

L_0045:
    FlagReset EVENT_FLAG_0x0323

L_0049:
    VMStackPushFlag EVENT_FLAG_0x03c5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0083
    VMStackPushFlag EVENT_FLAG_0x01e3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ea
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0083
    FlagReset EVENT_FLAG_0x03c5

L_0083:
    VMHalt

Script_2:
    ActorsPauseAll
    WordSetPlayerName 0
    RecordGet 71, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BD
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0000, did you find\nthat lady called Bianca?[f000]븁\u0000\nI hope you get a Pokémon soon!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity4_Text_DidFindLadyCalled, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01E1

L_00BD:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0100
    VMCall L_01E7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wish...[f000]븁\u0000\nI wish my big brother could go on a\njourney for his Pokémon, not for me."
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity4_Text_WishWishBigBrother, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01E1

L_0100:
    VMStackPush EVENT_WORK_0x40a7
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush EVENT_WORK_0x40a8
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0143
    VMCall L_01E7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, [f000]Ā\u0001\u0000!\nTake care of [f000]ā\u0001\u0001![f000]븁\u0000\nOnly Trainers can protect\ntheir own Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity4_Text_OhTakeCareOnly, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01E1

L_0143:
    VMStackPush EVENT_WORK_0x4124
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C7
    VMCall L_01E7
    VMStackPushFlag EVENT_FLAG_0x01e3
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ea
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x03c5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01A9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0000![f000]븁\u0000\nLook![f000]븁\u0000\nLiepard looks so happy\nwhen I pet its head!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity4_Text_LookLiepardLooksHappy, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01C1

L_01A9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what?\nMy big brother told me something.[f000]븁\u0000\nHe said to talk to the Liepard\ninside the Poké Ball lots and lots,[f000]븀\u0000\nlike I'm doing, until it remembers me![f000]븁\u0000\nAnd to pet it, even if it's just on\nthe outside of the Poké Ball!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity4_Text_KnowWhatBigBrother, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x01ea

L_01C1:
    VMJump L_01E1

L_01C7:
    VMCall L_01E7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi, [f000]Ā\u0001\u0000![f000]븁\u0000\nWow! It's [f000]ā\u0001\u0001![f000]븁\u0000\nYou know lots of other Pokémon, right?\nCool!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity4_Text_HiWowItsKnow, 0, 0
    LastKeyWait
    ActorMsgClose

L_01E1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01E7:
    PokePartyGetMemberByType 0x8020, 2
    WordSetPartyPokeSpecies 1, 0x8020
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 510, 0
    // "Preoooww... ♪"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity4_Text_Preoooww, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a map of the Unova region."
    InfoMsg AspertiaCity4_Text_ItsMapUnovaRegion, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
