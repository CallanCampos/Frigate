#include "asm/field_script.inc"
#include "text/script/mistralton_city.h"

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
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_21:
    VMCall L_0167
    VMHalt

Script_7:
    Cmd_02B2 13, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4049
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40cb
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x01f0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00C9
    FlagReset EVENT_FLAG_0x02fd
    ObjInitNPCGPos 3, 1, 78, 0, 269

L_00C9:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4049
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40cb
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x01f0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0116
    WorkSetConst EVENT_WORK_0x4154, 1
    FlagSet EVENT_FLAG_0x02fd

L_0116:
    VMCall L_0122
    FlagReset EVENT_FLAG_0x01f0
    VMHalt

L_0122:
    Cmd_02B2 13, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4049
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40cb
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0165
    WorkSetConst EVENT_WORK_0x4154, 0
    FlagSet EVENT_FLAG_0x02fd

L_0165:
    VMReturn

L_0167:
    Cmd_02B2 13, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4049
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40cb
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01C1
    VMStackPushFlag EVENT_FLAG_0x02fd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B7
    ActorDelete 3

L_01B7:
    WorkSetConst EVENT_WORK_0x4154, 0
    FlagSet EVENT_FLAG_0x02fd

L_01C1:
    VMReturn

Script_8:
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Mistralton City\nStrewn with Windblown Leaves"
    MsgPlaceSign MistraltonCity_Text_MistraltonCityStrewnWindblown, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Mistralton Cargo Service\nOur slogan is “Quick and Safe!\""
    MsgPlaceSign MistraltonCity_Text_MistraltonCargoServiceOur, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Mistralton City Pokémon Gym\nLeader: Skyla[f000]븀\u0000\nThe Highflying Girl"
    MsgPlaceSign MistraltonCity_Text_MistraltonCityPokemonGym, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    FlagReset EVENT_FLAG_0x026d
    ActorAdd 1
    ActorAdd 2
    ActorAdd 8
    ActorDelete 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 367
    WorkSet 0x8001, 1
    RTCallGlobal 2814
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0267
    FlagSet EVENT_FLAG_0x008b
    // "Oh! You found our treasure![f000]븁\u0000"
    ScreamMsg MistraltonCity_Text_OhFoundOurTreasure, 1
    MsgWinCloseAll
    ActorCmdExec 255, Movement_09B8
    ActorCmdWait
    ActorCmdExec 255, Movement_0988
    ActorCmdExec 1, Movement_0958
    ActorCmdExec 2, Movement_0960
    ActorCmdExec 8, Movement_0968
    ActorCmdWait
    // "Found it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_Found, 2, 6, 0
    MsgWinCloseAll
    VMSleep 32
    // "Just kidding![f000]븁\u0000\nOur Ducklett already knows Aerial Ace.\nSo we'll give you this TM![f000]븁\u0000"
    // "Just kidding![f000]븁\u0000\nOur Ducklett already knows Aerial Ace.\nSo we'll give you this TM![f000]븁\u0000"
    ActorMsgGendered 1024, MistraltonCity_Text_JustKiddingOurDucklett, MistraltonCity_Text_JustKiddingOurDucklett_2, 1, 1, 0
    MsgWinCloseAll
    // "Aerial Ace always hits its target!\nI hope it comes in handy![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_AerialAceAlwaysHits, 2, 6, 0
    MsgWinCloseAll
    PVPlay 580, 0
    // "Kwa!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_Kwa, 8, 1, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    VMSleep 8
    // "See you! Bye-bye![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SeeByeBye, 1, 5, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0970
    ActorCmdExec 2, Movement_0978
    ActorCmdExec 8, Movement_0980
    ActorCmdWait
    ActorDelete 1
    ActorDelete 2
    ActorDelete 8
    FlagSet EVENT_FLAG_0x026d
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40c2, 1
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 4, 0x8021, 297, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 4, Movement_091C
    ActorCmdWait
    BGMPlay SEQ_BGM_E_DOCTOR
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 1
    ActorWalkRoute 4, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait
    WordSetPlayerName 0
    // "Hi there, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_HiThere, 4, 0, 0
    // "It's nice to finally be able\nto talk to you in person![f000]븁\u0000\nI'm Professor Juniper![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_ItsNiceFinallyAble, 4, 0, 0
    // "You accepted the Pokédex and\ncame all the way out here with[f000]븀\u0000\nyour partners...[f000]븁\u0000\nHere, I'll evaluate your progress\nfor you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_AcceptedPokedexCameAll, 4, 0, 0
    WordSetPlayerName 0
    WorkSetConst 0x8023, 0
    PokeDexGetCount 0, 0x8023
    WordSetNumber 1, 0x8023, 3
    // "So, [f000]Ā\u0001\u0000, you've seen\n[f000]Ȃ\u0001\u0001 Pokémon up to this point![f000]븁\u0000\nI see! Thank you!\nThis is a token of my gratitude.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_YouveSeenPokemonUp, 4, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 1
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Juniper: This Master Ball is the most\npowerful kind of Poké Ball.[f000]븁\u0000\nIt can catch any Pokémon without fail.[f000]븁\u0000\nJourneys are about meeting Pokémon.\nDon't let a chance get away![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_JuniperMasterBallMost, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0990
    ActorCmdWait
    // "Still, I'm amazed how much Pokémon\ndistribution changes in two years.[f000]븁\u0000\nThat means my research will never end.\nStill, you could say that's what[f000]븀\u0000\nmakes it fun...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_StillImAmazedHow, 4, 0, 0
    MsgWinCloseAll
    VMSleep 30
    // "???: Professor Juniperrrrr![f000]븁\u0000"
    InfoMsg MistraltonCity_Text_ProfessorJuniperrrrr, 1
    MsgWinCloseAll
    ActorCmdExec 255, Movement_0928
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 100
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 301
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0471
    ActorWalkRoute 3, 101, 300, 1, 8, 0
    VMSleep 80
    ActorCmdExec 4, Movement_09A8
    ActorCmdExec 255, Movement_09A8
    ActorCmdWait
    ActorCmdExec 3, Movement_0990
    VMSleep 8
    ActorCmdExec 4, Movement_0988
    ActorCmdExec 255, Movement_0988
    ActorCmdWait
    VMJump L_04BF

