#include "asm/field_script.inc"
#include "text/script/join_avenue_2.h"

// Script plugin 8, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    VMStackPush EVENT_WORK_0x413a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0056
    Plugin8_Cmd1025 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0056
    WorkSetConst EVENT_WORK_0x4119, 1

L_0056:
    VMStackPush EVENT_WORK_0x4119
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0106
    Plugin8_Cmd1031 20, 0x8020
    DebugPrint 0x8020
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0086
    VMJump L_00A4

L_0086:
    Plugin8_Cmd1028 3, 6
    Plugin8_Cmd1027 2, 0x8022
    ActorSetGPos 0x8022, 6, 0, 4, 1
    VMJump L_0100

L_00A4:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_00B7
    VMJump L_00D5

L_00B7:
    Plugin8_Cmd1028 3, 7
    Plugin8_Cmd1027 3, 0x8022
    ActorSetGPos 0x8022, 6, 0, 4, 1
    VMJump L_0100

L_00D5:
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_00E8
    VMJump L_0100

L_00E8:
    WorkSetConst EVENT_WORK_0x4119, 0
    ActorDelete 1
    ActorDelete 2
    ActorDelete 3
    VMJump L_0100

L_0100:
    VMJump L_0118

L_0106:
    WorkSetConst EVENT_WORK_0x4119, 0
    ActorDelete 1
    ActorDelete 2
    ActorDelete 3

L_0118:
    VMStackPush EVENT_WORK_0x413a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014F
    WorkSetConst 0x8024, 0
    Plugin8_Cmd1027 0, 0x8024
    Plugin8_Cmd1027 1, 0x8024
    Plugin8_Cmd1027 2, 0x8024
    Plugin8_Cmd1027 3, 0x8024
    WorkSetConst EVENT_WORK_0x413a, 0

L_014F:
    WorkSetConst 0x8024, 0
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8025, 1

L_0171:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DA
    Plugin8_Cmd1007 4, 255, 0, 0
    Plugin8_Cmd1007 8, 255, 0, 1
    // "[f000]ĺ\u0001\u0000!\nDo you have any questions?"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_HaveAnyQuestions, 0, 0, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32806
    ListMenuAdd 22, 65535, 0
    ListMenuAdd 23, 65535, 1
    ListMenuAdd 24, 65535, 2
    ListMenuAdd 25, 65535, 3
    ListMenuAdd 26, 65535, 4
    ListMenuAdd 27, 65535, 5
    Plugin8_Cmd1002 1, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FE
    ListMenuAdd 40, 65535, 9

L_01FE:
    ListMenuAdd 29, 65535, 7
    ListMenuAdd 28, 65535, 8
    ListMenuAdd 30, 65535, 6
    ListMenuShow
    VMStackPush 0x8026
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0231
    WorkSetConst 0x8026, 6

L_0231:
    WorkCmpConst 0x8026, 0
    VMJumpIf CMP_EQ, L_0244
    VMJump L_0256

L_0244:
    // "This is the brand-new avenue in the\nUnova region, [f000]Ĺ\u0001\u0001![f000]븁\u0000\nIt is still under major development\nto create an avenue that's perfect for[f000]븀\u0000\nmeeting people, making discoveries,[f000]븀\u0000\nand having fun. Good times![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_BrandNewAvenueUnova, 0, 0, 0
    VMJump L_03D4

L_0256:
    WorkCmpConst 0x8026, 1
    VMJumpIf CMP_EQ, L_0269
    VMJump L_027B

L_0269:
    // "[f000]ĺ\u0001\u0000, you've been signed up\nby the owner to manage the avenue.[f000]븁\u0000\nYour goal is simple:\ncreate a dazzling, delightful avenue![f000]븁\u0000\nDesign an avenue that will fill all visitors\nwith joy and the desire to visit often.[f000]븁\u0000\nYou'll have assistants, so please let us\nknow how we can help you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_YouveBeenSignedUp, 0, 0, 0
    VMJump L_03D4

