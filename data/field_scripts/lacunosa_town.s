#include "asm/field_script.inc"
#include "text/script/lacunosa_town.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

Script_1:
    RTCGetDayPart 0x8023
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_009B
    FlagSet EVENT_FLAG_0x0313
    FlagSet EVENT_FLAG_0x030c
    VMJump L_00A3

L_009B:
    FlagReset EVENT_FLAG_0x0313
    FlagReset EVENT_FLAG_0x030c

L_00A3:
    VMStackPush EVENT_WORK_0x40cc
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00BE
    DebugPrint 2323
    FlagSet EVENT_FLAG_0x030c

L_00BE:
    VMStackPush EVENT_WORK_0x40cc
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E9
    ObjInitNPCGPos 6, 1, 655, 0, 170
    ObjInitNPCGPos 9, 0, 655, 0, 171

L_00E9:
    VMStackPush EVENT_WORK_0x40ce
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0120
    ObjInitNPCGPos 12, 2, 647, 0, 186
    ObjInitNPCGPos 10, 3, 645, 0, 186
    ObjInitNPCGPos 11, 3, 645, 0, 185

L_0120:
    VMHalt

Script_2:
    VMStackPush EVENT_WORK_0x40cc
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014D
    ActorSetGPos 6, 655, 0, 170, 1
    ActorSetGPos 9, 655, 0, 171, 0

L_014D:
    VMStackPush EVENT_WORK_0x40ce
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0184
    ActorSetGPos 12, 647, 0, 186, 2
    ActorSetGPos 10, 645, 0, 186, 3
    ActorSetGPos 11, 645, 0, 185, 3

L_0184:
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40cc, 1
    FlagSet EVENT_FLAG_0x03c0
    FlagSet EVENT_FLAG_0x0308
    FlagSet EVENT_FLAG_ARRIVED_LACUNOSA_TOWN
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 667
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DD
    ActorWalkRoute 6, 664, 171, 1, 8, 0
    ActorWalkRoute 9, 664, 170, 1, 8, 0
    ActorCmdWait
    WorkSetConst 0x8025, 0
    VMJump L_0219

L_01DD:
    ActorSetGPos 6, 669, 0, 173, 1
    ActorSetGPos 9, 670, 0, 173, 1
    ActorWalkRoute 6, 669, 181, 1, 8, 1
    ActorWalkRoute 9, 670, 181, 1, 8, 1
    ActorCmdWait
    WorkSetConst 0x8025, 3

L_0219:
    WordSetPlayerName 0
    // "Professor Juniper: Hi there, [f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperHiThere, 6, 0x8025, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 667
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029F
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0258
    PlayerSetSpecialSequence 1

L_0258:
    VMStackPush 0x8022
    VMStackPushConst 171
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_027F
    ActorWalkRoute 255, 666, 171, 1, 8, 0
    VMJump L_0297

L_027F:
    ActorWalkRoute 255, 666, 171, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 255, Movement_0F84

L_0297:
    ActorCmdWait
    VMJump L_0301

L_029F:
    ActorCmdExec 255, Movement_0FDC
    ActorCmdWait
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C4
    PlayerSetSpecialSequence 1

L_02C4:
    VMStackPush 0x8021
    VMStackPushConst 669
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E7
    ActorCmdExec 255, Movement_0F8C
    ActorCmdWait
    VMJump L_0301

L_02E7:
    ActorWalkRoute 255, 669, 183, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0F8C
    ActorCmdWait

L_0301:
    // "Bianca: Hee hee![f000]븁\u0000\nI used Fly, so it looks like\nI beat you here.[f000]븁\u0000\nThanks for your help\nin Reversal Mountain![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BiancaHeeHeeUsed, 9, 0, 0
    MsgWinCloseAll
    // "Professor Juniper: If you go straight\npast Lacunosa Town, you'll reach[f000]븀\u0000\nOpelucid City![f000]븁\u0000\nBut before you go, there's something\nI want you two to hear.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperIfGo, 6, 0x8025, 0
    MsgWinCloseAll
    VMStackPush 0x8021
    VMStackPushConst 667
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033E
    ActorCmdExec 9, Movement_0F94
    VMJump L_0346

