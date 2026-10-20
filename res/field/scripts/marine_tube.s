#include "asm/field_script.inc"
#include "text/script/marine_tube.h"

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
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_1:
    Cmd_02C4
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0142
    // "[f000]븉\u0001\u0002Oh... Oh...\nSo...thirsty...[f000]븁\u0000\nI met you on Marvelous Bridge...[f000]븁\u0000\nG-g-give me...\nFresh Water...?[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_OhOhThirstyMet, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012E
    ItemSub ITEM_FRESH_WATER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011A
    MsgWinCloseAll
    SEPlay SEQ_SE_ARDEMO_01
    SEWait
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_RefreshedIm100Rehydrated, 1, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    PlayerGetRailPos 0x8025, 0x8026, 0x8027
    VMStackPush 0x8026
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EC
    ActorCmdExec 4, Movement_016C
    VMJump L_00F4

L_00EC:
    ActorCmdExec 4, Movement_017C

L_00F4:
    VMSleep 20
    ActorCmdExec 255, Movement_0188
    ActorCmdWait
    ActorDelete 4
    WorkSetConst EVENT_WORK_0x4108, 6
    FlagSet EVENT_FLAG_0x035f
    FlagReset EVENT_FLAG_0x035a
    VMJump L_0128

L_011A:
    // "[f000]븉\u0001\u0002But... You don't have Fresh Water...\nI appreciate the thought, though...[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_ButDontHaveFresh, 1, 0
    LastKeyWait
    MsgWinCloseAll

L_0128:
    VMJump L_013C

L_012E:
    // "[f000]븉\u0001\u0002Thank...[f000]븁\u0000\nWhat?\nOh...[f000]븁\u0000\nWithout Fresh Water...\nI can't run on bridges anymore.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_ThankWhatOhWithout, 1, 0
    LastKeyWait
    MsgWinCloseAll

L_013C:
    VMJump L_0163

L_0142:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0163
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_RefreshedIm100Rehydrated, 1, 0
    LastKeyWait
    MsgWinCloseAll

L_0163:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_016C:
    Move 39, 4
    Move 19, 1
    Move 17, 13
    MoveEnd

Movement_017C:
    Move 37, 4
    Move 17, 13
    MoveEnd

Movement_0188:
    Move 1, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is what the undersea world\nlooks like![f000]븁\u0000\nI've never seen this before,\nbecause I can't swim![f000]븁\u0000\nI'd given up on seeing this.\nI'm so moved!"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_WhatUnderseaWorldLooks, 1, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This place is like a walk-through\naquarium, but isn't this a place[f000]븀\u0000\nfor Pokémon to see us?"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_PlaceLikeWalkThrough, 1, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Wow!\nIt's an ocean trench![f000]븁\u0000\nSo deep!\nMaybe six miles deep?"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_WowItsOceanTrench, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That's right... It's deep...\nLove is infinitely deep..."
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_ThatsRightItsDeep, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rain Dance is a move to use\nwith a feeling of yearning.[f000]븁\u0000\nAs for when to use it...[f000]븁\u0000\nSigh... What's wrong with me?\nI can't think of any gripping ideas.[f000]븁\u0000\nWith a condition like this,\nI can't explain well on TV."
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_RainDanceMoveUse, 1, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0185
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0316
    PokePartyGetCount 0x8020, 0

L_023D:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_02AB
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029F
    PokePartyGetParam 0x8022, 0x8021, 6
    PokePartyIsEgg 0x8024, 0x8021
    VMStackPush 0x8022
    VMStackPush 245
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_029F
    WordSetPartyPokeSpecies 0, 0x8021
    WorkSetConst 0x8023, 1

L_029F:
    WorkAdd 0x8021, 1
    VMJump L_023D

L_02AB:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0302
    // "Does your Pokémon have\na Poison Barb?[f000]븁\u0000\nWow! [f000]ā\u0001\u0000 has\na Poison Barb![f000]븁\u0000\nGood going. I think you can\nuse this well, too![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_DoesPokemonHavePoison_2, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 281
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Black Sludge! Few Pokémon\nlike to hold it, though.[f000]븁\u0000\nFYI, I'm the first fan of Roxie!"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_BlackSludgeFewPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0185
    VMJump L_0310

L_0302:
    // "Does your Pokémon have\na Poison Barb?"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_DoesPokemonHavePoison, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0310:
    VMJump L_0324

L_0316:
    // "Black Sludge! Few Pokémon\nlike to hold it, though.[f000]븁\u0000\nFYI, I'm the first fan of Roxie!"
    ParentActorMsg MSGFILE_SCRIPT, MarineTube_Text_BlackSludgeFewPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0324:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
