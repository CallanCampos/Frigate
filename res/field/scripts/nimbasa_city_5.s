#include "asm/field_script.inc"
#include "text/script/nimbasa_city_5.h"

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
    ScriptEntry Script_20
    ScriptEntriesEnd

Script_1:
    WorkSetConst 0x8020, 0
    VMStackPush EVENT_WORK_0x416d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x416e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x416f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4170
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4171
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4172
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00CD
    StadiumLoadTrainerTable
    Cmd_0249 EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f, EVENT_WORK_0x4170, EVENT_WORK_0x4171, EVENT_WORK_0x4172
    StadiumFreeTrainerTable

L_00CD:
    FlagSet EVENT_FLAG_0x0289
    FlagSet EVENT_FLAG_0x028a
    FlagSet EVENT_FLAG_0x028b
    FlagSet EVENT_FLAG_0x028c
    FlagSet EVENT_FLAG_0x028d
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FE
    FlagReset EVENT_FLAG_0x0289

L_00FE:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0121
    FlagReset EVENT_FLAG_0x028a
    FlagReset EVENT_FLAG_0x028b
    FlagReset EVENT_FLAG_0x028c
    FlagReset EVENT_FLAG_0x028d

L_0121:
    StadiumLoadTrainerTable
    StadiumSetupActorSingle 9, 5, 1
    StadiumSetupActorSingle 10, 6, 1
    StadiumSetupActorSingle 11, 7, 1
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0176
    StadiumSetupActorSingle 9, 5, 2
    StadiumSetupActorSingle 10, 6, 2
    StadiumSetupActorSingle 11, 7, 2
    StadiumSetupActorsDouble 15, 3, 23, 2

L_0176:
    TrainerCardHasBadge 0x8020, 6
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B1
    StadiumSetupActorSingle 9, 5, 3
    StadiumSetupActorSingle 10, 6, 3
    StadiumSetupActorSingle 11, 7, 3
    StadiumSetupActorsDouble 15, 3, 23, 3

L_01B1:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FE
    StadiumSetupActorSingle 9, 5, 4
    StadiumSetupActorSingle 10, 6, 4
    StadiumSetupActorSingle 11, 7, 4
    StadiumSetupActorsDouble 15, 3, 23, 4
    StadiumSetupActorsDouble 16, 24, 4, 4
    StadiumSetupActorsTriple 0, 1, 2, EVENT_WORK_0x416d, EVENT_WORK_0x416e, EVENT_WORK_0x416f

L_01FE:
    StadiumFreeTrainerTable
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Athletes who score points, and athletes\nwho support them...[f000]븁\u0000\nDetermining their different roles is the\nkey to building a team!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_AthletesWhoScorePoints, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Football is a fun sport that is divided\ninto offense and defense."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_FootballFunSportDivided, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "His... Those eyes...[f000]븁\u0000\nIt looks like he's coming\nright at me!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_HisThoseEyesLooks, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The helmet and pads weigh\n15 to 18 pounds!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_HelmetPadsWeigh15, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "I've trained and built up my muscles\nby tackling Pokémon![f000]븁\u0000\nNothing can move me!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_IveTrainedBuiltUp, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "OK! I will defend to the end with moves\nlike Protect and Detect!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_OkWillDefendEnd, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "My favorite moves?[f000]븁\u0000\n...Hmm.\nI'd say Tackle and Take Down."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_FavoriteMovesHmmId, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Reading your opponent's attack and\ndeciding your next move...[f000]븁\u0000\nBoth Pokémon battles and football have\nthe same thrill!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_ReadingOpponentsAttackDeciding, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "The appeal of football?\nLet me see...[f000]븁\u0000\nFirst, just watch a game without thinking\nabout the rules!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_AppealFootballLetSee, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "I'll protect my team with my whole body\nto avoid our opponents' interference!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_IllProtectTeamWhole, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "In football, the quarterback is the\nathlete who decides a strategy and[f000]븀\u0000\ncarries it out.[f000]븁\u0000\nHe's kind of like a Pokémon Trainer!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_FootballQuarterbackAthleteWho, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "In football, you have to learn each and\nevery formation by heart!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_FootballHaveLearnEach, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The ball used in football is oval-shaped\nand hard to throw, isn't it?[f000]븁\u0000\nBut once you get the hang of it, you can\nthrow it perfectly!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_BallUsedFootballOval, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am a greeeeeat secret weapon![f000]븁\u0000\nThe only problem is, I am so secret that\nI've never been in a game..."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_AmGreeeeeatSecretWeapon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Yoo-hoo! Pass me the ball!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_YooHooPassBall, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bodies crashing into other bodies!\nGo, go, go![f000]븁\u0000\nI am the owner. That’s what\nI like to see."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_BodiesCrashingIntoOther, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 511, 0
    // "Ook!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_Ook, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 513, 0
    // "Ookiii!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_Ookiii, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 515, 0
    // "Ook! Ook!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity5_Text_OokOok, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
