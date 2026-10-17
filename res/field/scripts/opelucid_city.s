#include "asm/field_script.inc"
#include "text/script/opelucid_city.h"

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
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntry Script_25
    ScriptEntry Script_26
    ScriptEntry Script_27
    ScriptEntry Script_28
    ScriptEntry Script_29
    ScriptEntry Script_30
    ScriptEntry Script_31
    ScriptEntry Script_32
    ScriptEntry Script_33
    ScriptEntry Script_34
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

L_00A2:
    VMStackPushFlag EVENT_FLAG_0x01bc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DD
    ObjInitWarpGPos 4, 0, 0, 0
    ObjInitWarpGPos 5, 0, 0, 0
    ObjInitWarpGPos 6, 0, 0, 0
    ObjInitWarpGPos 8, 0, 0, 0

L_00DD:
    VMReturn

Script_32:
    VMCall L_00A2
    VMCall L_025E
    VMHalt

Script_6:
    VMCall L_00A2
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0116
    WorkSetConst EVENT_WORK_0x4020, 205
    VMJump L_011C

L_0116:
    WorkSetConst EVENT_WORK_0x4020, 151

L_011C:
    WorkSetConst 0x8024, 0
    TrainerCardGetSex 0x8024
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0145
    WorkSetConst EVENT_WORK_0x4021, 231
    VMJump L_014B

L_0145:
    WorkSetConst EVENT_WORK_0x4021, 240

L_014B:
    WorkSetConst 0x8024, 0
    Cmd_02B2 15, EVENT_WORK_0x400f
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4048
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ee
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01A8
    FlagReset EVENT_FLAG_0x031a
    FlagSet EVENT_FLAG_0x0320
    WorkSetConst EVENT_WORK_0x413e, 1

L_01A8:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4048
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ee
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0320
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0209
    FlagSet EVENT_FLAG_0x031a
    FlagSet EVENT_FLAG_0x0320
    WorkSetConst EVENT_WORK_0x413e, 2

L_0209:
    VMCall L_0215
    FlagReset EVENT_FLAG_0x01ee
    VMHalt

L_0215:
    Cmd_02B2 15, EVENT_WORK_0x400f
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4048
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_025C
    FlagSet EVENT_FLAG_0x031a
    FlagReset EVENT_FLAG_0x0320
    WorkSetConst EVENT_WORK_0x413e, 0

L_025C:
    VMReturn

L_025E:
    Cmd_02B2 15, EVENT_WORK_0x400f
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4048
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02BC
    VMStackPushFlag EVENT_FLAG_0x031a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02AE
    ActorDelete 0

L_02AE:
    FlagSet EVENT_FLAG_0x031a
    FlagReset EVENT_FLAG_0x0320
    WorkSetConst EVENT_WORK_0x413e, 0

L_02BC:
    VMReturn

Script_7:
    WorkSetConst 0x8025, 0
    GameGetVersion 0x8025
    VMStackPush EVENT_WORK_0x40d5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 1
    VMStackCmp CMP_LT
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02FB
    DebugPrint 9
    ActorSetGPos 13, 432, 0, 160, 2

L_02FB:
    VMStackPush EVENT_WORK_0x40d6
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0334
    DebugPrint 0
    ActorSetGPos 0, 418, 65535, 169, 1
    VMJump L_056F

L_0334:
    VMStackPush EVENT_WORK_0x40d6
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_036D
    DebugPrint 1
    ActorSetGPos 0, 418, 0, 169, 1
    VMJump L_056F

L_036D:
    VMStackPush EVENT_WORK_0x40d6
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03A6
    DebugPrint 2
    ActorSetGPos 0, 418, 0, 162, 1
    VMJump L_056F

L_03A6:
    VMStackPush EVENT_WORK_0x40d6
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03DF
    DebugPrint 3
    ActorSetGPos 0, 418, 0, 162, 1
    VMJump L_056F

L_03DF:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0424
    DebugPrint 4
    ActorSetGPos 0, 419, 0, 164, 1
    ActorSetGPos 19, 418, 0, 164, 1
    VMJump L_056F

L_0424:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0469
    DebugPrint 5
    ActorSetGPos 0, 419, 0, 162, 1
    ActorSetGPos 19, 418, 0, 162, 1
    VMJump L_056F

L_0469:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 6
    VMStackCmp CMP_LT
    VMStackPush 0x8025
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04BE
    DebugPrint 6
    ActorSetGPos 0, 424, 0, 175, 0
    ActorSetGPos 5, 429, 65535, 179, 2
    VMJump L_056F

L_04BE:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 6
    VMStackCmp CMP_LT
    VMStackPush 0x8025
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0507
    DebugPrint 7
    ActorSetGPos 0, 424, 0, 175, 0
    VMJump L_056F

L_0507:
    VMStackPush EVENT_WORK_0x40d8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_053C
    DebugPrint 8
    ActorSetGPos 8, 442, 0, 175, 2
    ActorSetGPos 0, 398, 0, 167, 0
    VMJump L_056F

L_053C:
    VMStackPush EVENT_WORK_0x40d8
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_056F
    DebugPrint 9
    ActorSetGPos 0, 418, 0, 162, 1

L_056F:
    Cmd_02B2 15, EVENT_WORK_0x400f
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x413e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05CA
    ActorSetGPos 0, 418, 65535, 168, 1
    VMJump L_0619

L_05CA:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x413e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0619
    ActorSetGPos 0, 418, 0, 168, 1

L_0619:
    WorkSetConst 0x8025, 0
    VMHalt

Script_8:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    ActorCmdExec 11, Movement_246C
    ActorCmdWait
    ActorCmdExec 11, Movement_2464
    ActorCmdWait
    WorkSub 0x8021, 2
    ActorWalkRoute 11, 0x8021, 0x8022, 0, 8, 1
    ActorCmdWait
    // "Iris: Yahoo! I haven't seen you since\nI ran into you in Castelia City![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_IrisYahooHaventSeen, 11, 0, 0
    // "Are you here to battle Grandpa?[f000]븁\u0000\nYep! Opelucid City's Gym Leader,\nDrayden, is my grandpa.[f000]븁\u0000\nWe're not really related, though![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_HereBattleGrandpaYep, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_245C
    ActorCmdWait
    // "Oh! The Gym is that way![f000]븁\u0000\nBefore challenging it,\nyou might want to go to Route 9![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_OhGymWayBefore, 11, 0, 0
    MsgWinCloseAll
    ActorCmdExec 11, Movement_2464
    ActorCmdWait
    // "Still, your Pokémon really respect you![f000]븁\u0000\nI feel how much fun your Pokémon are\nhaving, even from inside their Poké Balls![f000]븁\u0000\nGood luck![f000]븁\u0000"
    // "Still, your Pokémon really respect you![f000]븁\u0000\nI feel how much fun they're having,\neven from inside their Poké Balls![f000]븁\u0000\nGood luck![f000]븁\u0000"
    ActorMsgGendered 1024, OpelucidCity_Text_StillPokemonReallyRespect, OpelucidCity_Text_StillPokemonReallyRespect_2, 11, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 11, 426, 0x8022, 0, 8, 0
    ActorCmdWait
    ActorDelete 11
    WorkSetConst EVENT_WORK_0x40d5, 1
    FlagSet EVENT_FLAG_0x0319
    FlagSet EVENT_FLAG_ARRIVED_OPELUCID_CITY
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    ActorWalkRoute 255, 405, 147, 0, 8, 0
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1958000, 0, 0x918000, 24
    FlagReset EVENT_FLAG_0x031a
    FlagSet EVENT_FLAG_0x031f
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 0
    SEWait
    ActorWalkRoute 0, 405, 145, 0, 8, 0
    VMSleep 8
    ActorCmdExec 255, Movement_244C
    ActorCmdWait
    EvCameraWait
    // "Drayden: I'll show you the way.\nFollow me.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenIllShowWay, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 406, 154, 4, 8, 0
    VMSleep 24
    ActorCmdExec 255, Movement_2454
    ActorCmdWait
    EvCameraMoveToDefault 24
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst 0x8026, 0
    GameGetVersion 0x8026
    VMStackPush 0x8026
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0788
    ActorSetGPos 0, 418, 65535, 169, 1
    VMJump L_07A7

