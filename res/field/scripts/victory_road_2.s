#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 6043, 65423, 0x158000, 0x2026c1, 0x196000, 0xfff53c61, 56
    ActorCmdExec 255, Movement_0050
    VMSleep 48
    FadeOutBlack
    FadeWait
    ActorCmdWait
    EvCameraWait
    EvCameraEnd
    RTReserveScript 7
    MapChangeCore ZONE_POKEMON_LEAGUE, 32, 0, 60, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0050:
    Move 12, 8
    MoveEnd
