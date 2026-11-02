// MOVE_SAFEGUARD's animation: member 219 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Safeguard

Safeguard:
    CameraMovePreset 1, 21, 16, 0, 8
    Wait EFFVM_WAIT_CAMERA
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 2, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 390
    EmitterAdd 390, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 390, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 0, 10, 3, 0x7fff
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 1, 1, 4
    McssScale EFFVM_ATTACKER, 2, 0x19a, -0x19a, 10, 1, 1
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 10, 0, 3, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
