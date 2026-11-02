// MOVE_METAL_BURST's animation: member 368 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MetalBurst

MetalBurst:
    CameraMovePreset 1, 14, 24, 0, 0
    PaletteFade 4, 0, 13, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W321_01, 2, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    MoveSE 2, 0, 0, 0, 500, 15, 20, 0, 0
    PlaySE SEQ_SE_W368_01, 1, EFFVM_ATTACKER, 40, 300, 120, 0, 0, 0
    SetYield TRUE
    PlaySE SEQ_SE_FLD_94, 1, EFFVM_DEFENDER, 50, -200, 110, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 50, -500, 127, 0, 0, 0
    ParticleLoad 546
    EmitterAddOrtho 546, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 546, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 546, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 40
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 6
    EmitterAddOrtho 546, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    PaletteFade 4, 13, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
