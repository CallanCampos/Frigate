// Effect BTLV_EFFECT_FIRST + 61 (0x26e): member 61 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect061

Effect061:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 1, EFFVM_ATTACKER, 0, -800, 120, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 25, 0, 50, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 1, 0, 0x1000, 8, 0, 0
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0x8000, 1, 0, 0
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 0, 0x1000, 0x1000, 0, 0, 0
    McssEffect 1, EFFVM_ATTACKER, 1
    McssSetValue EFFVM_ATTACKER, 1
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x8000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