L_033E:
    ActorCmdExec 9, Movement_0F84

L_0346:
    ActorCmdWait
    // "Bianca: What is it?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BiancaWhat, 9, 0, 0
    MsgWinCloseAll
    // "Professor Juniper: You'll know soon\nenough. Hurry now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperYoullKnow, 6, 0x8025, 0
    MsgWinCloseAll
    RTCGetDayPart 0x8023
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0391
    VMJump L_039F

L_0391:
    ActorNew 652, 170, 1, 251, 29, 0

L_039F:
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 171
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D8
    ActorCmdExec 6, Movement_0EEC
    ActorCmdExec 9, Movement_0EF8
    ActorCmdExec 255, Movement_0F0C
    ActorCmdWait
    VMJump L_03F2

L_03D8:
    ActorCmdExec 6, Movement_0F28
    ActorCmdExec 9, Movement_0F38
    ActorCmdExec 255, Movement_0F50
    ActorCmdWait

L_03F2:
    RTCGetDayPart 0x8023
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_046F
    // "Professor Juniper: This is the place![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperPlace, 6, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 6, 652, 170, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 6, Movement_0F8C
    ActorCmdWait
    BMCreateHandleByGPos 0x8024, 1, 652, 169
    BMHndAudioVisualAnmPlay 0x8024, 0
    BMHndAnmWait 0x8024
    ActorCmdExec 6, Movement_0FB4
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 6
    SEWait
    VMJump L_04F1

L_046F:
    // "You must be the ones who want to hear\nthat old tale about Lacunosa Town."
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_MustOnesWhoWant, 251, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    // "Professor Juniper: That's right.\nPlease tell us.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperThatsRight, 6, 0, 0
    MsgWinCloseAll
    // "All right, my dearies.\nPlease come in.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_AllRightDeariesPlease, 251, 0, 0
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0F8C
    ActorCmdWait
    BMCreateHandleByGPos 0x8024, 1, 652, 169
    BMHndAudioVisualAnmPlay 0x8024, 0
    BMHndAnmWait 0x8024
    ActorCmdExec 251, Movement_0FB4
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    ActorWalkRoute 6, 652, 170, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 6, Movement_0FB4
    ActorCmdWait
    ActorDelete 6
    SEPlay SEQ_SE_KAIDAN
    SEWait

