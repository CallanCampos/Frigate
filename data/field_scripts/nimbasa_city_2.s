#include "asm/field_script.inc"
#include "text/script/nimbasa_city_2.h"

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
    ScriptEntriesEnd

Script_3:
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0055
    FlagReset EVENT_FLAG_0x097d

L_0055:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    HollowRivalCmd_0266 0x8020
    RTCGetWeekDay 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_007C
    VMJump L_0088

L_007C:
    WorkSetConst 0x8021, 291
    VMJump L_00E5

L_0088:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_009B
    VMJump L_00A7

L_009B:
    WorkSetConst 0x8021, 249
    VMJump L_00E5

L_00A7:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_00BA
    VMJump L_00C6

L_00BA:
    WorkSetConst 0x8021, 103
    VMJump L_00E5

L_00C6:
    WorkCmpConst 0x8010, 6
    VMJumpIf CMP_EQ, L_00D9
    VMJump L_00E5

L_00D9:
    WorkSetConst 0x8021, 223
    VMJump L_00E5

L_00E5:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_012D
    TrainerCardGetSex 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011B
    WorkSetConst EVENT_WORK_0x4020, 290
    VMJump L_0121

L_011B:
    WorkSetConst EVENT_WORK_0x4020, 289

L_0121:
    WorkSetConst EVENT_WORK_0x4188, 2
    VMJump L_01E7

L_012D:
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0172
    WorkGet EVENT_WORK_0x4020, 0x8021
    WorkSetConst EVENT_WORK_0x4188, 1
    VMJump L_01E7

L_0172:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    RTCGetSeason 0x8023
    VMStackPush EVENT_WORK_0x4173
    VMStackPush 0x8023
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0199
    FlagReset EVENT_FLAG_DAILY_0x0ab4

L_0199:
    VMStackPush 0x8000
    VMStackPush 0x8001
    RTCGetSeason 0x8000
    TrainerCardGetSex 0x8001
    WorkSet 0x8022, 0x8000
    WorkMul 0x8022, 2
    WorkAdd 0x8022, 0x8001
    VMStackPop 0x8001
    VMStackPop 0x8000
    Cmd_021F 0x8022, 0x8010
    WorkGet EVENT_WORK_0x4020, 0x8010
    WorkGet EVENT_WORK_0x4173, 0x8023
    WorkSetConst EVENT_WORK_0x4188, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0

L_01E7:
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8024, 0
    RTCGetWeekDay 0x8024
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_023C
    HollowRivalCmd_0262 1, 44
    VMJump L_02B4

L_023C:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_027B
    HollowRivalCmd_0262 3, 11
    VMJump L_02B4

L_027B:
    VMStackPush 0x8024
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02B4
    HollowRivalCmd_0262 2, 13

L_02B4:
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_035E
    VMStackPushFlag EVENT_FLAG_0x01a1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01eb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0306
    HollowRivalCmd_0262 1, 41
    VMJump L_035E

L_0306:
    VMStackPushFlag EVENT_FLAG_0x01a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01eb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0335
    HollowRivalCmd_0262 1, 42
    VMJump L_035E

L_0335:
    VMStackPushFlag EVENT_FLAG_0x01a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01eb
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_035E
    HollowRivalCmd_0262 1, 43

L_035E:
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0387
    HollowRivalCmd_0262 3, 0

L_0387:
    VMStackPush 0x8024
    VMStackPushConst 6
    VMStackCmp CMP_NE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03E2
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C9
    HollowRivalCmd_0262 2, 14
    VMJump L_03E2

L_03C9:
    VMStackPush EVENT_WORK_0x4111
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E2
    HollowRivalCmd_0262 2, 0

L_03E2:
    WorkSetConst 0x8024, 0
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Nimbasa City Pokémon Gym\nLeader: Elesa[f000]븀\u0000\nThe Shining Beauty"
    MsgPlaceSign NimbasaCity2_Text_NimbasaCityPokemonGym, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "The Shining Roller Coaster\nFormer Nimbasa City Pokémon Gym"
    MsgPlaceSign NimbasaCity2_Text_ShiningRollerCoasterFormer, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8025, 0
    TrainerCardGetSex 0x8025
    ActorDelete 11
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_0445
    VMJump L_046B

L_0445:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 637
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_04A4

L_046B:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_047E
    VMJump L_04A4

L_047E:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 636
    WorkSet 0x8001, 1
    RTCallGlobal 2807
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_04A4

