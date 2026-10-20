#include "asm/field_script.inc"
#include "text/script/global_10675.h"

// Script plugin 6, from the only plugin whose commands it decodes with

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin6_Cmd1022 0, 0x8010
    VMCall L_0060
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin6_Cmd1022 1, 0x8010
    VMCall L_0060
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin6_Cmd1022 2, 0x8010
    VMCall L_0060
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0060:
    ParentActorMsg MSGFILE_SCRIPT, 0x8010, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WbtCmd_GetWinCount 4, 17, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B9
    // "I know much about strong Trainers![f000]븁\u0000\nSo, I'll tell you about Gym Leaders and\nChampions and so on[f000]븀\u0000\nonce you've battled them!"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_KnowMuchAboutStrong, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00BF

L_00B9:
    VMCall L_00C5

L_00BF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00C5:
    // "I know a lot about strong Trainers.[f000]븁\u0000\nHey! Which region's Trainers\ndo you want to know about?"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_KnowLotAboutStrong, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    VMCall L_11DE
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00F9
    ListMenuAdd 60, 65535, 60

L_00F9:
    VMCall L_129A
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_011A
    ListMenuAdd 61, 65535, 61

L_011A:
    VMCall L_131A
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_013B
    ListMenuAdd 62, 65535, 62

L_013B:
    VMCall L_138E
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_015C
    ListMenuAdd 63, 65535, 63

L_015C:
    VMCall L_141A
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_017D
    ListMenuAdd 64, 65535, 64

L_017D:
    ListMenuAdd 59, 65535, 59
    ListMenuShow
    WorkCmpConst 0x8022, 60
    VMJumpIf CMP_EQ, L_019A
    VMJump L_01A6

L_019A:
    VMCall L_0232
    VMJump L_0222

L_01A6:
    WorkCmpConst 0x8022, 61
    VMJumpIf CMP_EQ, L_01B9
    VMJump L_01C5

L_01B9:
    VMCall L_0672
    VMJump L_0222

L_01C5:
    WorkCmpConst 0x8022, 62
    VMJumpIf CMP_EQ, L_01D8
    VMJump L_01E4

L_01D8:
    VMCall L_095E
    VMJump L_0222

L_01E4:
    WorkCmpConst 0x8022, 63
    VMJumpIf CMP_EQ, L_01F7
    VMJump L_0203

L_01F7:
    VMCall L_0C06
    VMJump L_0222

L_0203:
    WorkCmpConst 0x8022, 64
    VMJumpIf CMP_EQ, L_0216
    VMJump L_0222

L_0216:
    VMCall L_0F36
    VMJump L_0222

L_0222:
    // "If you want to know about\nstrong Trainers, come back anytime!"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_IfWantKnowAbout, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_0232:
    WorkSetConst 0x8020, 1

L_0238:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0670
    // "Which Trainer do you want to\nknow about?"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WhichTrainerWantKnow, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    Cmd_02D5 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027F
    ListMenuAdd 65, 65535, 65

L_027F:
    Cmd_02D5 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A0
    ListMenuAdd 66, 65535, 66

L_02A0:
    Cmd_02D5 2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C1
    ListMenuAdd 67, 65535, 67

L_02C1:
    Cmd_02D5 3, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E2
    ListMenuAdd 68, 65535, 68

L_02E2:
    Cmd_02D5 4, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0303
    ListMenuAdd 69, 65535, 69

L_0303:
    Cmd_02D5 5, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0324
    ListMenuAdd 70, 65535, 70

L_0324:
    Cmd_02D5 6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0345
    ListMenuAdd 71, 65535, 71

L_0345:
    Cmd_02D5 7, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0366
    ListMenuAdd 72, 65535, 72

L_0366:
    Cmd_02D5 8, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0387
    ListMenuAdd 73, 65535, 73

L_0387:
    Cmd_02D5 9, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03A8
    ListMenuAdd 74, 65535, 74

L_03A8:
    Cmd_02D5 10, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C9
    ListMenuAdd 75, 65535, 75

L_03C9:
    Cmd_02D5 11, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03EA
    ListMenuAdd 76, 65535, 76

L_03EA:
    Cmd_02D5 12, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_040B
    ListMenuAdd 77, 65535, 77

L_040B:
    Cmd_02D5 13, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_042C
    ListMenuAdd 78, 65535, 78

L_042C:
    Cmd_02D5 19, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044D
    ListMenuAdd 84, 65535, 84

