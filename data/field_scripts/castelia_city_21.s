#include "asm/field_script.inc"
#include "text/script/castelia_city_21.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0041
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Even before Poké Balls were created,\npeople and Pokémon were good friends.[f000]븀\u0000\nI wonder if this relationship will last[f000]븀\u0000\nin the future, too."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity21_Text_EvenBeforePokeBalls, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0055

L_0041:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want Pokémon to be\nin the new office...[f000]븁\u0000\nBut I can't say such a thing\nin front of my girlfriend."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity21_Text_WantPokemonNewOffice, 0, 0
    LastKeyWait
    ActorMsgClose

L_0055:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008A
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I learned some Pokémon want to\nbe with Trainers...[f000]븁\u0000\nOf course, some Pokémon prefer\nto live wild.[f000]븁\u0000\nFor your information, I heard\nPokémon who have learned a hidden move[f000]븀\u0000\nmay come back, even if you try to[f000]븀\u0000\nrelease them."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity21_Text_LearnedSomePokemonWant, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_009E

L_008A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Missing Pokémon...[f000]븁\u0000\nEven if Team Plasma is responsible,\nwe don't know where they are."
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity21_Text_MissingPokemonEvenIf, 0, 0
    LastKeyWait
    ActorMsgClose

L_009E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon!\nScraggy came back![f000]븁\u0000\nI don't know if it was held captive\nby Team Plasma...[f000]븀\u0000\nor it was lost and came back by itself...[f000]븁\u0000\nBut anyway, I'm happy!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity21_Text_PokemonScraggyCameBack, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00E7

L_00D3:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon...\nWhere did it go...?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity21_Text_PokemonWhereDidGo, 0, 0
    LastKeyWait
    ActorMsgClose

L_00E7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 559, 0
    // "Gyscragg!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCity21_Text_Gyscragg, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
