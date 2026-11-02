// MOVE_THUNDER_WAVE's animation: member 86 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ThunderWave

ThunderWave:
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 14, 3, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 255
    EmitterAdd 255, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 255, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 4
    EmitterAdd 255, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 4
    EmitterAdd 255, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 255, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 12, 3, 0x3ff
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 6
    McssScale EFFVM_DEFENDER, 2, 0xcd, -0xcd, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 14, 0, 3, 0
    McssPalFade EFFVM_DEFENDER, 12, 0, 3, 0x3ff
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