L_044D:
    ListMenuAdd 59, 65535, 59
    ListMenuShow
    WorkCmpConst 0x8022, 65
    VMJumpIf CMP_EQ, L_046A
    VMJump L_047A

L_046A:
    // "Cheren is a new Gym Leader\nin Aspertia City.[f000]븁\u0000\nNonetheless, for his age,\nhe's very experienced and brilliant.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_CherenNewGymLeader, 2, 0
    VMJump L_066A

L_047A:
    WorkCmpConst 0x8022, 66
    VMJumpIf CMP_EQ, L_048D
    VMJump L_049D

L_048D:
    // "Roxie![f000]븁\u0000\nRoxie is performing brilliantly in her band\nwhile being a Gym Leader in Virbank City.[f000]븁\u0000\nShe is a vocalist and bass guitarist![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_RoxieRoxiePerformingBrilliantly, 2, 0
    VMJump L_066A

L_049D:
    WorkCmpConst 0x8022, 67
    VMJumpIf CMP_EQ, L_04B0
    VMJump L_04C0

L_04B0:
    // "Burgh is an artist and\nalso the Gym Leader of Castelia City.[f000]븁\u0000\nI heard he polished his skills\nin Nacrene City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BurghArtistAlsoGym, 2, 0
    VMJump L_066A

L_04C0:
    WorkCmpConst 0x8022, 68
    VMJumpIf CMP_EQ, L_04D3
    VMJump L_04E3

L_04D3:
    // "Elesa![f000]븁\u0000\nShe's the Gym Leader of Nimbasa City\nand also a top model.[f000]븁\u0000\nNo, she's the top model among\ntop models! The best top model![f000]븁\u0000\nEr... Everybody in the Unova region\nknows it, right?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_ElesaShesGymLeader, 2, 0
    VMJump L_066A

L_04E3:
    WorkCmpConst 0x8022, 69
    VMJumpIf CMP_EQ, L_04F6
    VMJump L_0506

L_04F6:
    // "Clay... He doesn't look it,\nbut he's a hardworking man.[f000]븁\u0000\nYou'll know much more about him\nif you go to the Pokémon Gym[f000]븀\u0000\nin Driftveil City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_ClayHeDoesntLook, 2, 0
    VMJump L_066A

L_0506:
    WorkCmpConst 0x8022, 70
    VMJumpIf CMP_EQ, L_0519
    VMJump L_0529

L_0519:
    // "Skyla![f000]븁\u0000\nAs a pilot, she exceeded her grandfather\nwho was called a gifted pilot.[f000]븁\u0000\nOn top of that, she's the Gym Leader\nof Mistralton City! She's great![f000]븁\u0000\nShe's a good friend of Elesa.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_SkylaPilotSheExceeded, 2, 0
    VMJump L_066A

L_0529:
    WorkCmpConst 0x8022, 71
    VMJumpIf CMP_EQ, L_053C
    VMJump L_054C

L_053C:
    // "Drayden is the mayor and\nthe Gym Leader of Opelucid City.[f000]븁\u0000\nHe recognized Iris's talent\nand brought her to the Unova region.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_DraydenMayorGymLeader, 2, 0
    VMJump L_066A

L_054C:
    WorkCmpConst 0x8022, 72
    VMJumpIf CMP_EQ, L_055F
    VMJump L_056F

L_055F:
    // "Marlon is the Gym Leader of Humilau City![f000]븁\u0000\nPutting that aside, being a man of the\nsea is not a profession, is it?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_MarlonGymLeaderHumilau, 2, 0
    VMJump L_066A

L_056F:
    WorkCmpConst 0x8022, 73
    VMJumpIf CMP_EQ, L_0582
    VMJump L_0592

L_0582:
    // "Bianca!\nShe's an assistant of Professor Juniper![f000]븁\u0000\nShe's from Nuvema Town and started her\njourney of adventure with Cheren[f000]븀\u0000\nand another friend![f000]븁\u0000\nShe's humble, but she's a pretty\nstrong Trainer.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BiancaShesAssistantProfessor, 2, 0
    VMJump L_066A

L_0592:
    WorkCmpConst 0x8022, 74
    VMJumpIf CMP_EQ, L_05A5
    VMJump L_05B5

L_05A5:
    // "Chili!\nHe's a triplet in Striaton City![f000]븀\u0000\nI believe he uses Fire-type Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_ChiliHesTripletStriaton, 2, 0
    VMJump L_066A

