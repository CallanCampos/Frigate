#include "asm/field_script.inc"
#include "text/script/striaton_city_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 255
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 1
    WorkSet 0x8001, 2
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If I were a Gym Leader,\nI wouldn't have quit...[f000]븀\u0000\nI would've felt like it was a waste."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCityPokemonCenter_Text_IfWereGymLeader, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Catching lots of Pokémon?[f000]븁\u0000\nHaving a lot of Pokémon\nmakes looking at the Pokédex[f000]븀\u0000\nor the PC Box so much fun!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCityPokemonCenter_Text_CatchingLotsPokemonHaving, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PokePartyFindEx 492, 0, 0x8020, 0x8021
    DebugPrint 0x8021
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0154
    VMStackPushFlag EVENT_FLAG_0x017d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0106
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Since early times in Sinnoh, people\nmade a bouquet of Gracidea flowers[f000]븀\u0000\nto give someone to show their[f000]븀\u0000\nfeelings of appreciation.[f000]븁\u0000\nIsn't that interesting?[f000]븁\u0000\nBy giving a Gracidea bouquet,\nyou don't have to say a word and[f000]븀\u0000\nsomeone will know how grateful you are.[f000]븁\u0000\nQuite a delightful custom!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCityPokemonCenter_Text_SinceEarlyTimesSinnoh, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_014E

L_0106:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Shaymin![f000]븁\u0000\nWhen it comes to Shaymin,\nGracidea flowers are important![f000]븁\u0000\nI have a lot of Gracidea flowers,\nso let me share one with you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, StriatonCityPokemonCenter_Text_OhShayminWhenComes, 8, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 466
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Since early times in Sinnoh, people\nmade a bouquet of Gracidea flowers[f000]븀\u0000\nto give someone to show their[f000]븀\u0000\nfeelings of appreciation.[f000]븁\u0000\nIsn't that interesting?[f000]븁\u0000\nBy giving a Gracidea bouquet,\nyou don't have to say a word and[f000]븀\u0000\nsomeone will know how grateful you are.[f000]븁\u0000\nQuite a delightful custom!"
    ActorMsg MSGFILE_SCRIPT, StriatonCityPokemonCenter_Text_SinceEarlyTimesSinnoh, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x017d

L_014E:
    VMJump L_0168

L_0154:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you know about Gracidea flowers?[f000]븁\u0000\nSince early times in Sinnoh, people\nmade a bouquet of Gracidea flowers[f000]븀\u0000\nto give someone to show their[f000]븀\u0000\nfeelings of appreciation."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCityPokemonCenter_Text_KnowAboutGracideaFlowers, 0, 0
    LastKeyWait
    ActorMsgClose

L_0168:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
