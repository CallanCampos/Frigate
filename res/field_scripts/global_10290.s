#include "asm/field_script.inc"
#include "text/script/global_10290.h"

// Script plugin 4, from the zones that start its scripts

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_004E
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x4085, 1
    VMCall L_0560
    VMCall L_004E
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_004E:
    VMStackPushFlag EVENT_FLAG_0x007b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0083
    FlagSet EVENT_FLAG_0x007b
    // "...So! You![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_SoYou, 0, 0, 0
    // "You are a fantastic Pokémon Trainer![f000]븁\u0000\nWill you participate in the great\nexperiment of the century?[f000]븁\u0000\nYou need two DS systems to use\nPoké Transfer![f000]븁\u0000\nWe will conduct the experiment with\nanother DS. Is that OK?"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_FantasticPokemonTrainerWill, 0, 0, 0
    VMJump L_008F

L_0083:
    // "Hey, le Trainer fantastique! Want to be\npart of the experiment of the century?[f000]븁\u0000\nYou need two DS systems to play with\nPoké Transfer![f000]븁\u0000\nDo you have a second DS system that you\ncan use?"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_HeyLeTrainerFantastique, 0, 0, 0

L_008F:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0199
    WorkSetConst 0x8021, 0
    Cmd_01DD 5, 0, 0
    BoxGetCount 0x8021, 5
    VMStackPush 0x8021
    VMStackPushConst 6
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0187
    // "OK! But just remember! ¡Muy importante![f000]븁\u0000\nOnce you bring a Pokémon here, you can't\nsend it back. Do you still want to do it?"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_OkButJustRemember, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_017B
    ActorMsgClose
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0119
    RTCallGlobal 2005
    VMCall L_01B3
    VMJump L_0175

L_0119:
    // "DS Wireless Communications will\nbe launched."
    SystemMsg Global10290_Text_DsWirelessCommunicationsWill, 0
    YesNoWin 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016F
    VMCall L_01C1
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0163
    VMCall L_0228
    VMCall L_01B3
    VMJump L_0169

L_0163:
    VMCall L_01B3

L_0169:
    VMJump L_0175

L_016F:
    VMCall L_01B3

L_0175:
    VMJump L_0181

L_017B:
    VMCall L_01B3

L_0181:
    VMJump L_0193

L_0187:
    WorkSetConst 0x8020, 2
    VMCall L_01A1

L_0193:
    VMJump L_019F

L_0199:
    VMCall L_01B3

L_019F:
    VMReturn

L_01A1:
    ActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn

L_01B3:
    WorkSetConst 0x8020, 1
    VMCall L_01A1
    VMReturn

L_01C1:
    WorkSetConst 0x8022, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    RTCallGlobal 2003
    WorkSet 0x8022, 0x8000
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0220
    WorkSetConst 0x8010, 0
    VMJump L_0226

L_0220:
    WorkSetConst 0x8010, 1

L_0226:
    VMReturn

L_0228:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    VMStackPush 0x8000
    RTCallGlobal 2004
    WorkSet 0x8023, 0x8000
    VMStackPop 0x8000
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0261
    VMReturn
    VMJump L_0276

L_0261:
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0276
    VMReturn

L_0276:
    FunfestBGMReturn
    // "OK, OK. Come here. Stand right there![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_OkOkComeHere, 0, 0, 0
    ActorMsgClose
    VMCall L_0460
    WorkSetConst EVENT_WORK_0x408d, 1
    PalParkCmd_CallMbParent
    WorkSetConst EVENT_WORK_0x408d, 0
    VMCall L_04CA
    PalParkCmd_GetInfo 0, 0x8024
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C4
    VMCall L_039D
    VMJump L_039B

L_02C4:
    VMStackPush 0x8024
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DD
    VMJump L_039B

L_02DD:
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0308
    VMCall L_039D
    // "I put the Pokémon you caught\nin your PC Box.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_PutPokemonCaughtPc, 0, 0, 0
    VMJump L_039B

L_0308:
    WorkSetConst 0x8025, 0
    // "Great! Molto bene![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_GreatMoltoBene, 0, 0, 0
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033F
    VMCall L_03AB
    WorkSetConst 0x8025, 15
    VMJump L_0389

L_033F:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0364
    VMCall L_03AB
    WorkSetConst 0x8025, 15
    VMJump L_0389

L_0364:
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0383
    WorkSetConst 0x8025, 16
    VMJump L_0389

L_0383:
    WorkSetConst 0x8025, 16

L_0389:
    ActorMsg MSGFILE_SCRIPT, 0x8025, 0, 0, 0
    WorkSetConst 0x8025, 0

L_039B:
    VMReturn

L_039D:
    // "Ugh...\nThere seems to be a communication error.[f000]븁\u0000\nI'm afraid you have to try it again![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_UghThereSeemsCommunication, 0, 0, 0
    VMReturn

L_03AB:
    WorkSetConst 0x8026, 0
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 900
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_03D0
    WorkSetConst 0x8026, 14
    VMJump L_0452

L_03D0:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 800
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_03EF
    WorkSetConst 0x8026, 13
    VMJump L_0452

