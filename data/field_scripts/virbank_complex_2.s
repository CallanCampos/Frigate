#include "asm/field_script.inc"
#include "text/script/virbank_complex_2.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_6:
    WorkCmpConst EVENT_WORK_0x40dd, 0
    VMJumpIf CMP_EQ, L_0043
    VMJump L_0055

L_0043:
    ActorSetGPos 7, 29, 0, 32, 0
    VMJump L_009F

L_0055:
    WorkCmpConst EVENT_WORK_0x40dd, 1
    VMJumpIf CMP_EQ, L_0068
    VMJump L_007A

L_0068:
    ActorSetGPos 7, 8, 0, 47, 2
    VMJump L_009F

L_007A:
    WorkCmpConst EVENT_WORK_0x40dd, 2
    VMJumpIf CMP_EQ, L_008D
    VMJump L_009F

L_008D:
    ActorSetGPos 7, 20, 65535, 58, 3
    VMJump L_009F

L_009F:
    VMHalt

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0132
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0109
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey there, Trainer!\nIt looks like you've got energetic[f000]븀\u0000\nPokémon there with ya![f000]븁\u0000\nCan I ask a favor?[f000]븁\u0000\nI need to get the Workers\nin this complex fired up![f000]븁\u0000\nHow about it?\nHelp a guy out, will ya?"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_HeyThereTrainerLooks, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F5
    // "Yeah! That's what I like to hear![f000]븁\u0000\nThere are some new Workers\nhere in the complex![f000]븁\u0000\nGo get those three raring to go!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_YeahThatsWhatLike, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0132
    VMJump L_0103

L_00F5:
    // "Really? Nobody really wants\nanything enough anymore!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_ReallyNobodyReallyWants, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0103:
    VMJump L_01BA

L_0109:
    VMStackPushFlag EVENT_FLAG_0x0133
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0134
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0135
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01A6
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 619
    WorkSet 0x8001, 1
    WorkSet 0x8002, 465
    WorkSet 0x8003, 4
    WorkSet 0x8004, 5
    WorkSet 0x8005, 5
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_01BA

L_01A6:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "OK![f000]븁\u0000\nThe three new Workers are\nin different parts of the complex!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_OkThreeNewWorkers, 0, 0
    LastKeyWait
    ActorMsgClose

L_01BA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0132
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01EF
    // "Working at this complex\nwas my dream...[f000]븁\u0000\nBut now that it's come true,\nI've got everything I want..."
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_WorkingComplexDreamBut, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_025D

L_01EF:
    VMStackPushFlag EVENT_FLAG_0x0133
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0216
    // "I came to this complex so\nmy Pokémon could accomplish a lot![f000]븁\u0000\nThat means that as a Trainer,\nI have to work like crazy!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_CameComplexPokemonCould, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_025D

L_0216:
    // "Working at this complex\nwas my dream...[f000]븁\u0000\nBut now that it's come true,\nI've got everything I want...[f000]븁\u0000\nWhat? The foreman said so? OK, fine.\nWe just have to battle, right?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_WorkingComplexDreamBut_2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_ISAAC, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0249
    CallTrainerBattleEnd
    VMJump L_024B

L_0249:
    CallTrainerLose

L_024B:
    // "I came to this complex so\nmy Pokémon could accomplish a lot![f000]븁\u0000\nThat means that as a Trainer,\nI have to work like crazy!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_CameComplexPokemonCould, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0133

L_025D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0132
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0292
    // "I'm happy just to be with\nmy awesome Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_ImHappyJustAwesome, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0300

L_0292:
    VMStackPushFlag EVENT_FLAG_0x0134
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    // "I completely forgot![f000]븁\u0000\nIf I don't work really hard, my Pokémon\nwon't have a chance to shine![f000]븁\u0000\nAll riiight! Time to get to work!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_CompletelyForgotIfDont, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0300

L_02B9:
    // "I'm happy just to be with\nmy awesome Pokémon![f000]븁\u0000\nWhat? A battle?\nI guess so... I'll play a bit.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_ImHappyJustAwesome_2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_MITCHELL, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EC
    CallTrainerBattleEnd
    VMJump L_02EE

L_02EC:
    CallTrainerLose

L_02EE:
    // "I completely forgot![f000]븁\u0000\nIf I don't work really hard, my Pokémon\nwon't have a chance to shine![f000]븁\u0000\nAll riiight! Time to get to work!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_CompletelyForgotIfDont, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0134

