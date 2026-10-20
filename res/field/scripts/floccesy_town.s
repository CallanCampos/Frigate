#include "asm/field_script.inc"
#include "text/script/floccesy_town.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    VMStackPush EVENT_WORK_0x40a5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0083
    ObjInitNPCGPos 0, 1, 113, 2, 669
    VMJump L_00B2

L_0083:
    VMStackPush EVENT_WORK_0x40a5
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40a5
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00B2
    ObjInitNPCGPos 0, 3, 111, 2, 669

L_00B2:
    VMHalt

Script_2:
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 0, Movement_0134
    ActorCmdWait
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 695
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00F5
    ActorWalkRoute 0, 110, 0x8023, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 0, Movement_071C
    ActorCmdWait

L_00F5:
    WordSetPlayerName 0
    // "Alder: Oh, that's right! [f000]Ā\u0001\u0000,\nare your Pokémon well?[f000]븁\u0000\nYour Pokémon are always doing\ntheir best for you, the Trainer,[f000]븀\u0000\nso you must always be kind to them![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_AlderOhThatsRight, 0, 0, 0
    // "My house is just a little farther![f000]븁\u0000\nStop by the Pokémon Center\nfirst if you'd like![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_HouseJustLittleFarther, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0140
    ActorCmdWait
    ActorSetGPos 0, 113, 2, 669, 1
    WorkSetConst EVENT_WORK_0x40a5, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0134:
    Move 75, 1
    Move 34, 1
    MoveEnd

Movement_0140:
    Move 15, 3
    Move 12, 11
    MoveEnd

Script_4:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8022
    VMStackPushConst 113
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0187
    WorkSub 0x8023, 1
    ActorWalkRoute 0, 0x8022, 0x8023, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0714
    ActorCmdWait

L_0187:
    // "Alder: Hey, this way!\nShall we start training?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_AlderHeyWayShall, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0734
    ActorCmdWait
    // "By the way...why are you\nholding two Town Maps?[f000]븁\u0000\n...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_ByWayWhyHolding, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_072C
    ActorCmdWait
    // "Oh ho!\nIt's your friend's Town Map, is it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_OhHoItsFriends, 0, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E3
    WordSetPokeSpecies 0, 501
    VMJump L_0206

L_01E3:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0201
    WordSetPokeSpecies 0, 498
    VMJump L_0206

L_0201:
    WordSetPokeSpecies 0, 495

L_0206:
    // "Your friend is the one with\nthe [f000]ā\u0001\u0000, isn't he?[f000]븁\u0000\nIt just so happens, he was training\nhis Pokémon on Route 20...[f000]븁\u0000\nWell, if that's the case, you should\ngo give him the Town Map first![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_FriendOneIsntHe, 0, 0, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x718000, 0x2001f, 0x29a5000, 40
    ActorCmdExec 0, Movement_027C
    ActorCmdWait
    EvCameraWait
    // "Just follow this road.\nIt goes to Route 20![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_JustFollowRoadGoes, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    ActorWalkRoute 0, 111, 669, 1, 8, 0
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 0, Movement_0724
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40a5, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_027C:
    Move 12, 2
    Move 35, 1
    MoveEnd

Script_5:
    ActorsPauseAll
    ActorCmdExec 0, Movement_072C
    ActorCmdWait
    PlayerGetGPos 0x8022, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 668
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C3
    ActorCmdExec 255, Movement_0714
    ActorCmdExec 0, Movement_070C
    VMJump L_02E6

L_02C3:
    VMStackPush 0x8023
    VMStackPushConst 670
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E6
    ActorCmdExec 255, Movement_070C
    ActorCmdExec 0, Movement_0714

