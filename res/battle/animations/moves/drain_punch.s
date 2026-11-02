// MOVE_DRAIN_PUNCH's animation: member 409 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DrainPunch

DrainPunch:
    PaletteFade 4, 0, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, -1000, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W295_01, 2, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 20, 30, 0, 0
    MoveSE 1, 0, 1, 120, 0, 70, 30, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 10, 48, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 10, 48, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 11, 10, 0, 6
    ParticleLoad 588
    EmitterAdd 588, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 588, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 588, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 588, 1, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x1000, 0, 0, 0x1000, 0xccd, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 4, 1, 1
    WaitFrames 30
    CameraMovePreset 1, 9, 10, 0, 6
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 8, 10, 0, 6
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 10, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
