#include "asm/field_script.inc"
#include "text/script/route_6_2.h"

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
    ScriptEntriesEnd

Script_5:
    WorkSetConst 0x8020, 0
    RTCGetSeason 0x8020
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0069
    Random EVENT_WORK_0x4003, 9
    WorkAdd EVENT_WORK_0x4003, 11
    VMJump L_00BF

L_0069:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008E
    Random EVENT_WORK_0x4003, 16
    WorkAdd EVENT_WORK_0x4003, 19
    VMJump L_00BF

L_008E:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B3
    Random EVENT_WORK_0x4003, 9
    WorkAdd EVENT_WORK_0x4003, 11
    VMJump L_00BF

L_00B3:
    Random EVENT_WORK_0x4003, 9
    WorkAdd EVENT_WORK_0x4003, 1

L_00BF:
    WorkSetConst 0x8020, 0
    VMHalt

Script_3:
    ActorsPauseAll
    ActorWalkRoute 255, 6, 7, 1, 8, 0
    ActorCmdWait
    // "Cheren: No one really talks about it, but\nthe record shows that there was a[f000]븀\u0000\nsudden drop in temperature near[f000]븀\u0000\nLacunosa Town.[f000]븁\u0000\nIt was only for a moment, but it went\ndown as low as -58° F.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_CherenNoOneReally, 0, 0, 0
    // "Not only that, but similar temperatures\nwere recorded around Castelia City[f000]븀\u0000\nand Driftveil City.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_NotOnlyButSimilar, 0, 0, 0
    // "On top of that, that incident with\nTeam Plasma![f000]븁\u0000\nI think this needs some investigation\nbefore things go bad.[f000]븁\u0000\nI hate cold weather, though.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_TopIncidentTeamPlasma, 0, 0, 0
    // "I'll give you this...[f000]븁\u0000\nIt's a Hidden Machine that contains Surf.\nWith this, please go to different places[f000]븀\u0000\nand check what's going on.[f000]븁\u0000\nLet one of your Pokémon learn Surf,\nand you can travel across the water.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_IllGiveItsHidden, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 6, 6, 1, 8, 0
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 422
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Sorry to have slowed you down.[f000]븁\u0000\nSome strange things may be happening,\nso be careful on your journey!"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_SorryHaveSlowedDown, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0987
    HollowRivalCmd_0262 2, 6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sorry to have slowed you down.[f000]븁\u0000\nSome strange things may be happening,\nso be careful on your journey!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_SorryHaveSlowedDown, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8021, 0
    RTCGetSeason 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A5
    // "The glass is slightly cold to the touch.[f000]븁\u0000\nThe plants inside the case\nhave grown some pretty Berries."
    InfoMsg Route62_Text_GlassSlightlyColdTouch, 2
    VMJump L_01E6

L_01A5:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C3
    // "The glass is very cold to the touch.[f000]븁\u0000\nThe plants inside the case\nhave lost all their leaves."
    InfoMsg Route62_Text_GlassVeryColdTouch, 2
    VMJump L_01E6

L_01C3:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E1
    // "The glass is slightly warm to the touch.[f000]븁\u0000\nThe plants inside the case\nhave grown small buds."
    InfoMsg Route62_Text_GlassSlightlyWarmTouch, 2
    VMJump L_01E6

L_01E1:
    // "The glass is very warm to the touch.[f000]븁\u0000\nThe plants inside the case\nhave big, healthy leaves."
    InfoMsg Route62_Text_GlassVeryWarmTouch, 2

L_01E6:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8021, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    RTCGetSeason 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0231
    WordSetNumber 0, EVENT_WORK_0x4003, 2
    // "Route 6's temperature [f000]ȁ\u0001\u0000 °C"
    InfoMsg Route62_Text_Route6sTemperatureC, 2
    VMJump L_0287

