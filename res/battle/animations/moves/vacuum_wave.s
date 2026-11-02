// MOVE_VACUUM_WAVE's animation: member 410 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant VacuumWave

VacuumWave:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W307_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 30, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 0, 30, 0, 0
    MoveSE 1, 0, 1, 120, 0, 20, 10, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 30, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 589
    EmitterAdd 589, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 589, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 589, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 589, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x800, 0x1000, 0x333, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