L_03EF:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 700
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_040E
    WorkSetConst 0x8026, 12
    VMJump L_0452

L_040E:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 550
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_042D
    WorkSetConst 0x8026, 11
    VMJump L_0452

L_042D:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 400
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_044C
    WorkSetConst 0x8026, 10
    VMJump L_0452

L_044C:
    WorkSetConst 0x8026, 9

L_0452:
    ActorMsg MSGFILE_SCRIPT, 0x8026, 0, 0, 0
    VMReturn

L_0460:
    WorkSetConst 0x8027, 0
    ActorCmdExec 0, Movement_0504
    ActorCmdWait
    ActorCmdExec 255, Movement_0514
    ActorCmdWait
    ActorCmdExec 0, Movement_051C
    ActorCmdWait
    BMCreateHandleByGPos 0x8027, 9, 10, 3
    SEPlay SEQ_SE_FLD_125
    BMHndAudioVisualAnmPlay 0x8027, 0
    BMHndAnmWait 0x8027
    BMHndAnmPlay 0x8027, 1
    SEWait
    ActorCmdExec 0, Movement_0534
    ActorCmdWait
    // "Well, let's begin! Allons-y![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_WellLetsBeginAllons, 0, 0, 0
    ActorMsgClose
    FadeOutBlackQ
    FadeWait
    BMHndAnmPause 0x8027
    BMReleaseHandle 0x8027
    VMReturn

L_04CA:
    FadeInBlackQ
    FadeWait
    ActorCmdExec 255, Movement_0544
    ActorCmdWait
    ActorCmdExec 0, Movement_054C
    ActorCmdWait
    ActorCmdExec 255, Movement_0558
    ActorCmdWait
    VMReturn

L_04EE:
    ActorCmdExec 255, Movement_0544
    ActorCmdWait
    ActorCmdExec 0, Movement_054C
    ActorCmdWait
    VMReturn

Movement_0504:
    Move 16, 1
    Move 18, 1
    Move 35, 1
    MoveEnd

Movement_0514:
    Move 12, 4
    MoveEnd

Movement_051C:
    Move 16, 1
    Move 18, 1
    Move 36, 1
    Move 19, 4
    Move 36, 1
    MoveEnd

Movement_0534:
    Move 18, 3
    Move 17, 1
    Move 36, 1
    MoveEnd

Movement_0544:
    Move 13, 4
    MoveEnd

Movement_054C:
    Move 19, 1
    Move 17, 1
    MoveEnd

Movement_0558:
    Move 32, 1
    MoveEnd

L_0560:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    PlayerGetGPos 0x8028, 0x8029
    VMStackPush 0x8028
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0593
    ActorCmdExec 1, Movement_0800
    VMJump L_05BC

L_0593:
    VMStackPush 0x8028
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05B4
    ActorCmdExec 1, Movement_0814
    VMJump L_05BC

L_05B4:
    ActorCmdExec 1, Movement_0828

L_05BC:
    ActorCmdWait
    // "Hi, hello![f000]븁\u0000\nYou came here.\nThat means you are a visitor?[f000]븁\u0000\nHuh? No?\nYou came all the way here?[f000]븁\u0000\nAh, you are a Trainer?\nAre you in the middle of your journey?[f000]븁\u0000\nAh, this is your Trainer Card?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_HiHelloCameHere, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_08D8
    ActorCmdWait
    // "W-w-what? You!\nYou have all the Gym Badges?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_WWWhatHave, 1, 0, 1
    ActorMsgClose
    // "Great!\nYou may be able to...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_GreatMayAble, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_08E0
    ActorCmdWait
    // "Professor Paaaaaark![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_ProfessorPaaaaaark, 1, 0, 0
    ActorMsgClose
    VMStackPush 0x8028
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_064B
    ActorCmdExec 1, Movement_085C
    ActorCmdWait
    // "Quick, quick![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_QuickQuick, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_0894
    ActorCmdExec 255, Movement_0834
    VMJump L_06B4

L_064B:
    VMStackPush 0x8028
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_068C
    ActorCmdExec 1, Movement_0870
    ActorCmdWait
    // "Quick, quick![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_QuickQuick, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_0894
    ActorCmdExec 255, Movement_0844
    VMJump L_06B4

L_068C:
    ActorCmdExec 1, Movement_0884
    ActorCmdWait
    // "Quick, quick![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_QuickQuick, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_0894
    ActorCmdExec 255, Movement_0854

