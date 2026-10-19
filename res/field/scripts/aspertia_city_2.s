#include "asm/field_script.inc"
#include "text/script/aspertia_city_2.h"

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

Script_1:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40a3
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x02e3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0081
    ActorSetGPos 0, 4, 0, 5, 3

L_0081:
    VMCall L_0091
    VMHalt

Script_18:
    VMCall L_0091
    VMHalt

L_0091:
    VMStackPushFlag EVENT_FLAG_0x02e0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AE
    ObjInitPointGPos 7, 11, 0, 1

L_00AE:
    VMStackPushFlag EVENT_FLAG_0x02e1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CB
    ObjInitPointGPos 8, 12, 0, 1

L_00CB:
    VMReturn

Script_17:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x58000, 0, 0xa8000, 1
    EvCameraWait
    FlagReset EVENT_FLAG_0x02e4
    ActorAdd 0
    ActorSetGPos 0, 5, 0, 10, 0
    FadeInBlackQ
    FadeWait
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorWalkRoute 0, 5, 8, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 0, Movement_0988
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000!\nI'm home![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_ImHome, 0, 1, 0
    MsgWinCloseAll
    EvCameraReturn 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 255, Movement_0978
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    ActorWalkRoute 0, 9, 8, 1, 8, 1
    ActorCmdWait
    // "Do you know Professor Juniper?\nShe's a famous Pokémon researcher.[f000]븁\u0000\nActually, she's an old friend of mine,\nand she called me today for the[f000]븀\u0000\nfirst time in ages![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_KnowProfessorJuniperShes, 0, 1, 0
    WordSetPlayerName 0
    // "This is out of the blue,\nbut, [f000]Ā\u0001\u0000![f000]븁\u0000\nDo you want to have a Pokémon?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_OutBlueButWant, 0, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F1
    WorkSetConst 0x8020, 1

L_019A:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F1
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0990
    ActorCmdWait
    ActorCmdExec 0, Movement_0928
    ActorCmdWait
    // "What?!\nThat's a shock![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_WhatThatsShock, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0938
    ActorCmdWait
    // "I'll ask you again.[f000]븁\u0000\nDo you want a Pokémon?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_IllAskAgainWant, 0, 1, 0
    YesNoWin 0x8020
    VMJump L_019A

L_01F1:
    // "OK!\nStep one completed![f000]븁\u0000\nWell then, do you know what a\nPokédex is?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_OkStepOneCompleted, 0, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0226
    // "I knew you would![f000]븁\u0000\nIsn't it amazing how it automatically\nrecords Pokémon you encounter?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_KnewWouldIsntAmazing, 0, 1, 0
    VMJump L_0232

L_0226:
    // "I see...[f000]븁\u0000\nIt's an amazing device that automatically\nrecords the Pokémon you encounter![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_SeeItsAmazingDevice, 0, 1, 0

L_0232:
    // "Yet another question![f000]븁\u0000\nYou want a Pokédex, right?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_YetAnotherQuestionWant, 0, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0284
    WorkSetConst 0x8020, 1

L_025B:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0284
    // "I don't mean to be selfish, but I'd like it\nif you were a bit more agreeable.[f000]븁\u0000\nHaving a Pokédex means\ntraveling around the world![f000]븀\u0000\nThink about that for a second.[f000]븁\u0000\nSo I'll ask you again...[f000]븁\u0000\nYou want a Pokédex, right?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_DontMeanSelfishBut, 0, 1, 0
    YesNoWin 0x8020
    VMJump L_025B

L_0284:
    // "OK!\nStep two completed![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_OkStepTwoCompleted, 0, 1, 0
    VMStackPushFlag EVENT_FLAG_0x0001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BC
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000!\nYour course of action has been set![f000]븁\u0000\nA girl named Bianca has come\nhere to meet you![f000]븁\u0000\nShe's Professor Juniper's assistant.\nI was told to simply look for[f000]븀\u0000\na big, green hat![f000]븁\u0000\nThat's right! You're going to\ngo look for Bianca.[f000]븁\u0000\nAnd then you'll get a Pokédex and a\nPokémon to be your partner!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_CourseActionHasBeen, 0, 1, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0308

L_02BC:
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000!\nYour course of action has been set![f000]븁\u0000\nA girl named Bianca has come\nhere to meet you![f000]븁\u0000\nShe's Professor Juniper's assistant.\nI was told to simply look for[f000]븀\u0000\na big, green hat![f000]븁\u0000\nThat's right! You're going\nto go look for Bianca.[f000]븁\u0000\nAnd then you'll get a Pokédex and a\nPokémon to be your partner![f000]븁\u0000\nOh! Your Xtransceiver's in your\nBag, right?[f000]븀\u0000\nDo you know how to open your Bag?"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_CourseActionHasBeen_2, 0, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F8
    // "The girl's name is Bianca. I was told\nyou should look for a big, green hat![f000]븁\u0000\nShe might be lost because\nthis is her first time here.[f000]븀\u0000\nGo look for her!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_GirlsNameBiancaTold, 0, 1, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMJump L_0308