L_0231:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0256
    WordSetNumber 0, EVENT_WORK_0x4003, 2
    // "Route 6's temperature [f000]ȁ\u0001\u0000 °C"
    InfoMsg Route62_Text_Route6sTemperatureC_2, 2
    VMJump L_0287

L_0256:
    VMStackPush 0x8022
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027B
    WordSetNumber 0, EVENT_WORK_0x4003, 2
    // "Route 6's temperature [f000]ȁ\u0001\u0000 °C"
    InfoMsg Route62_Text_Route6sTemperatureC_3, 2
    VMJump L_0287

L_027B:
    WordSetNumber 0, EVENT_WORK_0x4003, 2
    // "Route 6's temperature -[f000]Ȁ\u0001\u0000 °C"
    InfoMsg Route62_Text_Route6sTemperatureC_4, 2

L_0287:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is the Season Research Lab.[f000]븁\u0000\nTwist Mountain, just beyond Mistralton,\nhas snow, depending on the season."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_SeasonResearchLabTwist, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Some Pokémon change their appearance\ndepending on the season.[f000]븁\u0000\nCould that also be considered\na type of evolution?"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_SomePokemonChangeTheir, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01aa
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0335
    // "You can see Deerling from\nall four seasons in our lab![f000]븁\u0000\nWhen it comes to Deerling,\nwe're sort of the experts.[f000]븁\u0000\nWe found a rather rare Deerling.\nWill you raise it for us? What do you say?"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_CanSeeDeerlingFrom, 8, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_031F
    VMCall L_034B
    VMJump L_032F

L_031F:
    // "Are you sure? It's quite a rare\nDeerling, you know!"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_SureItsQuiteRare, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_032F:
    VMJump L_0345

L_0335:
    // "Its Ability is Serene Grace.[f000]븁\u0000\nThey say this Ability doubles the chances\nof getting a move's additional effect."
    ActorMsg MSGFILE_SCRIPT, Route62_Text_ItsAbilitySereneGrace, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0345:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_034B:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    PokePartyGetCount 0x8024, 0
    VMStackPush 0x8024
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_038C
    // "You're traveling with lots of\nPokémon already!"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_YoureTravelingLotsPokemon, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04A0

L_038C:
    // "Here you are!\nTake good care of it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route62_Text_HereTakeGoodCare, 8, 0, 0
    MsgWinCloseAll
    RTCGetSeason 0x8025
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CB
    PokePartyAddEx 0x8010, 585, 0, 30, 3, 2, 0, 0, 4
    VMJump L_044C

L_03CB:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F8
    PokePartyAddEx 0x8010, 585, 1, 30, 3, 2, 0, 0, 4
    VMJump L_044C

L_03F8:
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0425
    PokePartyAddEx 0x8010, 585, 2, 30, 3, 2, 0, 0, 4
    VMJump L_044C

L_0425:
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044C
    PokePartyAddEx 0x8010, 585, 3, 30, 3, 2, 0, 0, 4

L_044C:
    WordSetPlayerName 0
    MEPlay SEQ_ME_POKEGET
    // "[f000]Ā\u0001\u0000 received Deerling!"
    SystemMsg Route62_Text_ReceivedDeerling, 0
    MEWait
    MsgWaitAdvance
    // "Give a nickname\nto the Deerling you received?"
    SystemMsg Route62_Text_GiveNicknameDeerlingReceived, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_048A
    MsgWinCloseAll
    CallPokeNameInput 0x8026, 0x8024, 1
    VMJump L_048C

L_048A:
    MsgWinCloseAll

L_048C:
    // "Its Ability is Serene Grace.[f000]븁\u0000\nThey say this Ability doubles the chances\nof getting a move's additional effect."
    ActorMsg MSGFILE_SCRIPT, Route62_Text_ItsAbilitySereneGrace, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01aa

