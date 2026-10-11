#include "asm/field_script.inc"
#include "text/script/route_19.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x358000, 0x1000f, 0x2ba8000, 16
    EvCameraWait
    // "Bianca: Heeey!\nThis way![f000]븁\u0000"
    // "Bianca: Come on!\nThis way![f000]븁\u0000"
    ActorMsgGendered 1024, Route19_Text_BiancaHeeeyWay, Route19_Text_BiancaComeWay, 5, 0, 0
    MsgWinCloseAll
    VMCall L_0192
    ActorCmdExec 5, Movement_07A8
    ActorCmdWait
    // "Bianca: This kinda reminds me\nof that day on Route 1.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_BiancaKindaRemindsDay, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0798
    ActorCmdWait
    // "OK, here's how it works.[f000]븁\u0000\nThe Pokédex's pages fill up\nautomatically when you meet Pokémon![f000]븁\u0000\nAnd when you catch a Pokémon,\nmore detailed information on it[f000]븀\u0000\nis added to the Pokédex![f000]븁\u0000\nHere, I'll show you how to catch\na Pokémon! Starting...NOW![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_OkHeresHowWorks, 5, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    EvCameraMoveTo 9688, 0, 0xed000, 0x3d8000, 0x1000f, 0x2b88000, 72
    ActorCmdExec 5, Movement_0250
    ActorCmdExec 255, Movement_0258
    ActorCmdWait
    EvCameraWait
    CallCaptureDemo
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x3d8000, 0x1000f, 0x2b88000, 1
    EvCameraWait
    CallWildBattleEnd
    EvCameraMoveToDefault 32
    ActorWalkRoute 5, 58, 696, 1, 8, 1
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    // "Bianca: What a relief!\nI caught a Pokémon![f000]븁\u0000\nOh! Um...\nRight. I'll go over the important stuff.[f000]븁\u0000\nFirst, go find a healthy\nPokémon to catch![f000]븀\u0000\nYou need to remember this next bit![f000]븁\u0000\nIt's best to lower the Pokémon's HP\nbefore you try to catch it.[f000]븁\u0000\nUse your Pokémon's moves to lower the HP\nof the Pokémon you want to catch.[f000]븀\u0000\nMaking it fall asleep or paralyzing it[f000]븀\u0000\nwill make it even easier to catch![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_BiancaWhatReliefCaught, 5, 0, 0
    MsgWinCloseAll
    // "You're going to go deliver\nthe Town Map to your friend, right?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_YoureGoingGoDeliver, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_07A8
    ActorCmdWait
    // "Continue straight this way\nto get to Floccesy Town![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_ContinueStraightWayGet, 5, 0, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_07A0
    ActorCmdWait
    // "Bye now!\nMeet lots of Pokémon[f000]븀\u0000\nand catch a lot of them, OK?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_ByeNowMeetLots, 5, 0, 0
    MsgWinCloseAll
    VMCall L_0205
    WorkSetConst EVENT_WORK_0x40a3, 1
    FlagSet EVENT_FLAG_0x03db
    FlagReset EVENT_FLAG_0x02e3
    FlagSet EVENT_FLAG_0x02e8
    FlagSet EVENT_FLAG_0x02e7
    FlagReset EVENT_FLAG_0x02e4
    FlagReset EVENT_FLAG_0x0323
    HollowRivalCmd_0262 4, 0
    WorkSetConst EVENT_WORK_0x40a1, 8
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0192:
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 53
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01C1
    ActorWalkRoute 255, 53, 698, 1, 8, 0
    ActorCmdWait
    VMJump L_01CB

L_01C1:
    ActorCmdExec 255, Movement_0758
    ActorCmdWait

L_01CB:
    ActorCmdWait
    VMReturn
    ActorCmdExec 255, Movement_0798
    ActorCmdWait
    ActorNew 53, 703, 0, 251, 249, 0
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8022, 1
    ActorWalkRoute 251, 0x8021, 0x8022, 1, 8, 0
    ActorCmdWait
    VMReturn

L_0205:
    ActorCmdExec 5, Movement_0264
    VMSleep 16
    ActorCmdExec 255, Movement_0798
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 5
    SEWait
    VMReturn
    ActorWalkRoute 251, 53, 702, 1, 8, 1
    ActorCmdWait
    ActorCmdExec 251, Movement_0750
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    VMReturn
    .balign 4, 0

Movement_0250:
    Move 15, 8
    MoveEnd

Movement_0258:
    Move 12, 2
    Move 15, 3
    MoveEnd

