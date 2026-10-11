#include "asm/field_script.inc"
#include "text/script/village_bridge.h"

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

Script_14:
    VMStackPush EVENT_WORK_0x40d4
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0083
    ActorSetRailPos 8, 8, 2, 12

L_0083:
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Village Bridge"
    MsgPlaceSign VillageBridge_Text_VillageBridge, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Village Bridge"
    MsgPlaceSign VillageBridge_Text_VillageBridge, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Village Bridge Restaurant\nVillage Sandwiches are our specialty!"
    MsgPlaceSign VillageBridge_Text_VillageBridgeRestaurantVillage, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    ISSSwitchQuery 0x8010, 1
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013D
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011F
    ISSSwitchEnable 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Derleth: Fweet fweet...\nFweeeeeet fweet fweet..."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_DerlethFweetFweetFweeeeeet, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0137

L_011F:
    ISSSwitchEnable 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Derleth: Fwee... Fwee...\nFffweeet fweet..."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_DerlethFweeFweeFffweeet, 0, 0
    LastKeyWait
    ActorMsgClose

L_0137:
    VMJump L_018C

L_013D:
    VMStackPush EVENT_WORK_0x400a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0174
    ISSSwitchDisable 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Derleth: What is piercing my mind is\na sad sound.[f000]븁\u0000\nWhat is piercing my heart is\na cold night wind."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_DerlethWhatPiercingMind, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x400a, 1
    VMJump L_018C

L_0174:
    ISSSwitchDisable 1
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Derleth: The only things that come out\nof my mouth are my whistle tunes and[f000]븀\u0000\ncomplaints about my life.[f000]븁\u0000\nThis bridge is a meeting place for people\nlike me who like to complain."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_DerlethOnlyThingsCome, 0, 0
    LastKeyWait
    ActorMsgClose

L_018C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    ISSSwitchQuery 0x8010, 3
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FC
    VMStackPush EVENT_WORK_0x400b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DE
    ISSSwitchEnable 3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Aickman: How about this? This sound!\nDoesn't it get to your heart? Your mind?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_AickmanHowAboutSound, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01F6

L_01DE:
    ISSSwitchEnable 3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Aickman: This is my best friend, my pal.\nIt knows all my sorrow, all my tears."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_AickmanBestFriendPal, 0, 0
    LastKeyWait
    ActorMsgClose

L_01F6:
    VMJump L_024B

L_01FC:
    VMStackPush EVENT_WORK_0x400b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0233
    ISSSwitchDisable 3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Aickman: I know my sound doesn't fit\nthis city, this town.[f000]븁\u0000\nBut I... I cannot change\nmy life, my style."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_AickmanKnowSoundDoesnt, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x400b, 1
    VMJump L_024B

L_0233:
    ISSSwitchDisable 3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Aickman: Y-you have great sparkles...\nSparkles in your eyes.[f000]븁\u0000\nPlease make our hopes, our dreams,\ncome true for us.[f000]븁\u0000\nGo grab the glory--go take on the world!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_AickmanYHaveGreat, 0, 0
    LastKeyWait
    ActorMsgClose

L_024B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    ISSSwitchQuery 0x8010, 2
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BB
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_029D
    ISSSwitchEnable 2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Russo: La la la la la..."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_RussoLaLaLa, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02B5

L_029D:
    ISSSwitchEnable 2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Russo: Testing...\nCheck one, check two, check, check, yup."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_RussoTestingCheckOne, 0, 0
    LastKeyWait
    ActorMsgClose

L_02B5:
    VMJump L_030A

L_02BB:
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02F2
    ISSSwitchDisable 2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Russo: Ahem, ahem!\nNow, something's not quite right.[f000]븁\u0000\nThis here microphone's all screwy.\nI can sing real good, promise!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_RussoAhemAhemNow, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x400c, 1
    VMJump L_030A

L_02F2:
    ISSSwitchDisable 2
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Russo: Now, you're the first person in an\nawful long time who's hung around to[f000]븀\u0000\nlisten and hear what I was singin' about.[f000]븁\u0000\nMuch obliged!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_RussoNowYoureFirst, 0, 0
    LastKeyWait
    ActorMsgClose