L_027B:
    WorkCmpConst 0x8026, 2
    VMJumpIf CMP_EQ, L_028E
    VMJump L_02CB

L_028E:
    Plugin8_Cmd1002 1, 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    // "Bring in more customers by using\nthe communication features.[f000]븁\u0000\nPeople you've met through communication\nfeatures will visit the avenue.[f000]븁\u0000\nWhen you talk to them, there are three\nthings you can choose to do.[f000]븁\u0000\nYou can “invite\" the person to join\nthe avenue, “recommend\" a shop, or[f000]븀\u0000\n“recruit\" that person as an assistant.[f000]븁\u0000\nFor details, please listen to\neach explanation.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_BringMoreCustomersBy, 0, 0, 0
    VMJump L_02C5

L_02B9:
    // "You can use various communication\nfeatures to encourage people to visit.[f000]븁\u0000\nTalk to people during their visits here.\nThere are two things you can do.[f000]븁\u0000\nThe first thing is to “invite\" someone.\nThe other thing is to “recommend\" a shop[f000]븀\u0000\nto someone.[f000]븁\u0000\nTo really get the idea of all you can do,\nstay and listen to a little more about it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_CanUseVariousCommunication, 0, 0, 0

L_02C5:
    VMJump L_03D4

L_02CB:
    WorkCmpConst 0x8026, 3
    VMJumpIf CMP_EQ, L_02DE
    VMJump L_02F0

L_02DE:
    // "You can bring in more customers by\nusing the communication features.[f000]븁\u0000\nWhen you have the C-Gear turned on,\npassersby will visit the avenue.[f000]븁\u0000\nPeople you've met through the Union\nRoom, Infrared Communication, Random[f000]븀\u0000\nMatchups, and GTS visit the avenue, too.[f000]븁\u0000\nCommunication features aren't the\nonly way to bring in visitors.[f000]븁\u0000\nAs your adventure progresses,\nfans might sometimes visit the avenue.[f000]븁\u0000\nYou can use your Town Map to check\nthe current number of visitors.[f000]븁\u0000\nIt might be fun to check it now and then.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_CanBringMoreCustomers, 0, 0, 0
    VMJump L_03D4

L_02F0:
    WorkCmpConst 0x8026, 4
    VMJumpIf CMP_EQ, L_0303
    VMJump L_0315

L_0303:
    // "When you select “Invite,\" the visitor\nwill begin to live here and create a shop.[f000]븁\u0000\nEach person has his or her own\nhopes and dreams.[f000]븁\u0000\nI recommend listening carefully to the\nperson before you decide.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_WhenSelectInviteVisitor, 0, 0, 0
    VMJump L_03D4

L_0315:
    WorkCmpConst 0x8026, 5
    VMJumpIf CMP_EQ, L_0328
    VMJump L_033A

L_0328:
    // "“Recommend\" a shop to a visitor.[f000]븁\u0000\nIf the visitor is happy with the shop,\nthe popularity of the shop and[f000]븀\u0000\nthis avenue will go up.[f000]븁\u0000\nAs their popularity goes up, shops and\nthis avenue reach higher ranks.[f000]븁\u0000\nThe higher the shop rank goes up,\nthe more items and services it'll have.[f000]븁\u0000\nWhen the current avenue rank goes up,\nitem prices will become slightly[f000]븀\u0000\nmore reasonable.[f000]븁\u0000\nTry to recommend shops that your\nvisitors will be delighted with so the[f000]븀\u0000\npopularity of the shops goes up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_RecommendShopVisitorIf, 0, 0, 0
    VMJump L_03D4

L_033A:
    WorkCmpConst 0x8026, 7
    VMJumpIf CMP_EQ, L_034D
    VMJump L_035F