L_05B5:
    WorkCmpConst 0x8022, 75
    VMJumpIf CMP_EQ, L_05C8
    VMJump L_05D8

L_05C8:
    // "Cress!\nHe's a triplet in Striaton City![f000]븀\u0000\nI believe he uses Water-type Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_CressHesTripletStriaton, 2, 0
    VMJump L_066A

L_05D8:
    WorkCmpConst 0x8022, 76
    VMJumpIf CMP_EQ, L_05EB
    VMJump L_05FB

L_05EB:
    // "Cilan!\nHe's a triplet in Striaton City![f000]븀\u0000\nI believe he uses Grass-type Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_CilanHesTripletStriaton, 2, 0
    VMJump L_066A

L_05FB:
    WorkCmpConst 0x8022, 77
    VMJumpIf CMP_EQ, L_060E
    VMJump L_061E

L_060E:
    // "Lenora![f000]븁\u0000\nShe's the director of Nacrene Museum.[f000]븁\u0000\nI guess she was too busy with her\nresearch and quit being the Gym Leader.[f000]븁\u0000\nRetaliate that her Watchog used\nwas impressive![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_LenoraShesDirectorNacrene, 2, 0
    VMJump L_066A

L_061E:
    WorkCmpConst 0x8022, 78
    VMJumpIf CMP_EQ, L_0631
    VMJump L_0641

L_0631:
    // "Brycen! He's now an outstanding\nmovie star in Pokéstar Studios.[f000]븁\u0000\nHaving said that, he was originally\nan actor.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BrycenHesNowOutstanding, 2, 0
    VMJump L_066A

L_0641:
    WorkCmpConst 0x8022, 84
    VMJumpIf CMP_EQ, L_0654
    VMJump L_0664

L_0654:
    // "Alder![f000]븁\u0000\nHe's the previous Champion of\nthe Unova region.[f000]븁\u0000\nHe's gone through a lot of hardship,\nbut he's a person of high caliber.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_AlderHesPreviousChampion, 2, 0
    VMJump L_066A

L_0664:
    WorkSetConst 0x8020, 0

L_066A:
    VMJump L_0238

L_0670:
    VMReturn

L_0672:
    WorkSetConst 0x8020, 1

L_0678:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_095C
    // "Which Trainer do you want to\nknow about?"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WhichTrainerWantKnow, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    Cmd_02D5 20, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06BF
    ListMenuAdd 85, 65535, 85

L_06BF:
    Cmd_02D5 21, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06E0
    ListMenuAdd 86, 65535, 86

L_06E0:
    Cmd_02D5 22, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0701
    ListMenuAdd 87, 65535, 87

L_0701:
    Cmd_02D5 23, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0722
    ListMenuAdd 88, 65535, 88

L_0722:
    Cmd_02D5 24, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0743
    ListMenuAdd 89, 65535, 89

L_0743:
    Cmd_02D5 25, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0764
    ListMenuAdd 90, 65535, 90

L_0764:
    Cmd_02D5 26, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0785
    ListMenuAdd 91, 65535, 91

L_0785:
    Cmd_02D5 35, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07A6
    ListMenuAdd 100, 65535, 100

L_07A6:
    Cmd_02D5 14, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C7
    ListMenuAdd 79, 65535, 79

L_07C7:
    Cmd_02D5 53, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07E8
    ListMenuAdd 118, 65535, 118

L_07E8:
    ListMenuAdd 59, 65535, 59
    ListMenuShow
    WorkCmpConst 0x8022, 85
    VMJumpIf CMP_EQ, L_0805
    VMJump L_0815

L_0805:
    // "Brock!\nHe's the Gym Leader of Pewter City.[f000]븁\u0000\nHe uses Rock-type Pokémon. I heard\nhe's proud of his strong defense.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BrockHesGymLeader, 2, 0
    VMJump L_0956

L_0815:
    WorkCmpConst 0x8022, 86
    VMJumpIf CMP_EQ, L_0828
    VMJump L_0838

L_0828:
    // "Misty![f000]븁\u0000\nMisty is the Gym Leader of\nCerulean City.[f000]븁\u0000\nAs she's called the Tomboyish Mermaid,\nshe's an active, beautiful girl![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_MistyMistyGymLeader, 2, 0
    VMJump L_0956

L_0838:
    WorkCmpConst 0x8022, 87
    VMJumpIf CMP_EQ, L_084B
    VMJump L_085B

