// MOVE_VOLT_SWITCH's animation: member 521 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant VoltSwitch

VoltSwitch:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 2, 0, 12, 4, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_SHINKA_W025, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 2, EFFVM_ATTACKER, 0, 500, 127, 0, 0, 0
    PlaySE SEQ_SE_W028_02, 2, EFFVM_ATTACKER, 70, 0, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 70, 15, 0, 0
    SetYield TRUE
    ParticleLoad 692
    EmitterAdd 692, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 692, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 692, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 60
    EmitterMove 692, 4, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    EmitterMove 692, 5, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    EmitterAdd 692, 6, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 692, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 692, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 3, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    WaitFrames 20
    CameraMovePreset 1, 8, 10, 0, 8
    WaitFrames 10
    PaletteFade 2, 12, 0, 4, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
