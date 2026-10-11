#include "asm/field_script.inc"
#include "text/script/medal_office.h"

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
    ScriptEntriesEnd

Script_11:
    VMStackPush EVENT_WORK_0x409e
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_004D
    ActorSetGPos 4, 7, 0, 13, 0

L_004D:
    VMHalt

Script_1:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x03f6
    WordSetPlayerName 0
    ActorCmdExec 1, Movement_03A8
    VMSleep 12
    ActorCmdExec 0, Movement_07CC
    ActorCmdExec 2, Movement_07CC
    ActorCmdWait
    // "[f000]Ā\u0001\u0000,\ncome here![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_ComeHere, 1, 1, 0
    ActorMsgClose
    ActorCmdExec 255, Movement_03B4
    ActorCmdWait
    SEPlay SEQ_SE_FLD_87
    SEWait
    ActorAdd 4
    WorkSetConst 0x8020, 0
    BMCreateHandleByGPos 0x8020, 1, 7, 1
    BMHndAudioVisualAnmPlay 0x8020, 0
    BMHndAnmWait 0x8020
    ActorCmdExec 4, Movement_03BC
    ActorCmdExec 255, Movement_07EC
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8020, 1
    BMHndAnmWait 0x8020
    BMReleaseHandle 0x8020
    // "Hello, [f000]Ā\u0001\u0000!\nWell, let's go straight to the ceremony![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_HelloWellLetsGo, 4, 1, 0
    ActorMsgClose
    ActorCmdExec 4, Movement_03C4
    VMSleep 32
    ActorCmdExec 255, Movement_03E8
    VMSleep 40
    ActorCmdExec 1, Movement_07F4
    VMSleep 48
    ActorCmdExec 0, Movement_07F4
    ActorCmdExec 2, Movement_07F4
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000,\nyou achieved great results[f000]븀\u0000\nin the Medal Rally...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_AchievedGreatResultsMedal, 4, 2, 0
    MultiMsg 6, 21, 3, 1
    VMSleep 10
    MultiMsg 7, 5, 5, 2
    VMSleep 10
    MultiMsg 8, 6, 1, 3
    VMSleep 30
    MsgWinCloseNo 1
    VMSleep 5
    MsgWinCloseNo 2
    VMSleep 5
    MsgWinCloseNo 3
    WordSetPlayerName 0
    WordSetMedalName 1, 1
    // "To honor your achievement...\nI will present you with[f000]븀\u0000\nthe [f000][ff00]\u0001\u0002[f000]ĵ\u0001\u0001[f000][ff00]\u0001\u0000 Medal![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_HonorAchievementWillPresent, 4, 2, 0
    MsgWinCloseAll
    MEPlay SEQ_ME_MD_FAN03
    MedalGetFieldEffectID 1, EVENT_WORK_0x400f
    PlayFieldEffect EVENT_WORK_0x400f
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received\nthe [f000][ff00]\u0001\u0002[f000]ĵ\u0001\u0001[f000][ff00]\u0001\u0000 Medal![f000]븁\u0000"
    SystemMsg MedalOffice_Text_ReceivedMedal, 2
    InfoMsgClose
    WordSetPlayerName 0
    // "The Medal Rally is far from over![f000]븁\u0000\nKeep up the good work,\nand receive many more Medals![f000]븁\u0000\nSee you!"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_MedalRallyFarFrom, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x409e, 3
    MedalAcknowledge 1, 1
    FlagSet EVENT_FLAG_0x03f6
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x03f6
    ActorCmdExec 1, Movement_03A8
    VMSleep 12
    ActorCmdExec 0, Movement_07CC
    ActorCmdExec 2, Movement_07CC
    ActorCmdWait
    WordSetPlayerName 0
    // "Eeeeeee!\n[f000]Ā\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_Eeeeeee, 1, 1, 0
    ActorMsgClose
    ActorCmdExec 255, Movement_03B4
    ActorCmdWait
    SEPlay SEQ_SE_FLD_87
    SEWait
    ActorAdd 4
    WorkSetConst 0x8021, 0
    BMCreateHandleByGPos 0x8021, 1, 7, 1
    BMHndAudioVisualAnmPlay 0x8021, 0
    BMHndAnmWait 0x8021
    ActorCmdExec 4, Movement_03BC
    ActorCmdExec 255, Movement_07EC
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8021, 1
    BMHndAnmWait 0x8021
    BMReleaseHandle 0x8021
    // "Welcome, [f000]Ā\u0001\u0000![f000]븁\u0000\nAt last, the time has come...[f000]븁\u0000\nThe time when you will be\nat the top of all medalists![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_WelcomeLastTimeHas, 4, 1, 0
    ActorMsgClose
    ActorCmdExec 4, Movement_03C4
    VMSleep 32
    ActorCmdExec 255, Movement_03E8
    VMSleep 40
    ActorCmdExec 1, Movement_07F4
    VMSleep 48
    ActorCmdExec 0, Movement_07F4
    ActorCmdExec 2, Movement_07F4
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000,\nyou collected ALL of the Medals...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_CollectedAllMedals, 4, 2, 0
    MultiMsg 14, 6, 0, 1
    VMSleep 10
    MultiMsg 15, 8, 5, 2
    VMSleep 10
    MultiMsg 16, 11, 10, 3
    VMSleep 30
    MsgWinCloseNo 1
    VMSleep 5
    MsgWinCloseNo 2
    VMSleep 5
    MsgWinCloseNo 3
    WordSetPlayerName 0
    WordSetMedalName 1, 6
    // "...To honor your achievement,\nI will present you with[f000]븀\u0000\nthe [f000][ff00]\u0001\u0002[f000]ĵ\u0001\u0001[f000][ff00]\u0001\u0000 Medal![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_HonorAchievementWillPresent_2, 4, 2, 0
    MsgWinCloseAll
    MEPlay SEQ_ME_MD_FAN04
    MedalGetFieldEffectID 6, EVENT_WORK_0x400f
    PlayFieldEffect EVENT_WORK_0x400f
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received\nthe [f000][ff00]\u0001\u0002[f000]ĵ\u0001\u0001[f000][ff00]\u0001\u0000 Medal![f000]븁\u0000"
    SystemMsg MedalOffice_Text_ReceivedMedal, 2
    InfoMsgClose
    MultiMsg 37, 9, 8, 1
    VMSleep 10
    MultiMsg 38, 3, 3, 2
    VMSleep 10
    MultiMsg 39, 12, 12, 3
    VMSleep 30
    MsgWinCloseNo 1
    MultiMsg 40, 14, 5, 4
    VMSleep 10
    MsgWinCloseNo 2
    MultiMsg 41, 3, 20, 5
    VMSleep 10
    MsgWinCloseNo 3
    MultiMsg 42, 16, 15, 6
    VMSleep 30
    MsgWinCloseNo 4
    VMSleep 5
    MsgWinCloseNo 5
    VMSleep 5
    MsgWinCloseNo 6
    WordSetPlayerName 0
    // "The legend of [f000]Ā\u0001\u0000,\nwho collected all the Medals, will be[f000]븀\u0000\npassed down forever![f000]븁\u0000\nYou're the Top Medalist!"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_LegendWhoCollectedAll, 4, 2, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x409e, 5
    MedalAcknowledge 6, 1
    WorkSetConst 0x8021, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03A8:
    Move 75, 1
    Move 32, 1
    MoveEnd