L_04A0:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    VMReturn

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The temperature and humidity\ninside the case next to me[f000]븀\u0000\nare controlled by a machine."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_TemperatureHumidityInsideCase, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 585, 0
    // "Dreee!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_Dreee, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 585, 0
    // "Dreee! Dree dree!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_DreeeDreeDree, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 585, 0
    // "Dreen! Droooon!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_DreenDroooon, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 585, 0
    // "Droon..."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_Droon, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8039, 0
    WorkSetConst 0x8035, 1
    WorkSetConst 0x8036, 2
    WorkSetConst 0x8037, 3
    WorkSetConst 0x8038, 4
    WorkSetConst 0x8039, 5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardGetBirthDate 0x8027, 0x8028
    RTCGetDate 0x8029, 0x802a
    VMStackPush 0x8029
    VMStackPush 0x8027
    VMStackCmp CMP_EQ
    VMStackPush 0x802a
    VMStackPush 0x8028
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0631
    // "Is today a special day for some reason?[f000]븁\u0000\nIt feels like my mind has cleared up...\nas if a fog has lifted![f000]븁\u0000\nMaybe the weather somewhere\nis cleared up just like my mind...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_TodaySpecialDaySome, 0, 0

L_0631:
    VMStackPush 0x8029
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x802a
    VMStackPushConst 31
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_065E
    // "Here comes a question out of a clear sky!\nHave you heard of diamond dust?[f000]븁\u0000\nIt's a breathtaking phenomenon that is\ncreated by shiny icy particles coming[f000]븀\u0000\ndown from the sky![f000]븁\u0000\nI heard it can be seen in a certain city\naround this time of year.[f000]븁\u0000\nAhh... I would love to watch the\ndiamond dust flutter down with[f000]븀\u0000\nsomeone special! Teehee![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_HereComesQuestionOut, 0, 0

L_065E:
    PokePartyGetCount 0x802b, 0

L_0664:
    VMStackPush 0x802b
    VMStackPush 0x802c
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0B0B
    PokePartyGetParam 0x8030, 0x802c, 10
    PokePartyIsEgg 0x802d, 0x802c
    VMStackPush 0x8030
    VMStackPushConst 70
    VMStackCmp CMP_EQ
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06D4
    WorkGet 0x8034, 0x8035
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetLoadAbility 2, 0x8030
    WorkSetConst 0x8032, 1
    VMJump L_0859

L_06D4:
    VMStackPush 0x8030
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0723
    WorkGet 0x8034, 0x8036
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetLoadAbility 2, 0x8030
    WorkSetConst 0x8032, 1
    VMJump L_0859

L_0723:
    VMStackPush 0x8030
    VMStackPushConst 117
    VMStackCmp CMP_EQ
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0772
    WorkGet 0x8034, 0x8037
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetLoadAbility 2, 0x8030
    WorkSetConst 0x8032, 1
    VMJump L_0859

L_0772:
    VMStackPush 0x8030
    VMStackPushConst 45
    VMStackCmp CMP_EQ
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07C1
    WorkGet 0x8034, 0x8038
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetLoadAbility 2, 0x8030
    WorkSetConst 0x8032, 1
    VMJump L_0859

L_07C1:
    VMStackPush 0x8030
    VMStackPushConst 76
    VMStackCmp CMP_EQ
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0810
    WorkGet 0x8034, 0x8039
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetLoadAbility 2, 0x8030
    WorkSetConst 0x8032, 1
    VMJump L_0859

L_0810:
    VMStackPush 0x8030
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0859
    WorkGet 0x8034, 0x8039
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetLoadAbility 2, 0x8030
    WorkSetConst 0x8032, 1

L_0859:
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0907
    VMStackPush 0x8034
    VMStackPush 0x8035
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_088B
    VMCall L_0B42
    VMJump L_0901

L_088B:
    VMStackPush 0x8034
    VMStackPush 0x8036
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08AA
    VMCall L_0BF6
    VMJump L_0901

L_08AA:
    VMStackPush 0x8034
    VMStackPush 0x8037
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08C9
    VMCall L_0CAA
    VMJump L_0901