L_04F1:
    ActorWalkRoute 255, 652, 170, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0FB4
    ActorCmdWait
    RTReserveScript 3
    FlagSet EVENT_FLAG_0x030c
    MapChangeWarp ZONE_LACUNOSA_TOWN_3, 6, 9, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40cc, 3
    ActorWalkRoute 255, 653, 170, 1, 8, 1
    ActorCmdWait
    // "Professor Juniper: Wasn't that\nan interesting folktale?[f000]븁\u0000\nThe Pokémon's true identity may be\nunknown, but the power mentioned[f000]븀\u0000\nin the story is incredible![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperWasntInteresting, 6, 1, 0
    MsgWinCloseAll
    // "Bianca: I know...[f000]븁\u0000\nThe power to freeze everything around it\ncould even rival the power of the[f000]븀\u0000\nlegendary Dragon-type Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BiancaKnowPowerFreeze, 9, 0, 0
    MsgWinCloseAll
    // "Yes, Bianca.[f000]븁\u0000\nIt's almost like Reshiram, who scorched\nUnova with blazing fire long ago.[f000]븁\u0000"
    // "Yes, Bianca.[f000]븁\u0000\nIt's almost like Zekrom, who scorched\nUnova with intense lightning long ago.[f000]븁\u0000"
    ActorMsgVersioned 1024, LacunosaTown_Text_YesBiancaItsAlmost_2, LacunosaTown_Text_YesBiancaItsAlmost, 6, 1, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0F84
    VMSleep 8
    ActorCmdExec 9, Movement_0F84
    ActorCmdWait
    WordSetPlayerName 0
    // "By the way, [f000]Ā\u0001\u0000,\ndo you remember the story of Reshiram?"
    // "By the way, [f000]Ā\u0001\u0000,\ndo you remember the story of Zekrom?"
    ActorMsgVersioned 1024, LacunosaTown_Text_ByWayRememberStory_2, LacunosaTown_Text_ByWayRememberStory, 6, 1, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05BB
    // "So you remember I told you a little\nabout it in Lentimas Town![f000]븁\u0000\nReshiram is a legendary Dragon-type\nPokémon that lends its power to the[f000]븀\u0000\nperson it recognizes as a hero who[f000]븀\u0000\nseeks truth.[f000]븁\u0000\nIt has a white body, and it can send\nforth ferocious flames![f000]븁\u0000"
    // "I told you a little about it in\nLentimas Town, remember?[f000]븁\u0000\nZekrom is a legendary Dragon-type\nPokémon that lends its power to the[f000]븀\u0000\nperson it recognizes as a hero[f000]븀\u0000\npursuing ideals.[f000]븁\u0000\nIt has a black body, and it can\nunleash fearsome lightning![f000]븁\u0000"
    ActorMsgVersioned 1024, LacunosaTown_Text_RememberToldLittleAbout, LacunosaTown_Text_ToldLittleAboutLentimas, 6, 1, 0
    MsgWinCloseAll
    VMJump L_05CE

L_05BB:
    WordSetPlayerName 0
    // "Oh, [f000]Ā\u0001\u0000...[f000]븁\u0000\nI even told you a little about\nit in Lentimas Town.[f000]븁\u0000\nReshiram is a legendary Dragon-type\nPokémon that lends its power to the[f000]븀\u0000\nperson it recognizes as a hero who[f000]븀\u0000\nseeks truth.[f000]븁\u0000\nIt has a white body, and it can send\nforth ferocious flames![f000]븁\u0000"
    // "Oh, [f000]Ā\u0001\u0000...[f000]븁\u0000\nI even told you a little about\nit in Lentimas Town.[f000]븁\u0000\nZekrom is a legendary Dragon-type\nPokémon that lends its power to the[f000]븀\u0000\nperson it recognizes as a hero[f000]븀\u0000\npursuing ideals.[f000]븁\u0000\nIt has a black body, and it can\nunleash fearsome lightning![f000]븁\u0000"
    ActorMsgVersioned 1024, LacunosaTown_Text_OhEvenToldLittle_2, LacunosaTown_Text_OhEvenToldLittle, 6, 1, 0
    MsgWinCloseAll