L_02F8:
    // "I'll send you off with these\nwords from “Adventure Rules.\"[f000]븀\u0000\n“The X Button is vitally important[f000]븀\u0000\nfor Trainers.\"[f000]븁\u0000\nOK! Off with you now!\nGo look for Bianca, OK!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_IllSendOffThese, 0, 1, 0
    MsgWaitAdvance
    MsgWinCloseAll

L_0308:
    ActorWalkRoute 0, 8, 7, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0978
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40a0, 1
    HollowRivalCmd_0263 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WordSetPlayerName 0
    ActorCmdExec 0, Movement_0990
    ActorCmdWait
    ActorWalkRoute 0, 9, 8, 1, 8, 1
    ActorCmdWait
    // "Mom: Welcome home, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_MomWelcomeHome, 0, 0, 0
    // "Hmm. I barely recognize you![f000]븁\u0000\nIt seems like you've seen\nand thought about a lot[f000]븀\u0000\nand grown into an adult![f000]븁\u0000\nOh! Seems we're about to have\na visitor![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_HmmBarelyRecognizeSeems, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 255, 5, 6, 1, 8, 0
    ActorWalkRoute 0, 4, 6, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_0978
    ActorCmdExec 255, Movement_0978
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorNew 5, 10, 0, 251, 104, 0
    SEWait
    BGMPlay SEQ_BGM_E_DOCTOR2
    ActorWalkRoute 251, 5, 8, 1, 8, 0
    ActorCmdWait
    // "???: Oh, so you're [f000]Ā\u0001\u0000![f000]븁\u0000\nMy name's Juniper![f000]븁\u0000\nThe one who gave you\nyour Pokédex is my daughter![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_OhYoureNamesJuniper, 251, 2, 0
    MsgWinCloseAll
    // "Mom: It's been a long time,\nProfessor Juniper![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_MomItsBeenLong, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0998
    ActorCmdWait
    // "Cedric Juniper: Has it been that long?\nI can't remember...[f000]븁\u0000\nWell, that's not really why I came.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_CedricJuniperHasBeen, 251, 2, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 5, 7, 1, 8, 0
    ActorCmdWait
    // "[f000]Ā\u0001\u0000![f000]븁\u0000\nTo commemorate your entering the\nHall of Fame, I'm going to upgrade[f000]븀\u0000\nyour Pokédex![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_CommemorateEnteringHallFame, 251, 2, 0
    MsgWinCloseAll
    MEPlay SEQ_ME_KEYITEM
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000's Pokédex\nwas upgraded!"
    SystemMsg AspertiaCity2_Text_SPokedexUpgraded, 2
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll
    PokeDexGiveNational
    ActorCmdExec 251, Movement_0940
    ActorCmdWait
    // "Cedric Juniper: I'll tell you what\nI upgraded, so why don't you ask?[f000]븁\u0000\nWell, actually, it's really simple![f000]븁\u0000\nI made it so you can register\nall of the National Pokédex Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_CedricJuniperIllTell, 251, 2, 0
    MsgWinCloseAll
    // "Mom: Wow! That's amazing!\nThat must be why you and[f000]븀\u0000\nyour daughter are Pokémon Professors![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_MomWowThatsAmazing, 0, 1, 0
    MsgWinCloseAll
    // "Cedric Juniper: Ha ha ha!\nFlattery won't get you anywhere![f000]븁\u0000\nWell then, think I'd best\nbe taking my leave![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_CedricJuniperHaHa, 251, 2, 0
    // "Listen up! There are still many, many\nPokémon in this world![f000]븁\u0000\nSometimes Pokémon attack\neach other for food.[f000]븁\u0000\nSometimes they help one another.\nThey protect each other's places.[f000]븁\u0000\nI'd be happy if you think about things\nlike that while looking at the Pokédex.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_ListenUpThereStill, 251, 2, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 5, 10, 1, 8, 0
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    BGMChangeMap
    ActorCmdExec 0, Movement_0988
    VMSleep 8
    ActorCmdExec 255, Movement_0980
    ActorCmdWait
    // "Mom: He left...[f000]븁\u0000\nOh, that's right! [f000]Ā\u0001\u0000!\nI have a present for you, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_MomHeLeftOh, 0, 1, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 29
    WorkSet 0x8001, 2
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Mom: No matter what you do, your time\nis yours and your Pokémon's alone![f000]븁\u0000\nSo decide what you want to do\nfor yourself and do it![f000]븁\u0000\nI enjoy my own time\nin my own way, too!"
    ActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_MomNoMatterWhat, 0, 1, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40a0, 3
    FlagReset EVENT_FLAG_0x02e9
    FlagReset EVENT_FLAG_0x02e8
    WorkSetConst EVENT_WORK_0x4115, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0515
    VMCall L_057B
    VMJump L_0575

