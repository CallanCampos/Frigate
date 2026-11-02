// MOVE_ELECTROWEB's animation: member 527 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Electroweb

Electroweb:
    PaletteFade 2, 0, 8, 2, 0
    CameraMovePreset 1, 11, 16, 0, 8
    ParticleLoad 697
    EmitterAdd 697, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 697, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_FLD_80, 3, EFFVM_DEFENDER, 10, 500, 40, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 1, EFFVM_DEFENDER, 60, 200, 50, 0, 0, 0
    PlaySE SEQ_SE_W225_01, 2, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    WaitFrames 0
    EmitterAdd 697, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 697, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 697, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 2, 8, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
