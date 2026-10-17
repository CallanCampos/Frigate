#include "asm/field_script.inc"
#include "text/script/nacrene_city_2.h"

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
    WorkSetConst 0x8023, 0

Script_21:
    VMCall L_0213
    VMHalt

Script_1:
    VMStackPush EVENT_WORK_0x407c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01bf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00A5
    ActorSetGPos 2, 13, 0, 11, 0

L_00A5:
    VMStackPushFlag EVENT_FLAG_0x02b7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C4
    ActorSetGPos 9, 12, 3, 4, 1

L_00C4:
    GameGetVersion 0x8010
    DebugPrint 0x8010
    WorkCmpConst 0x8010, 23
    VMJumpIf CMP_EQ, L_00DF
    VMJump L_00EB

L_00DF:
    WorkSetConst EVENT_WORK_0x4020, 143
    VMJump L_010A

L_00EB:
    WorkCmpConst 0x8010, 22
    VMJumpIf CMP_EQ, L_00FE
    VMJump L_010A

L_00FE:
    WorkSetConst EVENT_WORK_0x4020, 144
    VMJump L_010A

L_010A:
    VMHalt

Script_2:
    Cmd_02B2 12, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4046
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ef
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_014F
    FlagReset EVENT_FLAG_0x02b7
    WorkSetConst EVENT_WORK_0x4140, 1

L_014F:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4046
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ef
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_018C
    FlagSet EVENT_FLAG_0x02b7
    WorkSetConst EVENT_WORK_0x4140, 2

L_018C:
    VMCall L_01DE
    FlagReset EVENT_FLAG_0x01ef
    GameGetVersion 0x8010
    DebugPrint 0x8010
    WorkCmpConst 0x8010, 23
    VMJumpIf CMP_EQ, L_01B1
    VMJump L_01BD

L_01B1:
    WorkSetConst EVENT_WORK_0x4020, 143
    VMJump L_01DC

L_01BD:
    WorkCmpConst 0x8010, 22
    VMJumpIf CMP_EQ, L_01D0
    VMJump L_01DC

L_01D0:
    WorkSetConst EVENT_WORK_0x4020, 144
    VMJump L_01DC

L_01DC:
    VMHalt

L_01DE:
    Cmd_02B2 12, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4046
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0211
    FlagSet EVENT_FLAG_0x02b7
    WorkSetConst EVENT_WORK_0x4140, 0

L_0211:
    VMReturn

L_0213:
    Cmd_02B2 12, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4046
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_025D
    VMStackPushFlag EVENT_FLAG_0x02b7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0253
    ActorDelete 9

L_0253:
    FlagSet EVENT_FLAG_0x02b7
    WorkSetConst EVENT_WORK_0x4140, 0

L_025D:
    VMReturn

Script_18:
    ActorsPauseAll
    ActorCmdExec 9, Movement_08C4
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0294
    ActorCmdExec 9, Movement_08EC
    ActorCmdWait
    VMJump L_02C7

L_0294:
    WorkSub 0x8022, 2
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_02C7
    ActorWalkRoute 9, 0x8021, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 9, Movement_08F4
    ActorCmdWait

L_02C7:
    // "You!\nBones can talk![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_BonesCanTalk, 9, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_NACRENE_CITY_11, 5, 1, 4, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x02b7
    ActorWalkRoute 255, 12, 6, 0, 8, 1
    ActorCmdWait
    ActorAdd 9
    SEPlay SEQ_SE_KAIDAN
    ActorSetGPos 9, 12, 0, 2, 1
    SEWait
    ActorCmdExec 255, Movement_08EC
    ActorCmdWait
    ActorWalkRoute 9, 12, 4, 4, 8, 1
    ActorCmdWait
    // "You!\nBones can talk![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_BonesCanTalk, 9, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_NACRENE_CITY_11, 5, 1, 4, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    FadeInBlack
    FadeWait
    // "You seem all right,\nbut it's natural for people[f000]븀\u0000\nto have different opinions.[f000]븁\u0000\nDon't fight over it--enjoy it.\nConsider why they might think like that!"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_SeemAllRightBut, 9, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 9, 12, 2, 1, 8, 1
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    VMSleep 8
    ActorDelete 9
    SEWait
    WorkSetConst EVENT_WORK_0x4140, 3
    WorkSetConst EVENT_WORK_0x4046, 1
    FlagSet EVENT_FLAG_0x02b7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    // "Wow! Whenever I look at this skeleton,\nI'm...fascinated.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_WowWheneverLookSkeleton, 2, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_03F2
    WorkSub 0x8022, 1
    ActorWalkRoute 2, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait

L_03F2:
    ActorCmdExec 2, Movement_08DC
    ActorCmdWait
    // "Welcome! I'm Hawes, the\nassistant director.[f000]븁\u0000\nSince you were kind enough to visit,\nI'll give you a tour of the museum.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_WelcomeImHawesAssistant, 2, 0, 0
    MsgWinCloseAll
    ActorNew 8, 11, 1, 251, 86, 0
    PlayerGetGPos 0x8021, 0x8022
    WorkSub 0x8021, 1
    ActorWalkRoute 251, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_08D4
    ActorCmdWait
    WordSetPlayerName 0
    // "???: Why, you're [f000]Ā\u0001\u0000, right?\nYou came all the way out here![f000]븁\u0000\nWell now, which do you like better--\nthe Cover Fossil or the Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_WhyYoureRightCame, 251, 2, 0

L_044D:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_05D6
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32804
    ListMenuAdd 6, 65535, 0
    ListMenuAdd 7, 65535, 1
    ListMenuAdd 8, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_0496
    VMJump L_0509

L_0496:
    // "Do you really want to choose\nthe Cover Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_ReallyWantChooseCover, 251, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F7
    // "The Cover Fossil!\nNow, that's a nice choice![f000]븁\u0000\nIf you want to restore it,\ngo to our reception counter![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_CoverFossilNowThats, 251, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 572
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01bd
    WorkSetConst 0x8025, 1
    VMJump L_0503

L_04F7:
    // "Which do you like better,\nthe Cover Fossil or the Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_WhichLikeBetterCover, 251, 2, 0

L_0503:
    VMJump L_05D0

L_0509:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_051C
    VMJump L_058F

L_051C:
    // "Do you really want to choose\nthe Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_ReallyWantChoosePlume, 251, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_057D
    // "The Plume Fossil!\nNow, that's a nice choice![f000]븁\u0000\nIf you want to restore it,\ngo to our reception counter![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_PlumeFossilNowThats, 251, 2, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 573
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01bd
    WorkSetConst 0x8025, 1
    VMJump L_0589

L_057D:
    // "Which do you like better,\nthe Cover Fossil or the Plume Fossil?"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_WhichLikeBetterCover, 251, 2, 0

L_0589:
    VMJump L_05D0

L_058F:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_05A2
    VMJump L_05BC

L_05A2:
    // "Oh, come now![f000]븁\u0000\nDon't be shy!\nYou're too young to be bashful![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_OhComeNowDont, 251, 2, 0
    MsgWinCloseAll
    WorkSetConst 0x8025, 1
    VMJump L_05D0

L_05BC:
    // "Oh, come now![f000]븁\u0000\nDon't be shy!\nYou're too young to be bashful![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_OhComeNowDont, 251, 2, 0
    MsgWinCloseAll
    WorkSetConst 0x8025, 1

L_05D0:
    VMJump L_044D

L_05D6:
    ActorCmdExec 2, Movement_08D4
    ActorCmdWait
    // "Hawes: Dear![f000]븁\u0000\nSuddenly interrupting my tours\nwill surprise the patrons.[f000]븁\u0000\nOh, and you haven't even\nintroduced yourself yet, have you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_HawesDearSuddenlyInterrupting, 2, 0, 0
    MsgWinCloseAll
    VMStackPushFlag EVENT_FLAG_0x01bd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0613
    // "Oh, that's right.[f000]븁\u0000\nI'm Lenora, the director of\nthis museum![f000]븁\u0000\nI'm always in my study,\nso come visit if you have time![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_OhThatsRightIm, 251, 0, 0
    VMJump L_061F

L_0613:
    // "Oh, that's right.[f000]븁\u0000\nI'm Lenora, the director of\nthis museum![f000]븁\u0000\nI'm always in my study,\nso if you want a Fossil,[f000]븀\u0000\ndrop by and say hello![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_OhThatsRightIm_2, 251, 0, 0

