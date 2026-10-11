#include "asm/field_script.inc"
#include "text/script/lacunosa_town_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    RTCGetDayPart 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_004D
    FlagReset EVENT_FLAG_0x0312
    VMJump L_0051

L_004D:
    FlagSet EVENT_FLAG_0x0312

L_0051:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When I think about it,\nmy Pokémon is much stronger than me...[f000]븁\u0000\nPoké Balls are sure amazing."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown2_Text_WhenThinkAboutPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 572, 0
    // "Gahoohoo..."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown2_Text_Gahoohoo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is Lacunosa Town. Here, everyone\nlives according to the rules, from the[f000]븀\u0000\nmoment they awaken to the time they[f000]븀\u0000\ngo to sleep."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown2_Text_LacunosaTownHereEveryone, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The scary monster that comes out of\nthe big hole at night is actually[f000]븀\u0000\na Pokémon right?[f000]븁\u0000\nAdults were just saying\nthat to frighten children, huh?"
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown2_Text_ScaryMonsterComesOut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
