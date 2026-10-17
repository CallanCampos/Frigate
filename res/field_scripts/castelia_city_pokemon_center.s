#include "asm/field_script.inc"
#include "text/script/castelia_city_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_5:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    PlayerGetGPos 0x8020, 0x8021
    VMStackPushFlag EVENT_FLAG_0x0104
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    VMStackPush 0x8020
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_006C
    VMJump L_007E

L_006C:
    ActorSetGPos 8, 7, 0, 12, 0
    WorkSetConst EVENT_WORK_0x4001, 1

L_007E:
    RTCallGlobal 10395
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetDir 0x8022
    UnityTowerGetVisitorCountry 0x8023
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0104
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0130
    // "Wanna recover Pokémon?\nOh, soooooorry![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_WannaRecoverPokemonOh, 0, 0
    MsgWinCloseAll
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_00FB
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_00FB
    VMJump L_0109

L_00FB:
    ActorCmdExec 8, Movement_01D8
    VMJump L_012A

L_0109:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_011C
    VMJump L_012A

L_011C:
    ActorCmdExec 8, Movement_01E8
    VMJump L_012A

L_012A:
    VMSleep 8
    ActorCmdWait

L_0130:
    // "Do you know Geonet?"
    ActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_KnowGeonet, 8, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0179
    // "You! You, using Geonet!\nI'll tell you something good[f000]븀\u0000\nbecause you're great.[f000]븁\u0000\nWith the latest technology, we can trade\nPokémon with people far away![f000]븁\u0000\n...I know it sounds crazy,\nbut give it a try. You'll be surprised!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_UsingGeonetIllTell, 8, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01CC

L_0179:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01B2
    // "Oh! You know Geonet!\nGreat, great![f000]븁\u0000\nThis is even greater, lemme tell ya![f000]븁\u0000\nIf you try Geonet, you can register\nthe place where you live!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_OhKnowGeonetGreat, 8, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01CC

L_01B2:
    ActorCmdExec 8, Movement_01F8
    ActorCmdWait
    // "You see the globe on the second floor\nof this Pokémon Center?[f000]븁\u0000\nThat is Geonet.[f000]븁\u0000\nIf you check on Geonet, you can register\nthe place where you live!"
    ActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_SeeGlobeSecondFloor, 8, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_01CC:
    FlagSet EVENT_FLAG_0x0104
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01D8:
    Move 15, 2
    Move 34, 1
    Move 63, 1
    MoveEnd

Movement_01E8:
    Move 14, 1
    Move 35, 1
    Move 63, 1
    MoveEnd

Movement_01F8:
    Move 32, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 32, 1
    MoveEnd

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
    WorkSet 0x8000, 3
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokePartyGetCount 0x8024, 4
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02C3
    VMCall L_02CF
    VMJump L_02C9

L_02C3:
    VMCall L_02DF

L_02C9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_02CF:
    // "I want to know\neveryone's favorite kind of Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_WantKnowEveryonesFavorite, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_02DF:
    PokePartyIsEgg 0x8025, 0
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0308
    // "Your favorite is that Egg, isn't it?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_FavoriteEggIsnt, 0, 0
    VMJump L_0317

L_0308:
    WordSetPartyPokeSpecies 0, 0
    // "Your favorite is [f000]ā\u0001\u0000, isn't it?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_FavoriteIsnt, 0, 0

L_0317:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0342
    TrainerCardSetFavePokemon 0
    // "Yes, I was right! I thought so!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_YesRightThought, 0, 0
    VMJump L_0374

L_0342:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0365
    // "...Oh? Your favorite really is that Egg,\nisn't it?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_OhFavoriteReallyEgg, 0, 0
    VMJump L_0374

L_0365:
    WordSetPartyPokeSpecies 0, 0
    // "What? Your favorite isn't [f000]ā\u0001\u0000?!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_WhatFavoriteIsnt, 0, 0

L_0374:
    LastKeyWait
    MsgWinCloseAll
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Fennel has moved to Castelia City![f000]븁\u0000\nFennel is a professor who is\nresearching about Pokémon Trainers!"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_FennelHasMovedCastelia, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 531, 0
    // "Au-di-no?"
    ParentActorMsg MSGFILE_SCRIPT, CasteliaCityPokemonCenter_Text_AuDiNo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
