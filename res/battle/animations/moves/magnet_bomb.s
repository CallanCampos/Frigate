// MOVE_MAGNET_BOMB's animation: member 443 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MagnetBomb

MagnetBomb:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W122_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W054_01, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 60, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 65, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 70, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 75, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 80, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 85, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 90, 1000, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 95, 0, 115, 0, 0, 0
    SetYield TRUE
    ParticleLoad 622
    EmitterAdd 622, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 622, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 622, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 7
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
