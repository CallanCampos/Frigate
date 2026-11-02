// Effect BTLV_EFFECT_FIRST + 10 (0x23b): member 10 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect010

Effect010:
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_NE, 2, Effect010_007E
    CameraMove 1, 0x8b33, 0x7b33, 0x17ccd, 0x2000, 0x399a, 0x6800, 10, 0, 3
    Wait EFFVM_WAIT_ALL

Effect010_007E:
    PlayCry EFFVM_ATTACKER, -5120, 0, 0, 0, 0, 0
    Wait EFFVM_WAIT_CRY
    PlaySE SEQ_SE_DEAD, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield FALSE
    McssCameraMode 1, 1
    McssBlink EFFVM_ATTACKER, 3, 0, 0
    McssPalFade EFFVM_ATTACKER, 0, 16, 0, 0
    McssAlpha EFFVM_ATTACKER, 1, 1, 16, 0, 0
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0xa000, 32, 0, 0
    McssSetValue EFFVM_ATTACKER, 1
    Wait EFFVM_WAIT_ALL
    GaugeHide EFFVM_ATTACKER
    IfVarConst EFFVM_VAR_BATTLE_STYLE, EFFVM_CMP_NE, 2, Effect010_017C
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL

Effect010_017C:
    End
    .balign 4
