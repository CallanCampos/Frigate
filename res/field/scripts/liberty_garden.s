#include "asm/field_script.inc"
#include "text/script/liberty_garden.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_12:
    VMStackPush EVENT_WORK_0x4138
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012F
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_012F
    PokePartyGetCount 0x8020, 0

L_007A:
    VMStackPush 0x8020
    VMStackPush 0x8022
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_00BC
    PokePartyGetSpecies 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 494
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AC
    WorkSetConst EVENT_WORK_0x400a, 1

L_00AC:
    DebugPrint 0x8021
    WorkAdd 0x8022, 1
    VMJump L_007A

L_00BC:
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DB
    WorkSetConst EVENT_WORK_0x4138, 1
    VMJump L_012F

L_00DB:
    PokeDexIsRegist 1, SPECIES_VICTINI, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0108
    WorkSetConst EVENT_WORK_0x4138, 1
    WorkSetConst EVENT_WORK_0x400a, 2
    VMJump L_012F

L_0108:
    PokeDexIsRegist 0, SPECIES_VICTINI, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012F
    WorkSetConst EVENT_WORK_0x4138, 1
    WorkSetConst EVENT_WORK_0x400a, 3

L_012F:
    VMHalt

Script_13:
    ActorsPauseAll
    CallPlaceNameDisp
    VMSleep 70
    ActorNew 308, 747, 2, 251, 105, 0
    ActorWalkRoute 251, 299, 747, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 251, Movement_052C
    ActorCmdWait
    BGMPlay SEQ_BGM_E_DOCTOR
    ActorCmdExec 251, Movement_0244
    ActorCmdWait
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0182
    VMJump L_0194

L_0182:
    // "Professor Juniper: Oh?\nVictini![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_ProfessorJuniperOhVictini, 251, 0, 0
    VMJump L_01DE

L_0194:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_01A7
    VMJump L_01B9

L_01A7:
    // "Professor Juniper: Oh?\nYour Pokédex...[f000]븁\u0000\nYou've caught Victini![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_ProfessorJuniperOhPokedex, 251, 0, 0
    VMJump L_01DE

L_01B9:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_01CC
    VMJump L_01DE

L_01CC:
    // "Professor Juniper: Oh?\nYour Pokédex...[f000]븁\u0000\nYou've seen Victini![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_ProfessorJuniperOhPokedex_2, 251, 0, 0
    VMJump L_01DE

L_01DE:
    MsgWinCloseAll
    ActorCmdExec 251, Movement_04B4
    ActorCmdWait
    VMSleep 30
    // "In the Unova Pokédex, Victini was\nassigned a special number: zero.[f000]븁\u0000\nI've heard the special number was\nassigned in the hope that Victini's power[f000]븀\u0000\nto bring victory would be shared with the[f000]븀\u0000\nTrainer who travels with this Pokédex...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_UnovaPokedexVictiniAssigned, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_04BC
    ActorCmdWait
    // "How do you relate to Pokémon?[f000]븁\u0000\nAs you fill your Pokédex,\nyou'll find your own answer.[f000]븁\u0000\nThat's what I'm hoping for.\nSee you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_HowRelatePokemonFill, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 298, 762, 0, 8, 1
    VMSleep 20
    ActorCmdExec 255, Movement_04EC
    ActorCmdWait
    BGMChangeMap
    ActorDelete 251
    WorkSetConst EVENT_WORK_0x4138, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0244:
    Move 13, 1
    Move 14, 2
    MoveEnd

Script_11:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    VMStackPush EVENT_WORK_0x4138
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_026B
    CallPlaceNameDisp

L_026B:
    ActorCmdExec 255, Movement_04AC
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you want to go back to Castelia City?"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_WantGoBackCastelia, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E6
    // "We are just about to leave, so please\nget on board and wait a moment.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_WeJustAboutLeave, 0, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_04BC
    VMSleep 8
    ActorCmdExec 255, Movement_04BC
    ActorCmdWait
    RTReserveScript 4
    FadeOutBlackQ
    FadeWait
    SEPlay SEQ_SE_VDEMO_02
    MapChangeRail ZONE_CASTELIA_CITY_8, 0, 2, 11, 3
    SEWait
    VMJump L_02F4