Movement_03B4:
    Move 13, 2
    MoveEnd

Movement_03BC:
    Move 13, 2
    MoveEnd

Movement_03C4:
    Move 13, 1
    Move 14, 1
    Move 13, 1
    Move 34, 1
    Move 14, 2
    Move 13, 9
    Move 15, 3
    Move 32, 1
    MoveEnd

Movement_03E8:
    Move 14, 3
    Move 13, 6
    Move 15, 3
    Move 13, 1
    MoveEnd

Script_10:
    ActorsPauseAll
    WordSetPlayerName 0
    VMStackPush EVENT_WORK_0x409e
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_042E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Medal Rally is far from over![f000]븁\u0000\nKeep up the good work,\nand receive many more Medals![f000]븁\u0000\nSee you!"
    ParentActorMsg MSGFILE_SCRIPT, MedalOffice_Text_MedalRallyFarFrom, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0442

L_042E:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The legend of [f000]Ā\u0001\u0000,\nwho collected all the Medals, will be[f000]븀\u0000\npassed down forever![f000]븁\u0000\nYou're the Top Medalist!"
    ParentActorMsg MSGFILE_SCRIPT, MedalOffice_Text_LegendWhoCollectedAll, 0, 0
    LastKeyWait
    ActorMsgClose

L_0442:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8022, 0
    MedalGetCount 7, 0x8022
    WordSetMedalRank 1, 0x8022
    WorkSetConst 0x8023, 0
    MedalGetCount 3, 0x8023
    WordSetNumber 0, 0x8023, 3
    WorkCmpConst 0x8022, 1
    VMJumpIf CMP_EQ, L_0485
    VMJump L_0497

