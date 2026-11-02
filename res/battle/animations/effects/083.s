// Effect BTLV_EFFECT_FIRST + 83 (0x284): member 83 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect083

Effect083:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 1
    McssMoveXY EFFVM_ATTACKER, 1, -0x1e000, 0xa000, 60, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
