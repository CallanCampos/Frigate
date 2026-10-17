#include "asm/field_script.inc"
#include "text/script/lentimas_town.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd

Script_9:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    VMStackPush EVENT_WORK_0x40cb
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0045
    CallPlaceNameDisp
    DebugPrint 22

L_0045:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    CallPlaceNameDisp
    VMSleep 70
    // "Professor Juniper: Well, I suppose\nI should tell you why[f000]븀\u0000\nI brought you out here.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LentimasTown_Text_ProfessorJuniperWellSuppose, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0278
    ActorCmdWait
    // "Cheren told me that a group of people\ncalling themselves Team Plasma[f000]븀\u0000\nare planning to use legendary Pokémon[f000]븀\u0000\nto take over the Unova region.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LentimasTown_Text_CherenToldGroupPeople, 2, 0, 0
    // "As you may know, in the Unova region,\nthere are two legendary[f000]븀\u0000\nDragon-type Pokémon:[f000]븀\u0000\nReshiram and Zekrom...[f000]븁\u0000"
    // "As you may know, in the Unova region,\nthere are two legendary[f000]븀\u0000\nDragon-type Pokémon:[f000]븀\u0000\nZekrom and Reshiram...[f000]븁\u0000"
    ActorMsgVersioned 1024, LentimasTown_Text_MayKnowUnovaRegion_2, LentimasTown_Text_MayKnowUnovaRegion, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0290
    ActorCmdWait
    // "But two years ago, Reshiram and Zekrom\neach recognized a Trainer as a hero.[f000]븀\u0000\nThey are following those Trainers.[f000]븁\u0000\nSo Team Plasma shouldn't be able\nto use the Dragon-type Pokémon...[f000]븁\u0000"
    // "But two years ago, Zekrom and Reshiram\neach recognized a Trainer as a hero.[f000]븀\u0000\nThey are following those Trainers.[f000]븁\u0000\nSo Team Plasma shouldn't be able\nto use the Dragon-type Pokémon...[f000]븁\u0000"
    ActorMsgVersioned 1024, LentimasTown_Text_ButTwoYearsAgo_2, LentimasTown_Text_ButTwoYearsAgo, 2, 0, 0
    MsgWinCloseAll
    // "Bianca: That's true...\nWhat could they be planning?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LentimasTown_Text_BiancaThatsTrueWhat, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0278
    ActorCmdWait
    // "Professor Juniper: There is much we\ndon't know about Reshiram and Zekrom...[f000]븁\u0000\nThat's why I want to hear what\nOpelucid City's Gym Leader, Drayden,[f000]븀\u0000\nhas to say about this.[f000]븁\u0000\nHe's a Dragon-type Gym Leader,\nso he might know something.[f000]븁\u0000\nSo, we're finally to the reason\nwhy I brought you here.[f000]븁\u0000\nI want you to go to Opelucid City\nand hear what Drayden has to say.[f000]븁\u0000\nAnd, I would also like you\nto help me if something happens!"
    // "Professor Juniper: There is much we\ndon't know about Zekrom and Reshiram...[f000]븁\u0000\nThat's why I want to hear what\nOpelucid City's Gym Leader, Drayden,[f000]븀\u0000\nhas to say about this.[f000]븁\u0000\nHe's a Dragon-type Gym Leader,\nso he might know something.[f000]븁\u0000\nSo, we're finally to the reason\nwhy I brought you here.[f000]븁\u0000\nI want you to go to Opelucid City\nand hear what Drayden has to say.[f000]븁\u0000\nAnd, I would also like you\nto help me if something happens!"
    ActorMsgVersioned 1024, LentimasTown_Text_ProfessorJuniperThereMuch_2, LentimasTown_Text_ProfessorJuniperThereMuch, 2, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F0
    // "Thank you![f000]븁\u0000\nIt's really best not to get\ninvolved with Team Plasma[f000]븀\u0000\nto start with.[f000]븁\u0000\nBut still...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LentimasTown_Text_ThankItsReallyBest, 2, 0, 0
    VMJump L_00FC

L_00F0:
    // "Yes.\nThat is the more reasonable response.[f000]븁\u0000\nIt's best not to get involved\nwith Team Plasma.[f000]븁\u0000\nBut...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LentimasTown_Text_YesMoreReasonableResponse, 2, 0, 0

L_00FC:
    // "Hearing what Drayden has to say\nabout the Dragon-type Pokémon[f000]븀\u0000\nwill be really interesting.[f000]븁\u0000\nAnd more importantly, it will help\nfill up the pages of your Pokédex![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LentimasTown_Text_HearingWhatDraydenHas, 2, 0, 0
    MsgWinCloseAll
    // "Bianca: I'll be in the volcano\njust beyond here...[f000]븁\u0000\nI'm going to investigate the rumors\nI've been hearing about a rare[f000]븀\u0000\nFire-type Pokémon in Reversal Mountain.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LentimasTown_Text_BiancaIllVolcanoJust, 5, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 5, 626, 306, 1, 8, 1
    VMSleep 8
    ActorCmdExec 255, Movement_0278
    ActorCmdWait
    ActorDelete 5
    FlagSet EVENT_FLAG_0x0309
    WorkSetConst EVENT_WORK_0x40cb, 2
    WorkSetConst EVENT_WORK_0x4120, 1
    HollowRivalCmd_0262 3, 4
    HollowRivalCmd_0262 0, 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Deliver a lot of cargo quickly!\nThis is Lentimas Cargo Service.[f000]븁\u0000\nWould you like to board the plane\nand fly back to Mistralton City?"
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_DeliverLotCargoQuickly, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B3
    // "OK! Please board the plane and wait![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_OkPleaseBoardPlane, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    Call3DDemo 25, 0
    FieldOpen
    RTReserveScript 11
    MapChangeCore ZONE_MISTRALTON_CITY_3, 14, 0, 19, 1
    VMJump L_01C1

L_01B3:
    // "OK!\nFeel free to ride anytime!"
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_OkFeelFreeRide, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01C1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Drayden is a Gym Leader,\nso unless you defeat him in a Pokémon[f000]븀\u0000\nbattle, he probably won't talk to you."
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_DraydenGymLeaderUnless, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If the season changes,\nthe scenery from the plane changes.[f000]븁\u0000\nIf the climate changes,\nthe local architecture changes!"
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_IfSeasonChangesScenery, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Why am I on a journey?[f000]븁\u0000\nDo you remember exactly\nwhy you're traveling?"
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_WhyAmJourneyRemember, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm fine even right by a volcano\nthanks to my li'l Krokorok![f000]븀\u0000\nIt really rocks!"
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_ImFineEvenRight, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Rah feh feh!"
    ParentActorMsg MSGFILE_SCRIPT, LentimasTown_Text_RahFehFeh, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Lentimas Town\nWhere Rough Mountain Trails Lead"
    MsgPlaceSign LentimasTown_Text_LentimasTownWhereRough, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0278:
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_0290:
    Move 33, 1
    MoveEnd