L_034D:
    // "[f000]ĺ\u0001\u0000, this is your office.[f000]븁\u0000\nYour assistants and I will support you\nin developing [f000]Ĺ\u0001\u0001.[f000]븁\u0000\nAs the avenue grows bigger and bigger,\nthere will be more and more visitors.[f000]븁\u0000\nYou may enjoy hearing what they have to\nsay, so please drop by from time to time.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_OfficeAssistantsWillSupport, 0, 0, 0
    VMJump L_03D4

L_035F:
    WorkCmpConst 0x8026, 8
    VMJumpIf CMP_EQ, L_0372
    VMJump L_037E

L_0372:
    VMCall L_03F0
    VMJump L_03D4

L_037E:
    WorkCmpConst 0x8026, 6
    VMJumpIf CMP_EQ, L_0391
    VMJump L_03A9

L_0391:
    // "I'm looking forward to seeing this grow\ninto a fantastic avenue!"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ImLookingForwardSeeing, 0, 0, 0
    WorkSetConst 0x8025, 0
    VMJump L_03D4

L_03A9:
    WorkCmpConst 0x8026, 9
    VMJumpIf CMP_EQ, L_03BC
    VMJump L_03CE

L_03BC:
    // "If you choose to “Recruit,\" the visitor\nwill become your assistant and help you.[f000]븁\u0000\nYour chances of recruiting the visitor\nimprove if your avenue ranks higher[f000]븀\u0000\nthan the person's ideals.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_IfChooseRecruitVisitor, 0, 0, 0
    VMJump L_03D4

L_03CE:
    WorkSetConst 0x8025, 0

L_03D4:
    VMJump L_0171

L_03DA:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_03F0:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 1

L_0402:
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0655
    Plugin8_Cmd1031 25, 0x8020
    // "Is there anything you'd like to know\nabout shops?"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ThereAnythingYoudLike, 0, 0, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32808
    ListMenuAdd 31, 65535, 0
    ListMenuAdd 33, 65535, 2
    ListMenuAdd 39, 65535, 8
    ListMenuAdd 36, 65535, 5
    ListMenuAdd 32, 65535, 1
    ListMenuAdd 34, 65535, 3
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047B
    ListMenuAdd 35, 65535, 4

L_047B:
    ListMenuAdd 37, 65535, 6
    ListMenuAdd 38, 65535, 7
    ListMenuAdd 30, 65535, 9
    ListMenuShow
    VMStackPush 0x8028
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04AE
    WorkSetConst 0x8028, 9

L_04AE:
    WorkCmpConst 0x8028, 0
    VMJumpIf CMP_EQ, L_04C1
    VMJump L_0502

L_04C1:
    Plugin8_Cmd1007 8, 255, 0, 0
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F0
    // "There are eight different kinds of shops\nyou can build in [f000]Ĺ\u0001\u0000.[f000]븁\u0000\nWhen someone is invited to build a shop,\nthe type of shop they open is based[f000]븀\u0000\non that person's hopes and dreams.[f000]븁\u0000\nWhen you recommend shops to visitors,\nthe shops get more popular and their[f000]븀\u0000\nrank improves.[f000]븁\u0000\nTalk to assistant 2 for advice on what\nkinds of shops will grow the fastest.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ThereEightDifferentKinds, 0, 0, 0
    VMJump L_04FC

L_04F0:
    // "There are seven different kinds of shops\nthat can be built in [f000]Ĺ\u0001\u0000.[f000]븁\u0000\nWhen someone is invited to build a shop,\nthe type of shop they open is based[f000]븀\u0000\non that person's hopes and dreams.[f000]븁\u0000\nWhen you recommend shops to visitors,\nthe shops get more popular and their[f000]븀\u0000\nrank improves.[f000]븁\u0000\nTalk to assistant 2 for advice on what\nkinds of shops will grow the fastest.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ThereSevenDifferentKinds, 0, 0, 0

L_04FC:
    VMJump L_064F

L_0502:
    WorkCmpConst 0x8028, 1
    VMJumpIf CMP_EQ, L_0515
    VMJump L_0527

