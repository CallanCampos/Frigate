// MOVE_CONVERSION_2's animation: member 176 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Conversion2

Conversion2:
    ParticleLoad 347
    EmitterAdd 347, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 50, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W028_02, 1, EFFVM_DEFENDER, 50, 0, 80, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 45, 30, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 45, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W113_02, 2, EFFVM_DEFENDER, 110, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W123_01, 1, EFFVM_DEFENDER, 120, 0, 100, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 11, 16, 0, 2
    EmitterMove 347, 1, 1, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 60
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 20
    EmitterAdd 347, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