L_06B4:
    ActorCmdWait
    // "Oh, there you are![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_OhThere, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_08D0
    ActorCmdWait
    // "Professor![f000]븁\u0000\nDo you have a minute?\nI think you do. Listen![f000]븁\u0000\nThis kid is a Trainer who has all the\nGym Badges. All of them![f000]븁\u0000\nI am sure this Trainer can operate it![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_ProfessorHaveMinuteThink, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_08A0
    ActorCmdWait
    ActorCmdExec 255, Movement_08B8
    ActorCmdWait
    // "I have told you many times.[f000]븁\u0000\nThis invention is great indeed, but\nwithout a professional Trainer...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_HaveToldManyTimes, 0, 4, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_08D8
    ActorCmdWait
    ActorCmdExec 0, Movement_08E8
    ActorCmdWait
    // "What? You have the Gym Badges?\nALL of them?![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_WhatHaveGymBadges, 0, 0, 1
    ActorMsgClose
    // "Fantastic![f000]븁\u0000\nFantastico![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_FantasticFantastico, 0, 0, 1
    ActorMsgClose
    // "Yahoooooooooooooooo![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_Yahoooooooooooooooo, 0, 0, 1
    ActorMsgClose
    ActorCmdExec 0, Movement_08F4
    ActorCmdWait
    // "What a great day! Great! Unbelievable!\nHi, I am Professor Andrew Park![f000]븁\u0000\nYou!\nEr...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_WhatGreatDayGreat, 0, 0, 0
    ActorMsgClose
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_Empty_2, 1, 0, 0
    ActorMsgClose
    // "[f000]Ā\u0001\u0000! Are you willing to\nparticipate in an ambitious experiment[f000]븀\u0000\nthat will make history?[f000]븁\u0000\nThis device is called Poké Transfer.[f000]븁\u0000\nIt connects...blah blah blah...\n...called DS system...meanwhile...[f000]븁\u0000\n...blah blah...of molecules...and then...\n...while evoking...blah blah...[f000]븁\u0000\n...ergo, energy particles will...\n...blah blah...and if the frequency...[f000]븁\u0000\n...as you see...with faraway Pokémon...\n...the Alpha waves...blah blah...[f000]븁\u0000\n...following which...reach convergence...\n...blah blah...spectacular results![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_WillingParticipateAmbitiousExperiment, 0, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_08C0
    ActorCmdWait
    ActorCmdExec 255, Movement_08C8
    ActorCmdWait
    // "To put it more simply, if you use this\ndevice, you may be able to bring Pokémon[f000]븀\u0000\nhere from other regions.[f000]븁\u0000\nBut just for safety's sake, the Pokémon\nyou're transferring shouldn't be holding[f000]븀\u0000\nanything! So make sure to take their[f000]븀\u0000\nitems first.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10290_Text_PutMoreSimplyIf, 1, 0, 0
    ActorMsgClose
    ActorCmdExec 1, Movement_08D0
    ActorCmdExec 255, Movement_08D0
    ActorCmdWait
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Your Pokémon can't be holding items when\nyou transfer them. It's safer that way.[f000]븁\u0000\nIn rare cases, there are Pokémon that\ncannot be transferred, but[f000]븀\u0000\nProfessor Park will explain it to you."
    ParentActorMsg MSGFILE_SCRIPT, Global10290_Text_PokemonCantHoldingItems, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "With this research of mine, I also want to\nhave an impact on people in the future...[f000]븀\u0000\npeople living 100 or 200 years from now!"
    ParentActorMsg MSGFILE_SCRIPT, Global10290_Text_ResearchMineAlsoWant, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Here, we are monitoring Poké Transfer.[f000]븁\u0000\nWe're keeping a careful eye to make sure\nall the Pokémon have safe travels!"
    ParentActorMsg MSGFILE_SCRIPT, Global10290_Text_HereWeMonitoringPoke, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0800:
    Move 75, 1
    Move 13, 1
    Move 14, 1
    Move 1, 1
    MoveEnd

Movement_0814:
    Move 75, 1
    Move 13, 1
    Move 15, 1
    Move 1, 1
    MoveEnd

Movement_0828:
    Move 75, 1
    Move 13, 1
    MoveEnd

Movement_0834:
    Move 12, 1
    Move 15, 1
    Move 12, 12
    MoveEnd

Movement_0844:
    Move 12, 1
    Move 14, 1
    Move 12, 12
    MoveEnd

Movement_0854:
    Move 12, 13
    MoveEnd

Movement_085C:
    Move 19, 1
    Move 16, 1
    Move 63, 1
    Move 37, 1
    MoveEnd

Movement_0870:
    Move 18, 1
    Move 16, 1
    Move 63, 1
    Move 37, 1
    MoveEnd

Movement_0884:
    Move 16, 1
    Move 63, 1
    Move 37, 1
    MoveEnd

Movement_0894:
    Move 16, 12
    Move 1, 1
    MoveEnd

Movement_08A0:
    Move 2, 1
    Move 71, 1
    Move 15, 1
    Move 72, 1
    Move 0, 1
    MoveEnd

Movement_08B8:
    Move 12, 1
    MoveEnd

Movement_08C0:
    Move 2, 1
    MoveEnd

Movement_08C8:
    Move 3, 1
    MoveEnd

Movement_08D0:
    Move 0, 1
    MoveEnd

Movement_08D8:
    Move 75, 1
    MoveEnd

Movement_08E0:
    Move 36, 1
    MoveEnd

Movement_08E8:
    Move 37, 1
    Move 17, 3
    MoveEnd

Movement_08F4:
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 49, 2
    MoveEnd

Script_6:
    VMCall L_04EE
    WorkSetConst EVENT_WORK_0x408d, 0
    RTEndGlobal
    VMHalt