L_0515:
    // "At Markets, you'll be able to purchase\nvarious items useful for your adventure.[f000]븁\u0000\nSometimes they carry items not available\nat Poké Marts, or so I've heard.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_MarketsYoullAblePurchase, 0, 0, 0
    VMJump L_064F

L_0527:
    WorkCmpConst 0x8028, 2
    VMJumpIf CMP_EQ, L_053A
    VMJump L_054C

L_053A:
    // "At Raffle Shops, you can win cool prizes.[f000]븁\u0000\nThe grand prize is a Master Ball![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_RaffleShopsCanWin, 0, 0, 0
    VMJump L_064F

L_054C:
    WorkCmpConst 0x8028, 3
    VMJumpIf CMP_EQ, L_055F
    VMJump L_0571

L_055F:
    // "Flower Shops carry Berries.[f000]븁\u0000\nDifferent shops may have\ndifferent Berries.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_FlowerShopsCarryBerries, 0, 0, 0
    VMJump L_064F

L_0571:
    WorkCmpConst 0x8028, 4
    VMJumpIf CMP_EQ, L_0584
    VMJump L_0596

L_0584:
    // "At Nurseries, you can put Pokémon Eggs\nin incubators to speed up hatching.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_NurseriesCanPutPokemon, 0, 0, 0
    VMJump L_064F

L_0596:
    WorkCmpConst 0x8028, 5
    VMJumpIf CMP_EQ, L_05A9
    VMJump L_05BB

L_05A9:
    // "Dojos train Pokémon.[f000]븁\u0000\nThey can level up Pokémon and\nraise the base stats of Pokémon.[f000]븁\u0000\nBe aware that Pokémon won't learn\ncertain moves or evolve while they[f000]븀\u0000\nlevel up this way.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_DojosTrainPokemonThey, 0, 0, 0
    VMJump L_064F

L_05BB:
    WorkCmpConst 0x8028, 6
    VMJumpIf CMP_EQ, L_05CE
    VMJump L_05E0

L_05CE:
    // "Antique Shops carry rare finds.[f000]븁\u0000\nYou may find out you have really rare\nitems when you have them appraised.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_AntiqueShopsCarryRare, 0, 0, 0
    VMJump L_064F

L_05E0:
    WorkCmpConst 0x8028, 7
    VMJumpIf CMP_EQ, L_05F3
    VMJump L_0605

L_05F3:
    // "Cafés give you and your Pokémon\nthe chance to have a nice meal.[f000]븁\u0000\nMeals make Pokémon happy,\nand they will grow friendly faster.[f000]븁\u0000\nYou can also level Pokémon up and\nraise their base stats.[f000]븁\u0000\nBe aware that Pokémon won't learn\ncertain moves or evolve while they[f000]븀\u0000\nlevel up this way.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_CafesGivePokemonChance, 0, 0, 0
    VMJump L_064F

L_0605:
    WorkCmpConst 0x8028, 8
    VMJumpIf CMP_EQ, L_0618
    VMJump L_062A

L_0618:
    // "Beauty Salons keep Pokémon tidy.[f000]븁\u0000\nHaircuts and cosmetics make Pokémon\nhappy, and they grow friendlier faster.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_BeautySalonsKeepPokemon, 0, 0, 0
    VMJump L_064F

L_062A:
    WorkCmpConst 0x8028, 9
    VMJumpIf CMP_EQ, L_063D
    VMJump L_0649

L_063D:
    WorkSetConst 0x8027, 0
    VMJump L_064F

L_0649:
    WorkSetConst 0x8027, 0

L_064F:
    VMJump L_0402

L_0655:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    VMReturn

Script_3:
    ActorsPauseAll
    Plugin8_Cmd1031 20, 0x8020
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_067E
    VMJump L_068A

L_067E:
    VMCall L_076F
    VMJump L_0763

L_068A:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_069D
    VMJump L_06A9

