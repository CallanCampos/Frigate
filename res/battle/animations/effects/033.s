// Effect BTLV_EFFECT_FIRST + 33 (0x252): member 33 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect033

Effect033:
    SetYield FALSE
    CameraMovePreset 1, 13, 16, 0, 0
    McssScale EFFVM_ALL, 1, 0x1000, 0x1000, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    Gauge 1, 2
    End