L_0471:
    WorkSub 0x8022, 1
    WorkSub 0x8021, 1
    ActorWalkRoute 3, 0x8021, 0x8022, 1, 8, 0
    VMSleep 80
    ActorCmdExec 4, Movement_09A8
    ActorCmdExec 255, Movement_09A8
    ActorCmdWait
    ActorCmdExec 3, Movement_0988
    VMSleep 8
    ActorCmdExec 4, Movement_0990
    ActorCmdExec 255, Movement_0990
    ActorCmdWait

L_04BF:
    // "Professor Juniper: Why, if it isn't Skyla![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_ProfessorJuniperWhyIf, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_09B0
    VMSleep 8
    ActorCmdExec 255, Movement_09A8
    ActorCmdWait
    // "This is Skyla.\nShe's Mistralton City's Gym Leader.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SkylaShesMistraltonCitys, 4, 0, 0
    MsgWinCloseAll
    // "Skyla: Why are you surprised, Professor?[f000]븁\u0000\nYou did ask for a lift in my plane to\ncross Twist Mountain, since you can't[f000]븀\u0000\nreach Opelucid City by foot.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SkylaWhySurprisedProfessor, 3, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 100
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 301
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_053C
    ActorCmdExec 4, Movement_0988
    VMSleep 8
    ActorCmdExec 255, Movement_0988
    VMJump L_0550

L_053C:
    ActorCmdExec 4, Movement_0990
    VMSleep 8
    ActorCmdExec 255, Movement_0990