L_0485:
    // "Will you show me your Medal Box?[f000]븁\u0000\n...[f000]븁\u0000\nThe number of Medals you've received:\n[f000]Ȃ\u0001\u0000![f000]븁\u0000\nYour medalist rank is\n[f000]Ķ\u0001\u0001 Rank![f000]븁\u0000\nGood job!\nBut there are many more Medals!"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_WillShowMedalBox, 1, 0, 0
    VMJump L_0512

L_0497:
    WorkCmpConst 0x8022, 2
    VMJumpIf CMP_EQ, L_04AA
    VMJump L_04BC

L_04AA:
    // "Will you show me your Medal Box?[f000]븁\u0000\n...[f000]븁\u0000\nThe number of Medals you've received:\n[f000]Ȃ\u0001\u0000![f000]븁\u0000\nYour medalist rank is\n[f000]Ķ\u0001\u0001 Rank![f000]븁\u0000\nWow, great!\nKeep up the good work!"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_WillShowMedalBox_2, 1, 0, 0
    VMJump L_0512

L_04BC:
    WorkCmpConst 0x8022, 3
    VMJumpIf CMP_EQ, L_04CF
    VMJump L_04E1

L_04CF:
    // "Will you show me your Medal Box?[f000]븁\u0000\n...[f000]븁\u0000\nThe number of Medals you've received:\n[f000]Ȃ\u0001\u0000![f000]븁\u0000\nYour medalist rank is\n[f000]Ķ\u0001\u0001 Rank![f000]븁\u0000\nNot many people reach this rank![f000]븁\u0000\nYou may be a genius\nat collecting Medals!"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_WillShowMedalBox_3, 1, 0, 0
    VMJump L_0512

L_04E1:
    WorkCmpConst 0x8022, 4
    VMJumpIf CMP_EQ, L_04F4
    VMJump L_0506

L_04F4:
    // "Will you show me your Medal Box?[f000]븁\u0000\n...[f000]븁\u0000\nThe number of Medals you've received:\n[f000]Ȃ\u0001\u0000![f000]븁\u0000\nYour medalist rank is\n[f000]Ķ\u0001\u0001 Rank![f000]븁\u0000\nI've never seen\n[f000]Ķ\u0001\u0001 Rank before.[f000]븁\u0000\nI've become a big fan!"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_WillShowMedalBox_4, 1, 0, 0
    VMJump L_0512

