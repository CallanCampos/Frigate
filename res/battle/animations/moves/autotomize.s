// MOVE_AUTOTOMIZE's animation: member 475 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Autotomize

Autotomize:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 10, 1, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_W075_01, 1, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_11, 2, EFFVM_ATTACKER, 0, 500, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_11, 2, EFFVM_ATTACKER, 20, 500, 127, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0, 3, 0, 6
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 30, 0, 1
    ParticleLoad 652
    EmitterAdd 652, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 652, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 1, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