L_069D:
    VMCall L_07F3
    VMJump L_0763

L_06A9:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_06BC
    VMJump L_06C8

L_06BC:
    VMCall L_0877
    VMJump L_0763

L_06C8:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_06DB
    VMJump L_06E7

L_06DB:
    VMCall L_0996
    VMJump L_0763

L_06E7:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_06FA
    VMJump L_0706

L_06FA:
    VMCall L_0A7F
    VMJump L_0763

L_0706:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_0719
    VMJump L_0725

L_0719:
    VMCall L_0AC3
    VMJump L_0763

L_0725:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_0738
    VMJump L_0744

L_0738:
    VMCall L_0B07
    VMJump L_0763

L_0744:
    WorkCmpConst 0x8020, 8
    VMJumpIf CMP_EQ, L_0757
    VMJump L_0763

L_0757:
    VMCall L_0B4D
    VMJump L_0763

L_0763:
    WorkSetConst EVENT_WORK_0x4119, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_076F:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort, 1, 0, 0
    // "I have a present for you today.\nI've recruited a new assistant for you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_HavePresentTodayIve, 1, 0, 0
    ActorMsgClose
    ActorFindByGPos 0x8022, 0x8020, 6, 0, 4
    VMCall L_0C48
    Plugin8_Cmd1007 0, 3, 2, 0
    Plugin8_Cmd1007 6, 3, 2, 1
    // "[f000]ķ\u0001\u0001\nI'm [f000]Ā\u0001\u0000![f000]븁\u0000\nWhen it comes to reorganizing things,\nleave it to me![f000]븁\u0000\nPC Boxes and Record Rankings!\nChanging the roles of assistants![f000]븀\u0000\nChanging the order of shops![f000]븀\u0000\nPlease talk to me when you[f000]븀\u0000\nwant to reorganize things.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ImWhenComesReorganizing, 0x8022, 0, 0
    ActorMsgClose
    ActorCmdExec 255, Movement_0C7C
    ActorCmdWait
    // "Keep up the good work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_KeepUpGoodWork, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1030 21, 1
    VMCall L_0BCB
    VMReturn

L_07F3:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort_2, 1, 0, 0
    // "I have a present for you today.\nI've recruited a new assistant for you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_HavePresentTodayIve_2, 1, 0, 0
    ActorMsgClose
    ActorFindByGPos 0x8022, 0x8020, 6, 0, 4
    VMCall L_0C48
    Plugin8_Cmd1007 0, 3, 3, 0
    Plugin8_Cmd1007 6, 3, 3, 1
    // "[f000]ķ\u0001\u0001\nI'm [f000]Ā\u0001\u0000![f000]븁\u0000\nPlease talk to me when you want to rest\nyour tired Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ImPleaseTalkWhen, 0x8022, 0, 0
    ActorMsgClose
    ActorCmdExec 255, Movement_0C7C
    ActorCmdWait
    // "Keep up the good work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_KeepUpGoodWork_2, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1030 21, 2
    VMCall L_0BCB
    VMReturn

