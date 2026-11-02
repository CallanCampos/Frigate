// MOVE_DRAGON_RUSH's animation: member 407 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant DragonRush
    ScriptVariant DragonRush_Variant1

DragonRush:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W017_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 586
    EmitterAdd 586, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 586, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 586, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 586, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 586, 4, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraShake 0, 0x333, 0, 2, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

DragonRush_Variant1:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 3, 0, 4
    McssMoveXY EFFVM_DEFENDER, 1, -0x18000, 0, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode EFFVM_DEFENDER, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