L_04A4:
    MEPlay SEQ_ME_CALL
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_04BB
    VMJump L_04C7

L_04BB:
    // "The Xtransceiver you found is ringing."
    SystemMsg NimbasaCity2_Text_XtransceiverFoundRinging, 2
    VMJump L_04E6

L_04C7:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_04DA
    VMJump L_04E6

L_04DA:
    // "The Xtransceiver you found is ringing."
    SystemMsg NimbasaCity2_Text_XtransceiverFoundRinging_2, 2
    VMJump L_04E6

L_04E6:
    MEWait
    WordSetPlayerName 0
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_04FE
    VMJump L_050A

L_04FE:
    // "[f000]Ā\u0001\u0000 picked up\nthe Xtransceiver.[f000]븁\u0000"
    SystemMsg NimbasaCity2_Text_PickedUpXtransceiver, 2
    VMJump L_0529

L_050A:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_051D
    VMJump L_0529

L_051D:
    // "[f000]Ā\u0001\u0000 picked up\nthe Xtransceiver.[f000]븁\u0000"
    SystemMsg NimbasaCity2_Text_PickedUpXtransceiver_2, 2
    VMJump L_0529

L_0529:
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_0542
    VMJump L_054E

L_0542:
    CallXTransceiver 2, 1
    VMJump L_056D

L_054E:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_0561
    VMJump L_056D

L_0561:
    CallXTransceiver 3, 1
    VMJump L_056D

L_056D:
    FadeInBlackQ
    FadeWait
    HollowRivalCmd_0263 5
    FlagSet EVENT_FLAG_0x029f
    WorkSetConst EVENT_WORK_0x4126, 1
    WorkSetConst EVENT_WORK_0x4127, 1
    WorkSetConst EVENT_WORK_0x4128, 1
    WorkSetConst EVENT_WORK_0x4129, 1
    WorkSetConst EVENT_WORK_0x412a, 1
    WorkSetConst EVENT_WORK_0x412b, 1
    WorkSetConst EVENT_WORK_0x412c, 1
    WorkSetConst EVENT_WORK_0x412d, 1
    WorkSetConst EVENT_WORK_0x412e, 1
    WorkSetConst EVENT_WORK_0x412f, 1
    WorkSetConst EVENT_WORK_0x4130, 1
    WorkSetConst EVENT_WORK_0x4131, 1
    WorkSetConst EVENT_WORK_0x4132, 1
    WorkSetConst EVENT_WORK_0x4133, 1
    WorkSetConst EVENT_WORK_0x4134, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's a shining, sparkling, bright\nfashion show!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_ItsShiningSparklingBright, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I hear that a Clown's makeup\nincludes a teardrop mark."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_HearClownsMakeupIncludes, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A roller coaster and a Ferris wheel!\nWhich one should I ride first?!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_RollerCoasterFerrisWheel, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ah ah ah ah aaah! ♪[f000]븁\u0000\nWh-what should I talk about\non my first date..."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_AhAhAhAh, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, this guy! Even in the amusement\npark, he does nothing but play guitar...[f000]븁\u0000\nHow cool! He loves music from\nthe bottom of his heart!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_OhGuyEvenAmusement, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What's that? Uh, I dunno. Audino?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_WhatsUhDunnoAudino, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A famous TV star came here\nfor a shoot recently! ♪[f000]븁\u0000\nIt's that one who's always on TV.[f000]븁\u0000\nOne thing I noticed while watching\nthe shoot is that star spends a lot[f000]븀\u0000\nof time on the Xtransceiver![f000]븁\u0000\nThe entire break it was talk, talk,\ntalk, laugh, laugh, laugh! ♪"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_FamousTvStarCame, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 531, 0
    // "Chuuu! ♪"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_Chuuu, 0, 0
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
    PVPlay 559, 0
    // "Uuugh!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_Uuugh, 0, 0
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
    // "Oh? Are you a challenger perhaps?[f000]븁\u0000\nI'm very sorry,\nthe Gym Leader is out right now...[f000]븁\u0000\nI know where she went though.[f000]븁\u0000\nShe should be in the building where\nyou can ride the roller coaster.[f000]븁\u0000\nIt's by the entrance\nto this amusement park."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_OhChallengerPerhapsIm, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The best part of riding a roller coaster\nis screaming your heart out!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity2_Text_BestPartRidingRoller, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