L_0788:
    VMStackPush 0x8026
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07A7
    ActorSetGPos 0, 418, 0, 169, 1

L_07A7:
    WorkSetConst EVENT_WORK_0x40d6, 2
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    PlayerGetGPos 0x8021, 0x8022
    // "This way.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_Way, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 418, 161, 4, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_2434
    ActorCmdWait
    WorkSetConst EVENT_WORK_0x40d6, 3
    WorkSetConst EVENT_WORK_0x40d9, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    // "We're here.\nOK! Come inside.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_WereHereOkCome, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 418, 160, 0, 8, 0
    ActorCmdWait
    WorkSetConst 0x8027, 0
    BMCreateHandleByGPos 0x8027, 1, 418, 159
    BMHndAudioVisualAnmPlay 0x8027, 0
    BMHndAnmWait 0x8027
    ActorCmdExec 0, Movement_2414
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    BMHndAudioVisualAnmPlay 0x8027, 1
    BMHndAnmWait 0x8027
    BMReleaseHandle 0x8027
    WorkSetConst EVENT_WORK_0x40d6, 4
    FlagReset EVENT_FLAG_0x0320
    FlagSet EVENT_FLAG_0x031a
    WorkSetConst 0x8027, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    // "What was that?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_What, 0, 5, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    WorkSetConst 0x8028, 0
    GameGetVersion 0x8028
    VMStackPush 0x8028
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08B2
    FieldClose
    Call3DDemo 14, 0
    FieldOpen
    VMJump L_08BC

L_08B2:
    FieldClose
    Call3DDemo 15, 0
    FieldOpen

L_08BC:
    WorkSetConst 0x8028, 0
    FlagSet EVENT_FLAG_0x01bc
    FlagReset EVENT_FLAG_0x031c
    FlagReset EVENT_FLAG_0x031b
    FlagSet EVENT_FLAG_0x03e1
    FlagSet EVENT_FLAG_0x03f1
    FlagReset EVENT_FLAG_0x03f2
    MapReplaceSetEvent 0, 1, 1
    RTReserveScript 31
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_090F
    MapChangeCore ZONE_OPELUCID_CITY, 418, 0, 162, 1
    VMJump L_091B

L_090F:
    MapChangeCore ZONE_OPELUCID_CITY, 418, 0, 164, 1

L_091B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_31:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    GameGetVersion 0x8023
    ActorCmdExec 255, Movement_0E9C
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_096A
    EvCameraMoveTo 9688, 0, 0xed000, 0x1a28000, 0, 0xa28000, 1
    VMJump L_0982

L_096A:
    EvCameraMoveTo 9688, 0, 0xed000, 0x1a28000, 0, 0xa48000, 1

L_0982:
    EvCameraWait
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09A9
    ActorSetGPos 255, 419, 0, 162, 1
    VMJump L_09B5

L_09A9:
    ActorSetGPos 255, 419, 0, 164, 1

L_09B5:
    // "This world of ice...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_WorldIce, 0, 5, 0
    MsgWinCloseAll
    GameGetVersion 0x8023
    VMCall L_0D6C
    // "Haxorus!\nUse Dragon Tail![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_HaxorusUseDragonTail, 0, 5, 0
    MsgWinCloseAll
    PVPlay 612, 0
    // "Rarh raaaah!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_RarhRaaaah, 251, 6, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorCmdExec 251, Movement_0EAC
    VMSleep 2
    SEPlay SEQ_SE_W121_01
    ActorCmdWait
    SEWait
    ActorCmdExec 251, Movement_2474
    ActorCmdWait
    VMSleep 8
    ActorCmdExec 19, Movement_2474
    ActorCmdExec 0, Movement_246C
    ActorCmdWait
    // "That's enough, Haxorus![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ThatsEnoughHaxorus, 0, 5, 0
    MsgWinCloseAll
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A5C
    ActorWalkRoute 251, 419, 163, 4, 8, 1
    VMJump L_0A6A

L_0A5C:
    ActorWalkRoute 251, 419, 165, 4, 8, 1

L_0A6A:
    ActorCmdWait
    VMCall L_0E10
    // "Not only did the ice stay unbroken,\nthere's not even a scratch on it![f000]븀\u0000\nWhat's going on?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_NotOnlyDidIce, 0, 5, 0
    MsgWinCloseAll
    // "???: I don't think that's going to work.[f000]븁\u0000"
    InfoMsg OpelucidCity_Text_DontThinkThatsGoing, 2
    MsgWinCloseAll
    BGMPlay SEQ_BGM_E_7_SAGE
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AF4
    ActorSetGPos 2, 419, 0, 169, 0
    ActorSetGPos 1, 419, 0, 170, 0
    ActorSetGPos 4, 419, 0, 171, 0
    ActorWalkRoute 2, 417, 164, 4, 8, 1
    ActorWalkRoute 1, 418, 164, 4, 8, 1
    ActorWalkRoute 4, 419, 164, 4, 8, 1
    ActorCmdWait
    VMJump L_0B44

L_0AF4:
    ActorSetGPos 2, 419, 0, 171, 0
    ActorSetGPos 1, 419, 0, 172, 0
    ActorSetGPos 4, 419, 0, 173, 0
    ActorWalkRoute 2, 417, 166, 4, 8, 1
    ActorWalkRoute 1, 418, 166, 4, 8, 1
    ActorWalkRoute 4, 419, 166, 4, 8, 1
    ActorCmdWait

