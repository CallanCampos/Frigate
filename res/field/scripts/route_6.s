#include "asm/field_script.inc"
#include "text/script/route_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_4:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    BGMPlay SEQ_BGM_E_CHEREN
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    WorkCmpConst 0x8022, 371
    VMJumpIf CMP_EQ, L_005D
    VMJump L_006B

L_005D:
    ActorCmdExec 8, Movement_0184
    VMJump L_00F7

L_006B:
    WorkCmpConst 0x8022, 372
    VMJumpIf CMP_EQ, L_007E
    VMJump L_008C

L_007E:
    ActorCmdExec 8, Movement_0190
    VMJump L_00F7

L_008C:
    WorkCmpConst 0x8022, 373
    VMJumpIf CMP_EQ, L_009F
    VMJump L_00AD

L_009F:
    ActorCmdExec 8, Movement_019C
    VMJump L_00F7

L_00AD:
    WorkCmpConst 0x8022, 374
    VMJumpIf CMP_EQ, L_00C0
    VMJump L_00CE

L_00C0:
    ActorCmdExec 8, Movement_01A4
    VMJump L_00F7

L_00CE:
    WorkCmpConst 0x8022, 375
    VMJumpIf CMP_EQ, L_00E1
    VMJump L_00F7

L_00E1:
    ActorCmdExec 8, Movement_01B0
    ActorCmdExec 255, Movement_04A4
    VMJump L_00F7

L_00F7:
    ActorCmdWait
    // "Cheren: Oh, [f000]Ā\u0001\u0000.\nCan you come with me for a moment?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_CherenOhCanCome, 8, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 8, 135, 370, 1, 8, 0
    ActorWalkRoute 255, 135, 371, 1, 8, 0
    ActorCmdWait
    WorkSetConst 0x8023, 0
    BMCreateHandleByGPos 0x8023, 1, 135, 369
    BMHndAudioVisualAnmPlay 0x8023, 0
    BMHndAnmWait 0x8023
    WorkSetConst 0x8023, 0
    ActorCmdExec 8, Movement_047C
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 8
    SEWait
    ActorCmdExec 255, Movement_01BC
    ActorCmdWait
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40c9, 2
    FlagSet EVENT_FLAG_0x0304
    RTReserveScript 3
    MapChangeWarp ZONE_ROUTE_6_2, 6, 9, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0184:
    Move 12, 2
    Move 15, 2
    MoveEnd

Movement_0190:
    Move 12, 1
    Move 15, 2
    MoveEnd

Movement_019C:
    Move 15, 2
    MoveEnd

Movement_01A4:
    Move 13, 1
    Move 15, 2
    MoveEnd

Movement_01B0:
    Move 13, 2
    Move 15, 2
    MoveEnd

Movement_01BC:
    Move 12, 2
    MoveEnd

Script_5:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x0305
    FlagReset EVENT_FLAG_0x0306
    ActorNew 110, 353, 1, 251, 369, 0
    PVPlay 638, 0
    // "Kawbraa!"
    ScreamMsg Route6_Text_Kawbraa, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_020E
    ActorCmdExec 255, Movement_04B4
    ActorCmdWait

L_020E:
    ActorCmdExec 255, Movement_04D4
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    ActorAnimationInit 251
    ActorAnimationPlay 0
    SEPlay SEQ_SE_SW_KOBALON_01
    ActorAnimationWait
    SEWait
    VMSleep 30
    // "Kawbraa!"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_Kawbraa, 251, 0, 0
    PVPlay 638, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMSleep 12
    ActorAnimationPlay 1
    SEPlay SEQ_SE_SW_KOBALON_02
    ActorAnimationWait
    SEWait
    ActorAnimationFree
    ActorDelete 251
    ActorAdd 10
    ActorAdd 9
    PlayerGetGPos 0x8021, 0x8022
    ActorSetGPos 9, 102, 0, 372, 0
    VMSleep 30
    WorkAdd 0x8022, 2
    ActorWalkRoute 10, 103, 367, 1, 16, 1
    VMSleep 8
    ActorWalkRoute 9, 102, 367, 1, 16, 1
    VMSleep 30
    VMStackPush 0x8021
    VMStackPushConst 102
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C9
    ActorCmdExec 255, Movement_0398
    VMJump L_02F2

L_02C9:
    VMStackPush 0x8021
    VMStackPushConst 104
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EA
    ActorCmdExec 255, Movement_03A4
    VMJump L_02F2

L_02EA:
    ActorCmdExec 255, Movement_04BC