L_084B:
    // "Lt. Surge![f000]븁\u0000\nHe used to be a soldier and acts as\nthe Gym Leader of Vermilion City.[f000]븁\u0000\nIs he from the Unova region?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_LtSurgeHeUsed, 2, 0
    VMJump L_0956

L_085B:
    WorkCmpConst 0x8022, 88
    VMJumpIf CMP_EQ, L_086E
    VMJump L_087E

L_086E:
    // "Lady Erika![f000]븁\u0000\nShe's a lady who is the Gym Leader\nof Celadon City![f000]븁\u0000\nIs her hobby flower arrangement?[f000]븁\u0000\nShe dozes off quite often, but her\nlaid-back personality is part of[f000]븀\u0000\nher charm.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_LadyErikaShesLady, 2, 0
    VMJump L_0956

L_087E:
    WorkCmpConst 0x8022, 89
    VMJumpIf CMP_EQ, L_0891
    VMJump L_08A1

L_0891:
    // "Queen Sabrina![f000]븁\u0000\nShe's a psychic and the Gym Leader\nthat Saffron City is proud of![f000]븁\u0000\nAww, calling her “queen\" fits her image\nfor me.[f000]븁\u0000\nAh! I want her to see my future\nby Future Sight![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_QueenSabrinaShesPsychic, 2, 0
    VMJump L_0956

L_08A1:
    WorkCmpConst 0x8022, 90
    VMJumpIf CMP_EQ, L_08B4
    VMJump L_08C4

L_08B4:
    // "Blaine!\nThe Hotheaded Quiz Master.[f000]븁\u0000\nHe's the Gym Leader of Cinnabar Island,\nbut he had a tough time.[f000]븁\u0000\nThe original Gym was destroyed and\nhe had to go to Seafoam Islands.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BlaineHotheadedQuizMaster, 2, 0
    VMJump L_0956

L_08C4:
    WorkCmpConst 0x8022, 91
    VMJumpIf CMP_EQ, L_08D7
    VMJump L_08E7

L_08D7:
    // "Giovanni...[f000]븁\u0000\nI heard a lot of rumors about him,\nbut I don't know much about him.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_GiovanniHeardLotRumors, 2, 0
    VMJump L_0956

L_08E7:
    WorkCmpConst 0x8022, 100
    VMJumpIf CMP_EQ, L_08FA
    VMJump L_090A

L_08FA:
    // "Janine![f000]븁\u0000\nAgain.\nJanine![f000]븁\u0000\nShe's the Gym Leader of Fuchsia City,\nand her father is Koga![f000]븀\u0000\nHe's one of the Elite Four![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_JanineAgainJanineShes, 2, 0
    VMJump L_0956

L_090A:
    WorkCmpConst 0x8022, 79
    VMJumpIf CMP_EQ, L_091D
    VMJump L_092D

L_091D:
    // "Blue is a grandson of\nProfessor Oak.[f000]븁\u0000\nAnything else?[f000]븁\u0000\nOh, I believe he's a former Champion\nand the Gym Leader of Viridian City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BlueGrandsonProfessorOak, 2, 0
    VMJump L_0956

L_092D:
    WorkCmpConst 0x8022, 118
    VMJumpIf CMP_EQ, L_0940
    VMJump L_0950

L_0940:
    // "Red...?[f000]븁\u0000\nI don't know much about him,\nbut I heard he is a legendary Trainer.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_RedDontKnowMuch, 2, 0
    VMJump L_0956

L_0950:
    WorkSetConst 0x8020, 0

L_0956:
    VMJump L_0678

L_095C:
    VMReturn

L_095E:
    WorkSetConst 0x8020, 1

L_0964:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C04
    // "Which Trainer do you want to\nknow about?"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WhichTrainerWantKnow, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    Cmd_02D5 27, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09AB
    ListMenuAdd 92, 65535, 92

L_09AB:
    Cmd_02D5 28, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09CC
    ListMenuAdd 93, 65535, 93

L_09CC:
    Cmd_02D5 29, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09ED
    ListMenuAdd 94, 65535, 94

L_09ED:
    Cmd_02D5 30, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A0E
    ListMenuAdd 95, 65535, 95

L_0A0E:
    Cmd_02D5 31, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A2F
    ListMenuAdd 96, 65535, 96

L_0A2F:
    Cmd_02D5 32, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A50
    ListMenuAdd 97, 65535, 97

L_0A50:
    Cmd_02D5 33, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A71
    ListMenuAdd 98, 65535, 98

L_0A71:
    Cmd_02D5 34, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A92
    ListMenuAdd 99, 65535, 99