L_0506:
    // "Will you show me your Medal Box?[f000]븁\u0000\n...[f000]븁\u0000\nThe number of Medals you've received:\n[f000]Ȃ\u0001\u0000![f000]븁\u0000\nYour medalist rank is\n[f000]Ķ\u0001\u0001 Rank![f000]븁\u0000\nThe Medal Rally has just started!"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_WillShowMedalBox_5, 1, 0, 0

L_0512:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

L_053C:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_065D
    // "Hi! Is there anything\nyou want to ask me?"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_HiThereAnythingWant, 0, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32804
    ListMenuAdd 23, 65535, 0
    ListMenuAdd 24, 65535, 1
    ListMenuAdd 25, 65535, 2
    ListMenuAdd 26, 65535, 3
    ListMenuAdd 27, 65535, 4
    ListMenuAdd 28, 65535, 5
    ListMenuShow
    WorkCmpConst 0x8024, 0
    VMJumpIf CMP_EQ, L_05A9
    VMJump L_05BB

L_05A9:
    // "The Medal Rally is a competition to\nevaluate various activities of Trainers.[f000]븁\u0000\nData and records are sent from Medal\nRally participants' Medal Boxes and then[f000]븀\u0000\nevaluated by staff at the Medal Office.[f000]븁\u0000\nThe details aren't important.\nJust enjoy your journey![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_MedalRallyCompetitionEvaluate, 0, 0, 0
    VMJump L_0657

L_05BB:
    WorkCmpConst 0x8024, 1
    VMJumpIf CMP_EQ, L_05CE
    VMJump L_05E0

L_05CE:
    // "Medals are gifts from the Medal Office\nfor rally participants.[f000]븁\u0000\nIf you meet the requirements to\nreceive Medals, you can get them[f000]븀\u0000\nfrom Mr. Medal at a Pokémon Center.[f000]븁\u0000\nFor your information, there are\nfive types of Medals:[f000]븀\u0000\norange Adventure Medals,[f000]븀\u0000\nblue Battle Medals,[f000]븀\u0000\npink Entertainment Medals,[f000]븀\u0000\npurple Challenge Medals,[f000]븀\u0000\nand yellow Special Medals.[f000]븁\u0000\nAlso, the more difficult the Medals are to\nearn, the more decorative they become.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_MedalsGiftsFromMedal, 0, 0, 0
    VMJump L_0657

L_05E0:
    WorkCmpConst 0x8024, 2
    VMJumpIf CMP_EQ, L_05F3
    VMJump L_0605

L_05F3:
    // "Hint Medals are gray Medals\nfor you to see hints to obtain Medals.[f000]븁\u0000\nWhen you receive proper Medals,\nMr. Medal will collect the Hint Medals[f000]븀\u0000\nthat achieved their purpose.[f000]븁\u0000\nEven if you wanted to keep\na lot of Hint Medals, they wouldn't fit[f000]븀\u0000\nin your Medal Box anyway.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_HintMedalsGrayMedals, 0, 0, 0
    VMJump L_0657

L_0605:
    WorkCmpConst 0x8024, 3
    VMJumpIf CMP_EQ, L_0618
    VMJump L_062A

L_0618:
    // "A Medal Box is a box-shaped device\nfor storing Medals.[f000]븁\u0000\nYou can check the number of Medals,\nnames of Medals,[f000]븀\u0000\ndescriptions of Medals,[f000]븀\u0000\ndates you received them, and so on.[f000]븁\u0000\nPress START to change the shape\nof the box so that you can see[f000]븀\u0000\na lot of Medals at once, or you can[f000]븀\u0000\nchange the order of Medals.[f000]븁\u0000\nFor your information, I'm the one who\ncreated the Medal Box![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_MedalBoxBoxShaped, 0, 0, 0
    VMJump L_0657

L_062A:
    WorkCmpConst 0x8024, 4
    VMJumpIf CMP_EQ, L_063D
    VMJump L_064F