L_0B44:
    ActorCmdExec 2, Movement_244C
    ActorCmdExec 1, Movement_244C
    ActorCmdExec 4, Movement_244C
    ActorCmdWait
    // "Drayden: I remember you from N's Castle.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenRememberFromNs, 0, 5, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0D58
    ActorCmdWait
    // "Zinzolin: It's bitter cold.[f000]븁\u0000\nI'm shivering.\nI'm suffering, but I'm alive![f000]븁\u0000\nIt's what the essence of life feels like!\nIt's proof of my existence![f000]븁\u0000\nBut that's enough of philosophy.\nHere are the facts for your admiration:[f000]븁\u0000\nThis ice was specially created by\nTeam Plasma's technology.[f000]븁\u0000\nAs long as we have our secret weapon,\nyou'll never be able[f000]븀\u0000\nto melt or break this ice![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ZinzolinItsBitterCold, 1, 4, 0
    MsgWinCloseAll
    // "Let me explain our purpose here.[f000]븁\u0000\nDrayden, hand over the DNA Splicers![f000]븁\u0000\nOpelucid City is a city where the\npast and the future are entwined.[f000]븁\u0000\nCould there be a more perfect place\nfor the splicers that connect the[f000]븀\u0000\nseparated Pokémon?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_LetExplainOurPurpose, 1, 4, 0
    MsgWinCloseAll
    // "Drayden: Do you think someone\nwho knows what you did two years[f000]븀\u0000\nago will just hand them over quietly?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenThinkSomeoneWho, 0, 5, 0
    MsgWinCloseAll
    // "Zinzolin: Humph.\nThat's what I thought you'd say.[f000]븁\u0000\nAt this point, I'd like to threaten\nyou with another volley of ice,[f000]븀\u0000\nbut we can't use it for a moment...[f000]븁\u0000\nSigh. It won't be enjoyable in this cold,\nbut I guess we'll just search for them.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ZinzolinHumphThatsWhat, 1, 4, 0
    MsgWinCloseAll
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BF3
    ActorWalkRoute 2, 419, 171, 4, 8, 1
    ActorWalkRoute 1, 419, 172, 4, 8, 1
    ActorWalkRoute 4, 419, 173, 4, 8, 1
    ActorCmdWait
    VMJump L_0C1F

L_0BF3:
    ActorWalkRoute 2, 419, 173, 4, 8, 1
    ActorWalkRoute 1, 419, 174, 4, 8, 1
    ActorWalkRoute 4, 419, 175, 4, 8, 1
    ActorCmdWait

L_0C1F:
    BGMChangeMap
    ActorDelete 1
    // "Drayden: Those foul villains![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenThoseFoulVillains, 0, 5, 0
    ActorCmdExec 19, Movement_2464
    ActorCmdExec 0, Movement_245C
    ActorCmdWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000!\nI want you to help me![f000]븁\u0000\nWe're going to drive Team Plasma\nout of Opelucid City![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_WantHelpWereGoing, 0, 5, 0
    MsgWinCloseAll
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C7B
    ActorWalkRoute 0, 421, 171, 1, 8, 1
    VMJump L_0C89

L_0C7B:
    ActorWalkRoute 0, 420, 170, 1, 8, 1

L_0C89:
    VMSleep 16
    ActorCmdExec 19, Movement_2454
    ActorCmdWait
    ActorSetGPos 0, 424, 0, 175, 0
    ActorSetGPos 2, 430, 0, 168, 1
    ActorSetGPos 4, 424, 0, 145, 1
    VMStackPush 0x8023
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CDA
    ActorSetGPos 5, 429, 65535, 179, 2

L_0CDA:
    BGMPlay SEQ_BGM_E_C08_ICE
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D03
    ActorSetGPos 255, 418, 0, 162, 1
    VMJump L_0D0F

L_0D03:
    ActorSetGPos 255, 418, 0, 164, 1

L_0D0F:
    ActorCmdExec 255, Movement_0EA4
    ActorCmdWait
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorDelete 19
    WorkSetConst EVENT_WORK_0x40d7, 2
    FlagSet EVENT_FLAG_0x031b
    FlagSet EVENT_FLAG_0x040a
    FlagSet EVENT_FLAG_0x09f2
    FlagReset EVENT_FLAG_0x09f9
    HollowRivalCmd_0262 1, 25
    BGMAmbienceResume
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 75, 1
    Move 32, 1
    MoveEnd

Movement_0D58:
    Move 30, 1
    Move 31, 1
    Move 30, 1
    Move 32, 1
    MoveEnd

L_0D6C:
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DA1
    Cmd_020E 0, 419, 0, 163, 3, 8
    Cmd_020F 0, 419, 0, 163
    VMJump L_0DB9

L_0DA1:
    Cmd_020E 0, 419, 0, 165, 3, 8
    Cmd_020F 0, 419, 0, 165

L_0DB9:
    Cmd_0211 0
    FadeEx 12, 0, 16, 2
    FadeExWait
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DF0
    ActorNew 419, 163, 1, 251, 322, 0
    VMJump L_0DFE

L_0DF0:
    ActorNew 419, 165, 1, 251, 322, 0

L_0DFE:
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 0
    VMReturn

L_0E10:
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E37
    Cmd_020F 1, 419, 0, 163
    VMJump L_0E41

L_0E37:
    Cmd_020F 1, 419, 0, 165

L_0E41:
    Cmd_0211 1
    FadeEx 12, 0, 16, 2
    FadeExWait
    ActorDelete 251
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_0210 1
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E8C
    Cmd_020E 1, 419, 3, 163, 3, 8
    VMJump L_0E9A

L_0E8C:
    Cmd_020E 1, 419, 3, 165, 3, 8

L_0E9A:
    VMReturn

Movement_0E9C:
    Move 69, 1
    MoveEnd

Movement_0EA4:
    Move 70, 1
    MoveEnd

Movement_0EAC:
    Move 13, 1
    Move 60, 1
    Move 3, 1
    Move 60, 1
    Move 0, 1
    Move 60, 1
    Move 2, 1
    Move 60, 1
    Move 1, 1
    Move 60, 1
    MoveEnd

L_0ED8:
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 404
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 145
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0F1B
    ActorCmdExec 0, Movement_245C
    VMSleep 4
    ActorCmdExec 255, Movement_2464
    VMJump L_0F83

L_0F1B:
    VMStackPush 0x8021
    VMStackPushConst 406
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 145
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0F58
    ActorCmdExec 0, Movement_2464
    VMSleep 4
    ActorCmdExec 255, Movement_245C
    VMJump L_0F83

L_0F58:
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 146
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0F83
    ActorCmdExec 255, Movement_244C

L_0F83:
    ActorCmdWait
    // "Drayden: You're even better than I\nhoped. Thanks to you, we drove them off.[f000]븁\u0000\nI'm grateful to your Pokémon, also.[f000]븁\u0000\nAfter all that, the least I can do\nis heal them with this medicine.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenYoureEvenBetter, 0, 1, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_RECOVERY
    SEWait
    PokePartyRecoverAll
    // "Wait here a moment.\nI'll be right back.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_WaitHereMomentIll, 0, 1, 0
    MsgWinCloseAll
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0FCE
    ActorCmdExec 0, Movement_13AC
    VMJump L_0FD6

L_0FCE:
    ActorCmdExec 0, Movement_139C

L_0FD6:
    VMSleep 4
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0FF5
    ActorCmdExec 255, Movement_244C

L_0FF5:
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    VMSleep 8
    SEPlay SEQ_SE_KAIDAN
    ActorAdd 0
    SEWait
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1030
    ActorCmdExec 0, Movement_13B4
    VMJump L_1038

L_1030:
    ActorCmdExec 0, Movement_13A4

L_1038:
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 404
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 145
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_1079
    ActorCmdExec 0, Movement_245C
    VMSleep 4
    ActorCmdExec 255, Movement_2464
    ActorCmdWait
    VMJump L_10B2

L_1079:
    VMStackPush 0x8021
    VMStackPushConst 406
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 145
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_10B2
    ActorCmdExec 0, Movement_2464
    VMSleep 4
    ActorCmdExec 255, Movement_245C
    ActorCmdWait