L_0A92:
    Cmd_02D5 15, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AB3
    ListMenuAdd 80, 65535, 80

L_0AB3:
    ListMenuAdd 59, 65535, 59
    ListMenuShow
    WorkCmpConst 0x8022, 92
    VMJumpIf CMP_EQ, L_0AD0
    VMJump L_0AE0

L_0AD0:
    // "Falkner![f000]븁\u0000\nHe's the Gym Leader of Violet City.\nHe's a young man who battles with[f000]븀\u0000\nPokémon that his father counted on.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_FalknerHesGymLeader, 2, 0
    VMJump L_0BFE

L_0AE0:
    WorkCmpConst 0x8022, 93
    VMJumpIf CMP_EQ, L_0AF3
    VMJump L_0B03

L_0AF3:
    // "Bugsy.[f000]븁\u0000\nHe's a Gym Leader who's called\n“The Walking Bug Pokémon Encyclopedia.\"[f000]븁\u0000\nOh! The town he's in is Azalea!\n...I think.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BugsyHesGymLeader, 2, 0
    VMJump L_0BFE

L_0B03:
    WorkCmpConst 0x8022, 94
    VMJumpIf CMP_EQ, L_0B16
    VMJump L_0B26

L_0B16:
    // "Whitney!\nShe's the Gym Leader of Goldenrod City![f000]븁\u0000\nI believe she loves softball and\nwears clothes that look like a uniform.[f000]븁\u0000\nThe first thing I think about her\nis Rollout![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WhitneyShesGymLeader, 2, 0
    VMJump L_0BFE

L_0B26:
    WorkCmpConst 0x8022, 95
    VMJumpIf CMP_EQ, L_0B39
    VMJump L_0B49

L_0B39:
    // "Morty! He's a stoic Gym Leader.\nHe's training to see the legendary[f000]븀\u0000\nPokémon in Ecruteak City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_MortyHesStoicGym, 2, 0
    VMJump L_0BFE

L_0B49:
    WorkCmpConst 0x8022, 96
    VMJumpIf CMP_EQ, L_0B5C
    VMJump L_0B6C

L_0B5C:
    // "Chuck.[f000]븁\u0000\nHis wife thinks he's getting chubby.\nHe's the Gym Leader of Cianwood City![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_ChuckHisWifeThinks, 2, 0
    VMJump L_0BFE

L_0B6C:
    WorkCmpConst 0x8022, 97
    VMJumpIf CMP_EQ, L_0B7F
    VMJump L_0B8F

L_0B7F:
    // "Jasmine![f000]븁\u0000\nShe's a compassionate woman of Steel![f000]븁\u0000\nShe's the Gym Leader of Olivine City.\nShe participates in Contests[f000]븀\u0000\nin the Sinnoh region.[f000]븁\u0000\nI heard she used to use\nRock-type Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_JasmineShesCompassionateWoman, 2, 0
    VMJump L_0BFE

L_0B8F:
    WorkCmpConst 0x8022, 98
    VMJumpIf CMP_EQ, L_0BA2
    VMJump L_0BB2

L_0BA2:
    // "Pryce.[f000]븁\u0000\nHe's the Gym Leader of Mahogany Town.\nHe's a cool gentleman![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_PryceHesGymLeader, 2, 0
    VMJump L_0BFE

L_0BB2:
    WorkCmpConst 0x8022, 99
    VMJumpIf CMP_EQ, L_0BC5
    VMJump L_0BD5

L_0BC5:
    // "Clair![f000]븁\u0000\nShe uses Dragon-type Pokémon, and she's\nthe Gym Leader of Blackthorn City![f000]븁\u0000\nI heard Lance is her senior when it comes\nto training, and she's no match for him![f000]븁\u0000\nI hope Clair will be stronger than\nLance very soon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_ClairSheUsesDragon, 2, 0
    VMJump L_0BFE

L_0BD5:
    WorkCmpConst 0x8022, 80
    VMJumpIf CMP_EQ, L_0BE8
    VMJump L_0BF8

L_0BE8:
    // "Lance.[f000]븁\u0000\nHe's the Champion of the Pokémon League\nin the Kanto region.[f000]븁\u0000\nI think that he uses Dragon-type Pokémon\nand that he's from Blackthorn City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_LanceHesChampionPokemon, 2, 0
    VMJump L_0BFE

L_0BF8:
    WorkSetConst 0x8020, 0

L_0BFE:
    VMJump L_0964

L_0C04:
    VMReturn

