// MOVE_DISABLE's animation: member 50 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Disable

Disable:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 12, 4, 21140
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W319_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 215
    EmitterAdd 215, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 215, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W050_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 3, 0
    McssScale EFFVM_DEFENDER, 2, 0x4cd, 0x4cd, 10, 1, 1
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 3, 0
    PaletteFade 4, 12, 0, 4, 21140
    Wait EFFVM_WAIT_ALL
    End