L_10B2:
    WordSetPlayerName 0
    // "Harrumph...[f000]븁\u0000\nThat's a new record for\nmaking it there and back.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_HarrumphThatsNewRecord, 0, 1, 0
    // "Look, [f000]Ā\u0001\u0000.\nThese are the DNA Splicers![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_LookTheseDnaSplicers, 0, 1, 0
    MsgWinCloseAll
    SEPlay SEQ_SE_W234_HIKARI
    FieldEffect 628
    SEWait
    // "This is what they were after![f000]븁\u0000\nWe're really fortunate Team Plasma\ndidn't get their hands on them![f000]븁\u0000\nI don't know what their goal is, but I'm\nsure they're planning something wicked.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_WhatTheyWereAfter, 0, 1, 0
    MsgWinCloseAll
    ActorAdd 8
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1110
    ActorSetGPos 8, 403, 0, 147, 1
    VMJump L_111C

L_1110:
    ActorSetGPos 8, 404, 0, 148, 1

L_111C:
    ActorCmdExec 8, Movement_13C8
    ActorCmdWait
    BGMPlay SEQ_BGM_DARK_TRINITY
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 146
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_1163
    ActorCmdExec 0, Movement_246C
    ActorCmdExec 255, Movement_13BC
    VMJump L_1175

L_1163:
    ActorCmdExec 255, Movement_13BC
    ActorCmdExec 0, Movement_13BC
    ActorCmdWait

L_1175:
    // "Shadow Triad: As we suspected...\nThe hiding place was in the Gym.[f000]븁\u0000\nWell thought out.\nIf Drayden isn't there, we can't get in.[f000]븁\u0000\nIf he is there, he's the strongest\nguard we could possibly face.[f000]븁\u0000\nIt also explains why the\nPokémon Gym was remodeled.[f000]븁\u0000\nWhatever.\nThe DNA Splicers are in our hands now.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ShadowTriadWeSuspected, 8, 2, 0
    MsgWinCloseAll
    // "Drayden: It's beyond my imagination to\nthink you'd use one of the Seven Sages[f000]븀\u0000\nto find out where the splicers were![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenItsBeyondImagination, 0, 1, 0
    MsgWinCloseAll
    // "Shadow Triad: Now you know, then.[f000]븁\u0000\nWe will accomplish our goals,\nno matter what the cost.[f000]븁\u0000\nWe don't have the ability to captivate\nthe hearts of others like Lord N does.[f000]븁\u0000\nInstead, we will bend people\nto our will with brute force.[f000]븀\u0000\nTwo years was a surprisingly long wait.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ShadowTriadNowKnow, 8, 2, 0
    MsgWinCloseAll
    ActorAdd 9
    ActorAdd 10
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_11D8
    ActorSetGPos 9, 408, 0, 145, 2
    ActorSetGPos 10, 408, 0, 146, 2
    VMJump L_11F0

L_11D8:
    ActorSetGPos 9, 403, 0, 147, 3
    ActorSetGPos 10, 408, 0, 147, 2

L_11F0:
    ActorCmdExec 9, Movement_13C8
    ActorCmdExec 10, Movement_13C8
    ActorCmdWait
    VMSleep 40
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1237
    ActorCmdExec 8, Movement_1450
    ActorCmdExec 9, Movement_14A4
    ActorCmdExec 10, Movement_14C8
    VMJump L_124F

L_1237:
    ActorCmdExec 8, Movement_13E0
    ActorCmdExec 9, Movement_1420
    ActorCmdExec 10, Movement_1428

L_124F:
    ActorCmdWait
    ActorDelete 9
    ActorDelete 10
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1278
    ActorCmdExec 255, Movement_244C
    ActorCmdWait

L_1278:
    // "Drayden: If they've been stolen from us,\nwe'll just have to take them back![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenIfTheyveBeen, 0, 1, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    GameGetVersion 0x8023
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_12C1
    ActorCmdExec 0, Movement_14EC
    VMJump L_134E

L_12C1:
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_12F2
    ActorCmdExec 0, Movement_151C
    VMJump L_134E

L_12F2:
    VMStackPush 0x8023
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_1323
    ActorCmdExec 0, Movement_1440
    VMJump L_134E

L_1323:
    VMStackPush 0x8023
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_134E
    ActorCmdExec 0, Movement_1430

L_134E:
    VMStackPush 0x8021
    VMStackPushConst 405
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_136D
    VMSleep 20
    ActorCmdExec 255, Movement_2454

L_136D:
    ActorCmdWait
    ActorSetGPos 0, 398, 0, 167, 0
    ActorSetMoveCode 0, 5
    ActorSetGPos 8, 442, 0, 175, 2
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40d8, 1
    FlagSet EVENT_FLAG_0x031e
    VMReturn
    .balign 4, 0

Movement_139C:
    Move 16, 1
    MoveEnd

Movement_13A4:
    Move 17, 1
    MoveEnd

Movement_13AC:
    Move 16, 1
    MoveEnd

Movement_13B4:
    Move 17, 1
    MoveEnd

Movement_13BC:
    Move 33, 1
    Move 75, 1
    MoveEnd

Movement_13C8:
    Move 70, 1
    Move 184, 1
    MoveEnd

Movement_13D4:
    Move 185, 1
    Move 69, 1
    MoveEnd

Movement_13E0:
    Move 17, 2
    Move 71, 1
    Move 17, 1
    Move 73, 1
    Move 17, 2
    Move 74, 1
    Move 72, 1
    Move 2, 1
    Move 71, 1
    Move 18, 1
    Move 73, 1
    Move 18, 1
    Move 74, 1
    Move 72, 1
    Move 17, 5
    MoveEnd

Movement_1420:
    Move 17, 9
    MoveEnd

Movement_1428:
    Move 19, 6
    MoveEnd

Movement_1430:
    Move 17, 5
    Move 18, 3
    Move 17, 4
    MoveEnd

Movement_1440:
    Move 18, 3
    Move 17, 5
    Move 17, 4
    MoveEnd

Movement_1450:
    Move 71, 1
    Move 17, 1
    Move 73, 1
    Move 17, 1
    Move 74, 1
    Move 72, 1
    Move 3, 1
    Move 71, 1
    Move 19, 1
    Move 73, 1
    Move 19, 3
    Move 74, 1
    Move 72, 1
    Move 17, 3
    Move 71, 1
    Move 17, 1
    Move 73, 1
    Move 17, 1
    Move 74, 1
    Move 72, 1
    MoveEnd

Movement_14A4:
    Move 19, 1
    Move 71, 1
    Move 19, 1
    Move 73, 1
    Move 19, 3
    Move 74, 1
    Move 72, 1
    Move 19, 3
    MoveEnd

Movement_14C8:
    Move 19, 1
    Move 71, 1
    Move 19, 1
    Move 73, 1
    Move 19, 5
    Move 74, 1
    Move 72, 1
    Move 19, 2
    MoveEnd

Movement_14EC:
    Move 19, 2
    Move 17, 3
    Move 74, 1
    Move 72, 1
    Move 17, 3
    Move 71, 1
    Move 17, 1
    Move 73, 1
    Move 17, 2
    Move 74, 1
    Move 72, 1
    MoveEnd

Movement_151C:
    Move 17, 4
    Move 3, 1
    Move 19, 2
    Move 74, 1
    Move 72, 1
    Move 17, 3
    Move 71, 1
    Move 17, 1
    Move 73, 1
    Move 17, 2
    Move 74, 1
    Move 72, 1
    MoveEnd

