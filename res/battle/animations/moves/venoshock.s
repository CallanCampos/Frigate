// MOVE_VENOSHOCK's animation: member 474 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Venoshock

Venoshock:
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    ParticleLoad 651
    SetYield FALSE
    PlaySE SEQ_SE_W474_DOKU, 1, EFFVM_DEFENDER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W075_01, 1, EFFVM_DEFENDER, 35, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_W474_DOKU, 1, EFFVM_DEFENDER, 70, -300, 100, 0, 0, 0
    PlaySE SEQ_SE_W474_GUTSU, 2, EFFVM_DEFENDER, 0, -500, 30, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 120, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 0, 500, 0, 120, 0, 0
    MoveSE 2, 0, 1, 40, 0, 100, 20, 0, 0
    SetYield TRUE
    EmitterAdd 651, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 651, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 651, 2, EFFVM_EMIT_DEFENDER, 8, 0x333, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 651, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 651, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    WaitFrames 40
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 5
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
