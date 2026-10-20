#include "asm/field_script.inc"
#include "text/script/pwt.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

L_005C:
    VMStackPush EVENT_WORK_0x40c6
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMStackPush EVENT_WORK_0x40c6
    VMStackPushConst 3
    VMStackCmp CMP_GT
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0089
    ObjInitWarpGPos 3, 202, 0, 492

L_0089:
    VMReturn

Script_5:
    VMCall L_005C
    VMStackPush EVENT_WORK_0x40c6
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B0
    ObjInitNPCGPos 9, 1, 197, 0, 468

L_00B0:
    VMHalt

Script_17:
    VMCall L_005C
    VMHalt

Script_6:
    VMStackPush EVENT_WORK_0x40c6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DF
    ActorSetGPos 6, 196, 0, 468, 0
    VMJump L_010A

L_00DF:
    VMStackPush EVENT_WORK_0x40c6
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010A
    ActorSetGPos 6, 196, 65535, 490, 3
    ActorSetGPos 8, 198, 65535, 490, 2

L_010A:
    VMHalt

Script_1:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 466
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0137
    ActorCmdExec 255, Movement_025C
    ActorCmdWait
    VMJump L_018D

L_0137:
    VMStackPush 0x8022
    VMStackPushConst 467
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015A
    ActorCmdExec 255, Movement_0268
    ActorCmdWait
    VMJump L_018D

L_015A:
    VMStackPush 0x8022
    VMStackPushConst 468
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_017D
    ActorCmdExec 255, Movement_0274
    ActorCmdWait
    VMJump L_018D

L_017D:
    ActorWalkRoute 255, 197, 469, 0, 8, 0
    ActorCmdWait

L_018D:
    ActorCmdExec 6, Movement_08C4
    ActorCmdExec 7, Movement_08C4
    VMStackPush 0x8022
    VMStackPushConst 469
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01B8
    ActorCmdExec 255, Movement_08BC

L_01B8:
    ActorCmdWait
    // "Clay: Here we are![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_ClayHereWe, 7, 0, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_08BC
    ActorCmdExec 7, Movement_08BC
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 8024, 0, 0xed000, 0xc58000, 0, 0x1ce3000, 40
    EvCameraWait
    ActorCmdWait
    // "Whaddya think?\nGreat buildin', huh?[f000]븁\u0000\nHere's where the Pokémon World\nTournament takes place![f000]븀\u0000\nAin't she purty?[f000]븁\u0000\nFollow me, tads![f000]븁\u0000"
    InfoMsg Pwt_Text_WhaddyaThinkGreatBuildin, 2
    InfoMsgClose_0039
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 7, Movement_0280
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 7
    SEWait
    ActorCmdExec 255, Movement_028C
    VMSleep 12
    ActorCmdExec 6, Movement_0294
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40c6, 1
    FlagSet EVENT_FLAG_0x02c7
    FlagSet EVENT_FLAG_0x02c9
    FlagSet EVENT_FLAG_0x03e8
    RTReserveScript 8
    MapChangeWarp ZONE_PWT_2, 15, 26, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_025C:
    Move 13, 3
    Move 15, 6
    MoveEnd

Movement_0268:
    Move 13, 2
    Move 15, 6
    MoveEnd

Movement_0274:
    Move 13, 1
    Move 15, 6
    MoveEnd

Movement_0280:
    Move 14, 1
    Move 12, 3
    MoveEnd

Movement_028C:
    Move 12, 4
    MoveEnd

Movement_0294:
    Move 15, 1
    Move 12, 2
    MoveEnd

