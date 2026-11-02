// MOVE_LICK's animation: member 122 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Lick

Lick:
    CameraMovePreset 1, 11, 14, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 291
    EmitterAdd 291, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 291, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 9, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W122_01, 1, EFFVM_DEFENDER, 0, 400, 120, 127, 100, 0
    PlaySE SEQ_SE_W122_02, 2, EFFVM_DEFENDER, 0, 0, 50, 0, 0, 0
    SetYield TRUE
    WaitFrames 8
    McssMoveXY EFFVM_DEFENDER, 2, 0x19a, 0, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 14, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