L_0C06:
    WorkSetConst 0x8020, 1

L_0C0C:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F34
    // "Which Trainer do you want to\nknow about?"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WhichTrainerWantKnow, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    Cmd_02D5 36, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C53
    ListMenuAdd 101, 65535, 101

L_0C53:
    Cmd_02D5 37, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C74
    ListMenuAdd 102, 65535, 102

L_0C74:
    Cmd_02D5 38, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C95
    ListMenuAdd 103, 65535, 103

L_0C95:
    Cmd_02D5 39, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CB6
    ListMenuAdd 104, 65535, 104

L_0CB6:
    Cmd_02D5 40, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CD7
    ListMenuAdd 105, 65535, 105

L_0CD7:
    Cmd_02D5 41, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CF8
    ListMenuAdd 106, 65535, 106

L_0CF8:
    Cmd_02D5 42, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D19
    ListMenuAdd 107, 65535, 107

L_0D19:
    Cmd_02D5 43, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D3A
    ListMenuAdd 108, 65535, 108

L_0D3A:
    Cmd_02D5 44, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D5B
    ListMenuAdd 109, 65535, 109

L_0D5B:
    Cmd_02D5 16, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D7C
    ListMenuAdd 81, 65535, 81

L_0D7C:
    Cmd_02D5 17, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D9D
    ListMenuAdd 82, 65535, 82

L_0D9D:
    ListMenuAdd 59, 65535, 59
    ListMenuShow
    WorkCmpConst 0x8022, 101
    VMJumpIf CMP_EQ, L_0DBA
    VMJump L_0DCA

L_0DBA:
    // "Roxanne![f000]븁\u0000\nShe's a teacher of a Trainers' School\nand also the Gym Leader of[f000]븀\u0000\nRustboro City![f000]븁\u0000\nIt's hard to tell how old she is--\nor any other women.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_RoxanneShesTeacherTrainers, 2, 0
    VMJump L_0F2E

L_0DCA:
    WorkCmpConst 0x8022, 102
    VMJumpIf CMP_EQ, L_0DDD
    VMJump L_0DED

L_0DDD:
    // "Brawly.\nHe's the Gym Leader of Dewford Town.[f000]븁\u0000\nIs he a Gym Leader or a surfer?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_BrawlyHesGymLeader, 2, 0
    VMJump L_0F2E

L_0DED:
    WorkCmpConst 0x8022, 103
    VMJumpIf CMP_EQ, L_0E00
    VMJump L_0E10

L_0E00:
    // "Wattson.[f000]븁\u0000\nHe's the Gym Leader of Mauville City.[f000]븁\u0000\nBeing cheerful all the time\nis the secret to his health.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WattsonHesGymLeader, 2, 0
    VMJump L_0F2E

L_0E10:
    WorkCmpConst 0x8022, 104
    VMJumpIf CMP_EQ, L_0E23
    VMJump L_0E33

L_0E23:
    // "Flannery![f000]븁\u0000\nShe's the Gym Leader of Lavaridge Town.\nI heard she loves hot springs![f000]븁\u0000\nAnd, I also heard her grandfather\nused to be one of the Elite Four![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_FlanneryShesGymLeader, 2, 0
    VMJump L_0F2E

L_0E33:
    WorkCmpConst 0x8022, 105
    VMJumpIf CMP_EQ, L_0E46
    VMJump L_0E56

L_0E46:
    // "Norman.[f000]븁\u0000\nHe's the Gym Leader of Petalburg City.\nHe is a friend of Professor Birch,[f000]븀\u0000\nwho is famous in the Hoenn region.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_NormanHesGymLeader, 2, 0
    VMJump L_0F2E

L_0E56:
    WorkCmpConst 0x8022, 106
    VMJumpIf CMP_EQ, L_0E69
    VMJump L_0E79

L_0E69:
    // "Winona![f000]븁\u0000\nShe's the Gym Leader of Fortree City.\nShe spreads her wings around the world.[f000]븁\u0000\nShe's a perfect Trainer\nfor this tournament![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WinonaShesGymLeader, 2, 0
    VMJump L_0F2E

L_0E79:
    WorkCmpConst 0x8022, 107
    VMJumpIf CMP_EQ, L_0E8C
    VMJump L_0E9C

L_0E8C:
    // "Tate is one of the Gym Leaders of\nMossdeep City.[f000]븁\u0000\nHe'll challenge you for Double Battle\nwith his twin, Liza.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_TateOneGymLeaders, 2, 0
    VMJump L_0F2E

