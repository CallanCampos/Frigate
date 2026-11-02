// MOVE_PRESENT's animation: member 217 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Present
    ScriptVariant Present_Variant1

Present:
    CameraMovePreset 1, 11, 13, 0, 8
    ParticleLoad 387
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W215_02, 2, EFFVM_DEFENDER, 30, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 30, 100, 115, 0, 0, 0
    SetYield FALSE
    EmitterMove 387, 3, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0x3000, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    EmitterAdd 387, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 387, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 387, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 2, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 13, 0, 8
    Wait EFFVM_WAIT_ALL
    End

Present_Variant1:
    CameraMovePreset 1, 11, 13, 0, 8
    ParticleLoad 388
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 2, EFFVM_DEFENDER, 40, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_DEFENDER, 30, 100, 115, 0, 0, 0
    SetYield FALSE
    EmitterMove 388, 3, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0x3000, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    EmitterAdd 388, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 388, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 388, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x3e0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x3e0
    CameraMovePreset 1, 8, 13, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