L_05CE:
    ActorCmdExec 9, Movement_0F8C
    VMSleep 8
    ActorCmdExec 6, Movement_0F94
    ActorCmdWait
    // "Bianca: Professor, do you think\nthere's a connection between[f000]븀\u0000\nthe Pokémon from the old story[f000]븀\u0000\nand the legendary Dragon-type Pokémon?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BiancaProfessorThinkTheres, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0FCC
    ActorCmdWait
    VMSleep 20
    ActorCmdExec 6, Movement_0FEC
    ActorCmdWait
    VMSleep 30
    // "Professor Juniper: The meteorite.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperMeteorite, 6, 1, 0
    MsgWinCloseAll
    // "Bianca: The meteorite?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BiancaMeteorite, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0FC4
    ActorCmdWait
    // "Reshiram was revived from a\nrock called the Light Stone.[f000]븁\u0000\nLet's suppose the meteorite\nfrom the story and this stone[f000]븀\u0000\nare one and the same...[f000]븁\u0000\nTake into account that elements\nfrom the same era were found in[f000]븀\u0000\nDragonspiral Tower, where Reshiram was,[f000]븀\u0000\nand in the Giant Chasm...[f000]븁\u0000\nIt doesn't prove anything, but it\ncould be a piece of the puzzle.[f000]븁\u0000\nLet's not write it off as a\ncoincidence just yet...[f000]븁\u0000"
    // "Zekrom was revived from a\nrock called the Dark Stone.[f000]븁\u0000\nLet's suppose the meteorite\nfrom the story and this stone[f000]븀\u0000\nare one and the same...[f000]븁\u0000\nTake into account that elements\nfrom the same era were found in[f000]븀\u0000\nDragonspiral Tower, where Zekrom was,[f000]븀\u0000\nand in the Giant Chasm...[f000]븁\u0000\nIt doesn't prove anything, but it\ncould be a piece of the puzzle.[f000]븁\u0000\nLet's not write it off as a\ncoincidence just yet...[f000]븁\u0000"
    ActorMsgVersioned 1024, LacunosaTown_Text_ReshiramRevivedFromRock, LacunosaTown_Text_ZekromRevivedFromRock, 6, 1, 0
    MsgWinCloseAll
    // "Bianca: If your theories are true,\nit should be a really strong Pokémon.[f000]븁\u0000\nWhat kind of a reason would there be\nfor it to come out only at night?[f000]븁\u0000\nLike, if, like, it doesn't like sunlight\nor something like that...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BiancaIfTheoriesTrue, 9, 0, 0
    MsgWinCloseAll
    // "Until we look into it more deeply, it would\nbe hard to say anything about that.[f000]븁\u0000\nNow that I think about it, the name\n“Lacunosa\" could be derived from[f000]븀\u0000\nlacunosus clouds, which are clouds[f000]븀\u0000\nthat resemble a net or a fence.[f000]븁\u0000\nI wonder if the name is related to\nthe part of the story where they[f000]븀\u0000\nbuilt walls to protect the town[f000]븀\u0000\nfrom that Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_UntilWeLookInto, 6, 1, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0F84
    VMSleep 8
    ActorCmdExec 6, Movement_0F84
    ActorCmdWait
    WordSetPlayerName 0
    // "Sorry, I rambled on a bit, didn't I?[f000]븁\u0000\n[f000]Ā\u0001\u0000, could you ask Drayden\nabout this, if you get a chance?[f000]븁\u0000\nI'm going to do a little fieldwork.\nBianca, help out, OK![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_SorryRambledBitDidnt, 6, 1, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0F8C
    ActorCmdWait
    // "Bianca: Sure thing![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BiancaSureThing, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0F60
    ActorCmdWait
    ActorDelete 6
    ActorWalkRoute 9, 653, 171, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 9, Movement_0F8C
    VMSleep 3
    ActorCmdExec 255, Movement_0F94
    ActorCmdWait
    // "Oh, just so you know, Opelucid City's\nmayor, Drayden, wrestles with his[f000]븀\u0000\nPokémon to toughen them up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_OhJustKnowOpelucid, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0F94
    ActorCmdWait
    // "Professor Juniper, wait up![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ProfessorJuniperWaitUp, 9, 0, 0
    MsgWinCloseAll
    ActorCmdExec 9, Movement_0F6C
    ActorCmdWait
    ActorDelete 9
    FlagSet EVENT_FLAG_0x030a
    FlagSet EVENT_FLAG_0x030b
    HollowRivalCmd_0262 3, 7
    HollowRivalCmd_0262 0, 6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40ce
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0748
    VMCall L_0895
    VMJump L_07A2

L_0748:
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Are you ready to pitch in?\nOK, let's go!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ReadyPitchOkLets, 12, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0788
    // "[f000]Ā\u0001\u0001: Just to let you know...[f000]븁\u0000\nYou're about to feel my rage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_JustLetKnowYoure, 12, 0, 0
    MsgWinCloseAll
    VMCall L_0B09
    VMJump L_07A2