L_08C9:
    VMStackPush 0x8034
    VMStackPush 0x8038
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08E8
    VMCall L_0D5E
    VMJump L_0901

L_08E8:
    VMStackPush 0x8034
    VMStackPush 0x8039
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0901
    VMCall L_0E06

L_0901:
    VMJump L_0AFF

L_0907:
    WorkSetConst 0x802f, 0
    PokePartyGetMoveCount 0x802e, 0x802c

L_0913:
    VMStackPush 0x802e
    VMStackPush 0x802f
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0AFF
    PokePartyGetMove 0x8031, 0x802c, 0x802f
    PokePartyIsEgg 0x802d, 0x802c
    VMStackPush 0x8031
    VMStackPushConst 241
    VMStackCmp CMP_EQ
    VMStackPush 0x8033
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0983
    WorkGet 0x8034, 0x8035
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetMoveName 1, MOVE_SUNNY_DAY
    WorkSetConst 0x8033, 1
    VMJump L_0A6A

L_0983:
    VMStackPush 0x8031
    VMStackPushConst 240
    VMStackCmp CMP_EQ
    VMStackPush 0x8033
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_09D2
    WorkGet 0x8034, 0x8036
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetMoveName 1, MOVE_RAIN_DANCE
    WorkSetConst 0x8033, 1
    VMJump L_0A6A

L_09D2:
    VMStackPush 0x8031
    VMStackPushConst 258
    VMStackCmp CMP_EQ
    VMStackPush 0x8033
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0A21
    WorkGet 0x8034, 0x8037
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetMoveName 1, MOVE_HAIL
    WorkSetConst 0x8033, 1
    VMJump L_0A6A

L_0A21:
    VMStackPush 0x8031
    VMStackPushConst 201
    VMStackCmp CMP_EQ
    VMStackPush 0x8033
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0A6A
    WorkGet 0x8034, 0x8038
    WordSetPartyPokeSpecies 0, 0x802c
    WordSetMoveName 1, MOVE_SANDSTORM
    WorkSetConst 0x8033, 1

L_0A6A:
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AF3
    VMStackPush 0x8034
    VMStackPush 0x8035
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A9C
    VMCall L_0B42
    VMJump L_0AF3

L_0A9C:
    VMStackPush 0x8034
    VMStackPush 0x8036
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ABB
    VMCall L_0BF6
    VMJump L_0AF3

L_0ABB:
    VMStackPush 0x8034
    VMStackPush 0x8037
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ADA
    VMCall L_0CAA
    VMJump L_0AF3

L_0ADA:
    VMStackPush 0x8034
    VMStackPush 0x8038
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AF3
    VMCall L_0D5E

L_0AF3:
    WorkAdd 0x802f, 1
    VMJump L_0913

L_0AFF:
    WorkAdd 0x802c, 1
    VMJump L_0664

L_0B0B:
    VMStackPush 0x8032
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8033
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0B3C
    // "Oh, my! None of the Pokémon traveling\nwith you have moves or Abilities that[f000]븀\u0000\nchange the weather.[f000]븁\u0000\nIf you find one that does, bring it to me.\nI'll give you a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhNonePokemonTraveling, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0B3C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0B42:
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B67
    // "Oh, my! Your [f000]ā\u0001\u0000 knows the\nmove [f000]ć\u0001\u0001![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhKnowsMoveTrainer, 0, 0
    MsgWaitAdvance
    VMJump L_0B86

L_0B67:
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B86
    // "Oh, my!\nThis is something I don't see often![f000]븁\u0000\nYour [f000]ā\u0001\u0000's Ability\nis [f000]Ć\u0001\u0002![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhSomethingDontSee, 0, 0
    MsgWaitAdvance

