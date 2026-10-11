#include "asm/field_script.inc"
#include "text/script/virbank_city_pokemon_center.h"

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
    WorkSet 0x8000, 22
    WorkSet 0x8001, 1
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
    // "Listen! Challenge Poison-type Pokémon\nwith Poison-type Pokémon![f000]븀\u0000\nAt least, that's what Roxie told me.[f000]븁\u0000\nRoxie's a Gym Leader!\nShe plays an instrument. Pretty cool!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_ListenChallengePoisonType, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    TrainerCardGetBadgeCount 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C5
    // "The more Gym Badges you have,\nthe more items you can buy at a shop.[f000]븀\u0000\nI didn't know that!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_MoreGymBadgesHave, 7, 0, 0
    VMJump L_00D8

L_00C5:
    WordSetNumber 0, 0x8020, 1
    // "So you have [f000]Ȁ\u0001\u0000 Badges?[f000]븁\u0000\nOh! Then, you must be able to buy\na lot of items at a shop."
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_HaveBadgesOhThen, 7, 0, 0

L_00D8:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi! I have some questions for you![f000]븁\u0000\nIf you want to play, where do\nyou prefer: outside or at home?"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_HiHaveSomeQuestions, 8, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    ListMenuAdd 11, 65535, 1
    ListMenuAdd 12, 65535, 2
    ListMenuAdd 13, 65535, 3
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0138
    WorkSetConst 0x8008, 1
    VMJump L_0170

L_0138:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0157
    WorkSetConst 0x8008, 2
    VMJump L_0170

L_0157:
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0170
    WorkSetConst 0x8008, 3

L_0170:
    // "I see! I see![f000]븁\u0000\nThen, which one are you interested in:\nthe thing everybody knows[f000]븀\u0000\nor the thing nobody knows?"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_SeeSeeThenWhich, 8, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    ListMenuAdd 14, 65535, 1
    ListMenuAdd 15, 65535, 2
    ListMenuAdd 13, 65535, 3
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BE
    WorkSetConst 0x8009, 1
    VMJump L_01F6

L_01BE:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DD
    WorkSetConst 0x8009, 2
    VMJump L_01F6

L_01DD:
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F6
    WorkSetConst 0x8009, 3

L_01F6:
    // "Oh, really?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_OhReally, 8, 2, 0
    VMStackPush 0x8008
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8009
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_023B
    // "In my view, you are a person with\ncommon sense!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_ViewPersonCommonSense, 8, 2, 0
    UnityTowerCmd_02DA 0
    VMJump L_0319

L_023B:
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0274
    // "I'd say you are quite active!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_IdSayQuiteActive, 8, 2, 0
    UnityTowerCmd_02DA 1
    VMJump L_0319

L_0274:
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8009
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02AD
    // "I'd say you are very curious!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_IdSayVeryCurious, 8, 2, 0
    UnityTowerCmd_02DA 2
    VMJump L_0319

L_02AD:
    VMStackPush 0x8008
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02E6
    // "I'd say you are quite composed!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_IdSayQuiteComposed, 8, 2, 0
    UnityTowerCmd_02DA 4
    VMJump L_0319

L_02E6:
    VMStackPush 0x8008
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8009
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0319
    // "I'd say you are quite relaxed!"
    ActorMsg MSGFILE_SCRIPT, VirbankCityPokemonCenter_Text_IdSayQuiteRelaxed, 8, 2, 0
    UnityTowerCmd_02DA 3

L_0319:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