Script_2:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x02c7
    FlagReset EVENT_FLAG_0x02c8
    FlagReset EVENT_FLAG_0x02c6
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xc58000, 0, 0x1d48000, 10
    ActorCmdExec 255, Movement_0490
    VMSleep 12
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 6
    SEWait
    ActorCmdExec 6, Movement_049C
    VMSleep 12
    ActorAdd 8
    ActorCmdExec 8, Movement_04AC
    EvCameraWait
    ActorCmdWait
    WordSetPlayerName 0
    // "Cheren: I barely recognized\nyou and [f000]Ā\u0001\u0000.[f000]븁\u0000\nYou two are way different from when\nwe battled in Aspertia City![f000]븁\u0000\nTraveling with Pokémon makes\neveryone grow so much...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_CherenBarelyRecognizedTwo, 8, 0, 0
    ActorNew 186, 470, 2, 251, 293, 0
    ActorCmdExec 251, Movement_04BC
    VMSleep 48
    ActorCmdExec 6, Movement_04C8
    ActorCmdWait
    MsgWinCloseAll
    ActorCmdExec 255, Movement_08C4
    ActorCmdExec 8, Movement_08C4
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: What was that just now?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_WhatJustNow, 6, 0, 0
    MsgWinCloseAll
    ActorCmdExec 8, Movement_08CC
    ActorCmdExec 255, Movement_08BC
    ActorCmdWait
    // "I'm going after him!\n[f000]Ā\u0001\u0000, come with me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_ImGoingAfterHim, 6, 0, 0
    MsgWinCloseAll
    // "Stop.[f000]븁\u0000"
    InfoMsg Pwt_Text_Stop, 1
    MsgWinCloseAll
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 9
    SEWait
    BGMPlay SEQ_BGM_E_ACHROMA
    ActorCmdExec 9, Movement_04D4
    VMSleep 8
    ActorCmdExec 6, Movement_08BC
    ActorCmdExec 8, Movement_08BC
    ActorCmdWait
    // "Colress: There's no reason\nfor you to stick your necks into[f000]븀\u0000\nsomething so dangerous![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_ColressTheresNoReason, 9, 0, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: The Pokémon I'm looking\nfor--my little sister's Purrloin--it[f000]븀\u0000\nmight be with them![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_PokemonImLookingLittle, 6, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 6, 196, 480, 1, 4, 1
    VMSleep 12
    ActorCmdExec 8, Movement_08C4
    ActorCmdExec 255, Movement_08C4
    ActorCmdWait
    ActorCmdExec 255, Movement_08BC
    ActorCmdWait
    // "Cheren: I'm going, too![f000]븁\u0000\nI've got his back![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_CherenImGoingToo, 8, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 8, 196, 480, 1, 8, 1
    VMSleep 24
    ActorCmdExec 255, Movement_08C4
    ActorCmdWait
    ActorCmdExec 9, Movement_04E4
    VMSleep 8
    ActorCmdExec 255, Movement_08BC
    ActorCmdWait
    // "Colress: I don't understand.\nThat's not courage, it's recklessness![f000]븁\u0000\nDoes he think anything is possible simply\nbecause he has Pokémon with him?[f000]븁\u0000\nNo, no...\nThat's not possible.[f000]븁\u0000\nAll Trainers and Pokémon are bound\nto one another by Poké Balls...[f000]븁\u0000\nThen maybe it is this bond that will allow\nTrainers to overcome the impossible if[f000]븀\u0000\nthey trust their partner Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_ColressDontUnderstandThats, 9, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 10
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    BGMChangeMap
    ActorDelete 6
    ActorDelete 8
    ActorDelete 251
    WorkSetConst EVENT_WORK_0x40c6, 3
    FlagSet EVENT_FLAG_0x02c7
    FlagSet EVENT_FLAG_0x02c8
    FlagSet EVENT_FLAG_0x037a
    FlagReset EVENT_FLAG_0x033e
    FlagReset EVENT_FLAG_0x033f
    FlagReset EVENT_FLAG_0x033d
    WorkSetConst EVENT_WORK_0x40f0, 1
    HollowRivalCmd_0262 1, 15
    HollowRivalCmd_0262 2, 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0490:
    Move 13, 4
    Move 32, 1
    MoveEnd

Movement_049C:
    Move 13, 3
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_04AC:
    Move 13, 3
    Move 15, 1
    Move 34, 1
    MoveEnd