L_0788:
    // "[f000]Ā\u0001\u0001: Got it!\nGo get ready and then come back here![f000]븁\u0000\nBeing careful against opponents\nlike these isn't a bad thing!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GotGoGetReady, 12, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 12, Movement_0F84
    ActorCmdWait

L_07A2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8022, 185
    VMJumpIf CMP_EQ, L_07C3
    VMJump L_07D9

L_07C3:
    ActorCmdExec 12, Movement_0F8C
    ActorCmdExec 255, Movement_0F94
    VMJump L_0802

L_07D9:
    WorkCmpConst 0x8022, 187
    VMJumpIf CMP_EQ, L_07EC
    VMJump L_0802

L_07EC:
    ActorCmdExec 12, Movement_0F94
    ActorCmdExec 255, Movement_0F8C
    VMJump L_0802

L_0802:
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Are you ready to pitch in?\nOK, let's go!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ReadyPitchOkLets, 12, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_085F
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0845
    PlayerSetSpecialSequence 1

L_0845:
    // "[f000]Ā\u0001\u0001: Just to let you know...[f000]븁\u0000\nYou're about to feel my rage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_JustLetKnowYoure, 12, 0, 0
    MsgWinCloseAll
    VMCall L_0B09
    VMJump L_0881

L_085F:
    // "[f000]Ā\u0001\u0001: Got it!\nGo get ready and then come back here![f000]븁\u0000\nBeing careful against opponents\nlike these isn't a bad thing!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GotGoGetReady, 12, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 12, Movement_0F84
    ActorCmdExec 255, Movement_0F9C
    ActorCmdWait

L_0881:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMCall L_0895
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0895:
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush EVENT_WORK_0x40ce
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A51
    WorkSetConst EVENT_WORK_0x40ce, 2
    FlagReset EVENT_FLAG_0x0311
    HollowRivalCmd_0262 1, 22
    ActorAdd 10
    ActorAdd 11
    VMStackPush 0x8022
    VMStackPushConst 185
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08E9
    ActorCmdExec 12, Movement_0F7C
    ActorCmdWait
    VMJump L_0940

L_08E9:
    VMStackPush 0x8022
    VMStackPushConst 186
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0916
    ActorCmdExec 12, Movement_0FDC
    ActorCmdWait
    ActorCmdExec 255, Movement_0F8C
    ActorCmdWait
    VMJump L_0940

L_0916:
    ActorCmdExec 12, Movement_0FDC
    ActorCmdWait
    WorkSub 0x8022, 1
    ActorWalkRoute 12, 0x8021, 0x8022, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0F8C
    ActorCmdWait

L_0940:
    PlayerGetExState 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_095B
    PlayerSetSpecialSequence 1

