#include "asm/field_script.inc"
#include "text/script/liberty_garden_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    PokePartyGetCount 0x8020, 0

L_002A:
    VMStackPush 0x8020
    VMStackPush 0x8022
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_006C
    PokePartyGetSpecies 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 494
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005C
    WorkSetConst EVENT_WORK_0x400a, 1

L_005C:
    DebugPrint 0x8021
    WorkAdd 0x8022, 1
    VMJump L_002A

L_006C:
    VMHalt

Script_2:
    VMHalt

Script_3:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    // "Victini seems to want to get out\nof the Poké Ball...[f000]븀\u0000\nWill you let it out?[f000]븁\u0000"
    InfoMsg LibertyGarden3_Text_VictiniSeemsWantGet, 1
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019F
    MsgWinCloseAll
    ActorWalkRoute 255, 4, 7, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 255, Movement_0298
    ActorCmdWait
    ActorNew 4, 8, 1, 251, 125, 0
    PVPlay 494, 0
    // "Ta-ta-ta-tah!"
    ActorMsg MSGFILE_SCRIPT, LibertyGarden3_Text_TaTaTaTah, 251, 0, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 251, 4, 4, 0, 8, 0
    VMSleep 20
    ActorCmdExec 255, Movement_0290
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x48000, 0, 0x48000, 30
    EvCameraWait
    ActorCmdWait
    ActorCmdExec 251, Movement_01B4
    ActorCmdWait
    ActorCmdExec 251, Movement_01C8
    ActorCmdWait
    PVPlay 494, 0
    PVWait
    ActorCmdExec 251, Movement_01DC
    ActorCmdWait
    ActorCmdExec 251, Movement_01F0
    ActorCmdWait
    PVPlay 494, 0
    // "Ta-ta-ta-tah!"
    ActorMsg MSGFILE_SCRIPT, LibertyGarden3_Text_TaTaTaTah, 251, 0, 0
    MsgWaitAdvance
    PVWait
    MsgWinCloseAll
    ActorWalkRoute 251, 4, 6, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 251, Movement_02B8
    ActorCmdWait
    PVPlay 494, 0
    PVWait
    ActorDelete 251
    // "Victini has returned to its Poké Ball\nwith an air of satisfaction."
    InfoMsg LibertyGarden3_Text_VictiniHasReturnedIts, 1
    LastKeyWait
    MsgWinCloseAll
    EvCameraReturn 20
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    WorkSetConst EVENT_WORK_0x400a, 0
    VMJump L_01AE

L_019F:
    // "Victini seems lonely\nin the Poké Ball..."
    InfoMsg LibertyGarden3_Text_VictiniSeemsLonelyPoke, 1
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x400a, 0

L_01AE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_01B4:
    Move 35, 1
    Move 34, 1
    Move 33, 1
    Move 160, 1
    MoveEnd

Movement_01C8:
    Move 15, 1
    Move 12, 1
    Move 35, 1
    Move 51, 2
    MoveEnd

Movement_01DC:
    Move 18, 3
    Move 32, 1
    Move 48, 2
    Move 160, 1
    MoveEnd

Movement_01F0:
    Move 17, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 19, 3
    Move 34, 1
    Move 35, 1
    Move 32, 1
    Move 33, 1
    Move 160, 1
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

Movement_0290:
    Move 32, 1
    MoveEnd

Movement_0298:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 48, 1
    MoveEnd

Movement_02B8:
    Move 49, 1
    MoveEnd
    LastKeyWait
    VMNop2
    PokePartyGetSpecies 0, 51
    VMNop2
    PokePartyGetSpecies 0, 49
    VMHalt
    .byte 0xfe
    .balign 4, 0
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
    Move 71, 1
    Move 170, 1
    Move 72, 1
    MoveEnd