L_061F:
    MsgWinCloseAll
    ActorWalkRoute 251, 8, 11, 1, 8, 0
    ActorCmdWait
    ActorDelete 251
    ActorCmdExec 2, Movement_08DC
    ActorCmdExec 255, Movement_08E4
    ActorCmdWait
    // "Hawes: Oh, dear...\nWell, let's continue our tour.[f000]븁\u0000\nThis skeleton is of a\nDragon-type Pokémon.[f000]븁\u0000\nThere's a theory that it had an accident\nwhile it was flying around the world, and[f000]븀\u0000\nso it became a Fossil.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_HawesOhDearWell, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 6, 13, 1, 8, 0
    ActorWalkRoute 255, 6, 14, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 2, Movement_08D4
    ActorCmdExec 255, Movement_08D4
    ActorCmdWait
    // "We learned that Deoxys changes\nits Forme by coming into contact[f000]븀\u0000\nwith the power of meteors.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_WeLearnedDeoxysChanges, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 7, 11, 1, 8, 1
    ActorWalkRoute 255, 6, 11, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 2, Movement_08E4
    ActorCmdWait
    // "Oh!\nNow, this is a truly interesting artifact![f000]븁\u0000\nThis is a replica of that Light Stone![f000]븁\u0000\nHave you heard of the\nLight Stone?[f000]븁\u0000\nThis is the form of the legendary\nPokémon Reshiram while it rests...[f000]븁\u0000\nWhat? You've seen Reshiram?\nThat's amazing![f000]븁\u0000\nIn that case, my explanation\nis superfluous, isn't it?[f000]븁\u0000"
    // "Oh!\nNow, this is a truly interesting artifact![f000]븁\u0000\nThis is a replica of that Dark Stone![f000]븁\u0000\nHave you heard of the\nDark Stone?[f000]븁\u0000\nThis is the form of the legendary\nPokémon Zekrom while it rests...[f000]븁\u0000\nWhat? You've seen Zekrom?\nThat's amazing![f000]븁\u0000\nIn that case, my explanation\nis superfluous, isn't it?[f000]븁\u0000"
    ActorMsgVersioned 1024, NacreneCity2_Text_OhNowTrulyInteresting_2, NacreneCity2_Text_OhNowTrulyInteresting, 2, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 2, 13, 11, 1, 8, 0
    ActorWalkRoute 255, 12, 11, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 2, Movement_08E4
    ActorCmdExec 255, Movement_08E4
    ActorCmdWait
    // "The library is in the next room.\nBeyond that is the director's room.[f000]븁\u0000\nBy the way, Lenora is a former\nGym Leader!"
    ActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_LibraryNextRoomBeyond, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x407c, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Nacrene Museum!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_WelcomeNacreneMuseum, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Currently, we're exhibiting a replica\nof the Light Stone, which was[f000]븀\u0000\na legendary Pokémon's dormant form."
    // "Currently, we're exhibiting a replica\nof the Dark Stone, which was[f000]븀\u0000\na legendary Pokémon's dormant form."
    ActorMsgVersioned 1024, NacreneCity2_Text_CurrentlyWereExhibitingReplica_2, NacreneCity2_Text_CurrentlyWereExhibitingReplica, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01bf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0782
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The library is in the next room.\nBeyond that is the director's room.[f000]븁\u0000\nBy the way, Lenora is a former\nGym Leader!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_LibraryNextRoomBeyond, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0796

L_0782:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Skeletal specimens are so mysterious...[f000]븁\u0000\nFrom the outside, you can't see their\nfunctional, efficient design.[f000]븁\u0000\nIt's almost as if it is an embodiment of\ntheir former essence... So fascinating!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_SkeletalSpecimensMysteriousFrom, 0, 0
    LastKeyWait
    ActorMsgClose

L_0796:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rarities from around the world...\nMuseums are packed with adventure!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_RaritiesFromAroundWorld, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I heard that the director, Lenora,\nis too busy with her research[f000]븀\u0000\non Fossils and Pokémon bones, so she[f000]븀\u0000\ntook a break from being a Gym Leader."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_HeardDirectorLenoraToo, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Perhaps there is a Pokémon that came\nfrom space along with this meteorite."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_PerhapsTherePokemonCame, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Thick Club held by Marowak...[f000]븁\u0000\nJust like how Cubone wears\nits mother's skull,[f000]븀\u0000\nMarowak could also battle with[f000]븀\u0000\nthe bone of someone dear to it..."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity2_Text_ThickClubHeldBy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is a skeletal specimen from a\nPokémon that flew around the world."
    InfoMsg NacreneCity2_Text_SkeletalSpecimenFromPokemon, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a Fossil of a Pokémon that was\nprotected by a very hard shell."
    InfoMsg NacreneCity2_Text_ItsFossilPokemonProtected, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "A meteor that has a space virus\nattached to it."
    InfoMsg NacreneCity2_Text_MeteorHasSpaceVirus, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0873
    // "Special Exhibition![f000]븁\u0000\nThis is a replica of the Dark Stone--\nthe legendary Pokémon Zekrom's[f000]븀\u0000\ndormant state."
    InfoMsg NacreneCity2_Text_SpecialExhibitionReplicaDark, 2
    VMJump L_0878

L_0873:
    // "Special Exhibition![f000]븁\u0000\nThis is a replica of the Light Stone--\nthe legendary Pokémon Reshiram's[f000]븀\u0000\ndormant state."
    InfoMsg NacreneCity2_Text_SpecialExhibitionReplicaLight, 2

L_0878:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "A plate with an unknown script carved\ninto it."
    InfoMsg NacreneCity2_Text_PlateUnknownScriptCarved, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "A mask that ancient people used to wear\nat festivals."
    InfoMsg NacreneCity2_Text_MaskAncientPeopleUsed, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Bones that were once carried as weapons\nby a certain kind of Pokémon."
    InfoMsg NacreneCity2_Text_BonesWereOnceCarried, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_08C4:
    Move 75, 1
    MoveEnd
    Move 35, 1
    MoveEnd

Movement_08D4:
    Move 34, 1
    MoveEnd

Movement_08DC:
    Move 33, 1
    MoveEnd

Movement_08E4:
    Move 32, 1
    MoveEnd

Movement_08EC:
    Move 32, 1
    MoveEnd

Movement_08F4:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