L_095B:
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: What's up?[f000]븁\u0000\nHave you seen Team Plasma\nanywhere around here?[f000]븀\u0000\nI heard a rumor to that effect...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_WhatsUpHaveSeen, 12, 0, 0
    MsgWinCloseAll
    BGMPlay SEQ_BGM_E_PLASMA
    ActorCmdExec 10, Movement_0F74
    ActorCmdExec 11, Movement_0F74
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x2868000, 0, 0xba8000, 30
    ActorCmdExec 12, Movement_0F84
    ActorCmdExec 255, Movement_0F84
    ActorCmdWait
    EvCameraWait
    // "Zinzolin: Oh, for crying out loud...\nThis is troublesome indeed,[f000]븀\u0000\nmy curious Trainers.[f000]븁\u0000\nPerhaps I should satiate\nyour curiosity somewhat.[f000]븁\u0000\nThe reason I am still part\nof Team Plasma is this:[f000]븁\u0000\nI want to know how the world will change.[f000]븁\u0000\nListen. Pokémon are nature.\nPoké Balls are civilization.[f000]븁\u0000\nHumans who are used to civilization\ndon't relinquish it easily.[f000]븁\u0000\nOf course, both nature and\ncivilization are important.[f000]븁\u0000\nBut what will happen to a world\ntaken over by Team Plasma?[f000]븁\u0000\nPeople will be forced to throw out\nPoké Balls--a product of civilization.[f000]븁\u0000\nI want to know what that looks like!\nAnd I want to enjoy it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ZinzolinOhCryingOut, 10, 2, 0
    MsgWinCloseAll
    ActorWalkRoute 12, 647, 186, 1, 4, 0
    ActorCmdWait
    ActorCmdExec 12, Movement_0FCC
    ActorCmdWait
    ActorCmdExec 12, Movement_0FFC
    ActorCmdWait
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Shut your mouth.[f000]븁\u0000\nAll I want is to get back\na stolen Pokémon![f000]븁\u0000\n[f000]Ā\u0001\u0000! Give me a hand!\nYou ready?"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ShutMouthAllWant, 12, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A31
    // "[f000]Ā\u0001\u0001: Just to let you know...[f000]븁\u0000\nYou're about to feel my rage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_JustLetKnowYoure, 12, 0, 0
    MsgWinCloseAll
    EvCameraReturn 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMCall L_0B09
    VMJump L_0A4B

L_0A31:
    // "[f000]Ā\u0001\u0001: Got it!\nGo get ready and then come back here![f000]븁\u0000\nBeing careful against opponents\nlike these isn't a bad thing!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GotGoGetReady, 12, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    EvCameraReturn 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd

L_0A4B:
    VMJump L_0B05

L_0A51:
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8022, 185
    VMJumpIf CMP_EQ, L_0A6A
    VMJump L_0A80

L_0A6A:
    ActorCmdExec 12, Movement_0F8C
    ActorCmdExec 255, Movement_0F94
    VMJump L_0AA9

L_0A80:
    WorkCmpConst 0x8022, 187
    VMJumpIf CMP_EQ, L_0A93
    VMJump L_0AA9

L_0A93:
    ActorCmdExec 12, Movement_0F94
    ActorCmdExec 255, Movement_0F8C
    VMJump L_0AA9

L_0AA9:
    ActorCmdWait
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Are you ready to pitch in?\nOK, let's go!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ReadyPitchOkLets, 12, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AEB
    // "[f000]Ā\u0001\u0001: Just to let you know...[f000]븁\u0000\nYou're about to feel my rage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_JustLetKnowYoure, 12, 0, 0
    MsgWinCloseAll
    VMCall Script_6
    VMJump L_0B05

L_0AEB:
    // "[f000]Ā\u0001\u0001: Got it!\nGo get ready and then come back here![f000]븁\u0000\nBeing careful against opponents\nlike these isn't a bad thing!"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GotGoGetReady, 12, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 12, Movement_0F84
    ActorCmdWait

L_0B05:
    BGMChangeMap
    VMReturn

L_0B09:
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8022, 186
    VMJumpIf CMP_EQ, L_0B22
    VMJump L_0B44

L_0B22:
    ActorWalkRoute 255, 647, 185, 2, 8, 1
    VMSleep 8
    ActorCmdExec 12, Movement_0F84
    ActorCmdWait
    VMJump L_0B83

L_0B44:
    WorkCmpConst 0x8022, 187
    VMJumpIf CMP_EQ, L_0B57
    VMJump L_0B83

L_0B57:
    ActorWalkRoute 255, 648, 185, 2, 8, 0
    VMSleep 8
    ActorCmdExec 12, Movement_0F84
    ActorCmdWait
    ActorCmdExec 255, Movement_0FA4
    ActorCmdWait
    VMJump L_0B83

L_0B83:
    VMStackPush 0x8022
    VMStackPushConst 185
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BCF
    VMStackPush 0x8021
    VMStackPushConst 650
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BBD
    ActorWalkRoute 255, 647, 185, 2, 8, 0
    VMJump L_0BCD

