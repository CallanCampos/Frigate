#include "asm/field_script.inc"
#include "text/script/entralink.h"

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
    WorkSetConst 0x8020, 0

Script_1:
    VMHalt

Script_2:
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4000, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMCall L_0064
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0064:
    // "Checked your Entree.[f000]븁\u0000"
    SystemMsg Entralink_Text_CheckedEntree, 2
    InfoMsgClose
    FadeOutBlackQ
    FadeWait
    FieldClose
    HighLinkCmd_016E
    FieldOpen
    FadeInBlackQ
    FadeWait
    IsFestMissionAvailable 0x8010
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0091
    VMJump L_0099

L_0091:
    VMReturn
    VMJump L_00BA

L_0099:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_00AC
    VMJump L_00BA

L_00AC:
    VMCall L_00BC
    VMReturn
    VMJump L_00BA

L_00BA:
    VMReturn

L_00BC:
    VMStackPushFlag EVENT_FLAG_0x0986
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D5
    VMCall L_00D9

L_00D5:
    FunfestMissionStart
    VMReturn

L_00D9:
    ActorCmdExec 0, Movement_0198
    ActorCmdWait
    ActorCmdExec 255, Movement_01A0
    ActorCmdWait
    // "Good! You've managed\nto receive a mission.[f000]븁\u0000\nThe first mission is the...\nBerry search, I see.[f000]븁\u0000\nThat should be just\nthe mission for a beginner like you.[f000]븁\u0000\nCities and routes have places\nthat glow.[f000]븁\u0000\nYou should find Berries there.\nSearch carefully![f000]븁\u0000\nAfter the mission, come back here, and\na new power will be granted...[f000]븁\u0000\nThat's it for now.\nGet going![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_GoodYouveManagedReceive, 0, 0, 0
    MsgWinCloseAll
    VMReturn

Script_8:
    ActorsPauseAll
    ActorCmdExec 0, Movement_01D0
    ActorCmdWait
    ActorCmdExec 0, Movement_01DC
    ActorCmdWait
    ActorCmdExec 255, Movement_01B0
    ActorCmdWait
    // "Welcome to the Entralink.[f000]븁\u0000\nHmm?[f000]븁\u0000\nJudging by your expression, you don't\nseem to know where you are.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_WelcomeEntralinkHmmJudging, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_01A0
    ActorCmdWait
    // "This place is called the Entralink...\nIt's a mysterious place[f000]븀\u0000\nthat links people.[f000]븁\u0000\nIt is also a place where you can hone\nyour skills by helping out[f000]븀\u0000\nnearby adventurers.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_PlaceCalledEntralinkIts, 0, 0, 0
    ActorCmdExec 0, Movement_01A8
    // "...Hmm.\nIt's a bit hard to explain with words.[f000]븁\u0000\nAs an adventurer,\nyou should test yourself[f000]븀\u0000\nto learn what it is...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_HmmItsBitHard, 0, 0, 0
    ActorCmdWait
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0198
    ActorCmdWait
    // "As a start, talk to this Entree\nto receive a mission.[f000]븁\u0000\nThen deliver your power to it!"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_StartTalkEntreeReceive, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    MedalDiscover 77
    MedalDiscover 221
    MedalDiscover 223
    MedalDiscover 225
    MedalDiscover 229
    MedalDiscover 227
    WorkSetConst EVENT_WORK_0x404d, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0198:
    Move 32, 1
    MoveEnd

Movement_01A0:
    Move 33, 1
    MoveEnd

Movement_01A8:
    Move 35, 1
    MoveEnd

Movement_01B0:
    Move 34, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    Move 3, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0

Movement_01D0:
    Move 3, 1
    Move 75, 1
    MoveEnd

Movement_01DC:
    Move 15, 1
    MoveEnd