L_0300:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0132
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0335
    // "Every day is the same..."
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_EveryDaySame, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03A3

L_0335:
    VMStackPushFlag EVENT_FLAG_0x0135
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035C
    // "It's OK if every day's the same\nwhen I'm working at this complex![f000]븀\u0000\nIt means there aren't any problems!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_ItsOkIfEvery, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03A3

L_035C:
    // "Every day is the same...[f000]븁\u0000\nI get bored when things don't change,\nso I don't mind battling you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_EveryDaySameGet, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_NATHAN, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_038F
    CallTrainerBattleEnd
    VMJump L_0391

L_038F:
    CallTrainerLose

L_0391:
    // "It's OK if every day's the same\nwhen I'm working at this complex![f000]븀\u0000\nIt means there aren't any problems!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_ItsOkIfEvery, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x0135

L_03A3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkCmpConst EVENT_WORK_0x40dd, 0
    VMJumpIf CMP_EQ, L_03C2
    VMJump L_04C6

L_03C2:
    // "Heating crude oil separates\nit into many different components...[f000]븀\u0000\nThat's what this distillation tower is for![f000]븁\u0000\nIt's also known as a topper,\nbut I want to call it a distillation tower."
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_HeatingCrudeOilSeparates, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 7, Movement_0850
    ActorCmdWait
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_03F1
    VMJump L_03FF

L_03F1:
    ActorCmdExec 7, Movement_0838
    VMJump L_0441

L_03FF:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0412
    VMJump L_0420

L_0412:
    ActorCmdExec 7, Movement_0840
    VMJump L_0441

L_0420:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0433
    VMJump L_0441

L_0433:
    ActorCmdExec 7, Movement_0848
    VMJump L_0441

L_0441:
    ActorCmdWait
    // "Oh! My knowledge overflowed,\nand it was overheard!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_OhKnowledgeOverflowedOverheard, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0464
    VMJump L_0472

L_0464:
    ActorCmdExec 7, Movement_0780
    VMJump L_04A0

L_0472:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0492
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0492
    VMJump L_04A0

L_0492:
    ActorCmdExec 7, Movement_078C
    VMJump L_04A0

L_04A0:
    VMSleep 16
    ActorCmdExec 255, Movement_0838
    ActorCmdWait
    ActorSetGPos 7, 8, 0, 47, 2
    WorkSetConst EVENT_WORK_0x40dd, 1
    VMJump L_0710

L_04C6:
    WorkCmpConst EVENT_WORK_0x40dd, 1
    VMJumpIf CMP_EQ, L_04D9
    VMJump L_05F1

L_04D9:
    // "The smokestacks of the complex\nrise up into the sky.[f000]븁\u0000\nThat powerful silhouette\nis sure breathtaking, isn't it?[f000]븁\u0000\nBy the way, they're burning up\nwaste gas.[f000]븁\u0000\nRecently, they've found all sorts\nof uses for this gas, though."
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_SmokestacksComplexRiseUp, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 7, Movement_0850
    ActorCmdWait
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0508
    VMJump L_0516

L_0508:
    ActorCmdExec 7, Movement_0838
    VMJump L_0558

L_0516:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0529
    VMJump L_0537

L_0529:
    ActorCmdExec 7, Movement_0848
    VMJump L_0558

L_0537:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_054A
    VMJump L_0558

L_054A:
    ActorCmdExec 7, Movement_0830
    VMJump L_0558

L_0558:
    ActorCmdWait
    // "Oh! You overheard some\nof my vast stock of knowledge!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_OhOverheardSomeVast, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_057B
    VMJump L_0589

L_057B:
    ActorCmdExec 7, Movement_0794
    VMJump L_05CB

L_0589:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_059C
    VMJump L_05AA

L_059C:
    ActorCmdExec 7, Movement_07A8
    VMJump L_05CB

L_05AA:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_05BD
    VMJump L_05CB

L_05BD:
    ActorCmdExec 7, Movement_07B8
    VMJump L_05CB

L_05CB:
    VMSleep 16
    ActorCmdExec 255, Movement_0848
    ActorCmdWait
    ActorSetGPos 7, 20, 65535, 58, 3
    WorkSetConst EVENT_WORK_0x40dd, 2
    VMJump L_0710

L_05F1:
    WorkCmpConst EVENT_WORK_0x40dd, 2
    VMJumpIf CMP_EQ, L_0604
    VMJump L_0710