L_0877:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    Plugin8_Cmd1007 4, 255, 0, 1
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort_3, 1, 0, 0
    // "Today, I want to say how grateful I am.[f000]븁\u0000\nThank you for doing so much to\ndevelop this avenue![f000]븁\u0000\nYou're a truly deserving “[f000]ĺ\u0001\u0001\"!\nI'd say you've made the avenue your own![f000]븁\u0000\nBecause of that, you have earned\nthe privilege of renaming the avenue.[f000]븁\u0000\nPlease consider a new name for Join\nAvenue that describes its character.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_TodayWantSayHow, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1023 0, 0
    Plugin8_Cmd1007 4, 255, 0, 1
    Plugin8_Cmd1007 8, 255, 0, 2
    // "Wonderful![f000]븁\u0000\n[f000]ĺ\u0001\u0001,\n[f000]Ĺ\u0001\u0002 belongs to you now![f000]븀\u0000\nI'll leave everything to you.[f000]븁\u0000\nContinue developing the avenue\nso visitors will be even more delighted.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_WonderfulBelongsNowIll, 1, 0, 0
    ActorMsgClose
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xe9000, 0, 0x58000, 60
    EvCameraWait
    ActorFindByGPos 0x8022, 0x8020, 13, 0, 5
    Plugin8_Cmd1002 35, 0x8020
    Plugin8_Cmd1002 37, 0x8023
    Plugin8_Cmd1007 0, 3, 0x8023, 0
    Plugin8_Cmd1007 6, 3, 0x8023, 1
    ActorCmdExec 255, Movement_0C84
    ActorCmdExec 0x8022, Movement_0C74
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0954
    // "[f000]ķ\u0001\u0001\nI'm [f000]Ā\u0001\u0000![f000]븁\u0000\nPlease talk to me when you want to\nchange the avenue's name.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ImPleaseTalkWhen_2, 0x8022, 0, 0
    VMJump L_0960

L_0954:
    // "[f000]ķ\u0001\u0001\nI'm [f000]Ā\u0001\u0000![f000]븁\u0000\nPlease talk to me when you want to\nchange the avenue's name.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ImPleaseTalkWhen_3, 0x8022, 0, 0

L_0960:
    ActorMsgClose
    EvCameraReturn 60
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 255, Movement_0C7C
    ActorCmdWait
    // "Keep up the good work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_KeepUpGoodWork_3, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1030 21, 3
    MedalGive 194
    VMCall L_0BCB
    VMReturn

L_0996:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort_4, 1, 0, 0
    // "Today, I have a present for you.[f000]븁\u0000\nNow you can change the avenue's\nneon arch as you wish.[f000]븁\u0000\nIt will give a fresh look to the avenue.\nWon't it delight our visitors even more?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_TodayHavePresentNow, 1, 0, 0
    ActorMsgClose
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0xb6000, 0, 0xa4000, 60
    EvCameraWait
    ActorFindByGPos 0x8022, 0x8020, 11, 0, 11
    Plugin8_Cmd1002 36, 0x8020
    Plugin8_Cmd1002 38, 0x8023
    Plugin8_Cmd1007 0, 3, 0x8023, 0
    Plugin8_Cmd1007 6, 3, 0x8023, 1
    ActorCmdExec 255, Movement_0C8C
    ActorCmdExec 0x8022, Movement_0C7C
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A41
    // "[f000]ķ\u0001\u0001\nI'm [f000]Ā\u0001\u0000![f000]븁\u0000\nPlease talk to me when you want to\nchange the neon arch.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ImPleaseTalkWhen_4, 0x8022, 0, 0
    VMJump L_0A4D

L_0A41:
    // "[f000]ķ\u0001\u0001\nI'm [f000]Ā\u0001\u0000![f000]븁\u0000\nPlease talk to me when you want to\nchange the neon arch.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_ImPleaseTalkWhen_5, 0x8022, 0, 0

L_0A4D:
    ActorMsgClose
    EvCameraReturn 60
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 255, Movement_0C7C
    ActorCmdWait
    // "Keep up the good work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_KeepUpGoodWork_4, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1030 21, 4
    VMCall L_0BCB
    VMReturn

L_0A7F:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort_5, 1, 0, 0
    // "I have a piece of advice for you today.[f000]븁\u0000\nYou'd want an assistant that suits\nyour preferences, wouldn't you?[f000]븁\u0000\nWhy don't you choose one of the visitors?\nIt's quite easy.[f000]븁\u0000\nTalk to a person and choose “Recruit.\"[f000]븁\u0000\nThe higher the current avenue rank is\ncompared to the person's ideals,[f000]븀\u0000\nthe better chance you'll have of[f000]븀\u0000\nrecruiting him or her successfully.[f000]븁\u0000\nPlease bring even more delight\nto our visitors![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_HavePieceAdviceToday, 1, 0, 0
    // "Keep up the good work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_KeepUpGoodWork_5, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1030 21, 5
    VMCall L_0BCB
    VMReturn