L_0B86:
    // "Ahem![f000]븁\u0000\nUsing the move Sunny Day or sending out\na Pokémon with the Ability Drought[f000]븀\u0000\nwill cause the sun to shine brightly."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_AhemUsingMoveSunny, 0, 0
    MsgWaitAdvance
    // "When it's sunny, the power of Fire-type\nmoves will be boosted by 50 percent![f000]븁\u0000\nThe power of Water-type moves\nwill be reduced by 50 percent![f000]븁\u0000\nWeather Ball will become a Fire-type\nmove, and its power will be boosted by[f000]븀\u0000\n100 percent![f000]븁\u0000\nThe stat increase of the move Growth\nis boosted by 200 percent!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WhenItsSunnyPower, 0, 0
    MsgWaitAdvance
    // "When it's sunny, it's been reported that\nPokémon can't be frozen, and frozen[f000]븀\u0000\nPokémon are defrosted![f000]븁\u0000\nBy the way, the Pokémon called Castform\nis known to change its form when it's[f000]븀\u0000\nsunny and become a Fire type."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WhenItsSunnyIts, 0, 0
    MsgWaitAdvance
    // "The clear sky increases the amount of\nHP recovered from the moves[f000]븀\u0000\nMorning Sun, Synthesis, and Moonlight."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_ClearSkyIncreasesAmount, 0, 0
    MsgWaitAdvance
    // "This is a weather advisory![f000]븁\u0000\nWhen it's sunny, the move SolarBeam can\nbe used every turn. It doesn't have[f000]븀\u0000\nto charge up first![f000]븁\u0000\nPlease be aware that the moves\nThunder and Hurricane will be[f000]븀\u0000\nmore likely to miss!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WeatherAdvisoryWhenIts, 0, 0
    MsgWaitAdvance
    // "By the way, there are some Abilities that\ntake effect when it's sunny, such as[f000]븀\u0000\nChlorophyll, Harvest, Solar Power,[f000]븀\u0000\nand Forecast.[f000]븁\u0000\nPlease check your Pokémon's status!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_ByWayThereSome_2, 0, 0
    MsgWaitAdvance
    // "The lucky item on a sunny day is\nthe Heat Rock![f000]븁\u0000\nBy the way, the sunshine may feel nice,\nbut remember to take care of your skin[f000]븀\u0000\nand try not to get a sunburn."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_LuckyItemSunnyDay, 0, 0
    MsgWaitAdvance
    // "How did you enjoy my Pokémon\nweather forecast?[f000]븁\u0000\nPlease bring me a Pokémon with other\nmoves or Abilities if you want to hear[f000]븀\u0000\na different forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_HowDidEnjoyPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x802c, 6
    WorkSetConst 0x802f, 4
    VMReturn

L_0BF6:
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C1B
    // "Oh, my! Your [f000]ā\u0001\u0000 knows the\nmove [f000]ć\u0001\u0001![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhKnowsMoveTrainer, 0, 0
    MsgWaitAdvance
    VMJump L_0C3A

L_0C1B:
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C3A
    // "Oh, my!\nThis is something I don't see often![f000]븁\u0000\nYour [f000]ā\u0001\u0000's Ability\nis [f000]Ć\u0001\u0002![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhSomethingDontSee, 0, 0
    MsgWaitAdvance

