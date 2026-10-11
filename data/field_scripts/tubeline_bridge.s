#include "asm/field_script.inc"
#include "text/script/tubeline_bridge.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_3:
    VMHalt

Script_1:
    VMHalt

Script_2:
    VMHalt
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

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This Tubeline Bridge was No. 1\nin the bridge rankings in Unova.[f000]븀\u0000\nThat means it's the sturdiest!"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_TubelineBridgeNo1, 0, 1, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4185
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_021D
    // "Argh! Loud! It's way too noisy![f000]븁\u0000\nIt's all the trains! They never stop![f000]븁\u0000\nWhen the train runs below,\nit's unbearably noisy!"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_ArghLoudItsWay, 1, 0, 1
    MsgWaitAdvance
    Random EVENT_WORK_0x4002, 4
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F2
    // "To be more specific...\nThe noise level is about 70 decibels![f000]븁\u0000\nIt's as noisy as the main street\nin Castelia City[f000]븀\u0000\nwhen there are a lot of people!"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_MoreSpecificNoiseLevel, 1, 0, 1
    MsgWaitAdvance
    VMJump L_0161

L_00F2:
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0119
    // "To be more specific...\nThe noise level is about 80 decibels![f000]븁\u0000\nIt's as noisy as loud people!"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_MoreSpecificNoiseLevel_2, 1, 0, 1
    MsgWaitAdvance
    VMJump L_0161

L_0119:
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0140
    // "To be more specific...\nThe noise level is about 100 decibels![f000]븁\u0000\nIt's as noisy as the horn of\na truck right next to my ear!"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_MoreSpecificNoiseLevel_3, 1, 0, 1
    MsgWaitAdvance
    VMJump L_0161

L_0140:
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0161
    // "To be more specific...\nThe noise level is about 120 decibels![f000]븁\u0000\nIt's as noisy as an engine of a plane\nin top gear!"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_MoreSpecificNoiseLevel_4, 1, 0, 1
    MsgWaitAdvance

L_0161:
    PokePartyGetCount 0x8020, 0

L_0167:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_01D8
    PokePartyGetParam 0x8022, 0x8021, 10
    PokePartyIsEgg 0x8024, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 43
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01CC
    PokePartyGetSpecies EVENT_WORK_0x4185, 0x8021
    WordSetPokeSpecies 0, EVENT_WORK_0x4185
    WorkSetConst 0x8023, 1

L_01CC:
    WorkAdd 0x8021, 1
    VMJump L_0167

L_01D8:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0207
    // "Oh, hey! Hey![f000]븁\u0000\nThat [f000]ā\u0001\u0000's Ability is\nSoundproof, isn't it?[f000]븁\u0000\nGreat! I'll catch my own [f000]ā\u0001\u0000\nand get rid of all this noise![f000]븁\u0000\nWait! What was that? I can't hear you!\nArgh! It's not getting any quieter![f000]븁\u0000\nEven though your [f000]ā\u0001\u0000\nwith the Soundproof Ability[f000]븀\u0000\nis right here, it's still so noisy.[f000]븁\u0000\n...[f000]븁\u0000\nCould it be?\nAm I the one making the most noise?"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_OhHeyHeyS, 1, 0, 1
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x4003, 1
    VMJump L_0217

L_0207:
    // "Argh! So loud! If we just had a Pokémon\nwith the Soundproof Ability, we could[f000]븀\u0000\nmake all this noise go away!"
    ActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_ArghLoudIfWe, 1, 0, 1
    LastKeyWait
    MsgWinCloseAll

L_0217:
    VMJump L_0230

L_021D:
    WordSetPokeSpecies 0, EVENT_WORK_0x4185
    // "This is what I've discovered.[f000]븁\u0000\nWhen I speak in a low voice,\nI don't mind the train noise.[f000]븁\u0000\nI was the one being noisy, after all...[f000]븁\u0000\nThanks to you and your [f000]ā\u0001\u0000,\nnow I know. Thank you."
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_WhatIveDiscoveredWhen, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0230:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 370
    WorkSet 0x8001, 1
    WorkSet 0x8002, 345
    WorkSet 0x8003, 15
    WorkSet 0x8004, 16
    WorkSet 0x8005, 16
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    Cmd_02B4 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_02DD
    Cmd_02B5 0, 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We were riding our motorbikes\nto our hearts' content...[f000]븁\u0000\nWe believed we could\nride forever and ever...[f000]븁\u0000\nYes, it's an infinite,\nlimitless, breakneck road...[f000]븁\u0000\nWith the breakneck team, Black Empoleon![f000]븁\u0000\nBack when I had a one-on-one\nbattle with [f000]Ā\u0001\u0001...[f000]븀\u0000\nthat was our golden age.[f000]븁\u0000\nIt was the age of gold."
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_WeWereRidingOur, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02F1

L_02DD:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We were riding our motorbikes\nto our hearts' content...[f000]븁\u0000\nWe believed we could\nride forever and ever...[f000]븁\u0000\nYes, it's an infinite,\nlimitless, breakneck road...[f000]븁\u0000\nWith the breakneck team, Black Empoleon![f000]븁\u0000\nBack when I had a one-on-one\nbattle with the Trainer...[f000]븀\u0000\nthat was our golden age.[f000]븁\u0000\nIt was the age of gold."
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_WeWereRidingOur_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_02F1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DF
    // "[f000]븉\u0001\u0002Oh... Oh...\nSo...thirsty...[f000]븁\u0000\nI met you on\nthe Driftveil Drawbridge...[f000]븁\u0000\nG-g-give me...\nFresh Water...?[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_OhOhThirstyMet, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CB
    ItemSub ITEM_FRESH_WATER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B7
    MsgWinCloseAll
    SEPlay SEQ_SE_ARDEMO_01
    SEWait
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0389
    ActorCmdExec 3, Movement_0408
    VMJump L_0391

L_0389:
    ActorCmdExec 3, Movement_041C

L_0391:
    VMSleep 20
    ActorCmdExec 255, Movement_042C
    ActorCmdWait
    ActorDelete 3
    WorkSetConst EVENT_WORK_0x4108, 3
    FlagSet EVENT_FLAG_0x035c
    FlagReset EVENT_FLAG_0x035d
    VMJump L_03C5

L_03B7:
    // "[f000]븉\u0001\u0002But... You don't have Fresh Water...\nI appreciate the thought, though...[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_ButDontHaveFresh, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03C5:
    VMJump L_03D9

L_03CB:
    // "[f000]븉\u0001\u0002Thank...[f000]븁\u0000\nWhat?\nOh...[f000]븁\u0000\nWithout Fresh Water...\nI can't run on bridges anymore.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_ThankWhatOhWithout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03D9:
    VMJump L_0400

L_03DF:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0400
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, TubelineBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0400:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0408:
    Move 39, 4
    Move 19, 1
    Move 16, 8
    Move 20, 25
    MoveEnd

Movement_041C:
    Move 36, 4
    Move 16, 8
    Move 20, 25
    MoveEnd

Movement_042C:
    Move 0, 1
    MoveEnd