L_030A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    ISSSwitchQuery 0x8010, 4
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_037A
    VMStackPush EVENT_WORK_0x400d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Koontz: Singing gives life to my spirit.\nWill you listen to the voice of my spirit?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_KoontzSingingGivesLife, 0, 0
    LastKeyWait
    ActorMsgClose
    ISSSwitchEnable 4
    VMJump L_0374

L_035C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Koontz: Oh, you want to listen to my song\nafter all! Yes!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_KoontzOhWantListen, 0, 0
    LastKeyWait
    ActorMsgClose
    ISSSwitchEnable 4

L_0374:
    VMJump L_03C9

L_037A:
    VMStackPush EVENT_WORK_0x400d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B1
    ISSSwitchDisable 4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Koontz: Huh? Are you leaving already?\nI am always here."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_KoontzHuhLeavingAlready, 0, 0
    LastKeyWait
    ActorMsgClose
    WorkSetConst EVENT_WORK_0x400d, 1
    VMJump L_03C9

L_03B1:
    ISSSwitchDisable 4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Koontz: My song...\nDon't you like it?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_KoontzSongDontLike, 0, 0
    LastKeyWait
    ActorMsgClose

L_03C9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've steadily extended my win streak\nfor two years... And now it's over...[f000]븁\u0000\nBut I have a strong will.\nI declare that I'll try again[f000]븀\u0000\nto have a 1,000-win streak![f000]븁\u0000\nI won't battle you next time, though.\nYou'll just break my streak."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_IveSteadilyExtendedWin, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_03EB:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_040A
    CallTrainerBattleEnd
    VMJump L_040C

L_040A:
    CallTrainerLose

L_040C:
    VMReturn

Script_9:
    ActorsPauseAll
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0427
    VMJump L_0435

L_0427:
    ActorCmdExec 8, Movement_0C14
    VMJump L_0456

L_0435:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0448
    VMJump L_0456

L_0448:
    ActorCmdExec 8, Movement_0C0C
    VMJump L_0456

L_0456:
    ActorCmdWait
    ActorCmdExec 8, Movement_0C88
    ActorCmdWait
    PlayerGetRailPos 0x8023, 0x8024, 0x8025
    WorkCmpConst 0x8025, 5
    VMJumpIf CMP_EQ, L_047D
    VMJump L_048B

L_047D:
    ActorCmdExec 8, Movement_0AD4
    VMJump L_056A

L_048B:
    WorkCmpConst 0x8025, 6
    VMJumpIf CMP_EQ, L_049E
    VMJump L_04AC

L_049E:
    ActorCmdExec 8, Movement_0AE0
    VMJump L_056A

L_04AC:
    WorkCmpConst 0x8025, 7
    VMJumpIf CMP_EQ, L_04BF
    VMJump L_04CD

L_04BF:
    ActorCmdExec 8, Movement_0AEC
    VMJump L_056A

L_04CD:
    WorkCmpConst 0x8025, 8
    VMJumpIf CMP_EQ, L_04E0
    VMJump L_04E6

L_04E0:
    VMJump L_056A

L_04E6:
    WorkCmpConst 0x8025, 9
    VMJumpIf CMP_EQ, L_04F9
    VMJump L_0507

L_04F9:
    ActorCmdExec 8, Movement_0AF8
    VMJump L_056A

L_0507:
    WorkCmpConst 0x8025, 10
    VMJumpIf CMP_EQ, L_051A
    VMJump L_0528

L_051A:
    ActorCmdExec 8, Movement_0B04
    VMJump L_056A

L_0528:
    WorkCmpConst 0x8025, 11
    VMJumpIf CMP_EQ, L_053B
    VMJump L_0549

L_053B:
    ActorCmdExec 8, Movement_0B10
    VMJump L_056A

L_0549:
    WorkCmpConst 0x8025, 12
    VMJumpIf CMP_EQ, L_055C
    VMJump L_056A

L_055C:
    ActorCmdExec 8, Movement_0B1C
    VMJump L_056A

L_056A:
    ActorCmdWait
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0583
    VMJump L_0591

L_0583:
    ActorCmdExec 8, Movement_0BDC
    VMJump L_05B2

L_0591:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_05A4
    VMJump L_05B2

L_05A4:
    ActorCmdExec 8, Movement_0BE4
    VMJump L_05B2