L_0C3A:
    // "Ahem![f000]븁\u0000\nUsing the move Rain Dance or sending out\na Pokémon with the Ability Drizzle[f000]븀\u0000\nwill cause it to rain."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_AhemUsingMoveRain, 0, 0
    MsgWaitAdvance
    // "When it's raining, the power of\nWater-type moves will be boosted by[f000]븀\u0000\n50 percent![f000]븁\u0000\nThe powers of Fire-type moves and\nSolarBeam will be reduced by 50 percent![f000]븁\u0000\nWeather Ball will become a Water-type\nmove, and its power will be boosted by[f000]븀\u0000\n100 percent!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WhenItsRainingPower, 0, 0
    MsgWaitAdvance
    // "By the way, the Pokémon called Castform\nis known to change its form in rain and[f000]븀\u0000\nbecome a Water type."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_ByWayPokemonCalled, 0, 0
    MsgWaitAdvance
    // "The darkened sky reduces the amount\nof HP recovered from the moves[f000]븀\u0000\nMorning Sun, Synthesis, and Moonlight."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_DarkenedSkyReducesAmount, 0, 0
    MsgWaitAdvance
    // "This is a weather advisory![f000]븁\u0000\nPlease be aware that the moves Thunder\nand Hurricane will not miss in the rain!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WeatherAdvisoryPleaseAware, 0, 0
    MsgWaitAdvance
    // "By the way, there are some Abilities that\ntake effect when it's raining, such as[f000]븀\u0000\nSwift Swim, Rain Dish,[f000]븀\u0000\nDry Skin, and Forecast.[f000]븁\u0000\nPlease check your Pokémon's status!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_ByWayThereSome, 0, 0
    MsgWaitAdvance
    // "The lucky item on a rainy day is\nthe Damp Rock![f000]븁\u0000\nParasol Ladies like me are in our element\nin rain, like a Basculin in water!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_LuckyItemRainyDay, 0, 0
    MsgWaitAdvance
    // "How did you enjoy my Pokémon\nweather forecast?[f000]븁\u0000\nPlease bring me a Pokémon with other\nmoves or Abilities if you want to hear[f000]븀\u0000\na different forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_HowDidEnjoyPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x802c, 6
    WorkSetConst 0x802f, 4
    VMReturn

L_0CAA:
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CCF
    // "Oh, my! Your [f000]ā\u0001\u0000 knows the\nmove [f000]ć\u0001\u0001![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhKnowsMoveTrainer, 0, 0
    MsgWaitAdvance
    VMJump L_0CEE

L_0CCF:
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CEE
    // "Oh, my!\nThis is something I don't see often![f000]븁\u0000\nYour [f000]ā\u0001\u0000's Ability\nis [f000]Ć\u0001\u0002![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhSomethingDontSee, 0, 0
    MsgWaitAdvance

L_0CEE:
    // "Ahem![f000]븁\u0000\nUsing the move Hail or sending out\na Pokémon with the Ability Snow Warning[f000]븀\u0000\nwill cause it to hail."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_AhemUsingMoveHail, 0, 0
    MsgWaitAdvance
    // "When it's hailing, Pokémon take\ndamage at the end of every turn[f000]븀\u0000\nunless they are Ice types![f000]븁\u0000\nThe power of SolarBeam will be\nreduced by 50 percent![f000]븁\u0000\nWeather Ball will become an Ice-type\nmove, and the power will be boosted by[f000]븀\u0000\n100 percent!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WhenItsHailingPokemon, 0, 0
    MsgWaitAdvance
    // "By the way, the Pokémon called Castform\nis known to change its form when it's[f000]븀\u0000\nhailing and become an Ice type."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_ByWayPokemonCalled_2, 0, 0
    MsgWaitAdvance
    // "The darkened sky reduces the amount\nof HP recovered from the moves[f000]븀\u0000\nMorning Sun, Synthesis, and Moonlight."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_DarkenedSkyReducesAmount_2, 0, 0
    MsgWaitAdvance
    // "This is a weather advisory![f000]븁\u0000\nPlease be aware that Blizzard\nwill not miss when it's hailing!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WeatherAdvisoryPleaseAware_2, 0, 0
    MsgWaitAdvance
    // "By the way, there are some Abilities that\ntake effect when it's hailing, such as[f000]븀\u0000\nIce Body, Snow Cloak, and Forecast.[f000]븁\u0000\nPlease check your Pokémon's status!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_ByWayThereSome_3, 0, 0
    MsgWaitAdvance
    // "The lucky item in a hailstorm is\nthe Icy Rock![f000]븁\u0000\nBy the way, it gets quiet when it snows\nbecause the icy particles absorb sound!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_LuckyItemHailstormIcy, 0, 0
    MsgWaitAdvance
    // "How did you enjoy my Pokémon\nweather forecast?[f000]븁\u0000\nPlease bring me a Pokémon with other\nmoves or Abilities if you want to hear[f000]븀\u0000\na different forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_HowDidEnjoyPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x802c, 6
    WorkSetConst 0x802f, 4
    VMReturn

