#include "asm/field_script.inc"
#include "text/script/route_13_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hope a new winner\nhas emerged in the PWT!"
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_HopeNewWinnerHas, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'd like to go to Pokéstar Studios.\nI haven't been there in a while.[f000]븁\u0000\nI'm a fan of Mr. Stu Deeoh.\nI wish he would be in a movie."
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_IdLikeGoPokestar, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0130
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This Pokémon used to deliver\nMail to everyone![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_PokemonUsedDeliverMail, 0, 0
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B8
    // "I know! I'll give you the Power Lens\nthis Pokémon brought me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_KnowIllGivePower, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 291
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_00E4

L_00B8:
    // "I know! I'll give you the Power Band\nthis Pokémon brought me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_KnowIllGivePower_2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 292
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_00E4:
    // "It doesn't matter to this Pokémon even\nif it doesn't battle very well!"
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_DoesntMatterPokemonEven, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0130
    VMJump L_0110

L_00FC:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It doesn't matter to this Pokémon even\nif it doesn't battle very well!"
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_DoesntMatterPokemonEven, 0, 0
    LastKeyWait
    ActorMsgClose

L_0110:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 278, 0
    // "Wree wreek!"
    ParentActorMsg MSGFILE_SCRIPT, Route132_Text_WreeWreek, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