Script_34:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    BGMPlay SEQ_BGM_DARK_TRINITY
    VMCall L_15B7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    BGMPlay SEQ_BGM_DARK_TRINITY
    PlayerGetGPos 0x8021, 0x8022
    PlayerGetDir 0x8020
    WorkAdd 0x8021, 1
    ActorWalkRoute 8, 0x8021, 0x8022, 4, 4, 1
    ActorCmdWait
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_15AB
    ActorCmdExec 255, Movement_2464
    ActorCmdWait

L_15AB:
    VMCall L_15B7
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_15B7:
    // "Shadow Triad: Heh heh...[f000]븁\u0000\nYou don't really think you can take\nthe DNA Splicers back, do you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ShadowTriadHehHeh, 8, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_SHADOW_4, 0, 0
    VMCall L_1EF9
    PlayerGetGPos 0x8021, 0x8022
    // "Shadow Triad: Awww. How unlucky.[f000]븁\u0000\nI don't happen to be the one\nholding the DNA Splicers.[f000]븁\u0000\nI was just buying time\nfor the others to escape.[f000]븀\u0000\nCheerio, bye-bye, whatever.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ShadowTriadAwwwHow, 8, 0, 0
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1612
    ActorSetGPos 0, 431, 0, 176, 3
    VMJump L_1624

L_1612:
    WorkAdd 0x8022, 1
    ActorSetGPos 0, 431, 0, 0x8022, 3

L_1624:
    ActorSetMoveCode 0, 0
    PlayerGetGPos 0x8021, 0x8022
    WorkCmpConst 0x8022, 173
    VMJumpIf CMP_EQ, L_1643
    VMJump L_1651

L_1643:
    ActorCmdExec 0, Movement_1A68
    VMJump L_16D5

L_1651:
    WorkCmpConst 0x8022, 174
    VMJumpIf CMP_EQ, L_1664
    VMJump L_1672

L_1664:
    ActorCmdExec 0, Movement_1A88
    VMJump L_16D5

L_1672:
    WorkCmpConst 0x8022, 175
    VMJumpIf CMP_EQ, L_1685
    VMJump L_1693

L_1685:
    ActorCmdExec 0, Movement_1AA8
    VMJump L_16D5

L_1693:
    WorkCmpConst 0x8022, 176
    VMJumpIf CMP_EQ, L_16A6
    VMJump L_16B4

L_16A6:
    ActorCmdExec 0, Movement_1AC8
    VMJump L_16D5

L_16B4:
    WorkCmpConst 0x8022, 177
    VMJumpIf CMP_EQ, L_16C7
    VMJump L_16D5

L_16C7:
    ActorCmdExec 0, Movement_1AA8
    VMJump L_16D5

L_16D5:
    VMSleep 16
    ActorCmdExec 8, Movement_13D4
    ActorCmdWait
    ActorDelete 8
    BGMChangeMap
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 441
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1712
    ActorWalkRoute 255, 440, 0x8022, 4, 8, 0
    ActorCmdWait

L_1712:
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_173B
    ActorCmdExec 0, Movement_2454
    ActorCmdExec 255, Movement_244C
    VMJump L_174B

L_173B:
    ActorCmdExec 0, Movement_244C
    ActorCmdExec 255, Movement_2454

L_174B:
    ActorCmdWait
    WordSetPlayerName 0
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1775
    // "Drayden: Oh no! Ah! No![f000]븁\u0000\nNo, no, it's not your fault, [f000]Ā\u0001\u0000.\nYou were impressive![f000]븁\u0000\nIt's me who's let everybody down.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenOhNoAh, 0, 3, 0
    VMJump L_1781

L_1775:
    // "Drayden: Oh no! Ah! No![f000]븁\u0000\nNo, no, it's not your fault, [f000]Ā\u0001\u0000.\nYou were impressive![f000]븁\u0000\nIt's me who's let everybody down.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenOhNoAh, 0, 4, 0

L_1781:
    MsgWinCloseAll
    MEPlay SEQ_ME_CALL
    // "The Xtransceiver is ringing."
    SystemMsg OpelucidCity_Text_XtransceiverRinging, 2
    MEWait
    InfoMsgClose
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 picked up the Xtransceiver.[f000]븁\u0000"
    SystemMsg OpelucidCity_Text_PickedUpXtransceiver, 2
    InfoMsgClose
    FadeOutBlackQ
    FadeWait
    CallXTransceiver 5, 0
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_17CF
    // "Drayden: Augh!\nIf there were only two of me![f000]븁\u0000\nThen I could protect the town\nand chase after them, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenAughIfThere, 0, 3, 0
    VMJump L_17DB

L_17CF:
    // "Drayden: Augh!\nIf there were only two of me![f000]븁\u0000\nThen I could protect the town\nand chase after them, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenAughIfThere, 0, 4, 0

L_17DB:
    MsgWinCloseAll
    PlayerGetGPos 0x8021, 0x8022
    SEPlay SEQ_SE_KAIDAN
    ActorNew 446, 175, 2, 251, 223, 0
    SEWait
    BGMPlay SEQ_BGM_E_CHEREN
    ActorCmdExec 0, Movement_2464
    ActorCmdExec 255, Movement_2464
    ActorCmdWait
    ActorWalkRoute 251, 442, 0x8022, 4, 4, 0
    ActorCmdWait
    VMStackPush 0x8022
    VMStackPushConst 175
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_183A
    ActorCmdExec 251, Movement_245C
    ActorCmdWait

L_183A:
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1865
    // "Cheren: [f000]Ā\u0001\u0000.\nDrayden, glad to see you're safe as well.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_CherenDraydenGladSee, 251, 6, 0
    VMJump L_1871

L_1865:
    // "Cheren: [f000]Ā\u0001\u0000.\nDrayden, glad to see you're safe as well.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_CherenDraydenGladSee, 251, 5, 0

L_1871:
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1898
    // "Drayden: Hello, Cheren.\nYou have certainly grown.[f000]븁\u0000\nI hear you filled in for Lenora by\nbecoming the Gym Leader in[f000]븀\u0000\nAspertia City.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenHelloCherenHave, 0, 3, 0
    VMJump L_18A4

L_1898:
    // "Drayden: Hello, Cheren.\nYou have certainly grown.[f000]븁\u0000\nI hear you filled in for Lenora by\nbecoming the Gym Leader in[f000]븀\u0000\nAspertia City.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenHelloCherenHave, 0, 4, 0

L_18A4:
    MsgWinCloseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_18D1
    // "Cheren: Thanks.\nWe should save catching up for later.[f000]븁\u0000\nI have a good idea of where\nTeam Plasma is hiding.[f000]븁\u0000\nThe place with the lowest temperature\nin the Unova region right now is[f000]븀\u0000\nan area close to Humilau City.[f000]븁\u0000\nDrayden, please protect Opelucid City.\n[f000]Ā\u0001\u0001 and I will find Team Plasma![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_CherenThanksWeShould, 251, 6, 0
    VMJump L_18DD

L_18D1:
    // "Cheren: Thanks.\nWe should save catching up for later.[f000]븁\u0000\nI have a good idea of where\nTeam Plasma is hiding.[f000]븁\u0000\nThe place with the lowest temperature\nin the Unova region right now is[f000]븀\u0000\nan area close to Humilau City.[f000]븁\u0000\nDrayden, please protect Opelucid City.\n[f000]Ā\u0001\u0001 and I will find Team Plasma![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_CherenThanksWeShould, 251, 5, 0