L_0515:
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0542
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh? Are the both of you\nout looking for Bianca?[f000]븁\u0000\nYou still haven't found her?\nLook for the big, green hat!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_OhBothOutLooking, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0575

L_0542:
    VMStackPush EVENT_WORK_0x40a1
    VMStackPushConst 2
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_056F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The girl's name is Bianca. I was told\nyou should look for a big, green hat![f000]븁\u0000\nShe might be lost because\nthis is her first time here.[f000]븀\u0000\nGo look for her!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_GirlsNameBiancaTold, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0575

L_056F:
    VMCall L_057B

L_0575:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_057B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 999
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05AD
    WordSetPlayerName 0
    VMCall L_060C
    ParentActorMsg MSGFILE_SCRIPT, 0x8008, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_05AD:
    WordSetPlayerName 0
    // "Mom: Welcome back!\nHey, how are your Pokémon?[f000]븁\u0000\nWell, why don't you rest for a moment?\nNothing but hard work will wear you out![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_MomWelcomeBackHey, 0, 0
    MsgWinCloseAll
    VMCall L_05DE
    Random EVENT_WORK_0x400a, 5
    VMCall L_060C
    ParentActorMsg MSGFILE_SCRIPT, 0x8008, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_05DE:
    FadeEx 3, 0, 16, 2
    FadeExWait
    PokePartyRecoverAll
    MEPlay SEQ_ME_ASA
    MEWait
    FadeEx 3, 16, 0, 2
    FadeExWait
    WorkSetConst EVENT_WORK_0x400f, 999
    RecordAdd 72, 1
    VMReturn

L_060C:
    WordSetPlayerName 0
    WorkCmpConst EVENT_WORK_0x400a, 0
    VMJumpIf CMP_EQ, L_0622
    VMJump L_062E

L_0622:
    WorkSetConst 0x8008, 18
    VMJump L_06AA

L_062E:
    WorkCmpConst EVENT_WORK_0x400a, 1
    VMJumpIf CMP_EQ, L_0641
    VMJump L_064D

L_0641:
    WorkSetConst 0x8008, 19
    VMJump L_06AA

L_064D:
    WorkCmpConst EVENT_WORK_0x400a, 2
    VMJumpIf CMP_EQ, L_0660
    VMJump L_066C

L_0660:
    WorkSetConst 0x8008, 20
    VMJump L_06AA

L_066C:
    WorkCmpConst EVENT_WORK_0x400a, 3
    VMJumpIf CMP_EQ, L_067F
    VMJump L_068B

L_067F:
    WorkSetConst 0x8008, 21
    VMJump L_06AA

L_068B:
    WorkCmpConst EVENT_WORK_0x400a, 4
    VMJumpIf CMP_EQ, L_069E
    VMJump L_06AA

L_069E:
    WorkSetConst 0x8008, 22
    VMJump L_06AA

L_06AA:
    VMReturn

Script_5:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4115
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4115
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_06EB
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hee hee!\nI came to visit![f000]븁\u0000\nProfessor Juniper is investigating\na cave on Route 20!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_HeeHeeCameVisit, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_071E

L_06EB:
    VMStackPush EVENT_WORK_0x4115
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_070A
    VMCall L_0724
    VMJump L_071E

L_070A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca: I was just talking to your mom![f000]븁\u0000\nShe told me an amazing\nstory about Professor Juniper!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_BiancaJustTalkingMom, 0, 0
    LastKeyWait
    ActorMsgClose

L_071E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0724:
    Random 0x8010, 5
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_073D
    VMJump L_0757

L_073D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Cave of Being...[f000]븁\u0000\nI wonder where those three Pokémon--\nUxie, Mesprit, and Azelf--flew off to?[f000]븁\u0000\nUxie is a Pokémon that\nsymbolizes knowledge...[f000]븁\u0000\nIf you mention a place in Unova\nwhere knowledge is gathered,[f000]븀\u0000\nthe first thing that comes to mind[f000]븀\u0000\nis Nacrene City's museum...[f000]븁\u0000\nMesprit is the Pokémon that\npresides over emotion, right?[f000]븁\u0000\nCelestial Tower's bell stirs emotions...[f000]븁\u0000\nAnd Azelf is willpower...[f000]븁\u0000\nThe desire to see something through...\nWhat place could represent that?"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_CaveBeingWonderWhere, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_080B