L_0D5E:
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D83
    // "Oh, my! Your [f000]ā\u0001\u0000 knows the\nmove [f000]ć\u0001\u0001![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhKnowsMoveTrainer, 0, 0
    MsgWaitAdvance
    VMJump L_0DA2

L_0D83:
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DA2
    // "Oh, my!\nThis is something I don't see often![f000]븁\u0000\nYour [f000]ā\u0001\u0000's Ability\nis [f000]Ć\u0001\u0002![f000]븁\u0000\nFor a Trainer like you,\nhere comes a Pokémon weather forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_OhSomethingDontSee, 0, 0
    MsgWaitAdvance

L_0DA2:
    // "Ahem![f000]븁\u0000\nUsing the move Sandstorm or sending out\na Pokémon with the Ability Sand Stream[f000]븀\u0000\nwill cause a sandstorm."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_AhemUsingMoveSandstorm, 0, 0
    MsgWaitAdvance
    // "When in a sandstorm, Pokémon take\ndamage at the end of every turn unless[f000]븀\u0000\nthey are Rock types, Ground types, or[f000]븀\u0000\nSteel types![f000]븁\u0000\nThe power of the move SolarBeam will be\nreduced by 50 percent![f000]븁\u0000\nWeather Ball will become a Rock-type\nmove, and its power will be boosted[f000]븀\u0000\nby 100 percent!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WhenSandstormPokemonTake, 0, 0
    MsgWaitAdvance
    // "When in a sandstorm, it's been reported\nthat Rock-type Pokémon boost their[f000]븀\u0000\nSp. Defense by 50 percent![f000]븁\u0000\nBy the way, the Pokémon called Castform\nis known to change its form and type[f000]븀\u0000\ndepending on the weather, but[f000]븀\u0000\nsandstorms don't affect it in that way."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_WhenSandstormItsBeen, 0, 0
    MsgWaitAdvance
    // "The darkened sky reduces the amount of\nHP recovered from the moves[f000]븀\u0000\nMorning Sun, Synthesis, and Moonlight."
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_DarkenedSkyReducesAmount_3, 0, 0
    MsgWaitAdvance
    // "By the way, there are some Abilities that\ntake effect in a sandstorm, such as[f000]븀\u0000\nSand Rush, Sand Veil, and Sand Force.[f000]븁\u0000\nPlease check your Pokémon's status!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_ByWayThereSome_4, 0, 0
    MsgWaitAdvance
    // "The lucky item in a sandstorm is\nthe Smooth Rock![f000]븁\u0000\nBy the way, I long for a life where\nI could be a free spirit and soar[f000]븀\u0000\nanywhere, like sand blown by the wind!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_LuckyItemSandstormSmooth, 0, 0
    MsgWaitAdvance
    // "How did you enjoy my Pokémon\nweather forecast?[f000]븁\u0000\nPlease bring me a Pokémon with other\nmoves or Abilities if you want to hear[f000]븀\u0000\na different forecast!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_HowDidEnjoyPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x802c, 6
    WorkSetConst 0x802f, 4
    VMReturn

L_0E06:
    // "My Pokémon weather forecast for you![f000]븁\u0000\nWell, that's how it normally goes, but...\nYour [f000]ā\u0001\u0000's Ability is[f000]븀\u0000\nnone other than [f000]Ć\u0001\u0002![f000]븁\u0000\nThat's a problematic one for me.[f000]븁\u0000\nSince [f000]Ć\u0001\u0002 turns off the effects\nof weather, it means my Pokémon weather[f000]븀\u0000\nforecast isn't too helpful!"
    ParentActorMsg MSGFILE_SCRIPT, Route62_Text_PokemonWeatherForecastWell, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x802c, 6
    VMReturn
