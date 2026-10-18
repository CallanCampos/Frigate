#include "asm/field_script.inc"
#include "text/script/mistralton_city_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    TrainerCardHasBadge 0x8008, 5
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0047
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Woooosh! Whooosh![f000]븁\u0000\nThe wind blows really hard\nin Skyla's Gym!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity2_Text_WooooshWhoooshWindBlows, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0088

L_0047:
    VMStackPushFlag EVENT_FLAG_0x008b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0074
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ruuunwaaaay! Ruuunwaaaay!\nA Technical Machine on the ruuuunwaaaay!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity2_Text_RuuunwaaaayRuuunwaaaayTechnicalMachine, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0088

L_0074:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ruuunwaaaay! Ruuunwaaaay!\nRacing there is so much fun![f000]븁\u0000\nHey, hey, which Pokémon\nflies the fastest?"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity2_Text_RuuunwaaaayRuuunwaaaayRacingThere, 0, 0
    LastKeyWait
    ActorMsgClose

L_0088:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    TrainerCardHasBadge 0x8008, 5
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you have a Gym Badge from Mistralton,\nI'll tell you something cool!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity2_Text_IfHaveGymBadge, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0108

L_00C3:
    VMStackPushFlag EVENT_FLAG_0x008b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wow! A Jet Badge! You won against Skyla!\nOK, I'll tell you something cool![f000]븁\u0000\nWe left our treasure at the edge of\nthe runway!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity2_Text_WowJetBadgeWon, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagReset EVENT_FLAG_0x0267
    VMJump L_0108

L_00F4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's TM40, Aerial Ace![f000]븁\u0000\nWe'll be happy if we gave you\nthe key to victory!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity2_Text_ItsTm40AerialAce, 0, 0
    LastKeyWait
    ActorMsgClose

L_0108:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 580, 0
    // "Kwa!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity2_Text_Kwa, 0, 0
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
    // "Why do we make vegetable gardens\naround the runway, you ask?[f000]븁\u0000\nThat's so we can send freshly picked\nvegetables as fast as possible!"
    // "Why did we put greenhouses\naround the runway, you ask?[f000]븁\u0000\nThat's so we can send freshly picked\nvegetables as fast as possible!"
    ActorMsgVersioned 1024, MistraltonCity2_Text_WhyWeMakeVegetable, MistraltonCity2_Text_WhyDidWePut, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
