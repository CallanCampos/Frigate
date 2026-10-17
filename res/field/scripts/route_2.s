#include "asm/field_script.inc"
#include "text/script/route_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    VMStackPush EVENT_WORK_0x4186
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004D
    WorkSetConst EVENT_WORK_0x4020, 17
    VMJump L_0053

L_004D:
    WorkSetConst EVENT_WORK_0x4020, 297

L_0053:
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 2"
    MsgPlaceSign Route2_Text_Route2, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    PlayerGetDir 0x8020
    MedalIsObtained 0x8021, 18
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00B5
    SEPlay SEQ_SE_MESSAGE
    // "There's some graffiti\non the other side of the signboard...[f000]븁\u0000\nNothing ventured, nothing gained.\nMedals await the adventurous!"
    InfoMsg Route2_Text_TheresSomeGraffitiOther, 2
    LastKeyWait
    InfoMsgClose_0039
    MedalGive 18
    VMJump L_00C7

L_00B5:
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 2"
    MsgPlaceSign Route2_Text_Route2_2, 3
    MsgPlaceSignClose

L_00C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nPokémon that participate in battle\nreceive Exp. Points.[f000]븁\u0000\nHave your Pokémon battle often,\nand make them stronger and stronger!"
    MsgPlaceSign Route2_Text_TrainerTipsPokemonParticipate, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a69
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    VMStackPush EVENT_WORK_0x4186
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0245
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokePartyGetCount 0x8022, 0

L_0130:
    VMStackPush 0x8022
    VMStackPush 0x8023
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_01A1
    PokePartyGetParam 0x8024, 0x8023, 10
    PokePartyIsEgg 0x8026, 0x8023
    VMStackPush 0x8024
    VMStackPushConst 116
    VMStackCmp CMP_EQ
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0195
    PokePartyGetSpecies EVENT_WORK_0x4186, 0x8023
    WordSetPokeSpecies 0, EVENT_WORK_0x4186
    WorkSetConst 0x8025, 1

L_0195:
    WorkAdd 0x8023, 1
    VMJump L_0130

L_01A1:
    // "What could be the perfect\ninstrument for me?[f000]븁\u0000\nFor example...[f000]븁\u0000\nI want a strong impact--\nan impact as strong as a Pokémon[f000]븀\u0000\nwith a tough Ability like Solid Rock[f000]븀\u0000\nthat reduces the power[f000]븀\u0000\nof supereffective moves![f000]븁\u0000\nIf you have a Pokémon like that,\nplease show it to me!"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_WhatCouldPerfectInstrument, 0, 0
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023B
    MsgWaitAdvance
    // "Th-that [f000]ā\u0001\u0000...\nThat Ability is Solid Rock![f000]븁\u0000\n[f000]ā\u0001\u0000\nis as hard as a rock![f000]븁\u0000\nAnd has a rocking heart![f000]븁\u0000\nIn other words, it has an impact\nthat rocks those who see it![f000]븁\u0000\nSolid Rock... Like a hard rock...\nHard rock?![f000]븀\u0000\nCould that be the sound[f000]븀\u0000\nI've been looking for?[f000]븁\u0000\nAt any rate, thanks!\nTake this, OK?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_ThAbilitySolidRock, 0, 0
    ItemCheckSpace ITEM_DAWN_STONE, 1, 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FF
    // "Oh, you can't fit any\nmore in your Bag."
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_OhCantFitAny, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4186, 0
    VMJump L_0235

L_01FF:
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 109
    WorkSet 0x8001, 1
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "What is this feeling\ncoursing through my veins?"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_WhatFeelingCoursingThrough, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4000, 1

L_0235:
    VMJump L_023F

L_023B:
    LastKeyWait
    MsgWinCloseAll

L_023F:
    VMJump L_0289

L_0245:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0272
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What is this feeling\ncoursing through my veins?"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_WhatFeelingCoursingThrough, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0289

L_0272:
    SEPlay SEQ_SE_MESSAGE
    WordSetPokeSpecies 0, EVENT_WORK_0x4186
    // "Jugga jya jaaaan![f000]븁\u0000\nDeedley deeedly deeedly deeedly,\nmeedley meedley meedley meedley,[f000]븀\u0000\nMEEEEEEE![f000]븁\u0000\nYeeeeah! Your [f000]ā\u0001\u0000's\nhard-rockin' Ability opened my eyes![f000]븁\u0000\nJuggah juggah jah!\nDuddah daaaaaah, bwan![f000]븁\u0000\nThat's it! I should join Roxie's band!\nI could rock out with her! Dual guitars![f000]븀\u0000\nAwesome!"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_JuggaJyaJaaaanDeedley, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0289:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what?[f000]븁\u0000\nYou know the guy at the ledge\non Route 19? He's my rival!"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_KnowWhatKnowGuy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon aren't feeling well,\nso I'm not walking in the tall grass.[f000]븁\u0000\nOh, wait... You have eight Badges!\nEep! I'm so embarrassed![f000]븁\u0000\nYou already know this stuff!\nI shouldn't try to show off!"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_PokemonArentFeelingWell, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Battles between Pokémon Trainers\nare serious affairs you can't run from![f000]븁\u0000\nI mean, more than anything, you can't\nrun away from other Trainers[f000]븀\u0000\nin front of your beloved Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, Route2_Text_BattlesBetweenPokemonTrainers, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