L_0550:
    ActorCmdWait
    WordSetPlayerName 0
    // "Professor Juniper: Aha ha! You're right.\nBut I have a quick favor to ask first.[f000]븁\u0000\nI want a look at Celestial Tower.\nDo you mind waiting till I'm through?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_ProfessorJuniperAhaHa, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_09B0
    VMSleep 8
    ActorCmdExec 255, Movement_09A8
    ActorCmdWait
    // "See you, [f000]Ā\u0001\u0000![f000]븁\u0000\nBe sure to always get along\nwith all kinds of Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SeeSureAlwaysGet, 4, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorWalkRoute 4, 109, 297, 1, 8, 1
    VMSleep 16
    ActorCmdExec 3, Movement_09A8
    ActorCmdWait
    ActorDelete 4
    BGMChangeMap
    // "Skyla: Honestly! I can't tell if she's\njust laid back or if she's not paying[f000]븀\u0000\nattention to anything outside her head.[f000]븁\u0000\nThe apple sure doesn't\nfall far from the tree.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SkylaHonestlyCantTell, 3, 0, 0
    MsgWinCloseAll
    VMSleep 15
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8022, 1
    ActorWalkRoute 3, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 3, Movement_09B0
    ActorCmdWait
    // "What's next?\nAre you coming to my Gym to challenge me?[f000]븁\u0000\nOr are you going to follow the professor\nto Celestial Tower and do some training?[f000]븁\u0000\nAs long as I get to battle with a strong\nTrainer, I'm fine either way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_WhatsNextComingGym, 3, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 3, 90, 297, 1, 8, 1
    ActorCmdWait
    ActorDelete 3
    FlagSet EVENT_FLAG_0x02fd
    FlagSet EVENT_FLAG_0x02bb
    HollowRivalCmd_0262 0, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    ActorWalkRoute 255, 78, 271, 1, 8, 1
    ActorCmdWait
    FlagReset EVENT_FLAG_0x02fd
    FlagReset EVENT_FLAG_0x02bb
    ActorAdd 3
    ActorAdd 4
    SEPlay SEQ_SE_KAIDAN
    ActorSetGPos 3, 78, 0, 268, 1
    SEWait
    ActorSetGPos 4, 84, 0, 279, 0
    VMSleep 15
    ActorWalkRoute 3, 78, 270, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_09A8
    ActorCmdWait
    // "Skyla: Time for a quick hop in my plane![f000]븁\u0000\nHey, where did Professor Juniper get to?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SkylaTimeQuickHop, 3, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_071F
    ActorCmdExec 4, Movement_0934
    ActorCmdWait
    ActorCmdExec 3, Movement_0988
    ActorCmdExec 255, Movement_0988
    ActorCmdWait
    WordSetPlayerName 0
    // "Professor Juniper: Hi there![f000]븁\u0000\nI hope we can get some good\nresearch done on the other side[f000]븀\u0000\nof the mountain as well.[f000]븁\u0000\nThat's right! I want you to\ntake this flight, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_ProfessorJuniperHiThere, 4, 0, 0
    // "There's someone I want you to\nmeet in Opelucid City,[f000]븀\u0000\nbut we can't get through[f000]븀\u0000\nTwist Mountain right now.[f000]븁\u0000\nWe'll just make a quick flight\nover to Lentimas Town![f000]븁\u0000\nI'll be waiting for you in\nMistralton Cargo Service![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_TheresSomeoneWantMeet, 4, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0940
    ActorCmdWait
    // "Skyla: Hey! Professor! Wait up![f000]븁\u0000\nHonestly... She just does\neverything at her own pace![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SkylaHeyProfessorWait, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 3, Movement_09B0
    ActorCmdExec 255, Movement_09A8
    ActorCmdWait
    // "OK! You come, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_OkComeToo, 3, 0, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x02ff
    FlagReset EVENT_FLAG_0x0300
    FlagSet EVENT_FLAG_0x02fe
    VMJump L_0731

L_071F:
    // "I wonder if she's still doing research\nin Celestial Tower?[f000]븁\u0000\nMmm... Could I ask you\nto go get the professor?[f000]븁\u0000\nI've got to finish flight preparations\nat Mistralton Cargo Service![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_WonderIfShesStill, 3, 0, 0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x02ff