L_18DD:
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1904
    // "Drayden: Very well.\nI'll do my part--and thanks.[f000]븁\u0000\nThe chase I'll leave to you!\nBut...don't do anything reckless.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenVeryWellIll, 0, 3, 0
    VMJump L_1910

L_1904:
    // "Drayden: Very well.\nI'll do my part--and thanks.[f000]븁\u0000\nThe chase I'll leave to you!\nBut...don't do anything reckless.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenVeryWellIll, 0, 4, 0

L_1910:
    MsgWinCloseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_193D
    // "Cheren: [f000]Ā\u0001\u0000!\nI could use your help, if you're willing.[f000]븁\u0000\nYou're a skilled Trainer who can go\ntoe-to-toe with Team Plasma.[f000]븀\u0000\nThat's my definition of useful.[f000]븁\u0000\nDrayden, we're off![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_CherenCouldUseHelp, 251, 6, 0
    VMJump L_1949

L_193D:
    // "Cheren: [f000]Ā\u0001\u0000!\nI could use your help, if you're willing.[f000]븁\u0000\nYou're a skilled Trainer who can go\ntoe-to-toe with Team Plasma.[f000]븀\u0000\nThat's my definition of useful.[f000]븁\u0000\nDrayden, we're off![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_CherenCouldUseHelp, 251, 5, 0

L_1949:
    MsgWinCloseAll
    VMStackPush 0x8022
    VMStackPushConst 173
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1972
    ActorWalkRoute 251, 446, 174, 4, 4, 1
    VMJump L_19A7

L_1972:
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1999
    ActorWalkRoute 251, 446, 176, 4, 4, 1
    VMJump L_19A7

L_1999:
    ActorWalkRoute 251, 446, 0x8022, 4, 4, 1

L_19A7:
    ActorCmdWait
    BGMChangeMap
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 251
    SEWait
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_19EA
    ActorCmdExec 0, Movement_2454
    ActorCmdExec 255, Movement_244C
    VMJump L_19FA

L_19EA:
    ActorCmdExec 0, Movement_244C
    ActorCmdExec 255, Movement_2454

L_19FA:
    ActorCmdWait
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1A21
    // "Drayden: Humilau City, huh...[f000]븁\u0000\nIs there anything to the\nnorth of Undella Town?[f000]븁\u0000\n[f000]Ā\u0001\u0000, protect Pokémon\nfrom Team Plasma![f000]븁\u0000\nAll people should think for themselves\nabout the nature of the relationship[f000]븀\u0000\nbetween people and Pokémon.[f000]븁\u0000\nIt's not something Team Plasma gets\nto decide for everyone!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenHumilauCityHuh, 0, 3, 0
    VMJump L_1A2D

L_1A21:
    // "Drayden: Humilau City, huh...[f000]븁\u0000\nIs there anything to the\nnorth of Undella Town?[f000]븁\u0000\n[f000]Ā\u0001\u0000, protect Pokémon\nfrom Team Plasma![f000]븁\u0000\nAll people should think for themselves\nabout the nature of the relationship[f000]븀\u0000\nbetween people and Pokémon.[f000]븁\u0000\nIt's not something Team Plasma gets\nto decide for everyone!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenHumilauCityHuh, 0, 4, 0

L_1A2D:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40d8, 2
    FlagSet EVENT_FLAG_0x031d
    WorkSetConst EVENT_WORK_0x411e, 1
    FlagSet EVENT_FLAG_0x03b3
    FlagReset EVENT_FLAG_0x03da
    FlagSet EVENT_FLAG_0x0307
    WorkSetConst EVENT_WORK_0x40de, 1
    WorkSetConst EVENT_WORK_0x4146, 1
    HollowRivalCmd_0262 1, 26
    HollowRivalCmd_0262 2, 7
    VMReturn
    .balign 4, 0

Movement_1A68:
    Move 71, 1
    Move 19, 1
    Move 73, 1
    Move 19, 1
    Move 74, 1
    Move 72, 1
    Move 19, 7
    MoveEnd

Movement_1A88:
    Move 71, 1
    Move 19, 1
    Move 73, 1
    Move 19, 4
    Move 74, 1
    Move 72, 1
    Move 19, 4
    MoveEnd

Movement_1AA8:
    Move 71, 1
    Move 19, 1
    Move 73, 1
    Move 19, 5
    Move 74, 1
    Move 72, 1
    Move 19, 3
    MoveEnd

Movement_1AC8:
    Move 71, 1
    Move 19, 1
    Move 73, 1
    Move 19, 6
    Move 74, 1
    Move 72, 1
    Move 19, 2
    MoveEnd

Script_13:
    ActorsPauseAll
    WordSetPlayerName 0
    TrainerCardHasBadge 0x8008, 7
    VMStackPush EVENT_WORK_0x413e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1B34
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I was just reminded of Iris.[f000]븁\u0000\nYes, the Champion.\nThat Iris...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_JustRemindedIrisYes, 0, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_OPELUCID_CITY_12, 9, 0, 9, 1
    VMJump L_1C2F

L_1B34:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 4
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_1B61
    SEPlay SEQ_SE_MESSAGE
    // "Drayden: They're getting tougher![f000]븁\u0000\n[f000]Ā\u0001\u0000!\nTake care of the others!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenTheyreGettingTougher, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_1C2F

L_1B61:
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_1B9E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Drayden: Little by little,\nthe ice is starting to melt.[f000]븁\u0000\nIt's all thanks to you.\nAs a fellow Trainer, I heartily thank you!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenLittleByLittle, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1C2F

L_1B9E:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40d8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_1BDB
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Drayden: The only one left is\nZinzolin of the Seven Sages![f000]븀\u0000\nWhere could he be?[f000]븁\u0000\nOh!\nWhat's going on at the Pokémon Gym?!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenOnlyOneLeft, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1C2F

L_1BDB:
    VMStackPush EVENT_WORK_0x40d8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1C08
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Drayden: They don't seem to be in\nthis area. However, they can hide their[f000]븀\u0000\npresence, so be on guard as you look!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenTheyDontSeem, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1C2F

L_1C08:
    VMStackPush EVENT_WORK_0x40d8
    VMStackPushConst 2
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1C2F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Drayden: Humilau City, huh...[f000]븁\u0000\nIs there anything to the\nnorth of Undella Town?[f000]븁\u0000\n[f000]Ā\u0001\u0000, protect Pokémon\nfrom Team Plasma![f000]븁\u0000\nAll people should think for themselves\nabout the nature of the relationship[f000]븀\u0000\nbetween people and Pokémon.[f000]븁\u0000\nIt's not something Team Plasma gets\nto decide for everyone!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_DraydenHumilauCityHuh, 0, 0
    LastKeyWait
    ActorMsgClose