L_05B2:
    ActorCmdWait
    // "Wait! Waaait![f000]븁\u0000\nI've been waiting for this day!\nYou're the 1,000th opponent![f000]븁\u0000\nI've got a 999-win streak.\nBe my battle opponent!"
    ActorMsg MSGFILE_SCRIPT, VillageBridge_Text_WaitWaaaitIveBeen, 8, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0719
    // "Gwa ha ha!\nEven though you're just a fledgling,[f000]븀\u0000\nyou'll still be my 1,000th win in a row![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VillageBridge_Text_GwaHaHaEven, 8, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_GENTLEMAN_STONEWALL, 0, 0
    VMCall L_03EB
    WorkSetConst EVENT_WORK_0x40d4, 1
    // "I've steadily extended my win streak\nfor two years... And now it's over...[f000]븁\u0000\nBut I have a strong will.\nI declare that I'll try again[f000]븀\u0000\nto have a 1,000-win streak![f000]븁\u0000\nI won't battle you next time, though.\nYou'll just break my streak."
    ActorMsg MSGFILE_SCRIPT, VillageBridge_Text_IveSteadilyExtendedWin, 8, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    PlayerGetRailPos 0x8023, 0x8024, 0x8025
    WorkCmpConst 0x8025, 5
    VMJumpIf CMP_EQ, L_0624
    VMJump L_0632

L_0624:
    ActorCmdExec 8, Movement_0B60
    VMJump L_0711

L_0632:
    WorkCmpConst 0x8025, 6
    VMJumpIf CMP_EQ, L_0645
    VMJump L_0653

L_0645:
    ActorCmdExec 8, Movement_0B6C
    VMJump L_0711

L_0653:
    WorkCmpConst 0x8025, 7
    VMJumpIf CMP_EQ, L_0666
    VMJump L_0674

L_0666:
    ActorCmdExec 8, Movement_0B78
    VMJump L_0711

L_0674:
    WorkCmpConst 0x8025, 8
    VMJumpIf CMP_EQ, L_0687
    VMJump L_0695

L_0687:
    ActorCmdExec 8, Movement_0B84
    VMJump L_0711

L_0695:
    WorkCmpConst 0x8025, 9
    VMJumpIf CMP_EQ, L_06A8
    VMJump L_06B6

L_06A8:
    ActorCmdExec 8, Movement_0B90
    VMJump L_0711

L_06B6:
    WorkCmpConst 0x8025, 10
    VMJumpIf CMP_EQ, L_06C9
    VMJump L_06D7

L_06C9:
    ActorCmdExec 8, Movement_0B9C
    VMJump L_0711

L_06D7:
    WorkCmpConst 0x8025, 11
    VMJumpIf CMP_EQ, L_06EA
    VMJump L_06F8

L_06EA:
    ActorCmdExec 8, Movement_0BA8
    VMJump L_0711

L_06F8:
    WorkCmpConst 0x8025, 12
    VMJumpIf CMP_EQ, L_070B
    VMJump L_0711

L_070B:
    VMJump L_0711

L_0711:
    ActorCmdWait
    VMJump L_0913

L_0719:
    // "I understand. I've got a 999-win streak!\nIt's natural to be intimidated.[f000]븁\u0000\nBut I can't let you go further\nunless you battle me![f000]븁\u0000\nAnd there's definitely no way around me.\nNope. You shouldn't use Surf[f000]븀\u0000\nto cross the river, for example."
    ActorMsg MSGFILE_SCRIPT, VillageBridge_Text_UnderstandIveGot999, 8, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0740
    VMJump L_0756

L_0740:
    ActorCmdExec 8, Movement_0BCC
    ActorCmdExec 255, Movement_0BCC
    VMJump L_077F

L_0756:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0769
    VMJump L_077F

L_0769:
    ActorCmdExec 8, Movement_0BD4
    ActorCmdExec 255, Movement_0BD4
    VMJump L_077F

L_077F:
    ActorCmdWait
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0794
    VMJump L_07A2

L_0794:
    ActorCmdExec 8, Movement_0BF4
    VMJump L_07C3

L_07A2:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_07B5
    VMJump L_07C3

L_07B5:
    ActorCmdExec 8, Movement_0BEC
    VMJump L_07C3