L_0E9C:
    WorkCmpConst 0x8022, 108
    VMJumpIf CMP_EQ, L_0EAF
    VMJump L_0EBF

L_0EAF:
    // "Liza![f000]븁\u0000\nShe and her twin, Tate, are\nthe Gym Leaders of Mossdeep City.[f000]븁\u0000\nAs you expect of twins,\nthey're good at Double Battles![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_LizaSheHerTwin, 2, 0
    VMJump L_0F2E

L_0EBF:
    WorkCmpConst 0x8022, 109
    VMJumpIf CMP_EQ, L_0ED2
    VMJump L_0EE2

L_0ED2:
    // "Juan![f000]븁\u0000\nHe's Wallace's teacher\nand the Gym Leader of Sootopolis City.[f000]븁\u0000\nI guess he's a ladies man.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_JuanHesWallacesTeacher, 2, 0
    VMJump L_0F2E

L_0EE2:
    WorkCmpConst 0x8022, 81
    VMJumpIf CMP_EQ, L_0EF5
    VMJump L_0F05

L_0EF5:
    // "Steven is a son of a wealthy family\nand a former Champion of Hoenn.[f000]븁\u0000\nIt might ring a bell if I say\nhis hobby is to collect Stones![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_StevenSonWealthyFamily, 2, 0
    VMJump L_0F2E

L_0F05:
    WorkCmpConst 0x8022, 82
    VMJumpIf CMP_EQ, L_0F18
    VMJump L_0F28

L_0F18:
    // "Wallace is the Champion of Hoenn.\nHe was originally a Gym Leader.[f000]븁\u0000\nI wonder why he replaced Steven.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WallaceChampionHoennHe, 2, 0
    VMJump L_0F2E

L_0F28:
    WorkSetConst 0x8020, 0

L_0F2E:
    VMJump L_0C0C

L_0F34:
    VMReturn

L_0F36:
    WorkSetConst 0x8020, 1

L_0F3C:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11DC
    // "Which Trainer do you want to\nknow about?"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_WhichTrainerWantKnow, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32802
    Cmd_02D5 45, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F83
    ListMenuAdd 110, 65535, 110

L_0F83:
    Cmd_02D5 46, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FA4
    ListMenuAdd 111, 65535, 111

L_0FA4:
    Cmd_02D5 47, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FC5
    ListMenuAdd 112, 65535, 112

L_0FC5:
    Cmd_02D5 48, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FE6
    ListMenuAdd 113, 65535, 113

L_0FE6:
    Cmd_02D5 49, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1007
    ListMenuAdd 114, 65535, 114

L_1007:
    Cmd_02D5 50, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1028
    ListMenuAdd 115, 65535, 115

L_1028:
    Cmd_02D5 51, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1049
    ListMenuAdd 116, 65535, 116

L_1049:
    Cmd_02D5 52, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_106A
    ListMenuAdd 117, 65535, 117

L_106A:
    Cmd_02D5 18, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_108B
    ListMenuAdd 83, 65535, 83

L_108B:
    ListMenuAdd 59, 65535, 59
    ListMenuShow
    WorkCmpConst 0x8022, 110
    VMJumpIf CMP_EQ, L_10A8
    VMJump L_10B8

L_10A8:
    // "Roark.[f000]븁\u0000\nHe's the young Gym Leader\nof Oreburgh City.[f000]븁\u0000\nHe supervises people who work in\nthe mines in the city.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_RoarkHesYoungGym, 2, 0
    VMJump L_11D6

L_10B8:
    WorkCmpConst 0x8022, 111
    VMJumpIf CMP_EQ, L_10CB
    VMJump L_10DB

L_10CB:
    // "Gardenia!\nI heard she's afraid of ghosts![f000]븁\u0000\nShe's the respectable Gym Leader\nof Eterna City, though![f000]븁\u0000\nHaving said that, everybody has\nsomething they don't like, right?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_GardeniaHeardShesAfraid, 2, 0
    VMJump L_11D6

L_10DB:
    WorkCmpConst 0x8022, 112
    VMJumpIf CMP_EQ, L_10EE
    VMJump L_10FE

L_10EE:
    // "Fantina![f000]븁\u0000\nShe tends to be misunderstood\ndue to her flashy appearance,[f000]븀\u0000\nbut she's a Gym Leader[f000]븀\u0000\nwith a compassionate heart![f000]븁\u0000\nShe consoles Pokémon's spirits\nat the Lost Tower near Hearthome City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_FantinaSheTendsMisunderstood, 2, 0
    VMJump L_11D6

