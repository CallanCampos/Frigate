// MOVE_COVET's animation: member 343 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Covet
    ScriptVariant Covet_Variant1

Covet:
    SetYield FALSE
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 0, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 5, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 10, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 15, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 30, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 35, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 40, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 45, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 45, 600, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 9, 10, 0, 6
    McssRotate EFFVM_ATTACKER, 3, 0xaaaa00, 4, 1, 2
    ParticleLoad 519
    EmitterAdd 519, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 40
    CameraMovePreset 1, 11, 10, 0, 6
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 1, 7
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 10, 0, 6
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End

Covet_Variant1:
    SetYield FALSE
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 0, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 5, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 10, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 15, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 30, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 35, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 40, 100, 110, 0, 0, 0
    PlaySE SEQ_SE_W331_01, 1, EFFVM_ATTACKER, 45, 400, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 45, 600, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 9, 10, 0, 6
    McssRotate EFFVM_ATTACKER, 3, 0xaaaa00, 4, 1, 2
    ParticleLoad 520
    EmitterAdd 520, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 40
    CameraMovePreset 1, 11, 10, 0, 6
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 1, 7
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 4, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W343_HOSHIGARI, 1, EFFVM_PAN_CENTER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W014_01, 2, EFFVM_PAN_CENTER, 0, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 30, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 30, 0, 0
    SetYield TRUE
    EmitterMove 520, 0, 2, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x2000, 0x2e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 6
    CameraMovePreset 1, 9, 10, 0, 6
    WaitFrames 40
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 10, 0, 6
    Wait EFFVM_WAIT_ALL
    End
