// MOVE_GASTRO_ACID's animation: member 380 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant GastroAcid

GastroAcid:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_DEFENDER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 35, 150, 120, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 50, 150, 40, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 65, 150, 10, 0, 0, 0
    PlaySE SEQ_SE_W148_01, 2, EFFVM_DEFENDER, 80, 150, 3, 0, 0, 0
    SetYield TRUE
    ParticleLoad 559
    EmitterAdd 559, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 559, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 559, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 1, 3
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x3ca
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x3ca
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x3ca
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x3ca
    Wait EFFVM_WAIT_MCSS
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
