#include "asm/field_script.inc"
#include "text/script/icirrus_city_2.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_17:
    VMCall L_00D4
    VMHalt

Script_15:
    Cmd_02B2 0, EVENT_WORK_0x400c
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4047
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_008D
    FlagReset EVENT_FLAG_0x038b

L_008D:
    VMCall L_0095
    VMHalt

L_0095:
    Cmd_02B2 0, EVENT_WORK_0x400c
    VMStackPushFlag EVENT_FLAG_0x01d0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4047
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00D2
    FlagSet EVENT_FLAG_0x038b

L_00D2:
    VMReturn

L_00D4:
    Cmd_02B2 0, EVENT_WORK_0x400c
    VMStackPushFlag EVENT_FLAG_0x01d0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4047
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0128
    VMStackPushFlag EVENT_FLAG_0x038b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0124
    ActorDelete 10

L_0124:
    FlagSet EVENT_FLAG_0x038b

L_0128:
    VMReturn

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Cmd_02B2 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4047
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0167
    VMCall L_01B5
    VMJump L_01AF

L_0167:
    VMStackPushFlag EVENT_FLAG_0x01d0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0186
    VMCall L_0218
    VMJump L_01AF

L_0186:
    VMStackPushFlag EVENT_FLAG_0x01d0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01AF
    VMCall L_0218

L_01AF:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01B5:
    // "Well, since you came all the way here...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity2_Text_WellSinceCameAll, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_FLOCCESY_TOWN_5, 6, 0, 3, 1
    WorkSetConst EVENT_WORK_0x4047, 1
    VMReturn

Script_16:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    // "I'm grateful for everyone's support..."
    ActorMsg MSGFILE_SCRIPT, IcirrusCity2_Text_ImGratefulEveryonesSupport, 10, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPushFlag EVENT_FLAG_0x01d0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_020C
    VMCall L_0218

L_020C:
    VMCall L_0375
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0218:
    // "Brycen: In the past, when I was hurt and\ndepressed, Alder shared this with me...[f000]븁\u0000\nThere is strength in being with other\npeople and Pokémon.[f000]븁\u0000\nReceiving their support makes\nyou stronger.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity2_Text_BrycenPastWhenHurt, 10, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_023D
    VMJump L_024B

L_023D:
    ActorCmdExec 10, Movement_05F8
    VMJump L_02AE

L_024B:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_025E
    VMJump L_026C

L_025E:
    ActorCmdExec 10, Movement_0600
    VMJump L_02AE

L_026C:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_027F
    VMJump L_028D

L_027F:
    ActorCmdExec 10, Movement_05F0
    VMJump L_02AE

L_028D:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_02A0
    VMJump L_02AE

L_02A0:
    ActorCmdExec 10, Movement_0608
    VMJump L_02AE

L_02AE:
    ActorCmdWait
    // "I worked as a Gym Leader,\nand I came to understand[f000]븀\u0000\nwhat he meant by that.[f000]븁\u0000\nMy desire became to strengthen\nthis relationship that makes everyone[f000]븀\u0000\nstronger--the relationship between[f000]븀\u0000\npeople and Pokémon.[f000]븁\u0000\nBy focusing on the path of an actor,\nI want to make everyone think[f000]븀\u0000\nthat living together with Pokémon[f000]븀\u0000\nis exciting and wonderful.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity2_Text_WorkedGymLeaderCame, 10, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_02D5
    VMJump L_02E3

L_02D5:
    ActorCmdExec 10, Movement_0608
    VMJump L_0346

L_02E3:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_02F6
    VMJump L_0304

L_02F6:
    ActorCmdExec 10, Movement_05F8
    VMJump L_0346

L_0304:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0317
    VMJump L_0325

L_0317:
    ActorCmdExec 10, Movement_0600
    VMJump L_0346

L_0325:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0338
    VMJump L_0346

L_0338:
    ActorCmdExec 10, Movement_05F0
    VMJump L_0346

L_0346:
    ActorCmdWait
    // "Teaching is being taught...\nExcuse me, it's time for training.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, IcirrusCity2_Text_TeachingBeingTaughtExcuse, 10, 0, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4047
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_036F
    VMCall L_0375

L_036F:
    FlagSet EVENT_FLAG_0x01d0
    VMReturn

L_0375:
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 15
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03BE
    ActorWalkRoute 10, 16, 23, 1, 8, 0
    VMSleep 16
    ActorCmdExec 255, Movement_0608
    VMJump L_03D8

L_03BE:
    ActorWalkRoute 10, 17, 23, 1, 8, 1
    VMSleep 8
    ActorCmdExec 255, Movement_0608

L_03D8:
    ActorCmdWait
    ActorDelete 10
    FlagSet EVENT_FLAG_0x038b
    VMReturn

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x038b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0413
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "So cool, isn't it?[f000]븁\u0000\nThat strong figure standing\nthere in the background!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity2_Text_CoolIsntStrongFigure, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0427

L_0413:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, I'm so sorry.[f000]븁\u0000\nBrycen decided to try to\nreturn to his acting roots.[f000]븁\u0000\nCurrently, he's working hard at\nPokéstar Studios!"
    ParentActorMsg MSGFILE_SCRIPT, IcirrusCity2_Text_OhImSorryBrycen, 0, 0
    LastKeyWait
    ActorMsgClose

L_0427:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Former Icirrus City\nPokémon Gym"
    InfoMsg IcirrusCity2_Text_FormerIcirrusCityPokemon, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_64
    Cmd_018F 0
    Cmd_0190 0
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0487
    ActorSetGPos 2, 19, 0, 54, 1
    ActorSetGPos 3, 18, 0, 53, 1
    WorkSetConst EVENT_WORK_0x4001, 1
    VMJump L_04A5

L_0487:
    ActorSetGPos 2, 18, 0, 55, 1
    ActorSetGPos 3, 17, 0, 54, 1
    WorkSetConst EVENT_WORK_0x4001, 0

L_04A5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_64
    Cmd_018F 1
    Cmd_0190 1
    VMStackPush EVENT_WORK_0x4002
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F0
    ActorSetGPos 5, 55, 3, 42, 1
    ActorSetGPos 6, 54, 3, 43, 1
    WorkSetConst EVENT_WORK_0x4002, 1
    VMJump L_050E

L_04F0:
    ActorSetGPos 5, 55, 3, 44, 1
    ActorSetGPos 6, 56, 3, 43, 1
    WorkSetConst EVENT_WORK_0x4002, 0

L_050E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_64
    Cmd_018F 2
    Cmd_0190 2
    VMStackPush EVENT_WORK_0x4003
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0559
    ActorSetGPos 8, 17, 6, 32, 1
    ActorSetGPos 9, 18, 6, 33, 1
    WorkSetConst EVENT_WORK_0x4003, 1
    VMJump L_0577

L_0559:
    ActorSetGPos 8, 16, 6, 33, 1
    ActorSetGPos 9, 17, 6, 34, 1
    WorkSetConst EVENT_WORK_0x4003, 0

L_0577:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 5
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 4
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    PlayerTurnByTrigger
    Cmd_0191 6
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_05F0:
    Move 35, 1
    MoveEnd

Movement_05F8:
    Move 34, 1
    MoveEnd

Movement_0600:
    Move 32, 1
    MoveEnd

Movement_0608:
    Move 33, 1
    MoveEnd