L_10FE:
    WorkCmpConst 0x8022, 113
    VMJumpIf CMP_EQ, L_1111
    VMJump L_1121

L_1111:
    // "Maylene!\nThe barefoot fighting genius![f000]븁\u0000\nShe's the Gym Leader of Veilstone City\nand very serious about learning[f000]븀\u0000\nwhat it is to be strong![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_MayleneBarefootFightingGenius, 2, 0
    VMJump L_11D6

L_1121:
    WorkCmpConst 0x8022, 114
    VMJumpIf CMP_EQ, L_1134
    VMJump L_1144

L_1134:
    // "Crasher Wake![f000]븁\u0000\nHe's a professional wrestler and\na Gym Leader.[f000]븁\u0000\nHe's an admirable man who uses his\nprize money for Pastoria City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_CrasherWakeHesProfessional, 2, 0
    VMJump L_11D6

L_1144:
    WorkCmpConst 0x8022, 115
    VMJumpIf CMP_EQ, L_1157
    VMJump L_1167

L_1157:
    // "Byron!\nHe's the Gym Leader of Canalave City.[f000]븀\u0000\nHe always carries a shovel.[f000]븁\u0000\nBy the way, he's the father of Roark,\nwho's the Gym Leader of[f000]븀\u0000\nOreburgh City.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_ByronHesGymLeader, 2, 0
    VMJump L_11D6

L_1167:
    WorkCmpConst 0x8022, 116
    VMJumpIf CMP_EQ, L_117A
    VMJump L_118A

L_117A:
    // "Candice![f000]븁\u0000\nShe's the spirited Gym Leader\nof Snowpoint City![f000]븁\u0000\nShe also guards the temple\nin the city![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_CandiceShesSpiritedGym, 2, 0
    VMJump L_11D6

L_118A:
    WorkCmpConst 0x8022, 117
    VMJumpIf CMP_EQ, L_119D
    VMJump L_11AD

L_119D:
    // "Volkner...[f000]븁\u0000\nHe's the Leader of Sunyshore City.\nI heard he renovated the Pokémon Gym[f000]븀\u0000\nand caused blackouts in the city.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_VolknerHesLeaderSunyshore, 2, 0
    VMJump L_11D6

L_11AD:
    WorkCmpConst 0x8022, 83
    VMJumpIf CMP_EQ, L_11C0
    VMJump L_11D0

L_11C0:
    // "Cynthia!![f000]븁\u0000\nShe's the Champion of the Sinnoh region\nand an archeologist![f000]븁\u0000\nShe balances the use of various types\nof Pokémon![f000]븁\u0000\nRumor has it that she comes to a villa\nin Undella Town for her research![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10675_Text_CynthiaShesChampionSinnoh, 2, 0
    VMJump L_11D6

L_11D0:
    WorkSetConst 0x8020, 0

L_11D6:
    VMJump L_0F3C

L_11DC:
    VMReturn

L_11DE:
    WorkSetConst 0x8021, 0
    Cmd_02D5 0, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 1, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 2, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 3, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 4, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 5, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 6, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 7, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 8, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 9, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 10, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 11, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 12, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 13, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 19, 0x8010
    WorkAdd 0x8021, 0x8010
    VMReturn

L_129A:
    WorkSetConst 0x8021, 0
    Cmd_02D5 20, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 21, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 22, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 23, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 24, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 25, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 26, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 35, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 14, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 53, 0x8010
    WorkAdd 0x8021, 0x8010
    VMReturn

L_131A:
    WorkSetConst 0x8021, 0
    Cmd_02D5 27, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 28, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 29, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 30, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 31, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 32, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 33, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 34, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 15, 0x8010
    WorkAdd 0x8021, 0x8010
    VMReturn

L_138E:
    WorkSetConst 0x8021, 0
    Cmd_02D5 36, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 37, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 38, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 39, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 40, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 41, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 42, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 43, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 44, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 16, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 17, 0x8010
    WorkAdd 0x8021, 0x8010
    VMReturn

L_141A:
    WorkSetConst 0x8021, 0
    Cmd_02D5 45, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 46, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 47, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 48, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 49, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 50, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 51, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 52, 0x8010
    WorkAdd 0x8021, 0x8010
    Cmd_02D5 18, 0x8010
    WorkAdd 0x8021, 0x8010
    VMReturn
    .balign 4, 0