Script_11:
    ActorsPauseAll
    ActorCmdExec 0, Movement_01D0
    ActorCmdWait
    ActorCmdExec 0, Movement_01DC
    ActorCmdWait
    ActorCmdExec 255, Movement_01B0
    ActorCmdWait
    // "Oh, you're back...[f000]븁\u0000\nWhoa!\nThis is wonderful![f000]븀\u0000\nYou've completed the mission![f000]븁\u0000\nAs I promised, you'll receive a\nnew power...not from me, but[f000]븀\u0000\nfrom this Entree.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_OhYoureBackWhoa, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0198
    ActorCmdWait
    // "The Entree gathers\nwishes from people.[f000]븁\u0000\nThese wishes resonate with each other\nand turn into Pass Powers, which[f000]븀\u0000\nhelp adventurers.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_EntreeGathersWishesFrom, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_01A8
    ActorCmdWait
    // "Go ahead and receive it.\nTalk to the Entree[f000]븀\u0000\nand receive a Pass Power!"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_GoAheadReceiveTalk, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x404d, 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    ActorCmdExec 0, Movement_01D0
    ActorCmdWait
    ActorCmdExec 0, Movement_01DC
    ActorCmdWait
    ActorCmdExec 255, Movement_01B0
    ActorCmdWait
    // "Oh, you're back...[f000]븁\u0000\nHmm...\nThe mission failed, I see...[f000]븁\u0000\nIt's a bit difficult in the beginning,\nso you need to try again and again.[f000]븀\u0000\nYou'll get better before too long.[f000]븁\u0000\nAnyway, I'll give you these\nfor your hard work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_OhYoureBackHmm, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 575
    WorkSet 0x8001, 10
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "As I promised, you'll receive\na new power...not from me,[f000]븀\u0000\nbut from this Entree.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_PromisedYoullReceiveNew, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0198
    ActorCmdWait
    // "The Entree gathers\nwishes from people.[f000]븁\u0000\nThese wishes resonate with each other\nand turn into Pass Powers, which[f000]븀\u0000\nhelp adventurers.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_EntreeGathersWishesFrom, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_01A8
    ActorCmdWait
    // "Go ahead and receive it.\nTalk to the Entree[f000]븀\u0000\nand receive a Pass Power!"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_GoAheadReceiveTalk, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x404d, 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    ActorCmdExec 0, Movement_0198
    ActorCmdWait
    ActorCmdExec 255, Movement_01A0
    ActorCmdWait
    // "You've received Pass Power(s), I see.[f000]븁\u0000\nNow you've become a real adventurer.[f000]븁\u0000\nUse your Pass Powers to help\nnearby adventurers![f000]븁\u0000\nTo use a Pass Power, you need a certain\nnumber of Pass Orbs.[f000]븁\u0000\nReceive missions from time to time\nand collect Pass Orbs.[f000]븁\u0000\nThat's all from me.[f000]븁\u0000\nIt is now up to you\nwhat you think and do.[f000]븁\u0000\nI hope you have a wonderful\nadventure waiting for you...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_YouveReceivedPassPower, 0, 0, 0
    MsgWinCloseAll
    // "You can use a Pass Power by tapping\nthe Tag Log on your C-Gear.[f000]븁\u0000\nTap the green triangle icon in\nthe Tag Log."
    SystemMsg Entralink_Text_CanUsePassPower, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x404d, 6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x404d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0351
    // "First, talk to the Entree and\naccept a mission.[f000]븁\u0000\nThen deliver your wishes\nto the Entree!"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_FirstTalkEntreeAccept, 0, 2, 0
    VMJump L_037C

L_0351:
    VMStackPush EVENT_WORK_0x404d
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0376
    // "Go ahead!\nReceive the Pass Power from the Entree!"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_GoAheadReceivePass, 0, 2, 0
    VMJump L_037C

L_0376:
    VMCall L_0386

L_037C:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0386:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