L_0731:
    ActorCmdExec 3, Movement_094C
    VMSleep 16
    ActorCmdExec 255, Movement_0988
    ActorCmdWait
    ActorDelete 3
    ActorDelete 4
    FlagSet EVENT_FLAG_0x02fd
    FlagSet EVENT_FLAG_0x02bb
    FlagSet EVENT_FLAG_0x03ee
    WorkSetConst EVENT_WORK_0x40c1, 2
    WorkAdd EVENT_WORK_0x40c2, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, what's up?[f000]븁\u0000\nI know! Since you're here,\nI'll tell you a little secret![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_HeyWhatsUpKnow, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_UNDELLA_TOWN_5, 6, 0, 5, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x02fd
    ActorWalkRoute 255, 78, 271, 0, 8, 1
    ActorCmdWait
    ActorAdd 3
    SEPlay SEQ_SE_KAIDAN
    ActorSetGPos 3, 78, 0, 268, 1
    SEWait
    ActorCmdExec 255, Movement_09A8
    ActorCmdWait
    ActorWalkRoute 3, 78, 269, 4, 8, 1
    ActorCmdWait
    // "Hey, what's up?[f000]븁\u0000\nI know! Since you're here,\nI'll tell you a little secret![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_HeyWhatsUpKnow, 3, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_UNDELLA_TOWN_5, 6, 0, 5, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    // "Please keep this a secret from Elesa, OK?"
    ActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_PleaseKeepSecretFrom, 3, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 3, Movement_0914
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 3
    SEWait
    WorkSetConst EVENT_WORK_0x4049, 1
    WorkSetConst EVENT_WORK_0x4154, 2
    FlagSet EVENT_FLAG_0x02fd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We've arranged it so Mistralton's planes\nare now available for passenger service.[f000]븁\u0000\nIt's not like everyone's Pokémon\ncan use Fly!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_WeveArrangedMistraltonsPlanes, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Fly is an amazing move![f000]븁\u0000\nEven a teeny-weeny Pokémon\ncan carry a person easily!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_FlyAmazingMoveEven, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That Skyla...\nShe's even surpassed her grandpa,[f000]븀\u0000\nwho was a legendary pilot!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_SkylaShesEvenSurpassed, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Mistralton City used to be a\ndesolate patch of land...[f000]븁\u0000\nThis place was built through the\ncooperation of people and Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_MistraltonCityUsedDesolate, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Planes need a runway to fly,\nbut Pokémon don't need a thing![f000]븁\u0000\nBut planes can carry a lot more\ncargo than Pokémon can."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_PlanesNeedRunwayFly, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma went after certain\nPokémon, like Purrloin![f000]븁\u0000\nMany people had their Pokémon stolen.\nThat's just unforgivable!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_TeamPlasmaWentAfter, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you want to eat delicious vegetables,\nthe ones grown in the wild are the best.[f000]븁\u0000\nSometimes they are eaten by Pokémon...\nAh, I mean we can give them to Pokémon."
    // "Vegetables grown efficiently\nin a greenhouse are the best.[f000]븁\u0000\nTheir nutrients all go into forming\na very delicious vegetable!"
    ActorMsgVersioned 1024, MistraltonCity_Text_IfWantEatDelicious, MistraltonCity_Text_VegetablesGrownEfficientlyGreenhouse, 15, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0914:
    Move 12, 1
    MoveEnd

Movement_091C:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_0928:
    Move 75, 1
    Move 34, 1
    MoveEnd

Movement_0934:
    Move 12, 9
    Move 14, 4
    MoveEnd

Movement_0940:
    Move 15, 4
    Move 13, 9
    MoveEnd

Movement_094C:
    Move 15, 5
    Move 13, 8
    MoveEnd

Movement_0958:
    Move 18, 7
    MoveEnd

Movement_0960:
    Move 14, 7
    MoveEnd

Movement_0968:
    Move 14, 7
    MoveEnd

Movement_0970:
    Move 15, 7
    MoveEnd

Movement_0978:
    Move 15, 7
    MoveEnd

Movement_0980:
    Move 79, 7
    MoveEnd

Movement_0988:
    Move 35, 1
    MoveEnd

Movement_0990:
    Move 34, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 30, 1
    MoveEnd

Movement_09A8:
    Move 32, 1
    MoveEnd

Movement_09B0:
    Move 33, 1
    MoveEnd

Movement_09B8:
    Move 75, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Script_18:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0991
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A36
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what?\nMy Pokémon loves Berries![f000]븁\u0000\nThat's why I'm wandering all over,\nlooking for Berries![f000]븁\u0000\nI dream about a wonderful city somewhere\nthat is overflowing with Berries...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_KnowWhatPokemonLoves, 0, 0
    MsgWinCloseAll
    Cmd_0275 0, 20, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg MistraltonCity_Text_FunfestMissionHasBeen, 0
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    // "My Pokémon's favorite\nBerry is the Leppa Berry![f000]븁\u0000\nIt restores PP!\nIsn't it a useful Berry?"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_PokemonsFavoriteBerryLeppa, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0991
    VMJump L_0A4A

L_0A36:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon's favorite\nBerry is the Leppa Berry![f000]븁\u0000\nIt restores PP!\nIsn't it a useful Berry?"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_PokemonsFavoriteBerryLeppa, 0, 0
    LastKeyWait
    ActorMsgClose

L_0A4A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    WordSetLoadJoinAvenueName 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you know the big street\ncalled [f000]Ĺ\u0001\u0000?[f000]븁\u0000\nThere are a lot of unique shops there!\nIt's so cool!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCity_Text_KnowBigStreetCalled, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