L_1C2F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    BGMPlayPush SEQ_BGM_E_7_SAGE
    // "Zinzolin: Oh, for crying out loud...[f000]븁\u0000\nI didn't expect to have to fight\nhampered by cold like this.[f000]븁\u0000\nWell, no matter! The fact that I'm\nshivering means I'm truly alive![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ZinzolinOhCryingOut, 1, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_TEAM_PLASMA_ZINZOLIN_3, 0, 0
    VMCall L_1EF9
    // "Zinzolin: You're a strong Trainer.[f000]븁\u0000\nYou definitely are adept\nat handling Pokémon.[f000]븁\u0000\nI believe I'll take my leave, simply\nbecause I can't stand this cold.[f000]븁\u0000\nBut...imagine this...[f000]븁\u0000\nA Unova region...completely...covered...\nin...ice.[f000]븁\u0000\nTo achieve that splendor,\nwe'll do whatever it takes[f000]븀\u0000\nto obtain the DNA Splicers.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ZinzolinYoureStrongTrainer, 1, 0, 0
    ActorMsgClose
    GameGetVersion 0x8023
    FadeEx 3, 0, 16, 4
    FadeExWait
    VMStackPush 0x8023
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1CAA
    ActorSetGPos 0, 405, 0, 145, 1
    ActorCmdExec 255, Movement_2434
    ActorCmdWait
    VMJump L_1CC0

L_1CAA:
    ActorSetGPos 0, 405, 0, 145, 1
    ActorCmdExec 255, Movement_2434
    ActorCmdWait

L_1CC0:
    ActorDelete 1
    ActorDelete 2
    ActorDelete 3
    ActorDelete 5
    ActorDelete 6
    ActorDelete 4
    ActorDelete 7
    FlagSet EVENT_FLAG_0x031b
    FlagSet EVENT_FLAG_0x031c
    FlagReset EVENT_FLAG_0x031d
    FlagReset EVENT_FLAG_0x031e
    WorkAdd EVENT_WORK_0x40d7, 1
    FadeEx 3, 16, 0, 4
    FadeExWait
    VMCall L_0ED8
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerFlagGet TRAINER_TEAM_PLASMA_GRUNT_41, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1D70
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_41
    // "Team Plasma: Pokémon are pawns!\nThey're a means to an end.[f000]븁\u0000\nThat's what I was taught![f000]븁\u0000\nDon't think about it too much.\nIt's easier that way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaPokemonPawns, 2, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_41, 0, 0
    VMCall L_1EF9
    WorkAdd EVENT_WORK_0x40d7, 1
    TrainerFlagSet TRAINER_TEAM_PLASMA_GRUNT_41
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1D70
    FlagReset EVENT_FLAG_0x031b
    ActorAdd 1

L_1D70:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1D99
    // "Team Plasma: The others were saying\nthat the only place we haven't yet[f000]븀\u0000\nsearched is the Pokémon Gym!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaOthersWere, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1DA9

L_1D99:
    // "Team Plasma: How can this be?[f000]븁\u0000\nIs that the kind of power Pokémon have\nwhen they're not treated like objects?"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaHowCan, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_1DA9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerFlagGet TRAINER_TEAM_PLASMA_GRUNT_42, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1E15
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_42
    // "Team Plasma: This time, we'll take over\nthe Unova region![f000]븁\u0000\nYou! You're trembling already?\nFrom fear or from cold--either's good![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaTimeWell, 3, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_42, 0, 0
    VMCall L_1EF9
    WorkAdd EVENT_WORK_0x40d7, 1
    TrainerFlagSet TRAINER_TEAM_PLASMA_GRUNT_42
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1E15
    FlagReset EVENT_FLAG_0x031b
    ActorAdd 1

L_1E15:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1E3E
    // "Team Plasma: Now that you mention it,\nI think I might have been ordered[f000]븀\u0000\nto comb the Pokémon Gym..."
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaNowMention, 3, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1E4E

L_1E3E:
    // "Team Plasma: It's not over!\nThe others will find the DNA Splicers![f000]븀\u0000\nWhen they do, the Unova region is ours!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaItsNot, 3, 0, 0
    LastKeyWait
    ActorMsgClose

L_1E4E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerFlagGet TRAINER_TEAM_PLASMA_GRUNT_43, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1EBA
    TrainerBGMPlayPush TRAINER_TEAM_PLASMA_GRUNT_43
    // "Team Plasma: Don't get in our way!\nYou bother![f000]븀\u0000\nBother, bother, bother![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaDontGet, 4, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_TEAM_PLASMA_GRUNT_43, 0, 0
    VMCall L_1EF9
    WorkAdd EVENT_WORK_0x40d7, 1
    TrainerFlagSet TRAINER_TEAM_PLASMA_GRUNT_43
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1EBA
    FlagReset EVENT_FLAG_0x031b
    ActorAdd 1

L_1EBA:
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1EE3
    // "Team Plasma: Zinzolin said to stick to\nDrayden, because that's where we'll[f000]븀\u0000\nfind anything important!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaZinzolinSaid, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1EF3

L_1EE3:
    // "Team Plasma: How humiliating...\nBeing bothered by a bother..."
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaHowHumiliating, 4, 0, 0
    LastKeyWait
    ActorMsgClose

L_1EF3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_1EF9:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1F18
    CallTrainerBattleEnd
    VMJump L_1F1A

L_1F18:
    CallTrainerLose

L_1F1A:
    VMReturn

Script_17:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1F4B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: I think Zinzolin was going\nto check the Pokémon Gym..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaThinkZinzolin, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1F5F

L_1F4B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: I lost.\nI didn't stand a chance...[f000]븁\u0000\nI can't believe Haxorus blasted me\nall the way over here!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaLostDidnt, 0, 0
    LastKeyWait
    ActorMsgClose

L_1F5F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Team Plasma: I've been nursing a\ngrudge for two years, old man.[f000]븁\u0000\n...Even if this is the first time\nI've battled you!"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaIveBeen, 6, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40d7
    VMStackPushConst 5
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_1FB0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: Oh, that Drayden!\nWhere did he hide the DNA Splicers?[f000]븀\u0000\nSomewhere familiar and common, right?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaOhDrayden, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1FC4

L_1FB0:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Team Plasma: That old guy won't hold\nback even if his opponent is a young girl."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_TeamPlasmaOldGuy, 0, 0
    LastKeyWait
    ActorMsgClose

L_1FC4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_29:
    ActorsPauseAll
    ActorCmdExec 0, Movement_246C
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    PlayerGetDir 0x8020
    WorkSub 0x8022, 2
    ActorWalkRoute 0, 0x8021, 0x8022, 0, 8, 1
    ActorCmdWait
    VMStackPush 0x8021
    VMStackPushConst 418
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_2013
    ActorCmdExec 0, Movement_2454
    ActorCmdWait

L_2013:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2030
    ActorCmdExec 255, Movement_244C
    ActorCmdWait

L_2030:
    // "I was just reminded of Iris.[f000]븁\u0000\nYes, the Champion.\nThat Iris...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_JustRemindedIrisYes, 0, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_OPELUCID_CITY_12, 9, 0, 9, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_33:
    ActorsPauseAll
    FlagReset EVENT_FLAG_0x031a
    ActorAdd 0
    ActorWalkRoute 255, 418, 162, 0, 8, 1
    ActorCmdWait
    WorkSetConst 0x8029, 0
    BMCreateHandleByGPos 0x8029, 1, 418, 159
    BMHndAudioVisualAnmPlay 0x8029, 0
    BMHndAnmWait 0x8029
    SEPlay SEQ_SE_KAIDAN
    ActorSetGPos 0, 418, 0, 159, 1
    SEWait
    ActorCmdExec 255, Movement_244C
    ActorCmdWait
    ActorCmdExec 0, Movement_240C
    ActorCmdWait
    BMHndAudioVisualAnmPlay 0x8029, 1
    BMHndAnmWait 0x8029
    BMReleaseHandle 0x8029
    // "I was just reminded of Iris.[f000]븁\u0000\nYes, the Champion.\nThat Iris...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_JustRemindedIrisYes, 0, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_OPELUCID_CITY_12, 9, 0, 9, 1
    WorkSetConst 0x8029, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Script_30:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    // "I wonder what kind of\nChampion she'll become."
    ActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_WonderWhatKindChampion, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 418, 161, 4, 8, 0
    ActorCmdWait
    ActorDelete 0
    WorkSetConst EVENT_WORK_0x413e, 3
    WorkSetConst EVENT_WORK_0x4048, 1
    FlagSet EVENT_FLAG_0x031a
    FlagReset EVENT_FLAG_0x0320
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_216B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Why do Pokémon stay by our sides?[f000]븁\u0000\nIf we could talk to them, we could ask.\nI'm a bit scared about what they'd say."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_WhyPokemonStayBy, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_2183