L_02E6:
    ActorCmdWait
    // "Just follow this road.\nIt goes to Route 20![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_JustFollowRoadGoes, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_06DC
    VMSleep 8
    ActorCmdExec 0, Movement_0724
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0312:
    // "Alder: Oh! It looks like you've\ndelivered the Town Map to your friend![f000]븁\u0000\nHrm...[f000]븁\u0000\nYou were only gone a moment,\nbut you and your Pokémon have grown.[f000]븁\u0000\nWhy, I could almost mistake you\nfor someone else![f000]븁\u0000\nWell then. Instead of training you,\nI would like you and your Pokémon[f000]븀\u0000\nto give me a hand![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_AlderOhLooksLike, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 107, 662, 1, 8, 0
    VMSleep 16
    ActorCmdExec 255, Movement_071C
    ActorCmdWait
    BMCreateHandleByGPos 0x8020, 1, 107, 661
    BMHndAudioVisualAnmPlay 0x8020, 0
    BMHndAnmWait 0x8020
    ActorCmdExec 0, Movement_06D4
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    BMHndAudioVisualAnmPlay 0x8020, 1
    BMHndAnmWait 0x8020
    BMReleaseHandle 0x8020
    WorkSetConst EVENT_WORK_0x40a5, 4
    FlagSet EVENT_FLAG_0x02db
    VMReturn

Script_6:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    ActorWalkRoute 0, 0x8022, 669, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_070C
    ActorCmdWait
    VMCall L_0312
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    PlayerGetGPos 0x8022, 0x8023
    ActorWalkRoute 0, 0x8022, 668, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0714
    ActorCmdWait
    VMCall L_0312
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    VMNop
    VMStackMul
    DebugPrint 12
    DebugStack 254
    VMNop

Script_17:
    ActorsPauseAll
    WordSetPlayerName 0
    ActorNew 113, 662, 0, 251, 249, 0
    WorkSetConst 0x8024, 0
    TrainerCardGetSex 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0423
    // "Bianca: Heeey![f000]븁\u0000"
    InfoMsg FloccesyTown_Text_BiancaHeeey, 2
    VMJump L_0428

L_0423:
    // "Bianca: Hey there![f000]븁\u0000"
    InfoMsg FloccesyTown_Text_BiancaHeyThere, 2

L_0428:
    MsgWinCloseAll
    WorkSetConst 0x8024, 0
    BGMPlay SEQ_BGM_E_BERU
    ActorCmdExec 255, Movement_071C
    ActorCmdWait
    PlayerGetGPos 0x8022, 0x8023
    WorkSub 0x8022, 2
    ActorWalkRoute 251, 0x8022, 0x8023, 1, 8, 1
    ActorCmdWait
    // "I'm sorry![f000]븁\u0000\nI forgot to upgrade the Pokédex\nthat I gave you![f000]븁\u0000\nI'm going to add the Habitat List!\nIt's an amazing feature![f000]븁\u0000\nI'm just going to borrow your\nPokédex for a second![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_ImSorryForgotUpgrade, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_06DC
    ActorCmdWait
    SEPlay SEQ_SE_SW_ZKN_KAIHOU
    // "[f000]Ā\u0001\u0000's Pokédex\nwas upgraded!"
    SystemMsg FloccesyTown_Text_SPokedexUpgraded, 0
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    PokeDexEnableHabitatList
    // "With the Habitat List, you can check\nwhich Pokémon are in the area![f000]븁\u0000\nIt's a mode in the Pokédex![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_HabitatListCanCheck, 251, 0, 0
    // "To use it, open up the Pokédex\nand tap the Habitat List button[f000]븀\u0000\non the lower left of the Touch Screen![f000]븁\u0000\nNext, pick the area you want to see![f000]븁\u0000\nYou can see all the Pokémon that live in\nthat area. It even tells you which ones[f000]븀\u0000\nyou've already caught![f000]븁\u0000\nWould you like to hear\nmy explanation again?"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_UseOpenUpPokedex, 251, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04EE
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8025, 0

L_04BF:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E8
    // "To use it, open up the Pokédex\nand tap the Habitat List button[f000]븀\u0000\non the lower left of the Touch Screen![f000]븁\u0000\nNext, pick the area you want to see![f000]븁\u0000\nYou can see all the Pokémon that live in\nthat area. It even tells you which ones[f000]븀\u0000\nyou've already caught![f000]븁\u0000\nWould you like to hear\nmy explanation again?"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_UseOpenUpPokedex, 251, 0, 0
    YesNoWin 0x8025
    VMJump L_04BF

