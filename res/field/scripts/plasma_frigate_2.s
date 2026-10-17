#include "asm/field_script.inc"
#include "text/script/plasma_frigate_2.h"

// Script plugin 12, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntriesEnd

Script_2:
    VMCall L_0044
    VMHalt

Script_3:
    VMCall L_0073
    VMHalt

Script_8:
    VMCall L_0073
    VMCall L_0044
    VMHalt

L_0044:
    VMStackPushFlag EVENT_FLAG_0x016c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0067
    ObjInitWarpGPos 0, 0, 0, 0
    VMJump L_0071

L_0067:
    ObjInitWarpGPos 6, 0, 0, 0

L_0071:
    VMReturn

L_0073:
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0092
    Plugin12_Cmd1006 0, 0
    VMJump L_0098

L_0092:
    Plugin12_Cmd1006 0, 1

L_0098:
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B7
    Plugin12_Cmd1006 1, 0
    VMJump L_00BD

L_00B7:
    Plugin12_Cmd1006 1, 1

L_00BD:
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DC
    Plugin12_Cmd1006 2, 0
    VMJump L_00E2

L_00DC:
    Plugin12_Cmd1006 2, 1

L_00E2:
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0101
    Plugin12_Cmd1006 3, 0
    VMJump L_0107

L_0101:
    Plugin12_Cmd1006 3, 1

L_0107:
    VMReturn

Script_9:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    ActorCmdExec 0, Movement_02A0
    ActorWalkRoute 255, 11, 12, 1, 8, 0
    ActorCmdWait
    // "Team Plasma: Who are you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate2_Text_TeamPlasmaWho, 2, 5, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_02B0
    ActorCmdWait
    // "[f000]Ā\u0001\u0001: Just to let you know...\nYou're about to feel my rage![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate2_Text_JustLetKnowYoure, 0, 6, 0
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0172
    CallTrainerMultiBattle 794, 798, 799, 0
    VMJump L_019F

L_0172:
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0195
    CallTrainerMultiBattle 795, 798, 799, 0
    VMJump L_019F

L_0195:
    CallTrainerMultiBattle 796, 798, 799, 0

L_019F:
    VMCall L_060F
    // "Team Plasma: Ugh!\nWe have to tell the others.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate2_Text_TeamPlasmaUghWe, 2, 5, 0
    MsgWinCloseAll
    // "Team Plasma: Oh no!\nWe have to protect the switches![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate2_Text_TeamPlasmaOhNo, 1, 3, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_06C0
    ActorCmdExec 1, Movement_06C8
    ActorCmdWait
    ActorCmdExec 2, Movement_02CC
    ActorCmdExec 1, Movement_02B8
    VMSleep 6
    ActorCmdExec 255, Movement_06B8
    ActorCmdExec 0, Movement_06B8
    ActorCmdWait
    ActorDelete 1
    ActorCmdExec 2, Movement_02DC
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 2
    SEWait
    // "[f000]Ā\u0001\u0001: They didn't have\nPurrloin with them![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate2_Text_TheyDidntHavePurrloin, 0, 6, 0
    MsgWinCloseAll
    ActorCmdExec 255, Movement_06B0
    ActorCmdExec 0, Movement_06B0
    ActorCmdWait
    // "Barriers, huh...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate2_Text_BarriersHuh, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_06C0
    ActorCmdExec 255, Movement_06C8
    ActorCmdWait
    // "The Team Plasma member was saying...[f000]븁\u0000\nOh, I got it. We should press the\nswitches to deactivate the barriers![f000]븁\u0000\n[f000]Ā\u0001\u0000! Let's split up and\nlook for them![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate2_Text_TeamPlasmaMemberSaying, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_02E4
    VMSleep 4
    ActorCmdExec 255, Movement_06B8
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    FlagSet EVENT_FLAG_0x0343
    WorkSetConst EVENT_WORK_0x40ff, 1
    WorkSetConst EVENT_WORK_0x4100, 1
    WorkSetConst EVENT_WORK_0x40f2, 3
    FlagSet EVENT_FLAG_0x033e
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_02A0:
    Move 12, 1
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_02B0:
    Move 36, 2
    MoveEnd