Movement_0264:
    Move 13, 1
    Move 14, 5
    Move 13, 6
    MoveEnd
    VMStackMul
    VMNop2
    VMStackSub
    VMStackPushConst 254
    VMNop

Script_2:
    ActorsPauseAll
    // "???: You there, Trainer![f000]븁\u0000"
    InfoMsg Route19_Text_ThereTrainer, 1
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x5d8000, 0x6005f, 0x2b38000, 20
    ActorCmdExec 255, Movement_0790
    ActorCmdWait
    EvCameraWait
    BGMPlay SEQ_BGM_E_CHAMPION
    // "My name is Alder![f000]븁\u0000\nI'm a Trainer with a keen interest in the\nworld. One of my goals is to tell people[f000]븀\u0000\nabout how wonderful it is to walk toward[f000]븀\u0000\nthe future together with Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_NameAlderImTrainer, 1, 0, 0
    MsgWinCloseAll
    EvCameraMoveToDefault 30
    ActorJumpToGPos 1, 93, 1, 693
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMSleep 14
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8021, 2
    VMStackPush 0x8022
    VMStackPushConst 693
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0308
    ActorWalkRoute 1, 0x8021, 0x8022, 1, 8, 1

L_0308:
    ActorCmdExec 255, Movement_0788
    ActorCmdWait
    ActorCmdExec 1, Movement_07A0
    ActorCmdWait
    WordSetPlayerName 0
    // "And you are?[f000]븁\u0000\n...\n...[f000]븁\u0000\nHmph! So you're [f000]Ā\u0001\u0000\nfrom Aspertia City![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_HmphYoureFromAspertia, 1, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8022, 693
    VMJumpIf CMP_EQ, L_0346
    VMJump L_0354

L_0346:
    ActorCmdExec 1, Movement_03C0
    VMJump L_037D

L_0354:
    WorkCmpConst 0x8022, 697
    VMJumpIf CMP_EQ, L_0367
    VMJump L_0375

L_0367:
    ActorCmdExec 1, Movement_03F4
    VMJump L_037D

L_0375:
    ActorCmdExec 1, Movement_0428

L_037D:
    ActorCmdWait
    WorkSetConst 0x8023, 0
    PokePartyGetMemberByType 0x8023, 2
    WordSetPartyPokeSpecies 1, 0x8023
    // "Your [f000]ā\u0001\u0001 is\na fine-looking Pokémon![f000]븁\u0000\nBut, you're not exactly\na seasoned Trainer yet...[f000]븁\u0000\nIndeed! I'll train you a little!\nFollow me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_FineLookingPokemonBut, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0464
    ActorCmdWait
    BGMChangeMap
    ActorDelete 1
    WorkSetConst EVENT_WORK_0x40a3, 2
    FlagSet EVENT_FLAG_0x02e2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_03C0:
    Move 14, 1
    Move 63, 1
    Move 13, 1
    Move 14, 1
    Move 32, 1
    Move 14, 1
    Move 12, 1
    Move 35, 1
    Move 13, 1
    Move 15, 3
    Move 12, 1
    Move 34, 1
    MoveEnd

Movement_03F4:
    Move 14, 1
    Move 63, 1
    Move 12, 1
    Move 14, 1
    Move 33, 1
    Move 14, 1
    Move 13, 1
    Move 35, 1
    Move 12, 1
    Move 15, 3
    Move 13, 1
    Move 34, 1
    MoveEnd

Movement_0428:
    Move 14, 1
    Move 63, 1
    Move 12, 1
    Move 14, 1
    Move 33, 1
    Move 14, 1
    Move 13, 1
    Move 35, 1
    Move 13, 1
    Move 15, 1
    Move 32, 1
    Move 15, 2
    Move 12, 1
    Move 34, 1
    MoveEnd

Movement_0464:
    Move 15, 7
    MoveEnd