L_04E8:
    WorkSetConst 0x8025, 0

L_04EE:
    MsgWinCloseAll
    ActorCmdExec 251, Movement_072C
    ActorCmdWait
    // "I have a tip for you![f000]븁\u0000\nWhen you're walking down\na path, you'll sometimes see[f000]븀\u0000\nrustling grass![f000]븀\u0000\nIf you go to that spot...[f000]븁\u0000\nWell, I'll let the rest\nbe a surprise![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_HaveTipWhenYoure, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_070C
    ActorCmdWait
    VMSleep 16
    ActorCmdExec 251, Movement_0724
    ActorCmdWait
    // "Filling up the Pokédex will\nmake your world bigger![f000]븁\u0000\nSo go to many different places\nand meet many different Pokémon, OK?[f000]븁\u0000\nSee you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_FillingUpPokedexWill, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 113, 662, 1, 8, 0
    VMSleep 16
    BGMChangeMap
    ActorCmdWait
    ActorDelete 251
    WorkSetConst EVENT_WORK_0x4153, 2
    HollowRivalCmd_0262 3, 0
    FlagSet EVENT_FLAG_0x02e6
    FlagSet EVENT_FLAG_0x02e5
    WorkSetConst EVENT_WORK_0x40a8, 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Just follow this road.\nIt goes right to Route 20![f000]븁\u0000\nI'll be waiting here until you\ndeliver the Town Map!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_JustFollowRoadGoes_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "People always think the past\nor the future would be so wonderful.[f000]븁\u0000\nBut the great time we're\nspending with Pokémon[f000]븀\u0000\nis right now!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_PeopleAlwaysThinkPast, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I always save a record of my adventure,\nso I don't forget what I've done so far![f000]븀\u0000\nIt's a good idea for any Trainer!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_AlwaysSaveRecordAdventure, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You see, in Floccesy Ranch,\nwild Pokémon might surprise you![f000]븁\u0000\nAt times like that, only your\nown Pokémon can help you out!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_SeeFloccesyRanchWild, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In the morning, my Pokémon come\nto wake me up when they're hungry.[f000]븁\u0000\nAt night, my Pokémon get tired\nfrom playing and take up the whole bed![f000]븁\u0000\nOh! It fills me with so much joy!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_MorningPokemonComeWake, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Floccesy Town\nProphecy Flocks Here"
    MsgPlaceSign FloccesyTown_Text_FloccesyTownProphecyFlocks, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "It's an old clock tower..."
    MsgPlaceSign FloccesyTown_Text_ItsOldClockTower, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Are you happy you're\nable to train Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_HappyYoureAbleTrain, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0665
    // "Right?!\nMe too!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_RightToo, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0673

L_0665:
    // "Huh? But you're traveling\nwith Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_HuhButYoureTraveling, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0673:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Want to know what Alder taught me?"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_WantKnowWhatAlder, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06B6
    // "He said the Pokémon you throw out at\nthe start of battle is the one[f000]븀\u0000\nin the upper-left slot in your party![f000]븁\u0000\nThat's why I put a Pokémon I want\nto make stronger or a Pokémon that's[f000]븀\u0000\nalready tough in that spot!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_HeSaidPokemonThrow, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_06C4

L_06B6:
    // "You're being too bashful!\nIt's a good opportunity! Ask away!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown_Text_YoureBeingTooBashful, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_06C4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_06D4:
    Move 12, 1
    MoveEnd

Movement_06DC:
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

Movement_070C:
    Move 32, 1
    MoveEnd

Movement_0714:
    Move 33, 1
    MoveEnd

Movement_071C:
    Move 34, 1
    MoveEnd

Movement_0724:
    Move 35, 1
    MoveEnd

Movement_072C:
    Move 75, 1
    MoveEnd

Movement_0734:
    Move 159, 1
    MoveEnd