L_07C3:
    ActorCmdWait
    PlayerGetRailPos 0x8023, 0x8024, 0x8025
    WorkCmpConst 0x8025, 5
    VMJumpIf CMP_EQ, L_07E0
    VMJump L_07EE

L_07E0:
    ActorCmdExec 8, Movement_0B28
    VMJump L_08CD

L_07EE:
    WorkCmpConst 0x8025, 6
    VMJumpIf CMP_EQ, L_0801
    VMJump L_080F

L_0801:
    ActorCmdExec 8, Movement_0B30
    VMJump L_08CD

L_080F:
    WorkCmpConst 0x8025, 7
    VMJumpIf CMP_EQ, L_0822
    VMJump L_0830

L_0822:
    ActorCmdExec 8, Movement_0B38
    VMJump L_08CD

L_0830:
    WorkCmpConst 0x8025, 8
    VMJumpIf CMP_EQ, L_0843
    VMJump L_0849

L_0843:
    VMJump L_08CD

L_0849:
    WorkCmpConst 0x8025, 9
    VMJumpIf CMP_EQ, L_085C
    VMJump L_086A

L_085C:
    ActorCmdExec 8, Movement_0B40
    VMJump L_08CD

L_086A:
    WorkCmpConst 0x8025, 10
    VMJumpIf CMP_EQ, L_087D
    VMJump L_088B

L_087D:
    ActorCmdExec 8, Movement_0B48
    VMJump L_08CD

L_088B:
    WorkCmpConst 0x8025, 11
    VMJumpIf CMP_EQ, L_089E
    VMJump L_08AC

L_089E:
    ActorCmdExec 8, Movement_0B50
    VMJump L_08CD

L_08AC:
    WorkCmpConst 0x8025, 12
    VMJumpIf CMP_EQ, L_08BF
    VMJump L_08CD

L_08BF:
    ActorCmdExec 8, Movement_0B58
    VMJump L_08CD

L_08CD:
    ActorCmdWait
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_08E2
    VMJump L_08F0

L_08E2:
    ActorCmdExec 8, Movement_0C60
    VMJump L_0911

L_08F0:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0903
    VMJump L_0911

L_0903:
    ActorCmdExec 8, Movement_0C58
    VMJump L_0911

L_0911:
    ActorCmdWait

L_0913:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My favorite thing nowadays\nis to compete in the PWT!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_FavoriteThingNowadaysCompete, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Practice as if it were a real game! Play\nin a real game as if it were a practice!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_PracticeIfWereReal, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'll cross all the bridges\nin the Unova region![f000]븁\u0000\nEven the Marine Tube from Undella Town!\nHmm! I am so looking forward to it!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_IllCrossAllBridges, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I tried to ask for directions, but\nit turned out I was talking to a[f000]븀\u0000\nPokémon Trainer![f000]븁\u0000\nYou need to be careful, too."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_TriedAskDirectionsBut, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A87
    // "[f000]븉\u0001\u0002Oh... Oh...\nSo...thirsty...[f000]븁\u0000\nI met you on\nthe Tubeline Bridge...[f000]븁\u0000\nG-g-give me...\nFresh Water...?[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_OhOhThirstyMet, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A73
    ItemSub ITEM_FRESH_WATER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A5F
    MsgWinCloseAll
    SEPlay SEQ_SE_ARDEMO_01
    SEWait
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    PlayerGetRailPos 0x8026, 0x8027, 0x8028
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A31
    ActorCmdExec 18, Movement_0AB0
    VMJump L_0A39

L_0A31:
    ActorCmdExec 18, Movement_0AC0

L_0A39:
    VMSleep 20
    ActorCmdExec 255, Movement_0ACC
    ActorCmdWait
    ActorDelete 18
    WorkSetConst EVENT_WORK_0x4108, 4
    FlagSet EVENT_FLAG_0x035d
    FlagReset EVENT_FLAG_0x035e
    VMJump L_0A6D

L_0A5F:
    // "[f000]븉\u0001\u0002But... You don't have Fresh Water...\nI appreciate the thought, though...[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_ButDontHaveFresh, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0A6D:
    VMJump L_0A81