L_0BBD:
    ActorCmdExec 12, Movement_0F84
    ActorCmdExec 255, Movement_0F84

L_0BCD:
    ActorCmdWait

L_0BCF:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BF2
    CallTrainerMultiBattle 701, 705, 704, 0
    VMJump L_0C1F

L_0BF2:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C15
    CallTrainerMultiBattle 702, 705, 704, 0
    VMJump L_0C1F

L_0C15:
    CallTrainerMultiBattle 703, 705, 704, 0

L_0C1F:
    VMCall L_0CB0
    // "Team Plasma: What's with these two?\nI'm battling alongside Zinzolin![f000]븀\u0000\nThis shouldn't be happening![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_TeamPlasmaWhatsThese, 11, 1, 0
    MsgWinCloseAll
    // "Zinzolin: These Trainers remind me\nof that one from two years ago.[f000]븁\u0000\nMore important, we must continue\nour search.[f000]븁\u0000\nLike that scientist said,\nit might be in Opelucid City![f000]븁\u0000\nWe'll play with you again later![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ZinzolinTheseTrainersRemind, 10, 2, 0
    MsgWinCloseAll
    VMSleep 15
    ActorWalkRoute 10, 637, 186, 1, 4, 0
    VMSleep 8
    ActorWalkRoute 11, 637, 185, 1, 4, 0
    ActorCmdWait
    ActorDelete 10
    ActorDelete 11
    FlagSet EVENT_FLAG_0x0311
    WordSetLoadRivalName 1
    // "[f000]Ā\u0001\u0001: Get back here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GetBackHere, 12, 0, 1
    ActorMsgClose
    ActorWalkRoute 12, 638, 186, 1, 4, 0
    ActorCmdWait
    ActorDelete 12
    FlagSet EVENT_FLAG_0x0310
    WorkSetConst EVENT_WORK_0x40ce, 3
    WorkSetConst EVENT_WORK_0x40cc, 4
    HollowRivalCmd_0262 1, 23
    VMReturn

L_0CB0:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CEA
    PokePartyGetCount 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CE2
    PokePartyRecoverAll

L_0CE2:
    CallTrainerBattleEnd
    VMJump L_0CEC

L_0CEA:
    CallTrainerLose

L_0CEC:
    VMReturn

Script_7:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Lacunosa Town\nMethodical and Orderly for Safety"
    MsgPlaceSign LacunosaTown_Text_LacunosaTownMethodicalOrderly, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you want to hear that old tale\nof Lacunosa Town again?[f000]븁\u0000\nIt always takes me a little time\nto tell it."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_WantHearOldTale, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DA2
    // "There's a great big hole behind\nthis town.[f000]븁\u0000\nA long time ago, a huge meteorite fell\nfrom the sky and made the big hole.[f000]븁\u0000\nA very scary monster was hiding\ninside the meteorite![f000]븁\u0000\nPeople say the monster appeared in the\nvillage at night, with a freezing wind,[f000]븀\u0000\nand it stole away people and Pokémon...[f000]븁\u0000\nSo the villagers built big walls to keep\nthe monster out and made a rule that[f000]븀\u0000\nno one could go out after dark.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_TheresGreatBigHole, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0F7C
    ActorCmdWait
    VMSleep 60
    VMStackPush 0x8021
    VMStackPushConst 670
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 167
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0D84
    ActorCmdExec 0, Movement_0F94
    ActorCmdWait
    VMJump L_0D8E

L_0D84:
    ActorCmdExec 0, Movement_0F84
    ActorCmdWait

L_0D8E:
    // "Whether you believe it or not\nis up to you...[f000]븁\u0000\nBut even now, the people of this town\nstay inside after dark.[f000]븁\u0000\nThe old stories and legends\ncontinue to influence our lives."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_WhetherBelieveNotUp, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0DB0