L_216B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The memories of everyone who has lived\nin this city have built up over the years[f000]븀\u0000\nwithout changing much."
    // "This city changes endlessly.[f000]븁\u0000\nAnd every change is engraved in the\nmemories of the people who live here."
    ActorMsgVersioned 1024, OpelucidCity_Text_MemoriesEveryoneWhoHas, OpelucidCity_Text_CityChangesEndlesslyEvery, 14, 0, 0
    LastKeyWait
    ActorMsgClose

L_2183:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_21B8
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I reached heights I never could\nhave arrived at because I had[f000]븀\u0000\nPokémon by my side.[f000]븁\u0000\nAnd I believe my Pokémon became\ntough because they were with me!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ReachedHeightsNeverCould, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_21D0

L_21B8:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I think some things shouldn't be\nchanged, even if it's inconvenient."
    // "I think some things must be changed\nno matter how much you love them!"
    ActorMsgVersioned 1024, OpelucidCity_Text_ThinkSomeThingsShouldnt, OpelucidCity_Text_ThinkSomeThingsMust, 12, 0, 0
    LastKeyWait
    ActorMsgClose

L_21D0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2205
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A person called N had the\nlegendary Pokémon with him,[f000]븀\u0000\nbut was he really the hero?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_PersonCalledNHad, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_221D

L_2205:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A world of ice.[f000]븁\u0000\nSuch beauty goes beyond old and new--\ninspirational, yet terrifying.[f000]븁\u0000\nThough, to me, there's a comfort in\nold things that you can't find in the new."
    // "A world of ice.[f000]븁\u0000\nSuch beauty goes beyond new and old--\ninspirational, yet terrifying.[f000]븁\u0000\nThough, to me, there's a coolness to new\nthings that I prize more than the old."
    ActorMsgVersioned 1024, OpelucidCity_Text_WorldIceSuchBeauty, OpelucidCity_Text_WorldIceSuchBeauty_2, 15, 0, 0
    LastKeyWait
    ActorMsgClose

L_221D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_25:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_2252
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Gym Leader Drayden leads\nOpelucid City as its mayor![f000]븁\u0000\nHe's always training by wrestling\nwith his Pokémon."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_GymLeaderDraydenLeads, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_2266

L_2252:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Pokémon show me affection,\nso I'm not lonely![f000]븁\u0000\nAnd I return their affection even more!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_PokemonShowAffectionIm, 0, 0
    LastKeyWait
    ActorMsgClose

L_2266:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_27:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_229B
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There was a group that told\npeople to release their Pokémon.[f000]븁\u0000\nThey were great big liars, but it\ncreated a good opportunity to think."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_ThereGroupToldPeople, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_22B3

L_229B:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "After that speech, my Pokémon\nand I have been thinking about[f000]븀\u0000\nwhat's true for us."
    // "After that speech, I've been thinking\nabout the ideal relationship for me and[f000]븀\u0000\nmy Pokémon as we move forward together."
    ActorMsgVersioned 1024, OpelucidCity_Text_AfterSpeechPokemonHave, OpelucidCity_Text_AfterSpeechIveBeen, 13, 0, 0
    LastKeyWait
    ActorMsgClose

L_22B3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_28:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_22E8
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Unova's symbols are\nZekrom and Reshiram,[f000]븀\u0000\nbut I wonder where they are now.[f000]븁\u0000\nAre they passing along their\nancient knowledge to someone?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCity_Text_UnovasSymbolsZekromReshiram, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_2300

L_22E8:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Pursuing ideals is different from person\nto person and Pokémon to Pokémon.[f000]븁\u0000\nIn this city, it seems the ideal is\nto cherish the past."
    // "The truth people pursue is different\nfrom person to person and[f000]븀\u0000\nPokémon to Pokémon.[f000]븁\u0000\nIn this city, it seems the truth is\nconstant change."
    ActorMsgVersioned 1024, OpelucidCity_Text_PursuingIdealsDifferentFrom, OpelucidCity_Text_TruthPeoplePursueDifferent, 16, 0, 0
    LastKeyWait
    ActorMsgClose

L_2300:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_26:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The hopes of an ancient people are put\ninto this melody...[f000]븁\u0000\nI will bring them back to us now."
    // "Our hopes are put into this melody...\nI will send them to the future."
    ActorMsgVersioned 1024, OpelucidCity_Text_HopesAncientPeoplePut, OpelucidCity_Text_OurHopesPutInto, 18, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Opelucid City\nTime's Dividing Line"
    MsgPlaceSign OpelucidCity_Text_OpelucidCityTimesDividing, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Drayden's House"
    MsgPlaceSign OpelucidCity_Text_DraydensHouse, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_238B
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Formerly the Battle House..."
    MsgPlaceSign OpelucidCity_Text_FormerlyBattleHouse_2, 2
    MsgPlaceSignClose
    VMJump L_239D

L_238B:
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Formerly the Battle House..."
    MsgPlaceSign OpelucidCity_Text_FormerlyBattleHouse, 2
    MsgPlaceSignClose

L_239D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Trainer Tips![f000]븁\u0000\n\nMayor Drayden will teach you if he\nrecognizes you as a strong Trainer.[f000]븁\u0000\nVisit his home to learn the\nmost powerful Dragon-type move!"
    MsgPlaceSign OpelucidCity_Text_TrainerTipsMayorDrayden, 0
    MsgPlaceSignClose
    FlagSet EVENT_FLAG_0x0a66
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_23F2
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Opelucid City Pokémon Gym\nLeader: Drayden[f000]븀\u0000\nThe Spartan Mayor"
    MsgPlaceSign OpelucidCity_Text_OpelucidCityPokemonGym_2, 2
    MsgPlaceSignClose
    VMJump L_2404

L_23F2:
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Opelucid City Pokémon Gym\nLeader: Drayden[f000]븀\u0000\nThe Spartan Mayor"
    MsgPlaceSign OpelucidCity_Text_OpelucidCityPokemonGym, 2
    MsgPlaceSignClose

L_2404:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_240C:
    Move 13, 1
    MoveEnd

Movement_2414:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_2434:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_244C:
    Move 32, 1
    MoveEnd

Movement_2454:
    Move 33, 1
    MoveEnd

Movement_245C:
    Move 34, 1
    MoveEnd

Movement_2464:
    Move 35, 1
    MoveEnd

Movement_246C:
    Move 75, 1
    MoveEnd

Movement_2474:
    Move 159, 1
    MoveEnd
