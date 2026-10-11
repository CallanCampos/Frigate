#include "asm/field_script.inc"
#include "text/script/nacrene_city.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0396
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4116
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_008D
    FlagSet EVENT_FLAG_0x0396
    WorkSetConst EVENT_WORK_0x4116, 1

L_008D:
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Nacrene City\nA Pearl of a Place"
    MsgPlaceSign NacreneCity_Text_NacreneCityPearlPlace, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Café Warehouse[f000]븁\u0000\n\nTry our delicious specials\non Wednesdays and Saturdays!"
    MsgPlaceSign NacreneCity_Text_CafeWarehouseTryOur, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Nacrene Museum"
    MsgPlaceSign NacreneCity_Text_NacreneMuseum, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "These old textile storehouses\nare being reused as studios.[f000]븁\u0000\nHow innovative![f000]븁\u0000\nNew ideas create new values, don't they!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_TheseOldTextileStorehouses, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, Trainer! Step inside for a moment!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_HeyTrainerStepInside, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You see, I believed it would become\npopular because it was a storehouse!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_SeeBelievedWouldBecome, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The rail line was abandoned, and\nthe storehouses went unused...[f000]븁\u0000\nThen young people with artistic\naspirations started renting them[f000]븀\u0000\ncheaply as art studios.[f000]븁\u0000\nIf people hadn't been so creative,\nPokémon might be living here now."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_RailLineAbandonedStorehouses, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My accordion's heavy!\nIt weighs over 20 pounds."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_AccordionsHeavyWeighsOver, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What do you call a storehouse\nyou can't find? A where-house!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_WhatCallStorehouseCant, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "How many houses could a warehouse\nwear if a warehouse could wear houses?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_HowManyHousesCould, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Walking on abandoned railroad tracks...\nEveryone does it sometimes, right?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_WalkingAbandonedRailroadTracks, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I was just collecting the Pokémon\nI like, and before I knew it, I had six!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_JustCollectingPokemonLike, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The one trying to draw\nSmeargle's move Sketch is me!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_OneTryingDrawSmeargles, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    // "A mysterious presence can be felt here!\nCheck the surrounding area?"
    SystemMsg NacreneCity_Text_MysteriousPresenceCanFelt, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029E
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x0396
    WorkSetConst EVENT_WORK_0x4116, 2
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0241
    ActorCmdExec 255, Movement_0360
    ActorCmdWait

L_0241:
    VMSleep 30
    PVPlay 480, 0
    // "Kyouuuun!"
    InfoMsg NacreneCity_Text_Kyouuuun, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    PlayerGetGPos 0x8021, 0x8022
    ActorAdd 0
    ActorSetGPos 0, 633, 2, 580, 3
    WorkSub 0x8022, 2
    ActorMoveLinear 0, 0x8021, 0, 0x8022, 48
    ActorSetGPos 0, 0x8021, 0, 0x8022, 2
    ActorCmdExec 0, Movement_0368
    ActorCmdWait
    VMSleep 16
    VMJump L_02A0

L_029E:
    MsgWinCloseAll

L_02A0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 480, 0
    // "Kyouuuun!"
    ScreamMsg NacreneCity_Text_Kyouuuun, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 480, 65, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F4
    FlagSet EVENT_FLAG_0x0396
    WorkSetConst EVENT_WORK_0x4116, 3
    ActorDelete 0
    CallWildBattleEnd
    VMJump L_02F6

L_02F4:
    CallWildLose

L_02F6:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_030D
    VMJump L_0317

L_030D:
    FlagSet EVENT_FLAG_0x018c
    VMJump L_0347

L_0317:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0337
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0337
    VMJump L_0347

L_0337:
    // "Uxie went flying off somewhere..."
    SystemMsg NacreneCity_Text_UxieWentFlyingOff, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0347

L_0347:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0360:
    Move 32, 1
    MoveEnd

Movement_0368:
    Move 33, 1
    MoveEnd

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Nacrene Museum\nExhibit Schedule"
    InfoMsg NacreneCity_Text_NacreneMuseumExhibitSchedule, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Special Exhibit\nThat Pokémon's dormant form!"
    InfoMsg NacreneCity_Text_SpecialExhibitPokemonsDormant, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WordSetLoadJoinAvenueName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I heard there's a café in [f000]Ĺ\u0001\u0000\nwhere Pokémon can eat![f000]븁\u0000\nMaybe I should go try it and see how it\ncompares to Café Warehouse!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity_Text_HeardTheresCafeWhere, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
