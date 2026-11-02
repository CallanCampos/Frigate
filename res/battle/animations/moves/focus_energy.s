// MOVE_FOCUS_ENERGY's animation: member 116 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FocusEnergy

FocusEnergy:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 286
    EmitterAdd 286, 0, EFFVM_EMIT_ATTACKER, 8, 0x1800, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 286, 1, EFFVM_EMIT_ATTACKER, 8, 0x1800, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W116_01, 1, EFFVM_ATTACKER, 0, -100, 120, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 2000, 0, 50, 0, 0
    MoveSE 3, 0, 0, -2000, 2000, 0, 50, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
