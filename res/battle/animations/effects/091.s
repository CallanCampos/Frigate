// Effect BTLV_EFFECT_FIRST + 91 (0x28c): member 91 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect091

Effect091:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 3, 0, 4
    SetYield FALSE
    PlaySE SEQ_SE_W028_02, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 1, -0x18000, 0, 10, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetMode EFFVM_ATTACKER, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
