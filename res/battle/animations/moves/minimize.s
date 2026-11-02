// MOVE_MINIMIZE's animation: member 107 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Minimize

Minimize:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 1
    SetYield FALSE
    PlaySE SEQ_SE_W075_05, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W075_05, 1, EFFVM_ATTACKER, 40, 0, 120, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0x800, -0x800, 8, 1, 2
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    MoveSE 1, 0, 0, 0, -2000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 120, 0, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_ATTACKER, 0, 50, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 2, -0xe66, -0xe66, 16, 1, 1
    WaitFrames 32
    McssSetValue EFFVM_ATTACKER, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
