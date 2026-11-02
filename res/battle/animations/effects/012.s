// Effect BTLV_EFFECT_FIRST + 12 (0x23d): member 12 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect012

Effect012:
    ParticleLoad 48
    EmitterMoveFromOrtho 48, 0, 2, 0, -0x2000, 0xa000, 1, 0x2666, 0x23000, 0x5000, 0x1000, 0x1000, 0xc00
    PlaySE SEQ_SE_NAGERU, 1, EFFVM_PAN_LEFT, 0, 0, 127, 0, 0, 0
    PanSE 1, 0, 0, 1, 0, 35, 0, 0
    WaitFrames 27
    PlaySE SEQ_SE_KON, 1, EFFVM_PAN_RIGHT, 0, 0, 127, 0, 0, 0
    SetYield FALSE
    ParticleDelete 48
    ParticleLoad 48
    EmitterMoveFromOrtho 48, 0, 1, 0x1e, 0x4d2c, -0x8c19, 2, 0, 0x10000, 0, 0x1000, 0x1000, 0xc00
    WaitFrames 16
    End
