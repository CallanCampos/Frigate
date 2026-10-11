#include "asm/field_script.inc"
#include "text/script/driftveil_city_6.h"

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

Script_15:
    Cmd_02B2 0, EVENT_WORK_0x400f
    DebugPrint EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x404a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x413f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00AB
    FlagSet EVENT_FLAG_0x02ca
    FlagSet EVENT_FLAG_0x02b8
    FlagSet EVENT_FLAG_0x03ce
    FlagReset EVENT_FLAG_0x02b9
    WorkSetConst EVENT_WORK_0x413f, 2

L_00AB:
    VMStackPushFlag EVENT_FLAG_0x01a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01eb
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00DA
    HollowRivalCmd_0262 1, 43
    VMJump L_0103

L_00DA:
    VMStackPushFlag EVENT_FLAG_0x01a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01eb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0103
    HollowRivalCmd_0262 1, 42

L_0103:
    VMHalt

Script_16:
    ActorsPauseAll
    ActorWalkRoute 13, 7, 21, 1, 16, 1
    ActorCmdWait
    // "Kwip yip!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_KwipYip, 13, 0, 0
    PVPlay 571, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_DRIFTVEIL_CITY_17, 7, 0, 18, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    FadeInBlack
    FadeWait
    WorkSetConst EVENT_WORK_0x413f, 3
    WorkSetConst EVENT_WORK_0x404a, 1
    FlagReset EVENT_FLAG_0x02b8
    VMStackPushFlag EVENT_FLAG_0x0121
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0176
    FlagReset EVENT_FLAG_0x02ca

L_0176:
    VMStackPushFlag EVENT_FLAG_0x01a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018D
    FlagReset EVENT_FLAG_0x03ce

L_018D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    ActorWalkRoute 255, 7, 20, 1, 8, 0
    VMSleep 16
    SEPlay SEQ_SE_KAIDAN
    ActorNew 7, 25, 0, 251, 291, 0
    SEWait
    ActorCmdWait
    ActorWalkRoute 251, 6, 20, 1, 8, 0
    ActorCmdWait
    // "Let me introduce myself again.\nMy name is Rood.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_LetIntroduceMyselfAgain, 0, 3, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: You guys are all\nTeam Plasma too, right?[f000]븁\u0000\nTell me, what makes you different\nfrom the Team Plasma back there?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_GuysAllTeamPlasma, 251, 0, 0
    MsgWinCloseAll
    // "Rood: More accurately, we're\nformer members of Team Plasma.[f000]븁\u0000\nBecause of the incident two years ago,\nwe started taking care of the Pokémon[f000]븀\u0000\nthat were separated from their Trainers[f000]븀\u0000\nas a way to atone for our misdeeds.[f000]븁\u0000\nAnd you are?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodMoreAccuratelyWere, 0, 3, 0
    MsgWinCloseAll
    // "I'm [f000]Ā\u0001\u0001.\nFrom Aspertia City...[f000]븁\u0000\nFive years ago, Team Plasma--I mean\nyou--stole my little sister's Pokémon.[f000]븁\u0000\nI'm the pathetic Trainer who wasn't able\nto stop you.[f000]븁\u0000\n“Separated?\" What a joke!\nYOU were the thieves who STOLE them![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_ImFromAspertiaCity, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0A68
    ActorCmdWait
    // "Rood: Is that so...\nMy sincerest apologies...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodSincerestApologies, 0, 3, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_06E4
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Just an apology?\nThat's it?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_JustApologyThats, 251, 0, 0
    MsgWinCloseAll
    // "Where's my sister's Pokémon?![f000]븁\u0000\nPurrloin! WHERE'S PURRLOIN?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_WheresSistersPokemonPurrloin, 251, 0, 1
    MsgWinCloseAll
    // "Rood: The Pokémon you speak of\nis not in this place.[f000]븁\u0000\nI imagine it is still being\nused by Team Plasma now.[f000]븁\u0000\nJust as you say, our apologizing\ndoesn't solve anything.[f000]븁\u0000\nBut you can't move forward unless\nyou admit you were wrong and apologize...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodPokemonSpeakNot, 0, 3, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0001: Enough already![f000]븁\u0000\nApologizing isn't going to get\nmy sister's Pokémon back![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_EnoughAlreadyApologizingIsnt, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0A60
    VMSleep 4
    ActorCmdExec 255, Movement_0A58
    ActorCmdWait
    // "[f000]Ā\u0001\u0000!\nI'm going to the Pokémon Gym![f000]븁\u0000\nI'll get stronger and crush every\nsingle member of Team Plasma![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_ImGoingPokemonGym, 251, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 251, 6, 25, 1, 4, 0
    VMSleep 8
    ActorCmdExec 255, Movement_0A50
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    ActorCmdExec 0, Movement_0A78
    VMSleep 8
    ActorCmdExec 255, Movement_0A48
    ActorCmdWait
    // "Rood: Team Plasma made\nTrainers like him suffer...[f000]븁\u0000\nI feel terrible about it...\nHow foolish we were...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodTeamPlasmaMade, 0, 3, 0
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000, as you can see,\nI can't do anything to thank you.[f000]븁\u0000\nActually, I have a favor to ask of you.[f000]븁\u0000\nCould you please look after this\nPokémon, Zorua?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_CanSeeCantAnything, 0, 3, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0310
    // "Rood: If you change your mind,\nplease come speak to me again."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodIfChangeMind, 0, 3, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0316