L_0757:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_076A
    VMJump L_0784

L_076A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Route 23's mysterious Pokémon...[f000]븁\u0000\nJust by being near it you can feel\nsome kind of willpower...[f000]븁\u0000\nIt's best to go have a look\nfor yourself!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_Route23sMysteriousPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_080B

L_0784:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0797
    VMJump L_07B1

L_0797:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sometimes, there are mass outbreaks\nof Pokémon, right?[f000]븁\u0000\nA lot of the exact same Pokémon\nshow up at the same time,[f000]븀\u0000\nand it's such a surprise![f000]븁\u0000\nLike where were all of you before?[f000]븁\u0000\nIf you look at the electronic bulletin\nboards, you can learn about them!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_SometimesThereMassOutbreaks, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_080B

L_07B1:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_07C4
    VMJump L_07DE

L_07C4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Professor Juniper--I'm talking about\nAurea Juniper, mind you...[f000]븁\u0000\nShe's researching the origins of Pokémon![f000]븁\u0000\nIt's interesting!\nAmong the Pokémon that exist now,[f000]븀\u0000\nthere were some that have been[f000]븀\u0000\naround from the past and some[f000]븀\u0000\nthat were discovered recently.[f000]븁\u0000\nBy the way, her dad is researching\nPokémon distribution and biology!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_ProfessorJuniperImTalking, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_080B

L_07DE:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_07F1
    VMJump L_080B

L_07F1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Your mom's amazing![f000]븁\u0000\nIt's sweet how she met\nyour dad while working[f000]븀\u0000\nreception at the Pokémon Center.[f000]븁\u0000\nHee hee!\nShe's taught me a lot!"
    ParentActorMsg MSGFILE_SCRIPT, AspertiaCity2_Text_MomsAmazingItsSweet, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_080B

L_080B:
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a Wii console!\nIt has a Wii Remote!"
    InfoMsg AspertiaCity2_Text_ItsWiiConsoleHas, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a brand-new bed!"
    InfoMsg AspertiaCity2_Text_ItsBrandNewBed, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0000 checked the PC.[f000]븁\u0000\nAdventure Rule No. 1\nThe X Button opens the menu![f000]븁\u0000\nAdventure Rule No. 2\nRecord your progress with SAVE."
    InfoMsg AspertiaCity2_Text_CheckedPcAdventureRule, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's an award for completing\nthe Unova Pokédex![f000]븁\u0000"
    InfoMsg AspertiaCity2_Text_ItsAwardCompletingUnova, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 0, 1
    FieldOpen
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's an award for completing\nthe National Mode Pokédex![f000]븁\u0000"
    InfoMsg AspertiaCity2_Text_ItsAwardCompletingNational, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 1, 1
    FieldOpen
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a trophy proving you defeated\nthe Single Master in the Battle Subway!"
    InfoMsg AspertiaCity2_Text_ItsTrophyProvingDefeated, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a trophy proving you defeated\nthe Double Master in the Battle Subway!"
    InfoMsg AspertiaCity2_Text_ItsTrophyProvingDefeated_2, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a trophy proving you defeated\nthe Multi Master in the Battle Subway!"
    InfoMsg AspertiaCity2_Text_ItsTrophyProvingDefeated_3, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a model of a Ferris wheel\nMom bought as a souvenir."
    InfoMsg AspertiaCity2_Text_ItsModelFerrisWheel, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a Darumaka Pokémon doll\nMom received in the past.[f000]븁\u0000\nWhen you get knocked down,\njust get up again!"
    InfoMsg AspertiaCity2_Text_ItsDarumakaPokemonDoll, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    VMNop
    VMHalt
    VMNop2
    VMNop
    VMNop2
    VMSleep 1
    VMNop2
    VMNop2
    VMHalt
    .byte 0x01
    .balign 4, 0
    Move 0, 1
    Move 3, 1
    Move 61, 1
    MoveEnd

Movement_0928:
    Move 71, 1
    Move 14, 1
    Move 72, 1
    MoveEnd

Movement_0938:
    Move 15, 1
    MoveEnd

Movement_0940:
    Move 71, 1
    Move 13, 1
    Move 72, 1
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

Movement_0978:
    Move 33, 1
    MoveEnd

Movement_0980:
    Move 34, 1
    MoveEnd

Movement_0988:
    Move 35, 1
    MoveEnd

Movement_0990:
    Move 75, 1
    MoveEnd

Movement_0998:
    Move 159, 1
    MoveEnd