L_063D:
    // "A Favorite Medal is a Medal\nthat is shown to the public[f000]븀\u0000\nin communication.[f000]븁\u0000\nYou can recommend or brag\nabout the Medal in the Tag Log[f000]븀\u0000\nor in the Union Room.[f000]븁\u0000\nTo register your Favorite Medal,\npress the A Button after choosing[f000]븀\u0000\na Medal in your Medal Box.[f000]븁\u0000\nPress the A Button again\nto cancel it.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MedalOffice_Text_FavoriteMedalMedalShown, 0, 0, 0
    VMJump L_0657

L_064F:
    MsgWinCloseAll
    WorkSetConst 0x8025, 1

L_0657:
    VMJump L_053C

L_065D:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x409e
    VMStackPushConst 1
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_069E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Actually, I've been secretly cheering\nfor you.[f000]븁\u0000\nOf course, as a staff member at the\nMedal Office,[f000]븀\u0000\nI will judge fairly, though."
    ParentActorMsg MSGFILE_SCRIPT, MedalOffice_Text_ActuallyIveBeenSecretly, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_06F2

L_069E:
    VMStackPush EVENT_WORK_0x409e
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06CB
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Actually, I believed that you would be\nable to reach the goal.[f000]븁\u0000\nI was right![f000]븁\u0000\nI'll keep cheering for you.\nGood luck!"
    ParentActorMsg MSGFILE_SCRIPT, MedalOffice_Text_ActuallyBelievedWouldAble_2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_06F2

L_06CB:
    VMStackPush EVENT_WORK_0x409e
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06F2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Actually, I believed that you would be\nable to collect all the Medals.[f000]븁\u0000\nYou lived up to my expectation!\nYou're really great![f000]븁\u0000\nI'm very moved.\nThank you!"
    ParentActorMsg MSGFILE_SCRIPT, MedalOffice_Text_ActuallyBelievedWouldAble, 0, 0
    LastKeyWait
    ActorMsgClose

L_06F2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Welcome to the Medal Office."
    InfoMsg MedalOffice_Text_WelcomeMedalOffice, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WordSetPlayerName 0
    WorkSetConst 0x8026, 0
    MedalGetCount 3, 0x8026
    WordSetNumber 1, 0x8026, 3
    WorkSetConst 0x8027, 0
    MedalGetCount 7, 0x8027
    WordSetMedalRank 2, 0x8027
    SEPlay SEQ_SE_MESSAGE
    // "It's a graph showing the results\nof the Medal Rally.[f000]븁\u0000\n...[f000]븁\u0000\n[f000]Ā\u0001\u0000\nMedals received: [f000]Ȃ\u0001\u0001.[f000]븀\u0000\n[f000]Ķ\u0001\u0002 Rank[f000]븁\u0000\n..."
    InfoMsg MedalOffice_Text_ItsGraphShowingResults, 2
    LastKeyWait
    InfoMsgClose_0039
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Many types of Medals\nare on the wall."
    InfoMsg MedalOffice_Text_ManyTypesMedalsWall, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x40e2
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0138
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07B7
    SEPlay SEQ_SE_FLD_41
    // "I'm from the Castelia Harlequin Hunt![f000]븁\u0000\nYou found the Medal Office.\nAll riiight!"
    ParentActorMsg MSGFILE_SCRIPT, MedalOffice_Text_ImFromCasteliaHarlequin, 0, 0
    FlagSet EVENT_FLAG_0x0138
    WorkAdd EVENT_WORK_0x40e2, 1
    SEWait
    LastKeyWait
    MsgWinCloseAll
    VMJump L_07C5

L_07B7:
    // "You hunted for the Harlequin in the Medal\nOffice, too! Collect more Medals!"
    ParentActorMsg MSGFILE_SCRIPT, MedalOffice_Text_HuntedHarlequinMedalOffice, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_07C5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_07CC:
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

Movement_07EC:
    Move 32, 1
    MoveEnd

Movement_07F4:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
