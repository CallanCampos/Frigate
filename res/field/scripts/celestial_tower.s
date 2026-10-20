#include "asm/field_script.inc"
#include "text/script/celestial_tower.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPushFlag EVENT_FLAG_0x0397
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4117
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0051
    FlagSet EVENT_FLAG_0x0397
    WorkSetConst EVENT_WORK_0x4117, 1

L_0051:
    VMHalt

Script_2:
    ActorsPauseAll
    WordSetPlayerName 0
    // "It's the Celestial Tower bell...\nWill you ring the bell?"
    InfoMsg CelestialTower_Text_ItsCelestialTowerBell, 2
    SEPlay SEQ_SE_MESSAGE
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F0
    VMStackPushFlag EVENT_FLAG_0x0198
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0095
    FunfestMissionBroadcast 24, 0
    FlagSet EVENT_FLAG_0x0198

L_0095:
    Cmd_0240 64, 30
    SEPlay SEQ_SE_FLD_151
    // "[f000]Ā\u0001\u0000 rang the bell..."
    InfoMsg CelestialTower_Text_RangBell, 2
    WorkSetConst 0x8023, 0
    BMCreateHandleByGPos 0x8023, 8, 16, 7
    BMHndAudioVisualAnmPlay 0x8023, 0
    BMHndAnmWait 0x8023
    BMReleaseHandle 0x8023
    SEWait
    Cmd_0241 60
    // "The sound of the bell\nreverberates through the area..."
    InfoMsg CelestialTower_Text_SoundBellReverberatesThrough, 2
    LastKeyWait
    InfoMsgClose_0039
    VMStackPush EVENT_WORK_0x40ee
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EA
    WorkSetConst EVENT_WORK_0x40ee, 6

L_00EA:
    VMJump L_00F9

L_00F0:
    // "The bell was left untouched..."
    InfoMsg CelestialTower_Text_BellLeftUntouched, 2
    LastKeyWait
    InfoMsgClose_0039

L_00F9:
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    // "A mysterious presence can be felt here!\nCheck the surrounding area?"
    SystemMsg CelestialTower_Text_MysteriousPresenceCanFelt, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B0
    MsgWinCloseAll
    FlagReset EVENT_FLAG_0x0397
    WorkSetConst EVENT_WORK_0x4117, 2
    VMSleep 30
    PVPlay 481, 0
    // "Kyauun!"
    InfoMsg CelestialTower_Text_Kyauun, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    PlayerGetGPos 0x8021, 0x8022
    ActorAdd 0
    ActorSetGPos 0, 24, 9, 26, 2
    WorkAdd 0x8022, 2
    ActorMoveLinear 0, 0x8021, 5, 0x8022, 48
    ActorSetGPos 0, 0x8021, 5, 0x8022, 2
    ActorCmdExec 0, Movement_0270
    VMSleep 8
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01A4
    ActorCmdExec 255, Movement_0278

L_01A4:
    ActorCmdWait
    VMSleep 16
    VMJump L_01B2

L_01B0:
    MsgWinCloseAll

L_01B2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 481, 0
    // "Kyauun!"
    ScreamMsg CelestialTower_Text_Kyauun, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 481, 65, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0206
    FlagSet EVENT_FLAG_0x0397
    WorkSetConst EVENT_WORK_0x4117, 3
    ActorDelete 0
    CallWildBattleEnd
    VMJump L_0208

L_0206:
    CallWildLose

L_0208:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_021F
    VMJump L_0229

L_021F:
    FlagSet EVENT_FLAG_0x018d
    VMJump L_0259

L_0229:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0249
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0249
    VMJump L_0259

L_0249:
    // "Mesprit went flying off somewhere..."
    SystemMsg CelestialTower_Text_MespritWentFlyingOff, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0259

L_0259:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 35, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0270:
    Move 32, 1
    MoveEnd

Movement_0278:
    Move 33, 1
    MoveEnd