L_02F2:
    ActorCmdWait
    // "Rood: That Pokémon...?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_RoodPokemon, 10, 6, 0
    MsgWinCloseAll
    // "Elderly Man: It's the legendary Pokémon\ncalled Cobalion![f000]븁\u0000\nI told you the story the other\nday, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_ElderlyManItsLegendary, 9, 4, 0
    MsgWinCloseAll
    // "Rood: Oh, Cobalion!\nWhat a noble presence![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_RoodOhCobalionWhat, 10, 6, 0
    MsgWinCloseAll
    // "Elderly Man: They say the three Pokémon\nCobalion, Virizion, and Terrakion[f000]븀\u0000\nfought against people to protect[f000]븀\u0000\nPokémon from the war those[f000]븀\u0000\npeople started...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_ElderlyManTheySay, 9, 4, 0
    // "When people fight, there's no peace for\nPokémon, either.[f000]븁\u0000\nThe three legends learned that\nthe deeds of humans could lead[f000]븀\u0000\nto dire consequences for their world.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_WhenPeopleFightTheres, 9, 4, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_04DC
    ActorCmdWait
    // "But... Why did it show up\nin front of people?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_ButWhyDidShow, 9, 4, 0
    // "Is it patrolling the Unova region\nto protect Pokémon because it fears for[f000]븀\u0000\ntheir safety after the commotion caused[f000]븀\u0000\nby Team Plasma two years ago?[f000]븁\u0000\nOr does it sense a new problem...?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_PatrollingUnovaRegionProtect, 9, 4, 0
    MsgWinCloseAll
    // "Rood: Unforgivable...[f000]븁\u0000\nSo the reach of Ghetsis's ambition\nand malice is growing ever wider and[f000]븀\u0000\nwarping the lives of all it touches...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route6_Text_RoodUnforgivableReachGhetsiss, 10, 6, 0
    MsgWinCloseAll
    ActorCmdExec 10, Movement_047C
    ActorCmdWait
    // "Did Cobalion appear in front of you\nmerely by accident?[f000]븁\u0000\nOr to plead with you to solve a problem?[f000]븁\u0000\nUnlike Lord N, I don't have the ability\nto understand the minds of Pokémon.[f000]븁\u0000\nTherefore, I don't know\nwhat that Pokémon is thinking.[f000]븁\u0000\nBut if you can befriend Cobalion,\nit will be a great asset to you[f000]븀\u0000\non your journey."
    ActorMsg MSGFILE_SCRIPT, Route6_Text_DidCobalionAppearFront, 10, 6, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0305
    FlagSet EVENT_FLAG_0x0306
    WorkSetConst EVENT_WORK_0x40ca, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0398:
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_03A4:
    Move 14, 1
    Move 33, 1
    MoveEnd

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Not just people and Pokémon...[f000]븁\u0000\nIt's best for all creatures to accept\nand trust one another..."
    ParentActorMsg MSGFILE_SCRIPT, Route6_Text_NotJustPeoplePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Did Cobalion appear in front of you\nmerely by accident?[f000]븁\u0000\nOr to plead with you to solve a problem?[f000]븁\u0000\nUnlike Lord N, I don't have the ability\nto understand the minds of Pokémon.[f000]븁\u0000\nTherefore, I don't know\nwhat that Pokémon is thinking.[f000]븁\u0000\nBut if you can befriend Cobalion,\nit will be a great asset to you[f000]븀\u0000\non your journey."
    ParentActorMsg MSGFILE_SCRIPT, Route6_Text_DidCobalionAppearFront, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Water Absorb and Dry Skin.[f000]븁\u0000\nPokémon with those Abilities love\nWater-type moves and rainy weather. ♪[f000]븁\u0000\nThe Gym Leader in Aspertia City\nis very familiar with Pokémon Abilities,[f000]븀\u0000\nisn't he?[f000]븁\u0000\nOh, do you know him? Then you can ask him\nabout Abilities through the Xtransceiver!"
    ParentActorMsg MSGFILE_SCRIPT, Route6_Text_WaterAbsorbDrySkin, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Chargestone Cave is great![f000]븁\u0000\nThe Ferroseed I met here\nwere absolutely adorable!"
    ParentActorMsg MSGFILE_SCRIPT, Route6_Text_ChargestoneCaveGreatFerroseed, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 6"
    MsgPlaceSign Route6_Text_Route6, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nYou can register certain items with\nthe Y Button to use them easily![f000]븁\u0000\nLook for a square check box beside\nthe name of a Key Item."
    MsgPlaceSign Route6_Text_TrainerTipsCanRegister, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a6d
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Researching the Year's Seasons:\nThe Season Research Lab"
    MsgPlaceSign Route6_Text_ResearchingYearsSeasonsSeason, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_047C:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd

Movement_04A4:
    Move 2, 1
    MoveEnd
    Move 3, 1
    MoveEnd

Movement_04B4:
    Move 32, 1
    MoveEnd

Movement_04BC:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_04D4:
    Move 75, 1
    MoveEnd

Movement_04DC:
    Move 159, 1
    MoveEnd