L_0A73:
    // "[f000]븉\u0001\u0002Thank...[f000]븁\u0000\nWhat?\nOh...[f000]븁\u0000\nWithout Fresh Water...\nI can't run on bridges anymore.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_ThankWhatOhWithout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0A81:
    VMJump L_0AA8

L_0A87:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AA8
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0AA8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0AB0:
    Move 36, 4
    Move 16, 1
    Move 19, 13
    MoveEnd

Movement_0AC0:
    Move 39, 4
    Move 19, 13
    MoveEnd

Movement_0ACC:
    Move 3, 1
    MoveEnd

Movement_0AD4:
    Move 36, 2
    Move 16, 3
    MoveEnd

Movement_0AE0:
    Move 36, 2
    Move 16, 2
    MoveEnd

Movement_0AEC:
    Move 36, 2
    Move 16, 1
    MoveEnd

Movement_0AF8:
    Move 37, 2
    Move 17, 1
    MoveEnd

Movement_0B04:
    Move 37, 2
    Move 17, 2
    MoveEnd

Movement_0B10:
    Move 37, 2
    Move 17, 3
    MoveEnd

Movement_0B1C:
    Move 37, 2
    Move 17, 4
    MoveEnd

Movement_0B28:
    Move 17, 3
    MoveEnd

Movement_0B30:
    Move 17, 2
    MoveEnd

Movement_0B38:
    Move 17, 1
    MoveEnd

Movement_0B40:
    Move 16, 1
    MoveEnd

Movement_0B48:
    Move 16, 2
    MoveEnd

Movement_0B50:
    Move 16, 3
    MoveEnd

Movement_0B58:
    Move 16, 4
    MoveEnd

Movement_0B60:
    Move 13, 7
    Move 32, 1
    MoveEnd

Movement_0B6C:
    Move 13, 6
    Move 32, 1
    MoveEnd

Movement_0B78:
    Move 13, 5
    Move 32, 1
    MoveEnd

Movement_0B84:
    Move 13, 4
    Move 32, 1
    MoveEnd

Movement_0B90:
    Move 13, 3
    Move 32, 1
    MoveEnd

Movement_0B9C:
    Move 13, 2
    Move 32, 1
    MoveEnd

Movement_0BA8:
    Move 13, 1
    Move 32, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0BCC:
    Move 15, 1
    MoveEnd

Movement_0BD4:
    Move 14, 1
    MoveEnd

Movement_0BDC:
    Move 19, 1
    MoveEnd

Movement_0BE4:
    Move 18, 1
    MoveEnd

Movement_0BEC:
    Move 19, 2
    MoveEnd

Movement_0BF4:
    Move 18, 2
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd

Movement_0C0C:
    Move 2, 1
    MoveEnd

Movement_0C14:
    Move 3, 1
    MoveEnd
    VMStackDiv
    VMHalt
    PokePartyGetSpecies 0, 14
    VMHalt
    .byte 0xfe
    .balign 4, 0
    Move 10, 1
    MoveEnd
    Move 61, 1
    Move 32, 1
    Move 33, 1
    Move 61, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_0C58:
    Move 34, 1
    MoveEnd

Movement_0C60:
    Move 35, 1
    MoveEnd
    Move 71, 1
    Move 11, 1
    Move 72, 1
    MoveEnd
    Move 71, 1
    Move 13, 1
    Move 72, 1
    MoveEnd

Movement_0C88:
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 161, 1
    MoveEnd
    Move 160, 1
    MoveEnd

Script_15:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x01d4
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CD7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Keep somebody's secret.\nOtherwise, your secret will be out."
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_KeepSomebodysSecretOtherwise, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0D68

L_0CD7:
    VMStackPushFlag EVENT_FLAG_0x01d2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D54
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 158
    WorkSet 0x8001, 5
    WorkSet 0x8002, 468
    WorkSet 0x8003, 29
    WorkSet 0x8004, 30
    WorkSet 0x8005, 30
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0D68

L_0D54:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hear a sound from somewhere.\nSometimes it sounds sad.[f000]븀\u0000\nSometimes it sounds a little goofy...[f000]븀\u0000\nDo you think it could be a ghost?"
    ParentActorMsg MSGFILE_SCRIPT, VillageBridge_Text_HearSoundFromSomewhere, 0, 0
    LastKeyWait
    ActorMsgClose

L_0D68:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