L_0392:
    VMStackPush 0x8021
    VMStackPushConst 555
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_042A
    // "Is there anything you'd like to know\nabout the Entralink?"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_ThereAnythingYoudLike_2, 0, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    ListMenuAdd 27, 65535, 0
    ListMenuAdd 28, 65535, 1
    ListMenuAdd 29, 65535, 10
    ListMenuShow
    WorkCmpConst 0x8022, 0
    VMJumpIf CMP_EQ, L_03E7
    VMJump L_03F9

L_03E7:
    // "Missions test adventurers.[f000]븁\u0000\nComplete these missions and you'll\nreceive helpful things like Pass Orbs[f000]븀\u0000\nand Pass Powers.[f000]븁\u0000\nTalk to the Entree to start a mission, or\nuse Tag Log on your C-Gear[f000]븀\u0000\nto join someone else's mission.[f000]븁\u0000\nYou'll gain more power that way.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_MissionsTestAdventurersComplete, 0, 2, 0
    VMJump L_0424

L_03F9:
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_040C
    VMJump L_041E

L_040C:
    // "Pass Powers are mysterious powers.[f000]븁\u0000\nYou can register up to three of them\nin your C-Gear.[f000]븁\u0000\nYou use them by aiming at\npeople you pass by with the[f000]븀\u0000\nTag Log.[f000]븁\u0000\nBut you need Pass Orbs to use\nPass Powers.[f000]븁\u0000\nYou should carry out a mission\nand collect Pass Orbs.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_PassPowersMysteriousPowers, 0, 2, 0
    VMJump L_0424

L_041E:
    WorkSetConst 0x8021, 555

L_0424:
    VMJump L_0392

L_042A:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    // "The Entralink is an island where\nadventurers are linked...[f000]븁\u0000\nMeet many adventurers\nand hone your skills."
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_EntralinkIslandWhereAdventurers, 0, 2, 0
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

L_0458:
    VMStackPush 0x8023
    VMStackPushConst 555
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_04F0
    // "Is there anything you'd like to know\nabout participants?"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_ThereAnythingYoudLike_4, 1, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32804
    ListMenuAdd 43, 65535, 0
    ListMenuAdd 44, 65535, 1
    ListMenuAdd 45, 65535, 10
    ListMenuShow
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_04AD
    VMJump L_04BF

L_04AD:
    // "When you start a Funfest Mission,\nnearby adventurers can participate[f000]븀\u0000\nin the same mission using the Tag Log[f000]븀\u0000\non their C-Gear.[f000]븁\u0000\nIt may take some time before the mission\ninvitations are received. They should wait[f000]븀\u0000\na little while and check their Tag Logs.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_WhenStartFunfestMission, 1, 2, 0
    VMJump L_04EA

L_04BF:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_04D2
    VMJump L_04E4

L_04D2:
    // "When you have participants in your\nFunfest Mission, their actions also count[f000]븀\u0000\nas part of your score.[f000]븁\u0000\nIn a mission, items are found at the same\nlocation for everyone.[f000]븁\u0000\nYou may be able to talk to each other\nto make missions easier for everybody.[f000]븁\u0000\nOne more thing. Sometimes you may fail\nto receive scores from other people.[f000]븁\u0000\nWorlds don't always resonate\nas they should, it seems.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_WhenHaveParticipantsFunfest, 1, 2, 0
    VMJump L_04EA

L_04E4:
    WorkSetConst 0x8023, 555

L_04EA:
    VMJump L_0458

L_04F0:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    // "It seems that the more adventurers\nyou gather, the more power you bring[f000]븀\u0000\nto the Entralink."
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_SeemsMoreAdventurersGather, 1, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

L_0526:
    VMStackPush 0x8025
    VMStackPushConst 555
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_05BE
    // "Is there anything you'd like to know\nabout the Entree?"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_ThereAnythingYoudLike_3, 2, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32806
    ListMenuAdd 36, 65535, 0
    ListMenuAdd 37, 65535, 1
    ListMenuAdd 38, 65535, 10
    ListMenuShow
    WorkCmpConst 0x8026, 0
    VMJumpIf CMP_EQ, L_057B
    VMJump L_058D

