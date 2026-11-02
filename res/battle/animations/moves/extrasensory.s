// MOVE_EXTRASENSORY's animation: member 326 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Extrasensory

Extrasensory:
    CameraMovePreset 1, 14, 24, 0, 0
    PaletteFade 4, 0, 12, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 30, 30, 0, 0
    PlaySE SEQ_SE_W103_01, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 500
    EmitterAddOrtho 500, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W253_02, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_05, 2, EFFVM_ATTACKER, 60, 500, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 0, 10
    Wait EFFVM_WAIT_ALL
    WaitFrames 20
    McssScale EFFVM_ATTACKER, 2, 0x333, 0x333, 1, 0, 20
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