L_0DA2:
    // "Oh, fine, fine.\nCome back again when you have time."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_OhFineFineCome, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0DB0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Lacunosa Town.[f000]븁\u0000\nIn this town, people live as methodically\nas clockwork from morning to night.[f000]븁\u0000\nIf you live your life soaking up sunlight,\nyou can sleep very well at night."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_WelcomeLacunosaTownTown, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The big scary monster that\ncomes out at night is[f000]븀\u0000\na Pokémon, right?[f000]븁\u0000\nIt must be a really scary Pokémon\nif everyone believes the legend[f000]븀\u0000\nand follows these rules..."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_BigScaryMonsterComes, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E1D
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My grandma's stories were\nreally about Kyurem, it seems.[f000]븁\u0000\nI guess old stories sometimes\nhave a kernel of truth to them."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GrandmasStoriesWereReally, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0E31

L_0E1D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My granny loves old stories![f000]븁\u0000\nI'm always having to listen\nto her really, really long stories."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GrannyLovesOldStories, 0, 0
    LastKeyWait
    ActorMsgClose

L_0E31:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are a lot of people in the world,\nand there are just as many different[f000]븀\u0000\ncharacteristics and ideas."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ThereLotPeopleWorld, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E82
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So the reason nobody goes outside\nat night and it's so peaceful[f000]븀\u0000\nis because of a Pokémon?[f000]븀\u0000\nI don't know how to feel about that!"
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_ReasonNobodyGoesOutside, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0E96

L_0E82:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't have anything to do ever since\nI took a post here.[f000]븁\u0000\nSince nobody goes outside at night,\nit's very peaceful."
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_DontHaveAnythingEver, 0, 0
    LastKeyWait
    ActorMsgClose

L_0E96:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 572, 0
    // "Gahoo! Gahoo!"
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_GahooGahoo, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon just runs around on its own.\nMaybe it doesn't need a Trainer?"
    ParentActorMsg MSGFILE_SCRIPT, LacunosaTown_Text_PokemonJustRunsAround, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 15, 6
    MoveEnd
    Move 15, 6
    MoveEnd

Movement_0EEC:
    Move 14, 13
    Move 32, 1
    MoveEnd

Movement_0EF8:
    Move 63, 2
    Move 13, 1
    Move 14, 11
    Move 32, 1
    MoveEnd

Movement_0F0C:
    Move 14, 14
    Move 32, 1
    MoveEnd
    Move 13, 8
    MoveEnd
    Move 13, 8
    MoveEnd

Movement_0F28:
    Move 12, 10
    Move 14, 18
    Move 32, 1
    MoveEnd

Movement_0F38:
    Move 63, 2
    Move 14, 1
    Move 12, 10
    Move 14, 16
    Move 32, 1
    MoveEnd

Movement_0F50:
    Move 12, 12
    Move 14, 17
    Move 32, 1
    MoveEnd

Movement_0F60:
    Move 14, 1
    Move 13, 4
    MoveEnd

Movement_0F6C:
    Move 17, 4
    MoveEnd

Movement_0F74:
    Move 15, 6
    MoveEnd

Movement_0F7C:
    Move 35, 1
    MoveEnd

Movement_0F84:
    Move 34, 1
    MoveEnd

Movement_0F8C:
    Move 32, 1
    MoveEnd

Movement_0F94:
    Move 33, 1
    MoveEnd

Movement_0F9C:
    Move 15, 1
    MoveEnd

Movement_0FA4:
    Move 14, 1
    MoveEnd
    Move 13, 1
    MoveEnd

Movement_0FB4:
    Move 12, 1
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_0FC4:
    Move 1, 1
    MoveEnd

Movement_0FCC:
    Move 2, 1
    MoveEnd
    Move 3, 1
    MoveEnd

Movement_0FDC:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd

Movement_0FEC:
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd

Movement_0FFC:
    Move 100, 1
    MoveEnd
