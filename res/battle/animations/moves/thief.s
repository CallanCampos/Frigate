// MOVE_THIEF's animation: member 168 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Thief
    ScriptVariant Thief_Variant1

Thief:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W028_02, 1, EFFVM_ATTACKER, 0, 1000, 110, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 6, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 16
    ParticleLoad 338
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x5294
    EmitterAdd 338, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 338, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 338, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W123_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x5294
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

Thief_Variant1:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W028_02, 1, EFFVM_ATTACKER, 0, 1000, 110, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 6, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 16
    ParticleLoad 339
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x5294
    EmitterAdd 339, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 339, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 339, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W123_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x5294
    McssSetVanish EFFVM_DEFENDER, FALSE
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_EQ, 1, Thief_044A
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_EQ, 2, Thief_044A
    CameraMovePreset 1, 8, 16, 0, 8
    Jump Thief_0460

Thief_044A:
    CameraMovePreset 1, 20, 16, 0, 8

Thief_0460:
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 1, EFFVM_PAN_CENTER, 0, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 50, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterMove 339, 1, 2, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x2000, 0x2e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 40
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 0, 1
    WaitFrames 30
    CameraMovePreset 1, 8, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
