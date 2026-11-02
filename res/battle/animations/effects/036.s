// Effect BTLV_EFFECT_FIRST + 36 (0x255): member 36 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect036

Effect036:
    SetYield FALSE
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
