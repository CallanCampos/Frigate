// MOVE_SHOCK_WAVE's animation: member 351 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ShockWave

ShockWave:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 528
    EmitterAdd 528, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 528, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 528, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 528, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 528, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 12, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 40, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 120, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 2000, 0, 80, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_ATTACKER, 70, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_ATTACKER, 90, 0, 115, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_FLD_34, 1, EFFVM_ATTACKER, 130, 200, 115, 0, 0, 0
    PlaySE SEQ_SE_FLD_33, 2, EFFVM_ATTACKER, 130, 300, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 125, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 125, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W077_02, 2, EFFVM_DEFENDER, 175, -200, 115, 0, 0, 0
    SetYield TRUE
    WaitFrames 150
    CameraMovePreset 1, 11, 16, 0, 12
    WaitFrames 20
    EmitterAdd 528, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x3ff
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 8
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x3ff
    PaletteFade 4, 12, 0, 1, 0
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