L_0AC3:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort_6, 1, 0, 0
    // "Today, I have a big present for you![f000]븁\u0000\nTo commemorate your entering the\nHall of Fame, shops have been renovated.[f000]븁\u0000\nYou'll notice new services and items,\nso you might want to check those out.[f000]븁\u0000\nWhat's more...\nHave you heard of Pokémon Eggs?[f000]븁\u0000\nYou'll now be able to build what they call\na “Nursery,\" which enables Eggs to[f000]븀\u0000\nbe hatched more quickly than normal.[f000]븁\u0000\nFind a visitor who wants to build a\nNursery, and give it a try![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_TodayHaveBigPresent, 1, 0, 0
    // "I have nothing more to\npoint out to you![f000]븁\u0000\nJust keep up the good work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_HaveNothingMorePoint, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1030 21, 6
    VMCall L_0BCB
    VMReturn

L_0B07:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort_7, 1, 0, 0
    // "Today, I have a surprise for you.[f000]븁\u0000\nEverybody here is celebrating the\ngreat effort you've been putting in.[f000]븁\u0000\nWhat are you waiting for? Don't be shy!\nGo and greet everyone![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_TodayHaveSurpriseEverybody, 1, 0, 0
    Plugin8_Cmd1030 21, 7
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x413a, 1
    FlagSet EVENT_FLAG_0x09f1
    MapChangeWarp ZONE_JOIN_AVENUE, 15, 70, 0
    VMReturn

L_0B4D:
    VMCall L_0BB9
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]Ā\u0001\u0000,\nI see you are putting in a lot of effort![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_SeePuttingLotEffort_8, 1, 0, 0
    // "I have a present for you to recognize\nall your continuous effort.[f000]븁\u0000\nIt's just a little something.\nPlease, take it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_HavePresentRecognizeAll, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1002 61, 0x8020
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8020
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "I have nothing more to\npoint out to you![f000]븁\u0000\nJust keep up the good work![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue2_Text_HaveNothingMorePoint_2, 1, 0, 0
    ActorMsgClose
    Plugin8_Cmd1030 21, 8
    VMCall L_0BCB
    VMReturn

L_0BB9:
    ActorWalkRoute 255, 8, 5, 0, 8, 1
    ActorCmdWait
    VMReturn

L_0BCB:
    ActorCmdExec 255, Movement_0C04
    ActorCmdExec 1, Movement_0C18
    ActorCmdExec 2, Movement_0C24
    ActorCmdExec 3, Movement_0C34
    ActorCmdWait
    ActorDelete 2
    ActorDelete 3
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 1
    SEWait
    VMReturn
    .balign 4, 0

Movement_0C04:
    Move 15, 1
    Move 2, 1
    Move 63, 1
    Move 1, 2
    MoveEnd

Movement_0C18:
    Move 13, 9
    Move 69, 1
    MoveEnd

Movement_0C24:
    Move 14, 1
    Move 13, 9
    Move 69, 1
    MoveEnd

Movement_0C34:
    Move 63, 1
    Move 15, 1
    Move 13, 9
    Move 69, 1
    MoveEnd

L_0C48:
    ActorCmdExec 0x8022, Movement_0C5C
    ActorCmdExec 255, Movement_0C68
    ActorCmdWait
    VMReturn

Movement_0C5C:
    Move 13, 1
    Move 15, 1
    MoveEnd

Movement_0C68:
    Move 63, 1
    Move 2, 1
    MoveEnd

Movement_0C74:
    Move 2, 1
    MoveEnd

Movement_0C7C:
    Move 0, 1
    MoveEnd

Movement_0C84:
    Move 3, 1
    MoveEnd

Movement_0C8C:
    Move 1, 1
    MoveEnd