L_0604:
    // "Those gas holders are sure an\nexpression of harmony, aren't they?[f000]븁\u0000\nThere's a reason behind that shape.[f000]븁\u0000\nA sphere is best for withstanding\nthe tremendous pressure inside.[f000]븁\u0000\nIt would be nice if they would draw\na Pokémon on these gas holders, too."
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_ThoseGasHoldersSure, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0629
    VMJump L_0637

L_0629:
    ActorCmdExec 7, Movement_0840
    VMJump L_0658

L_0637:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_064A
    VMJump L_0658

L_064A:
    ActorCmdExec 7, Movement_0830
    VMJump L_0658

L_0658:
    ActorCmdWait
    VMStackPushFlag EVENT_FLAG_0x0136
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06F6
    // "Oh! You overheard some\nof my breathtaking knowledge.[f000]븁\u0000\nAre you actually...interested\nin what I have to say?[f000]븁\u0000\nThat would mean I have\na complex buddy![f000]븁\u0000\nThat makes me really happy!\nWell then, take this with you!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_OhOverheardSomeBreathtaking, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ItemCheckSpace ITEM_ETHER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06C6
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 38
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0136
    VMCall L_0716
    VMJump L_06F0

L_06C6:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 38
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 7, Movement_0848
    ActorCmdWait

L_06F0:
    VMJump L_070A

L_06F6:
    // "I know a lot about the complex, right?[f000]븁\u0000\nI was even scouted by the foreman,\nbut I'm happy just looking at it!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_KnowLotAboutComplex, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMCall L_0716

L_070A:
    VMJump L_0710

L_0710:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0716:
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_072D
    VMJump L_073B

L_072D:
    ActorCmdExec 7, Movement_07E0
    VMJump L_075C

L_073B:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_074E
    VMJump L_075C

L_074E:
    ActorCmdExec 7, Movement_07CC
    VMJump L_075C

L_075C:
    VMSleep 16
    ActorCmdExec 255, Movement_0840
    ActorCmdWait
    ActorSetGPos 7, 29, 0, 32, 0
    WorkSetConst EVENT_WORK_0x40dd, 0
    VMReturn
    .balign 4, 0

Movement_0780:
    Move 15, 1
    Move 13, 8
    MoveEnd

Movement_078C:
    Move 13, 8
    MoveEnd

Movement_0794:
    Move 15, 1
    Move 13, 1
    Move 15, 6
    Move 13, 5
    MoveEnd

Movement_07A8:
    Move 13, 1
    Move 15, 7
    Move 13, 5
    MoveEnd

Movement_07B8:
    Move 15, 1
    Move 13, 2
    Move 15, 6
    Move 13, 5
    MoveEnd

Movement_07CC:
    Move 14, 1
    Move 12, 1
    Move 14, 3
    Move 12, 8
    MoveEnd

Movement_07E0:
    Move 12, 1
    Move 14, 4
    Move 12, 8
    MoveEnd
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

Movement_0830:
    Move 32, 1
    MoveEnd

Movement_0838:
    Move 33, 1
    MoveEnd

Movement_0840:
    Move 34, 1
    MoveEnd

Movement_0848:
    Move 35, 1
    MoveEnd

Movement_0850:
    Move 75, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01d3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08F2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh my! Why, you have a\nPokédex, don't you?[f000]븁\u0000\nI heard that the newest Pokédex\nhas an amazing function called[f000]븀\u0000\nthe Habitat List![f000]븁\u0000\nCould you use it to show me what kind\nof Pokémon live in this complex?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_OhWhyHavePokedex, 0, 0
    PokeDexCheckHabitatList 456, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08DE
    // "Amazing!\nThe Habitat List is filled up![f000]븁\u0000\nSo amazing! There are so many\nPokémon living in this complex![f000]븁\u0000\nHere, take these as thanks![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_AmazingHabitatListFilled, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 3
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x01d3
    // "If you fill up the Habitat Lists, you might\ncomplete the Pokédex before you know it!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_IfFillUpHabitat, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08EC

L_08DE:
    // "Oh? You don't have the\nHabitat List for the complex[f000]븀\u0000\nfilled up yet..."
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_OhDontHaveHabitat, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_08EC:
    VMJump L_0906

L_08F2:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you fill up the Habitat Lists, you might\ncomplete the Pokédex before you know it!"
    ParentActorMsg MSGFILE_SCRIPT, VirbankComplex2_Text_IfFillUpHabitat, 0, 0
    LastKeyWait
    ActorMsgClose

L_0906:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
