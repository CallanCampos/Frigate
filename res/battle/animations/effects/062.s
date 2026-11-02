// Effect BTLV_EFFECT_FIRST + 62 (0x26f): member 62 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect062

Effect062:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_01, 1, EFFVM_ATTACKER, 30, 0, 100, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ATTACKER, 0, 16, 0, 0
    McssAlpha EFFVM_ATTACKER, 1, 1, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 1, -0x8000, 0, 1, 0, 0
    Wait EFFVM_WAIT_ALL
    McssEffect 0, EFFVM_ATTACKER, 1
    McssAlpha EFFVM_ATTACKER, 1, 31, 1, 0, 0
    McssMoveXY EFFVM_ATTACKER, 1, 0x8000, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
