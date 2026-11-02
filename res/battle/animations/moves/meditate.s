// MOVE_MEDITATE's animation: member 96 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Meditate

Meditate:
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 10, 1, 0
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W096_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, 0xccd, -0x666, 6, 1, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W096_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0x333, 0x666, 6, 1, 1
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 4, 1, 1
    PaletteFade 4, 10, 0, 1, 0
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
