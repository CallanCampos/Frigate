#include "asm/field_script.inc"
#include "text/script/giant_chasm_4.h"

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
    ScriptEntriesEnd

Script_8:
    VMHalt

Script_9:
    VMStackPush EVENT_WORK_0x4071
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0093
    ActorSetGPos 3, 26, 0, 65, 1
    ActorSetGPos 13, 24, 0, 62, 1
    ActorSetGPos 4, 28, 0, 69, 2
    ActorSetGPos 5, 28, 0, 70, 2
    ActorSetGPos 0, 25, 0, 65, 0
    ActorSetGPos 1, 27, 0, 65, 0

L_0093:
    VMHalt

Script_1:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 6, Movement_06DC
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Come here.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_ComeHere, 6, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 255, 25, 70, 1, 8, 0
    ActorCmdExec 6, Movement_06E4
    ActorCmdWait
    ActorCmdExec 255, Movement_06EC
    ActorCmdWait
    // "Wait a sec.[f000]븁\u0000\nHe said he wants to talk to them\nso his old allies won't get hurt.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_WaitSecHeSaid, 6, 0, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_06D4
    ActorCmdExec 6, Movement_06D4
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1a8000, 0, 0x428000, 40
    ActorCmdExec 6, Movement_06D4
    ActorCmdWait
    EvCameraWait
    // "Rood: Aah! I will say it as many times\nas it takes until you understand![f000]븁\u0000\nGhetsis's real plan was\nto take over the Unova region![f000]븁\u0000\nLiberating Pokémon was nothing\nmore than an excuse![f000]븁\u0000\nIf anything, it would've made\nPokémon suffer![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_RoodAahWillSay, 2, 6, 0
    MsgWinCloseAll
    // "Team Plasma: Uh-huh, yeah.\nThat's a pretty speech, gramps![f000]븁\u0000\nYou fool![f000]븁\u0000\nWe're not going to listen\nto what a traitor has to say![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_TeamPlasmaUhHuh, 4, 3, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 6, Movement_04A4
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Well, that didn't work...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_WellDidntWork, 6, 6, 0
    MsgWinCloseAll
    ActorWalkRoute 6, 26, 69, 1, 8, 0
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Hey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_Hey, 6, 6, 1
    ActorMsgClose
    ActorCmdExec 4, Movement_06F4
    ActorCmdExec 7, Movement_06FC
    ActorCmdExec 8, Movement_06FC
    ActorCmdExec 12, Movement_06FC
    ActorCmdExec 5, Movement_06F4
    ActorCmdExec 2, Movement_054C
    ActorCmdExec 0, Movement_0540
    ActorCmdExec 1, Movement_0540
    ActorCmdWait
    // "Let me through![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_LetThrough, 6, 6, 0
    MsgWinCloseAll
    // "Team Plasma: What are you saying?\nLooking to get hurt?[f000]븁\u0000"
    InfoMsg GiantChasm4_Text_TeamPlasmaWhatSaying, 1
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: I'm going to get a\nstolen Pokémon back![f000]븁\u0000\nI'm not gonna listen to villains\nlike you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_ImGoingGetStolen, 6, 6, 0
    MsgWinCloseAll
    // "Rood!\nEx-Team Plasma![f000]븁\u0000\nWhy do you have\nPokémon by your sides?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_RoodExTeamPlasma, 6, 6, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_06FC
    VMSleep 8
    ActorCmdExec 0, Movement_06FC
    ActorCmdExec 1, Movement_06FC
    ActorCmdWait
    // "To protect what's important\nto you, right?[f000]븁\u0000\nEven if your precious Pokémon get hurt,\neven if your ideals are damaged,[f000]븀\u0000\nthe time to fight is NOW![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_ProtectWhatsImportantRight, 6, 6, 0
    MsgWinCloseAll
    // "Team Plasma: You're just a kid!\nQuit trying to act so cool![f000]븁\u0000\nWhatever! Nobody's getting\nclose to the Plasma Frigate![f000]븀\u0000\nWipe them ALL out![f000]븁\u0000"
    InfoMsg GiantChasm4_Text_TeamPlasmaYoureJust, 1
    MsgWinCloseAll
    ActorWalkRoute 4, 28, 69, 1, 8, 0
    ActorWalkRoute 5, 28, 70, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 4, Movement_06E4
    ActorCmdExec 5, Movement_06E4
    ActorCmdWait
    ActorCmdExec 6, Movement_06EC
    ActorCmdExec 255, Movement_06EC
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: [f000]Ā\u0001\u0000!\nAs usual, take the other one![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_UsualTakeOtherOne, 6, 5, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_06EC
    ActorWalkRoute 255, 26, 70, 1, 8, 0
    ActorCmdWait
    ActorWalkRoute 4, 27, 69, 1, 8, 0
    VMSleep 4
    ActorWalkRoute 5, 27, 70, 1, 8, 0
    ActorCmdWait
    // "Team Plasma: Like he said![f000]븁\u0000\nWe're going to crush you\nalong with the traitors![f000]븁\u0000\nBecause Team Plasma exists\nto cause trouble![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_TeamPlasmaLikeHe, 5, 6, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_46, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EC
    FlagReset EVENT_FLAG_0x0372
    ActorAdd 3
    ActorAdd 13
    CallTrainerBattleEnd
    VMJump L_02EE

L_02EC:
    CallTrainerLose

L_02EE:
    ActorCmdExec 4, Movement_0530
    VMSleep 4
    ActorCmdExec 5, Movement_0530
    ActorCmdWait
    ActorCmdExec 2, Movement_0500
    VMSleep 24
    ActorCmdExec 6, Movement_06E4
    ActorCmdExec 255, Movement_06E4
    ActorCmdWait
    ActorCmdExec 2, Movement_06EC
    ActorCmdWait
    // "Rood: Are your Pokémon OK?\nYou should take these with you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_RoodPokemonOkShould, 2, 3, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 29
    WorkSet 0x8001, 3
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 2, Movement_0514
    ActorCmdWait
    ActorCmdExec 2, Movement_06EC
    ActorCmdWait
    // "You, too.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_Too, 2, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Thanks...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_Thanks, 6, 5, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_06D4
    ActorCmdExec 255, Movement_06D4
    ActorCmdWait
    // "I'm passing through![f000]븁\u0000\nOh, it looks like their backup\nhas arrived.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_ImPassingThroughOh, 6, 5, 0
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1a8000, 0, 0x428000, 40
    ActorCmdExec 3, Movement_04B0
    ActorCmdExec 13, Movement_04D8
    ActorCmdWait
    EvCameraWait
    ActorCmdExec 2, Movement_0520
    ActorWalkRoute 0, 25, 65, 1, 8, 1
    ActorWalkRoute 1, 27, 65, 1, 8, 1
    ActorCmdWait
    // "Rood: At times like these,\nthose whose hearts weaken,[f000]븀\u0000\nthose whose determination falters,[f000]븀\u0000\ncan accomplish nothing![f000]븁\u0000\nTo save our old allies,\nto protect Unova,[f000]븀\u0000\nwe will fight![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_RoodTimesLikeThese, 2, 3, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_06DC
    ActorCmdWait
    // "Both of you, go![f000]븁\u0000\nNo, just a moment...\n[f000]Ā\u0001\u0001, was it?[f000]븁\u0000\nAbout the Pokémon you're looking for...\nIn all likelihood, it is in the possession[f000]븀\u0000\nof the Shadow Triad--the dark warriors[f000]븀\u0000\nwho appear silently.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_BothGoNoJust, 2, 3, 0
    MsgWinCloseAll
    // "[f000]Ā\u0001\u0001: Got it![f000]븁\u0000\nIf I rescue it, that helps you guys\nabsolve your guilt, doesn't it?[f000]븀\u0000\nGuess I'll help you out![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_GotIfRescueHelps, 6, 5, 0
    MsgWinCloseAll
    ActorCmdExec 6, Movement_0558
    ActorCmdExec 2, Movement_06D4
    ActorCmdWait
    // "Rood: At that time, I believed\nwe were on the side of justice.[f000]븁\u0000\nBy serving my king, N,\nI was going to make a world without war.[f000]븁\u0000\nBut I was conceited, and I couldn't\nsee the unhappiness we were causing.[f000]븁\u0000\nThat's why I can't let it happen again!"
    ActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_RoodTimeBelievedWe, 2, 3, 0
    MsgWaitAdvance
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorDelete 6
    FlagSet EVENT_FLAG_0x0374
    WorkSetConst EVENT_WORK_0x4071, 1
    WorkSetConst EVENT_WORK_0x4072, 1
    FlagSet EVENT_FLAG_0x03cf
    HollowRivalCmd_0262 0, 6
    HollowRivalCmd_0262 1, 35
    HollowRivalCmd_0262 2, 9
    HollowRivalCmd_0262 3, 7
    HollowRivalCmd_0262 4, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04A4:
    Move 1, 1
    Move 100, 1
    MoveEnd

Movement_04B0:
    Move 71, 1
    Move 17, 1
    Move 73, 1
    Move 17, 1
    Move 74, 1
    Move 72, 1
    Move 13, 2
    Move 14, 6
    Move 13, 4
    MoveEnd

Movement_04D8:
    Move 71, 1
    Move 17, 1
    Move 73, 1
    Move 17, 1
    Move 74, 1
    Move 72, 1
    Move 13, 2
    Move 14, 9
    Move 13, 1
    MoveEnd

Movement_0500:
    Move 13, 2
    Move 14, 1
    Move 13, 2
    Move 35, 0
    MoveEnd

Movement_0514:
    Move 12, 1
    Move 35, 0
    MoveEnd

Movement_0520:
    Move 12, 2
    Move 15, 1
    Move 12, 1
    MoveEnd

Movement_0530:
    Move 71, 1
    Move 11, 1
    Move 72, 1
    MoveEnd

Movement_0540:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_054C:
    Move 33, 1
    Move 159, 1
    MoveEnd

Movement_0558:
    Move 16, 1
    Move 19, 2
    Move 16, 5
    Move 19, 4
    Move 16, 2
    Move 71, 1
    Move 16, 1
    Move 73, 1
    Move 16, 3
    Move 74, 1
    Move 72, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Rood: At that time, I believed\nwe were on the side of justice.[f000]븁\u0000\nBy serving my king, N,\nI was going to make a world without war.[f000]븁\u0000\nBut I was conceited, and I couldn't\nsee the unhappiness we were causing.[f000]븁\u0000\nThat's why I can't let it happen again!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_RoodTimeBelievedWe, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Don't ignore Pokémon's feelings\nand separate them from their Trainers!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_DontIgnorePokemonsFeelings, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Is this where you come to an\nunderstanding by trading blows?[f000]븀\u0000\nThis is what being young is, right?"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_WhereComeUnderstandingBy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That's what I would expect from\nsomeone who binds their Pokémon[f000]븀\u0000\nwith Poké Balls!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_ThatsWhatWouldExpect, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "We're going to conquer Unova and\nmake all the Pokémon ours![f000]븁\u0000\nThen our failure two years ago\nwon't matter anymore!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_WereGoingConquerUnova, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Oh! I can feel how strongly\nthis person feels![f000]븀\u0000\nI-it's making me doubt myself!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_OhCanFeelHow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "That traitorous Sage!\nI'm going to pound him into a pulp!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_TraitorousSageImGoing, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What you're doing now is nothing\nmore than a futile struggle!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_WhatYoureDoingNow, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't care about you at all!\n'Cause you can't beat our boss![f000]븁\u0000\nI'll stay here and pound these traitors!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_DontCareAboutAll, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We're gonna freeze Unova solid\nand steal everyone's Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, GiantChasm4_Text_WereGonnaFreezeUnova, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
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

Movement_06D4:
    Move 32, 1
    MoveEnd

Movement_06DC:
    Move 33, 1
    MoveEnd

Movement_06E4:
    Move 34, 1
    MoveEnd

Movement_06EC:
    Move 35, 1
    MoveEnd

Movement_06F4:
    Move 75, 1
    MoveEnd

Movement_06FC:
    Move 159, 1
    MoveEnd
