#include "asm/field_script.inc"
#include "text/script/nimbasa_city_3.h"

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

Script_7:
    VMStackPush EVENT_WORK_0x4160
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0043
    VMCall L_0045

L_0043:
    VMHalt

L_0045:
    RTCGetWeekDay 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0076
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0076
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_0076
    VMJump L_00B5

L_0076:
    WorkSetConst EVENT_WORK_0x4160, 1
    WorkSetConst EVENT_WORK_0x4020, 57
    RTCGetTime 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 14
    VMStackCmp CMP_GE
    VMStackPush 0x8008
    VMStackPushConst 16
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00AF
    FlagSet EVENT_FLAG_0x00dc

L_00AF:
    VMJump L_014D

L_00B5:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_00D5
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_00D5
    VMJump L_0114

L_00D5:
    WorkSetConst EVENT_WORK_0x4160, 2
    WorkSetConst EVENT_WORK_0x4020, 58
    RTCGetTime 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 14
    VMStackCmp CMP_GE
    VMStackPush 0x8008
    VMStackPushConst 15
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_010E
    FlagSet EVENT_FLAG_0x00dc

L_010E:
    VMJump L_014D

L_0114:
    WorkSetConst EVENT_WORK_0x4160, 3
    WorkSetConst EVENT_WORK_0x4020, 59
    RTCGetTime 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 14
    VMStackCmp CMP_GE
    VMStackPush 0x8008
    VMStackPushConst 15
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_014D
    FlagSet EVENT_FLAG_0x00dc

L_014D:
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016E
    FlagReset EVENT_FLAG_0x028f
    FlagSet EVENT_FLAG_0x029a
    VMJump L_0176

L_016E:
    FlagSet EVENT_FLAG_0x028f
    FlagReset EVENT_FLAG_0x029a

L_0176:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B7
    StadiumLoadTrainerTable
    StadiumResetTrainerFlags
    StadiumFreeTrainerTable
    FlagSet EVENT_FLAG_DAILY_0x0ab5
    WorkSetConst EVENT_WORK_0x416d, 0
    WorkSetConst EVENT_WORK_0x416e, 0
    WorkSetConst EVENT_WORK_0x416f, 0
    WorkSetConst EVENT_WORK_0x4170, 0
    WorkSetConst EVENT_WORK_0x4171, 0
    WorkSetConst EVENT_WORK_0x4172, 0

L_01B7:
    VMReturn

Script_6:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4160
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DE
    MapChangeWarpPad ZONE_NIMBASA_CITY_6, 9, 61, 0
    VMJump L_020B

L_01DE:
    VMStackPush EVENT_WORK_0x4160
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0201
    MapChangeWarpPad ZONE_NIMBASA_CITY_7, 10, 29, 0
    VMJump L_020B

L_0201:
    MapChangeWarpPad ZONE_NIMBASA_CITY_5, 9, 29, 0

L_020B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4160, 3
    VMJumpIf CMP_EQ, L_022C
    VMJump L_025F

L_022C:
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_024F
    // "There is a football game in\nBig Stadium now!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_ThereFootballGameBig, 0, 0
    VMJump L_0259

L_024F:
    // "Football players are practicing in\nBig Stadium now.[f000]븁\u0000\nPeople can watch them practicing!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_FootballPlayersPracticingBig, 0, 0

L_0259:
    VMJump L_02D2

L_025F:
    WorkCmpConst EVENT_WORK_0x4160, 1
    VMJumpIf CMP_EQ, L_0272
    VMJump L_02A5

L_0272:
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0295
    // "There is a baseball game\nin Big Stadium now!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_ThereBaseballGameBig, 0, 0
    VMJump L_029F

L_0295:
    // "Infielders are practicing\nin Big Stadium now.[f000]븁\u0000\nPeople can watch them practicing!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_InfieldersPracticingBigStadium, 0, 0

L_029F:
    VMJump L_02D2

L_02A5:
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C8
    // "There is a soccer game\nin Big Stadium now!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_ThereSoccerGameBig, 0, 0
    VMJump L_02D2

L_02C8:
    // "Soccer players are practicing\nin Big Stadium now![f000]븁\u0000\nPeople can watch them practicing!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_SoccerPlayersPracticingBig, 0, 0

L_02D2:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You sure look up to athletes when\nyou're a kid."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_SureLookUpAthletes, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm raising the same Pokémon as\nmy favorite athlete's Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_ImRaisingSamePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4160, 3
    VMJumpIf CMP_EQ, L_032F
    VMJump L_033F

L_032F:
    // "When I throw a Poké Ball, I copy the\nthrowing form of a quarterback!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_WhenThrowPokeBall, 0, 0
    VMJump L_036C

L_033F:
    WorkCmpConst EVENT_WORK_0x4160, 1
    VMJumpIf CMP_EQ, L_0352
    VMJump L_0362

L_0352:
    // "When I throw a Poké Ball, I copy the form\nof a pitcher!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_WhenThrowPokeBall_2, 0, 0
    VMJump L_036C

L_0362:
    // "When I throw a Poké Ball, I copy the\nthrowing technique of a keeper!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_WhenThrowPokeBall_3, 0, 0

L_036C:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4160, 3
    VMJumpIf CMP_EQ, L_0391
    VMJump L_03A1

L_0391:
    // "Ha ha! I am a football player![f000]븁\u0000\nI injured my hand, so I can't sign\nautographs! I'm sorry about that.[f000]븁\u0000\nSo, what does the autograph on your\nTrainer Card look like?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_HaHaAmFootball, 0, 0
    VMJump L_03CE

L_03A1:
    WorkCmpConst EVENT_WORK_0x4160, 1
    VMJumpIf CMP_EQ, L_03B4
    VMJump L_03C4

L_03B4:
    // "Yeah! I am an Infielder![f000]븁\u0000\nI hurt my hand, so I can't write\nan autograph! Sorry![f000]븁\u0000\nSay, how did you sign your Trainer Card?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_YeahAmInfielderHurt, 0, 0
    VMJump L_03CE

L_03C4:
    // "Bwa ha ha! I am a Striker![f000]븁\u0000\nMy hand got hurt, so I won't be able\nto sign any autographs! Regrets![f000]븁\u0000\nWhat does your Trainer Card signature\nlook like?"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_BwaHaHaAm, 0, 0

L_03CE:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am sorry.[f000]븁\u0000\nBut you cannot go onto the field\nbecause a game is in progress."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_AmSorryButCannot, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am sorry.[f000]븁\u0000\nBut you cannot go onto the field\nbecause a game is in progress."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_AmSorryButCannot_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am sorry.[f000]븁\u0000\nBut you cannot go onto the field\nbecause a game is in progress."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity3_Text_AmSorryButCannot_3, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