L_0310:
    VMCall L_03A6

L_0316:
    FlagSet EVENT_FLAG_0x02cf
    WorkSetConst EVENT_WORK_0x413f, 1
    HollowRivalCmd_0262 1, 10
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0121
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_038C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Rood: Could you please look after this\nPokémon, Zorua?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodCouldPleaseLook, 0, 3, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0380
    // "Rood: If you change your mind,\nplease come speak to me again."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodIfChangeMind, 0, 3, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0386

L_0380:
    VMCall L_03A6

L_0386:
    VMJump L_03A0

L_038C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My lord N is a wonderful person\nwho has the power to understand[f000]븀\u0000\nthe hearts of Pokémon.[f000]븁\u0000\nBut still, he has much to learn about\nunderstanding the hearts of people...[f000]븁\u0000\nI hope he will develop this skill while\nhe travels with the legendary Pokémon[f000]븀\u0000\nto atone for the trouble he caused[f000]븀\u0000\nin Unova as the king of Team Plasma."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_LordNWonderfulPerson, 0, 0
    LastKeyWait
    ActorMsgClose

L_03A0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_03A6:
    WorkSetConst 0x8023, 0
    PokePartyGetCount 0x8023, 0
    VMStackPush 0x8023
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DB
    // "Rood: Oh! Thank you![f000]븁\u0000\n...But what's this? It seems you can't\ntake any more Pokémon with you."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodOhThankBut, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0695

L_03DB:
    ActorCmdExec 0, Movement_0A60
    ActorCmdWait
    // "Rood: Oh! Thank you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_RoodOhThank, 0, 3, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 20
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0430
    ActorWalkRoute 1, 7, 19, 1, 8, 1
    VMJump L_047B

L_0430:
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0467
    ActorWalkRoute 1, 6, 17, 1, 8, 1
    VMJump L_047B

L_0467:
    WorkAdd 0x8021, 1
    ActorWalkRoute 1, 0x8021, 0x8022, 1, 8, 1

L_047B:
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04B4
    ActorCmdExec 0, Movement_0A58
    VMJump L_051E

L_04B4:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04E5
    ActorCmdExec 0, Movement_0A48
    VMJump L_051E

L_04E5:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 19
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0516
    ActorCmdExec 0, Movement_0A50
    VMJump L_051E

L_0516:
    ActorCmdExec 0, Movement_0A50

L_051E:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 20
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_054F
    ActorCmdExec 1, Movement_0A50
    VMJump L_0598

L_054F:
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0588
    ActorCmdExec 1, Movement_0A50
    ActorCmdExec 255, Movement_0A48
    VMJump L_0598

L_0588:
    ActorCmdExec 1, Movement_0A58
    ActorCmdExec 255, Movement_0A60

