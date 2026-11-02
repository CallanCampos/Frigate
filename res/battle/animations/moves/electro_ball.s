// MOVE_ELECTRO_BALL's animation: member 486 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ElectroBall

ElectroBall:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 12, 1, 0
    ParticleLoad 661
    EmitterAdd 661, 2, EFFVM_EMIT_ATTACKER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 661, 5, EFFVM_EMIT_ATTACKER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 661, 1, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_03, 1, EFFVM_ATTACKER, 70, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 70, 30, 0, 0
    SetYield TRUE
    WaitFrames 70
    EmitterMove 661, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    EmitterMove 661, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 661, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 3, 0, 16
    SetYield FALSE
    PlaySE SEQ_SE_W255_BU, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 1, EFFVM_DEFENDER, 30, 0, 100, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 12, 0, 1, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
