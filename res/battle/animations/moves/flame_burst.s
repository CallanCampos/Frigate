// MOVE_FLAME_BURST's animation: member 481 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FlameBurst

FlameBurst:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 656
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 1, EFFVM_DEFENDER, 0, 300, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    EmitterAdd 656, 3, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x800, 0x1000, 0x1000
    WaitFrames 12
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 656, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 656, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 656, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 4
    SetYield FALSE
    PlaySE SEQ_SE_W310_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_67, 1, EFFVM_DEFENDER, 0, 0, 70, 0, 0, 0
    MoveSE 1, 0, 1, 70, 0, 30, 20, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 8, 1, 0x1f
    McssScale EFFVM_DEFENDER, 2, -0x19a, 0x4cd, 6, 0, 1
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 8, 0, 1, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