L_0598:
    ActorCmdWait
    VMSleep 32
    ActorDelete 1
    PokePartyAddNPoke 0x8010, 570, 25, 11, 0, 0
    WordSetPlayerName 0
    MEPlay SEQ_ME_POKEGET
    // "[f000]Ā\u0001\u0000 received Zorua!"
    SystemMsg DriftveilCity6_Text_ReceivedZorua, 0
    MEWait
    MsgWaitAdvance
    InfoMsgClose
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 18
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05F4
    ActorCmdExec 255, Movement_0A60
    VMJump L_0650

L_05F4:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0625
    ActorCmdExec 255, Movement_0A50
    VMJump L_0650

L_0625:
    VMStackPush 0x8021
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 19
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0650
    ActorCmdExec 255, Movement_0A48

L_0650:
    ActorCmdWait
    Cmd_02B2 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_067D
    // "This Zorua was one of my lord N's friends,\nand it supported him.[f000]븁\u0000\nIn the Unova region, there are many\nother Pokémon that helped my lord N[f000]븀\u0000\nbesides this Zorua."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_ZoruaOneLordNs, 0, 3, 0
    VMJump L_0689

L_067D:
    // "That Zorua is one of the Pokémon\nthat my lord N relied on as a friend[f000]븀\u0000\nduring his journey."
    ActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_ZoruaOnePokemonLord, 0, 3, 0

L_0689:
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0121
    FlagSet EVENT_FLAG_0x02ca

L_0695:
    WorkSetConst 0x8023, 0
    VMReturn
    PlayerGetGPos 0x8021, 0x8022
    Cmd_020F 1, 8, 3, 18
    Cmd_0211 1
    FadeEx 12, 0, 16, 2
    FadeExWait
    ActorDelete 1
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 1
    Cmd_020E 1, 0x8021, 3, 0x8022, 3, 8
    VMReturn
    .balign 4, 0

Movement_06E4:
    Move 36, 2
    MoveEnd

Script_3:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_071B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma is an organization\ncreated by a man named Ghetsis[f000]븀\u0000\nto help him take over the Unova region.[f000]븁\u0000\nThe one he groomed to help him\nfurther his nefarious aims was N.[f000]븁\u0000\nN was a strange boy who was\ncalled the child of the Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_TeamPlasmaOrganizationCreated, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_072F

L_071B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if N understands now.[f000]븁\u0000\nTrainers battle with Pokémon not to hurt\nthem, but so Trainers and Pokémon can[f000]븀\u0000\nunderstand one another better![f000]븁\u0000\nIt's the simplest way\nfor them to do this.[f000]븁\u0000\nThe more serious the battle,\nthe more the true nature of Pokémon[f000]븀\u0000\nand people becomes apparent!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_WonderIfNUnderstands, 0, 0
    LastKeyWait
    ActorMsgClose

L_072F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0764
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "N was an orphan.[f000]븁\u0000\nI heard that right after he was born,\nhe upset people with behavior that[f000]븀\u0000\nsuggested he could talk to Pokémon.[f000]븁\u0000\nWhen he was living in the woods\nwith Darmanitan and Zorua,[f000]븀\u0000\nGhetsis took him in.[f000]븁\u0000\nWe are also orphans Ghetsis took in.\nOur task was to take care of N."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_NOrphanHeardRight, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0778

L_0764:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't understand N's\npure and innocent feelings.[f000]븁\u0000\nBut I will be very happy if he\nfigured out what he wants to do[f000]븀\u0000\non his own during his travels[f000]븀\u0000\nwith the legendary Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_DontUnderstandNsPure, 0, 0
    LastKeyWait
    ActorMsgClose

