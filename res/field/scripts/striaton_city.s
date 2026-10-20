#include "asm/field_script.inc"
#include "text/script/striaton_city.h"

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
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    RTCGetSeason 0x8020
    RTCGetDayPart 0x8021
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B2
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00A8
    FlagReset EVENT_FLAG_0x032f
    VMJump L_00AC

L_00A8:
    FlagSet EVENT_FLAG_0x032f

L_00AC:
    VMJump L_00B6

L_00B2:
    FlagSet EVENT_FLAG_0x032f

L_00B6:
    VMStackPushFlag EVENT_FLAG_0x0176
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x410c
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00DF
    WorkSetConst EVENT_WORK_0x410c, 0

L_00DF:
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Striaton City\nThree Stand Together as One!"
    MsgPlaceSign StriatonCity_Text_StriatonCityThreeStand, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Dreamyard Ahead"
    MsgPlaceSign StriatonCity_Text_DreamyardAhead, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainers' School\nBrush up on Pokémon knowledge!"
    MsgPlaceSign StriatonCity_Text_TrainersSchoolBrushUp, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a well-kept flower bed.[f000]븁\u0000\nSomeone who loves plants\nmust be taking care of it."
    InfoMsg StriatonCity_Text_ItsWellKeptFlower, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 89
    WorkSet 0x8001, 1
    WorkSet 0x8002, 140
    WorkSet 0x8003, 11
    WorkSet 0x8004, 12
    WorkSet 0x8005, 12
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    VMStackPush 0x8006
    WorkSet 0x8000, 13
    WorkSet 0x8001, 5
    WorkSet 0x8002, 142
    WorkSet 0x8003, 13
    WorkSet 0x8004, 14
    WorkSet 0x8005, 14
    RTGetTextFile 0x8006
    RTCallGlobal 2800
    VMStackPop 0x8006
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    RTCGetSeason 0x8020
    RTCGetDayPart 0x8021
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0290
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0276
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Right now, the hot thing is\nStriaton City's Stunfisk nights![f000]븁\u0000\nA huge school of them gathers.\nIt's a sight that's hard to describe.[f000]븁\u0000\nYou have to be careful not to\nstep on them."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_RightNowHotThing, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_028A

L_0276:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Recently, you can see Stunfisk\nin this pond when the sun goes down!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_RecentlyCanSeeStunfisk, 0, 0
    LastKeyWait
    ActorMsgClose

L_028A:
    VMJump L_02A4

L_0290:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Summer's so far away. I want to see\nagain the sight I saw that night."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_SummersFarAwayWant, 0, 0
    LastKeyWait
    ActorMsgClose

L_02A4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 618, 0
    // "Unn unnn?!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_UnnUnnn, 0, 0
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
    // "Well, I'll be!\nThose are some sparkling Gym Badges![f000]븁\u0000\nAnd you have eight of them, too![f000]븁\u0000\nThose Badges shine so brightly, it's like\nyou're gleaming as much as they are!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_WellIllThoseSome, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I still haven't defeated the\nStriaton City Gym Leaders...[f000]븁\u0000\nBut that's all right.[f000]븁\u0000\nI'm going to become such a strong\nTrainer, they'll want to challenge me!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_StillHaventDefeatedStriaton, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If a Pokémon type and a move type are\nthe same, the move's power will increase![f000]븁\u0000\nIf the Pokémon is holding a gem of that\ntype, the move's power goes up yet more!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_IfPokemonTypeMove, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "From the factory's once-busy days, many\ndreams still linger in the Dreamyard.[f000]븁\u0000\nA Pokémon led there by those dreams\nmay be somewhere about."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_FromFactorysOnceBusy, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When I explain to someone what I learned\nat school, I'm more connected to people,[f000]븀\u0000\nthanks to Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_WhenExplainSomeoneWhat, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Technical Machines can be used\nover and over, right?[f000]븁\u0000\nI tried so many different things!\nIt's sure hard to decide, eh?"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_TechnicalMachinesCanUsed, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Random EVENT_WORK_0x4000, 3
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03AB
    // "Er... Um...[f000]븁\u0000\nGrass-type Pokémon are weak\nagainst Fire-type moves.[f000]븁\u0000\nThat's why Cilan has trouble\nwinning against Chili!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_ErUmGrassType, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03F3

L_03AB:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D2
    // "Er... Um...[f000]븁\u0000\nWater-type Pokémon are weak\nagainst Grass-type moves.[f000]븁\u0000\nThat's why Cress has trouble\nwinning against Cilan."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_ErUmWaterType, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03F3

L_03D2:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F3
    // "Um... Er...[f000]븁\u0000\nFire-type Pokémon are weak\nagainst Water-type moves.[f000]븁\u0000\nThat's why Chili has trouble\nwinning against Cress."
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_UmErFireType, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03F3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Gym is gone, but the Dreamyard still\nbustles with Trainers looking to improve!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_GymGoneButDreamyard, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, my!\nYour Medal Box...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_OhMedalBox, 0, 0
    MedalGetMostCompleteCategory 0x8022
    VMStackPush 0x8022
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0452
    // "The gold Special Medals\nare particularly sparkly![f000]븁\u0000\nLife is special![f000]븁\u0000\nTreasure every day, and don't\npass a day the same way twice!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_GoldSpecialMedalsParticularly, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04E8

L_0452:
    VMStackPush 0x8022
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0479
    // "The purple Challenge Medals\nare particularly sparkly![f000]븁\u0000\nLife is a challenge![f000]븁\u0000\nThe harder it is to do,\nthe more it's worth doing![f000]븁\u0000\nYou can feel happy when\nyou've grown as a person!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_PurpleChallengeMedalsParticularly, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04E8

L_0479:
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A0
    // "The blue Battle Medals\nare particularly sparkly![f000]븁\u0000\nIn life, you have to draw a line\nbetween black and white.[f000]븁\u0000\nCompete, aim for the top,\nand grow together!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_BlueBattleMedalsParticularly, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04E8

L_04A0:
    VMStackPush 0x8022
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C7
    // "The pink Fun Medals\nare particularly sparkly![f000]븁\u0000\nLife is entertainment![f000]븁\u0000\nAlways have a smile on your face!\nThe one who has the most fun wins!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_PinkFunMedalsParticularly, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_04E8

L_04C7:
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E8
    // "The orange Adventure Medals\nare particularly sparkly![f000]븁\u0000\nLife is an adventure![f000]븁\u0000\nNo matter how old you get,\ndon't lose your sense of adventure!"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_OrangeAdventureMedalsParticularly, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_04E8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is a delicious restaurant where\nyou can also enjoy Pokémon battles![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, StriatonCity_Text_DeliciousRestaurantWhereCan, 0, 0
    MsgWinCloseAll
    ActorCmdExec 23, Movement_0548
    ActorCmdWait
    BMCreateHandleByGPos 0x8010, 1, 788, 586
    BMHndAudioVisualAnmPlay 0x8010, 0
    BMHndAnmWait 0x8010
    ActorCmdExec 23, Movement_0570
    ActorCmdWait
    ActorDelete 23
    BMHndAudioVisualAnmPlay 0x8010, 1
    BMHndAnmWait 0x8010
    BMReleaseHandle 0x8010
    FlagSet EVENT_FLAG_0x03f5
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0548:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 13, 1
    MoveEnd

Movement_0570:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