Script_7:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x02e2
    ActorAdd 1
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000![f000]븁\u0000"
    InfoMsg Route19_Text_Empty_8, 2
    MsgWinCloseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x5d8000, 0x6005f, 0x2b38000, 40
    ActorCmdExec 255, Movement_0790
    ActorCmdWait
    EvCameraWait
    BGMPlay SEQ_BGM_E_CHAMPION
    VMSleep 40
    EvCameraMoveToDefault 64
    ActorJumpToGPos 1, 93, 1, 693
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMSleep 14
    PlayerGetGPos 0x8021, 0x8022
    WorkAdd 0x8021, 2
    ActorWalkRoute 1, 0x8021, 0x8022, 1, 8, 1
    ActorCmdExec 255, Movement_07A8
    ActorCmdWait
    // "Excuse me! I forgot to tell\nyou something important![f000]븁\u0000\nFirst, take these![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_ExcuseForgotTellSomething, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0768
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 155
    WorkSet 0x8001, 5
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    ActorCmdExec 1, Movement_05B0
    ActorCmdWait
    WordSetPlayerName 0
    // "Those are Oran Berries![f000]븁\u0000\nIf you give one to your Pokémon,\nits HP will be restored.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_ThoseOranBerriesIf, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0798
    ActorCmdWait
    // "What's more! You can give your Pokémon\na Berry to hold![f000]븁\u0000\nLike this Oran Berry, for instance.\nWhen a Pokémon holds this Berry, it can[f000]븀\u0000\neat the Berry if it gets hurt in the heat[f000]븀\u0000\nof battle and regain some of its lost HP![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_WhatsMoreCanGive, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_05A8
    ActorCmdWait
    // "Well... I just told you\nto challenge the Gym Leader.[f000]븁\u0000\nAspertia's Gym Leader\nis a very strong Pokémon Trainer![f000]븁\u0000\nBut you have nothing to worry about![f000]븁\u0000\nIf you think hard about what the\nPokémon at your side can do,[f000]븀\u0000\nand what you should do as a Trainer,[f000]븀\u0000\nvictory will be yours![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_WellJustToldChallenge, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_07A0
    ActorCmdWait
    // "And then you should take on\nstronger and stronger Trainers...[f000]븀\u0000\nActually, take on the Gym Leaders[f000]븀\u0000\nof each city![f000]븁\u0000\nWorking together with your Pokémon\nis what makes you grow as a Trainer.[f000]븁\u0000\nAs you and your Pokémon grow stronger,\nyour world will get broader![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_ThenShouldTakeStronger, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0464
    ActorCmdWait
    BGMChangeMap
    ActorDelete 1
    WorkSetConst EVENT_WORK_0x40a3, 4
    FlagSet EVENT_FLAG_0x02e2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_05A8:
    Move 100, 1
    MoveEnd

Movement_05B0:
    Move 15, 1
    Move 34, 1
    MoveEnd

Script_8:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 13, 1
    Move 32, 1
    MoveEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_069F
    // "Hey, you know about ledge jumping?"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_HeyKnowAboutLedge, 0, 4, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0689
    FlagSet EVENT_FLAG_0x0010
    // "That so...\nWell then, watch me carefully![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_WellThenWatchCarefully, 0, 4, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_07A0
    ActorCmdWait
    // "From the top of the ledge![f000]븁\u0000\nBoing![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_FromTopLedgeBoing, 0, 4, 0
    VMSleep 4
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0740
    VMSleep 4
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0667
    ActorCmdExec 255, Movement_07A0

L_0667:
    ActorCmdWait
    ActorCmdExec 0, Movement_07A8
    ActorCmdWait
    // "It's really cool how you can take\nshortcuts by jumping off these, right?"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_ItsReallyCoolHow, 0, 3, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0699

L_0689:
    // "Oh...[f000]븁\u0000\nI guess you would know you can\njump off these little ledges."
    ActorMsg MSGFILE_SCRIPT, Route19_Text_OhGuessWouldKnow, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0699:
    VMJump L_06AF

L_069F:
    // "It's really cool how you can take\nshortcuts by jumping off these, right?"
    ActorMsg MSGFILE_SCRIPT, Route19_Text_ItsReallyCoolHow, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_06AF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what? I'm searching for\nPokémon in the tall grass![f000]븁\u0000\nThat's right! If you don't want\nto meet Pokémon, avoid the tall grass!"
    ParentActorMsg MSGFILE_SCRIPT, Route19_Text_KnowWhatImSearching, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can use Surf?\nNice! We can be Surf buddies!"
    ParentActorMsg MSGFILE_SCRIPT, Route19_Text_CanUseSurfNice, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 19"
    MsgPlaceSign Route19_Text_Route19, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 19"
    MsgPlaceSign Route19_Text_Route19_2, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nMake an effort to talk to all the\npeople you meet during your journey![f000]븁\u0000\nChances are they will have something\nuseful to tell you."
    MsgPlaceSign Route19_Text_TrainerTipsMakeEffort, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a75
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0740:
    Move 58, 1
    Move 10, 1
    Move 2, 1
    MoveEnd

Movement_0750:
    Move 13, 1
    MoveEnd

Movement_0758:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd

Movement_0768:
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

Movement_0788:
    Move 3, 1
    MoveEnd

Movement_0790:
    Move 32, 1
    MoveEnd

Movement_0798:
    Move 33, 1
    MoveEnd

Movement_07A0:
    Move 34, 1
    MoveEnd

Movement_07A8:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