L_0778:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm waiting for my lord N to return.\nHe can talk to Pokémon.[f000]븁\u0000\nIf he comes back, we can find out\nwhat the Pokémon here want."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_ImWaitingLordN, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We're taking care of the Pokémon\nwhose Trainers we can't find.[f000]븁\u0000\nI know it seems arrogant,\nbut it's a small way to make up[f000]븀\u0000\nfor what we've done."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_WereTakingCarePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can't change the past,\nbut you can change the future![f000]븁\u0000\nThat's why I changed my outfit.\nI can still fit into the old one, though![f000]븀\u0000\nReally!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_CantChangePastBut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This Pokémon has become attached to me.[f000]븁\u0000\nThat's why I'm treating it like\na friend and not like a tool!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_PokemonHasBecomeAttached, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 507, 0
    // "Bwoaf bowoaf!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_BwoafBowoaf, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 596, 0
    // "Swwaaa!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_Swwaaa, 0, 0
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
    PVPlay 559, 0
    // "Scrarara!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_Scrarara, 0, 0
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
    PVPlay 504, 0
    // "Skreeree..."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_Skreeree, 0, 0
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
    PVPlay 570, 0
    // "Yeowwln!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_Yeowwln, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPushFlag EVENT_FLAG_0x01ae
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0912
    FlagSet EVENT_FLAG_0x01ae
    // "[f000]Ā\u0001\u0001: What?[f000]븁\u0000\nI heard that Team Plasma left\nmany Pokémon behind when they fled.[f000]븁\u0000\nAnd I'm helping find their\nreal Trainers...[f000]븁\u0000\nThat aside...[f000]븁\u0000\nSince you're here, you should have a\nbattle with me before you go!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_WhatHeardTeamPlasma, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08FE
    VMCall L_097A
    HollowRivalCmd_0262 1, 43
    FlagSet EVENT_FLAG_0x01eb
    VMJump L_090C

L_08FE:
    // "[f000]Ā\u0001\u0001: Well, that's all right\ntoo, I guess!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_WellThatsAllRight, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_090C:
    VMJump L_0974

L_0912:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0adb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0966
    // "[f000]Ā\u0001\u0001: Hey, [f000]Ā\u0001\u0000,\nhave a battle with me before you go!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_HeyHaveBattleBefore, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0952
    VMCall L_097A
    VMJump L_0960

L_0952:
    // "[f000]Ā\u0001\u0001: Well, that's all right\ntoo, I guess!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_WellThatsAllRight, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0960:
    VMJump L_0974

L_0966:
    // "[f000]Ā\u0001\u0001: I suppose that's it.[f000]븁\u0000\nIf winning in battles is strength,\nthen believing that your Pokémon[f000]븀\u0000\nwill come back and waiting for its return[f000]븀\u0000\nis also strength.[f000]븁\u0000\nDoing what you think is right\nno matter what anyone else says,[f000]븀\u0000\nlike these guys do, is strength, too.[f000]븁\u0000\n[f000]Ā\u0001\u0000!\nCome back tomorrow![f000]븁\u0000\nI'll take you on again!\nThat's my way of saying thanks!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_SupposeThatsIfWinning, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0974:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_097A:
    // "[f000]Ā\u0001\u0001: Here I come![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_HereCome, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09A7
    CallTrainerBattle TRAINER_RIVAL_22, 0, 0
    VMJump L_09D0

L_09A7:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09C8
    CallTrainerBattle TRAINER_RIVAL_23, 0, 0
    VMJump L_09D0

L_09C8:
    CallTrainerBattle TRAINER_RIVAL_24, 0, 0

L_09D0:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09EF
    CallTrainerBattleEnd
    VMJump L_09F1

L_09EF:
    CallTrainerLose

L_09F1:
    // "[f000]Ā\u0001\u0001: I suppose that's it.[f000]븁\u0000\nIf winning in battles is strength,\nthen believing that your Pokémon[f000]븀\u0000\nwill come back and waiting for its return[f000]븀\u0000\nis also strength.[f000]븁\u0000\nDoing what you think is right\nno matter what anyone else says,[f000]븀\u0000\nlike these guys do, is strength, too.[f000]븁\u0000\n[f000]Ā\u0001\u0000!\nCome back tomorrow![f000]븁\u0000\nI'll take you on again!\nThat's my way of saying thanks!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity6_Text_SupposeThatsIfWinning, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0adb
    VMReturn
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
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

Movement_0A48:
    Move 32, 1
    MoveEnd

Movement_0A50:
    Move 33, 1
    MoveEnd

Movement_0A58:
    Move 34, 1
    MoveEnd

Movement_0A60:
    Move 35, 1
    MoveEnd

Movement_0A68:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_0A78:
    Move 161, 1
    MoveEnd