Movement_04BC:
    Move 19, 10
    Move 17, 10
    MoveEnd

Movement_04C8:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_04D4:
    Move 13, 1
    Move 14, 1
    Move 33, 1
    MoveEnd

Movement_04E4:
    Move 15, 1
    Move 13, 2
    MoveEnd

Script_3:
    ActorsPauseAll
    ActorCmdExec 6, Movement_05A8
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: The Shadow Triad?\nWhat's their deal, anyway![f000]븁\u0000\nAAAAH!\nTeam Plasma! Where did you vanish to![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_ShadowTriadWhatsTheir, 6, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 6, 196, 476, 1, 4, 1
    ActorCmdWait
    ActorDelete 6
    ActorCmdExec 8, Movement_08D4
    ActorCmdWait
    // "Cheren: The Shadow Triad...[f000]븁\u0000\nWith their superhuman powers, they\ncan immobilize people and then disappear![f000]븁\u0000\nBut I'm more concerned with what\nZinzolin said...[f000]븁\u0000\n“Once again, we will use the\nlegendary Dragon-type Pokémon[f000]븀\u0000\nand we will rule the Unova region!\"[f000]븀\u0000\nWhat could that mean?[f000]븁\u0000\nThe legendary Dragon-type Pokémon\nReshiram and Zekrom[f000]븀\u0000\naren't in Unova anymore...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_CherenShadowTriadTheir, 8, 0, 0
    MsgWinCloseAll
    ActorCmdExec 8, Movement_05BC
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000.[f000]븁\u0000\nThere's something I want to look into,\nso I'm going to head to Route 6![f000]븁\u0000\nBe careful out there![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Pwt_Text_TheresSomethingWantLook, 8, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 8, 197, 476, 1, 8, 1
    ActorCmdWait
    ActorDelete 8
    WorkSetConst EVENT_WORK_0x40c6, 5
    FlagSet EVENT_FLAG_0x02c7
    FlagSet EVENT_FLAG_0x02c8
    FlagReset EVENT_FLAG_0x0304
    FlagReset EVENT_FLAG_0x0307
    WorkSetConst EVENT_WORK_0x4135, 3
    WorkSetConst EVENT_WORK_0x40c9, 1
    WorkSetConst EVENT_WORK_0x40ca, 1
    FlagSet EVENT_FLAG_0x03c2
    HollowRivalCmd_0262 2, 5
    HollowRivalCmd_0262 1, 17
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_05A8:
    Move 34, 1
    Move 35, 1
    Move 34, 1
    Move 35, 1
    MoveEnd

Movement_05BC:
    Move 14, 1
    Move 33, 1
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Colress: Trust...\nIt's too much of an unknown factor.[f000]븁\u0000\nBut if believing in your Pokémon\ngives you the courage to stand up[f000]븀\u0000\nto Team Plasma...[f000]븁\u0000\nAnd the courage to help your friends...[f000]븁\u0000\nThen follow them south to the dock!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_ColressTrustItsToo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 540
    WorkSet 0x8001, 1
    WorkSet 0x8002, 448
    WorkSet 0x8003, 14
    WorkSet 0x8004, 15
    WorkSet 0x8005, 15
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

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Water Pledge,\nFire Pledge,[f000]븀\u0000\nand Grass Pledge.[f000]븁\u0000\nWhen combinations of these\nthree moves are used in battle,[f000]븀\u0000\nspecial things happen!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_WaterPledgeFirePledge, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My husband can teach some Pokémon the\nultimate moves! I'll tell you their names.[f000]븁\u0000\nThe blazing Fire-type Pokémon:\nCharizard, Typhlosion, Blaziken,[f000]븀\u0000\nInfernape, and Emboar![f000]븁\u0000\nThe restless Water-type Pokémon:\nBlastoise, Feraligatr, Swampert,[f000]븀\u0000\nEmpoleon, and Samurott![f000]븁\u0000\nThe quiet Grass-type Pokémon:\nVenusaur, Meganium, Sceptile,[f000]븀\u0000\nTorterra, and Serperior!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_HusbandCanTeachSome, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What kind of Trainers will come?\nWhat kind of battle will it be?"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_WhatKindTrainersWill, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When battling in front of people,\nit's well known that you should stand[f000]븀\u0000\nyour ground and not dance around.[f000]븁\u0000\nBut I can't resist moves that groove,\nlike Petal Dance, Quiver Dance,[f000]븀\u0000\nFiery Dance, and Dragon Dance.[f000]븁\u0000\nAnd on rare occasions, even Lunar Dance!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_WhenBattlingFrontPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The crowd will go wild for my Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_CrowdWillGoWild, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Isn't it wonderful how people\nchallenging themselves helps[f000]븀\u0000\nbring the world together!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_IsntWonderfulHowPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A ship's only really a ship when\nit's crossing an ocean.[f000]븀\u0000\nDocked ships sure look lonely."
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_ShipsOnlyReallyShip, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That cave down there\nis the Relic Passage![f000]븁\u0000\nIt was recently discovered,\nbut amazingly, it's...[f000]븁\u0000\nWait? Where was it\nconnected to again?"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_CaveDownThereRelic, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "The Pokémon World Tournament\naka the PWT[f000]븀\u0000\nCall it what you like!"
    MsgPlaceSign Pwt_Text_PokemonWorldTournamentAka, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0087
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0779
    // "When a Pokémon learns the move\nHidden Power, somehow I can tell[f000]븀\u0000\nwhat type that move will be![f000]븁\u0000\nShould I tell you what type of\nHidden Power your Pokémon will learn?"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_WhenPokemonLearnsMove, 0, 0
    FlagSet EVENT_FLAG_0x0087
    VMJump L_0783

L_0779:
    // "Now that I'm aware of my hidden power,\nI can tell you what type of Hidden Power[f000]븀\u0000\nyour Pokémon will learn![f000]븀\u0000\nDo you want to know?"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_NowImAwareHidden, 0, 0

L_0783:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07AA
    // "If you want to know, ask me, and I'll\nactivate my hidden power for you!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_IfWantKnowAsk, 0, 0
    VMJump L_07B2

L_07AA:
    ActorMsgClose
    VMCall L_07BC

L_07B2:
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_07BC:
    WorkSetConst 0x8023, 0
    CallPokeSelect 0, 0x8010, 0x8023, 0
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07EB
    // "If you want to know, ask me, and I'll\nactivate my hidden power for you!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_IfWantKnowAsk, 0, 0
    VMReturn

L_07EB:
    PokePartyIsEgg 0x8010, 0x8023
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0810
    // "It's not possible for an Egg to use\nHidden Power!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_ItsNotPossibleEgg, 0, 0
    VMReturn

L_0810:
    PokePartyGetHiddenPowerType 0x8010, 0x8023
    VMStackPush 0x8010
    VMStackPushConst 17
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0835
    // "I'm sorry, but this Pokémon can't learn\nto use Hidden Power."
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_ImSorryButPokemon, 0, 0
    VMReturn

L_0835:
    WordSetPokeTypeName 0, 0x8010
    DebugPrint 0x8010
    PokePartyHasMove 0x8010, 237, 0x8023
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0869
    // "The type of this Pokémon's\nHidden Power is [f000]ă\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_TypePokemonsHiddenPower, 0, 0
    VMJump L_0873

L_0869:
    // "If this Pokémon were to learn\nHidden Power, the move's type[f000]븀\u0000\nwould be [f000]ă\u0001\u0000!"
    ParentActorMsg MSGFILE_SCRIPT, Pwt_Text_IfPokemonWereLearn, 0, 0

L_0873:
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x23
    Move 128, 0
    Move 13, 1
    MoveEnd
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
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_08BC:
    Move 32, 1
    MoveEnd

Movement_08C4:
    Move 33, 1
    MoveEnd

Movement_08CC:
    Move 34, 1
    MoveEnd

Movement_08D4:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
