// MOVE_ACID_SPRAY's animation: member 491 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AcidSpray

AcidSpray:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 665
    EmitterMove 665, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0x3000, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W014_01, 2, EFFVM_DEFENDER, 0, 500, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    WaitFrames 1
    EmitterMove 665, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0x3000, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_NAGERU, 1, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    WaitFrames 1
    CameraMovePreset 1, 11, 24, 0, 8
    EmitterMove 665, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0x3000, 0x1000, 0x1000, 0
    WaitFrames 1
    EmitterMove 665, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0x3000, 0x1000, 0x1000, 0
    McssSetValue EFFVM_ALL, 1
    WaitFrames 20
    EmitterAdd 665, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 665, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 665, 6, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    WaitFrames 4
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x4cd, 8, 0, 1
    EmitterAdd 665, 0, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 665, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W028_01, 1, EFFVM_DEFENDER, 0, -500, 70, 0, 0, 0
    SetYield TRUE
    EmitterAdd 665, 1, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