L_02E6:
    // "Just come talk to me when you want\nto return to Castelia City!"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_JustComeTalkWhen, 0, 0
    LastKeyWait
    ActorMsgClose

L_02F4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Somehow, coming here gives me power!"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_SomehowComingHereGives, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8024, 0
    TrainerCardGetSex 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Have you heard about this?[f000]븁\u0000\nA rich person was protecting\na Pokémon from bad people here!"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_HaveHeardAboutRich, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0363

L_034F:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Have you heard about this?[f000]븁\u0000\nA rich person was protecting\na Pokémon from bad people here!"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_HaveHeardAboutRich_2, 0, 0
    LastKeyWait
    ActorMsgClose

L_0363:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder how the Pokémon felt\nwhile it was in that room...[f000]븀\u0000\nIt must have been lonely for a long time."
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_WonderHowPokemonFelt, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    PokePartyGetCount 0x8020, 0

L_038D:
    VMStackPush 0x8020
    VMStackPush 0x8022
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_03CF
    PokePartyGetSpecies 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 494
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03BF
    WorkSetConst EVENT_WORK_0x400f, 1

L_03BF:
    DebugPrint 0x8021
    WorkAdd 0x8022, 1
    VMJump L_038D

L_03CF:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Th...that... Your Pokémon...\nIs that the Victory Pokémon, Victini?![f000]븁\u0000\nIs it true?\nHave you just kept winning and winning[f000]븀\u0000\nwith Victini's help?[f000]븁\u0000\nI wish I had your luck..."
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_ThPokemonVictoryPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0410

L_03FC:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Victory Pokémon, Victini...[f000]븁\u0000\nThey say it can give its Trainer\nincredible power.[f000]븁\u0000\nI wonder who has access\nto that power now..."
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_VictoryPokemonVictiniThey, 0, 0
    LastKeyWait
    ActorMsgClose

L_0410:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pokémon follow their Trainers'\norders without question.[f000]븁\u0000\nAnd yet, some people try\nto make Pokémon do bad things!"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_PokemonFollowTheirTrainers, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Nowadays, there aren't as many tourists\nvisiting here. It's boring...[f000]븁\u0000\nBut if the alternative is guys like\nTeam Plasma, I'm OK with being bored!"
    ParentActorMsg MSGFILE_SCRIPT, LibertyGarden_Text_NowadaysThereArentMany, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Two hundred years ago, an ultra-rich\nfamily bought this island.\nThey named it Liberty Garden.\n\nIt's a place where people and Pokémon\ncan live freely."
    MsgPlaceSign LibertyGarden_Text_TwoHundredYearsAgo, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Dock 2\nTo board the tour boat, go to Dock 1."
    MsgPlaceSign LibertyGarden_Text_Dock2BoardTour, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "This lighthouse shines with the light\nof freedom.[f000]븁\u0000\nOnly authorized personnel may enter."
    MsgPlaceSign LibertyGarden_Text_LighthouseShinesLightFreedom, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_04AC:
    Move 15, 1
    MoveEnd

Movement_04B4:
    Move 11, 1
    MoveEnd

Movement_04BC:
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
    Move 32, 1
    MoveEnd

Movement_04EC:
    Move 33, 1
    MoveEnd
    FieldGetContinueFlag 1
    PokePartyGetSpecies 0, 35
    VMNop2
    PokePartyGetSpecies 0, 48
    VMNop2
    PokePartyGetSpecies 0, 49
    VMNop2
    PokePartyGetSpecies 0, 50
    VMNop2
    PokePartyGetSpecies 0, 51
    VMNop2
    PokePartyGetSpecies 0, 49
    VMHalt
    .byte 0xfe
    .balign 4, 0

Movement_052C:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 71, 1
    Move 170, 1
    Move 72, 1
    MoveEnd
