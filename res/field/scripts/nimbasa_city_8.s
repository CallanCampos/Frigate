#include "asm/field_script.inc"
#include "text/script/nimbasa_city_8.h"

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

Script_4:
    VMStackPush EVENT_WORK_0x4160
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0043
    VMCall L_0045

L_0043:
    VMHalt

L_0045:
    RTCGetWeekDay 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0076
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_0076
    WorkCmpConst 0x8010, 6
    VMJumpIf CMP_EQ, L_0076
    VMJump L_00B5

L_0076:
    WorkSetConst EVENT_WORK_0x4160, 5
    WorkSetConst EVENT_WORK_0x4020, 133
    RTCGetTime 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 10
    VMStackCmp CMP_GE
    VMStackPush 0x8008
    VMStackPushConst 11
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00AF
    FlagSet EVENT_FLAG_0x00dc

L_00AF:
    VMJump L_00DE

L_00B5:
    WorkSetConst EVENT_WORK_0x4160, 4
    WorkSetConst EVENT_WORK_0x4020, 60
    RTCGetTime 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DE
    FlagSet EVENT_FLAG_0x00dc

L_00DE:
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00FF
    FlagReset EVENT_FLAG_0x028f
    FlagSet EVENT_FLAG_0x029a
    VMJump L_0107

L_00FF:
    FlagSet EVENT_FLAG_0x028f
    FlagReset EVENT_FLAG_0x029a

L_0107:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ab5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0148
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

L_0148:
    VMReturn

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4160, 4
    VMJumpIf CMP_EQ, L_0165
    VMJump L_0198

L_0165:
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0188
    // "There is a tennis match\nin Small Court now!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_ThereTennisMatchSmall, 0, 0
    VMJump L_0192

L_0188:
    // "Tennis players are practicing\nin Small Court now![f000]븁\u0000\nPeople can watch them practicing!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_TennisPlayersPracticingSmall, 0, 0

L_0192:
    VMJump L_01C5

L_0198:
    VMStackPushFlag EVENT_FLAG_0x00dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BB
    // "There is a basketball game\nin Small Court now!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_ThereBasketballGameSmall, 0, 0
    VMJump L_01C5

L_01BB:
    // "Basketball players are practicing\nin Small Court now![f000]븁\u0000\nPeople can watch them practicing!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_BasketballPlayersPracticingSmall, 0, 0

L_01C5:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x4160
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F4
    MapChangeWarpPad ZONE_NIMBASA_CITY_11, 6, 25, 0
    VMJump L_01FE

L_01F4:
    MapChangeWarpPad ZONE_NIMBASA_CITY_10, 6, 25, 0

L_01FE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am sorry.[f000]븁\u0000\nBut you cannot enter the court\nbecause a game is in progress."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_AmSorryButCannot, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am sorry.[f000]븁\u0000\nBut you cannot enter the court\nbecause a game is in progress."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_AmSorryButCannot_2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I am sorry.[f000]븁\u0000\nBut you cannot enter the court\nbecause a game is in progress."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_AmSorryButCannot_3, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We love sports.\nWatching games is great, but we enjoy[f000]븀\u0000\nwatching practices, too!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_WeLoveSportsWatching, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "New styles of basketball and tennis\ncreated by people and Pokémon...[f000]븀\u0000\nThese may be advanced forms of sports."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_NewStylesBasketballTennis, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you speak to athletes during a\npractice, they may challenge you to a[f000]븀\u0000\nPokémon battle!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_IfSpeakAthletesDuring, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4160, 4
    VMJumpIf CMP_EQ, L_02C7
    VMJump L_02D7

L_02C7:
    // "First-rank Smashers are waiting for you\nin the court with their Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_FirstRankSmashersWaiting, 0, 0
    VMJump L_02E1

L_02D7:
    // "First-rank Hoopsters are waiting for you\nin the court with their Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity8_Text_FirstRankHoopstersWaiting, 0, 0

L_02E1:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