Movement_02B8:
    Move 18, 1
    Move 17, 3
    Move 19, 1
    Move 17, 2
    MoveEnd

Movement_02CC:
    Move 19, 1
    Move 17, 4
    Move 18, 2
    MoveEnd

Movement_02DC:
    Move 17, 1
    MoveEnd

Movement_02E4:
    Move 17, 2
    Move 18, 1
    Move 17, 2
    MoveEnd

Script_4:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f5, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "From behind the wall,\nyou heard the sound[f000]븀\u0000\nof a barrier being deactivated!"
    InfoMsg PlasmaFrigate2_Text_FromBehindWallHeard, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0350
    MsgWaitAdvance
    VMJump L_0352

L_0350:
    LastKeyWait

L_0352:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1007 0
    Plugin12_Cmd1008 0
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03AA
    // "All the barriers were deactivated,\nand now you can proceed!"
    InfoMsg PlasmaFrigate2_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_03AA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f6, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "From behind the wall,\nyou heard the sound[f000]븀\u0000\nof a barrier being deactivated!"
    InfoMsg PlasmaFrigate2_Text_FromBehindWallHeard, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_040C
    MsgWaitAdvance
    VMJump L_040E

L_040C:
    LastKeyWait

L_040E:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1007 1
    Plugin12_Cmd1008 1
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0466
    // "All the barriers were deactivated,\nand now you can proceed!"
    InfoMsg PlasmaFrigate2_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_0466:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f7, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "From behind the wall,\nyou heard the sound[f000]븀\u0000\nof a barrier being deactivated!"
    InfoMsg PlasmaFrigate2_Text_FromBehindWallHeard, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04C8
    MsgWaitAdvance
    VMJump L_04CA

L_04C8:
    LastKeyWait

L_04CA:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1007 2
    Plugin12_Cmd1008 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0522
    // "All the barriers were deactivated,\nand now you can proceed!"
    InfoMsg PlasmaFrigate2_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_0522:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst EVENT_WORK_0x40f8, 1
    SEPlay SEQ_SE_SW_PLAZMASHIP_04
    // "From behind the wall,\nyou heard the sound[f000]븀\u0000\nof a barrier being deactivated!"
    InfoMsg PlasmaFrigate2_Text_FromBehindWallHeard, 2
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0584
    MsgWaitAdvance
    VMJump L_0586

L_0584:
    LastKeyWait

L_0586:
    MsgWinCloseAll
    SEWait
    Plugin12_Cmd1007 3
    Plugin12_Cmd1008 3
    VMStackPush EVENT_WORK_0x40f5
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f6
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f8
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05DE
    // "All the barriers were deactivated,\nand now you can proceed!"
    InfoMsg PlasmaFrigate2_Text_AllBarriersWereDeactivated, 2
    LastKeyWait
    MsgWinCloseAll

L_05DE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    ActorCmdExec 255, Movement_0650
    ActorCmdWait
    SEPlay SEQ_SE_SW_PLAZMASHIP_08
    ActorCmdExec 255, Movement_065C
    ActorCmdWait
    SEWait
    // "Be careful!\nThe barriers are electrified!"
    InfoMsg PlasmaFrigate2_Text_CarefulBarriersElectrified, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_060F:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0649
    PokePartyGetCount 0x8008, 2
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0641
    PokePartyRecoverAll

L_0641:
    CallTrainerBattleEnd
    VMJump L_064B

L_0649:
    CallTrainerLose

L_064B:
    VMReturn
    .balign 4, 0

Movement_0650:
    Move 0, 1
    Move 75, 1
    MoveEnd

Movement_065C:
    Move 71, 1
    Move 36, 2
    Move 17, 2
    Move 72, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
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
    Move 3, 1
    MoveEnd

Movement_06B0:
    Move 32, 1
    MoveEnd

Movement_06B8:
    Move 33, 1
    MoveEnd

Movement_06C0:
    Move 34, 1
    MoveEnd

Movement_06C8:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
