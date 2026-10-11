#include "asm/field_script.inc"
#include "text/script/village_bridge_9.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x98000, 0, 0x98000, 30
    EvCameraWait
    MultiMsg 0, 8, 6, 1
    VMSleep 60
    MsgWinCloseNo 1
    MultiMsg 1, 18, 13, 2
    VMSleep 60
    MsgWinCloseNo 2
    VMSleep 30
    ActorCmdExec 0, Movement_00E4
    ActorCmdWait
    VMSleep 30
    ActorCmdExec 0, Movement_00FC
    ActorCmdWait
    // "Eeek![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VillageBridge9_Text_Eeek, 0, 1, 0
    MsgWinCloseAll
    EvCameraReturn 20
    ActorWalkRoute 0, 14, 9, 1, 4, 0
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 0, Movement_00EC
    ActorCmdWait
    // "I'm practicing.\nGet out!![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, VillageBridge9_Text_ImPracticingGetOut, 0, 1, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_00F4
    ActorCmdExec 255, Movement_00F4
    ActorCmdWait
    MapChangeWarp ZONE_VILLAGE_BRIDGE, 31, 39, 3
    FlagSet EVENT_FLAG_0x01d2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0

Movement_00E4:
    Move 3, 1
    MoveEnd

Movement_00EC:
    Move 51, 2
    MoveEnd

Movement_00F4:
    Move 15, 1
    MoveEnd

Movement_00FC:
    Move 75, 1
    MoveEnd