L_057B:
    // "You'll find more and more types of\nFunfest Missions you can receive from the[f000]븀\u0000\nEntree as you advance in your adventure.[f000]븁\u0000\nTalking to people or battling other\nTrainers will give you more mission types.[f000]븁\u0000\nPlease listen to other people\nwhen they express their wishes.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_YoullFindMoreMore, 2, 2, 0
    VMJump L_05B8

L_058D:
    WorkCmpConst 0x8026, 1
    VMJumpIf CMP_EQ, L_05A0
    VMJump L_05B2

L_05A0:
    // "Raising the Entralink's level\ngives you new types of Pass Powers.[f000]븁\u0000\nThe Entralink level can sometimes go\nup as you complete a Funfest Mission.[f000]븁\u0000\nThe higher the score is,\nthe more likely the level is to go up.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_RaisingEntralinksLevelGives, 2, 2, 0
    VMJump L_05B8

L_05B2:
    WorkSetConst 0x8025, 555

L_05B8:
    VMJump L_0526

L_05BE:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    // "The Entree grows little by little,\nas if it responds to people's wishes."
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_EntreeGrowsLittleBy, 2, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Entree Forest connects dreams\nand reality.[f000]븁\u0000\nPeople say dreams come true here.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_EntreeForestConnectsDreams, 5, 2, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

L_0600:
    VMStackPush 0x8027
    VMStackPushConst 555
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0698
    // "Is there anything you'd like to know\nabout the Entree Forest?"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_ThereAnythingYoudLike, 5, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32808
    ListMenuAdd 7, 65535, 0
    ListMenuAdd 8, 65535, 1
    ListMenuAdd 9, 65535, 10
    ListMenuShow
    WorkCmpConst 0x8028, 0
    VMJumpIf CMP_EQ, L_0655
    VMJump L_0667

L_0655:
    // "The Entree Forest is a place where\ndreams come true.[f000]븁\u0000\nWhen a dreaming Pokémon wakes up in\nGame Sync, its dream will come true in[f000]븀\u0000\nthis forest.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_EntreeForestPlaceWhere, 5, 2, 0
    VMJump L_0692

L_0667:
    WorkCmpConst 0x8028, 1
    VMJumpIf CMP_EQ, L_067A
    VMJump L_068C

L_067A:
    // "Pokémon that your Pokémon met in its\ndream will show up in the Entralink[f000]븀\u0000\nin a Forest Clearing.[f000]븁\u0000\nPokémon in a Forest Clearing will be\nyour friends for sure![f000]븁\u0000\nIf you move the Pokémon to the Deepest\nClearing, they will wait for you there.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_PokemonPokemonMetIts, 5, 2, 0
    VMJump L_0692

L_068C:
    WorkSetConst 0x8027, 555

L_0692:
    VMJump L_0600

L_0698:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    // "When your Pokémon wakes up from a\ndream, please come to this forest."
    ActorMsg MSGFILE_SCRIPT, Entralink_Text_WhenPokemonWakesUp, 5, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can go back to the original world\nfrom this place.[f000]븁\u0000\nIf you want to go back to the original\nworld, please come back here."
    ParentActorMsg MSGFILE_SCRIPT, Entralink_Text_CanGoBackOriginal, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    // "You can go back to your\nown world from here.[f000]븁\u0000\nDo you want to go back?"
    SystemMsg Entralink_Text_CanGoBackOwn, 2
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06FF
    CallEntralinkWarpOut
    VMJump L_0709

L_06FF:
    ActorCmdExec 255, Movement_0710
    ActorCmdWait

L_0709:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0710:
    Move 8, 1
    MoveEnd
