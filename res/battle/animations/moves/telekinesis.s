// MOVE_TELEKINESIS's animation: member 477 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Telekinesis

Telekinesis:
    PaletteFade 4, 0, 10, 2, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 12, 3, 0x7fff
    ParticleLoad 367
    SetYield FALSE
    PlaySE SEQ_SE_W006_04, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 367, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 367, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 20
    PaletteFade 4, 10, 0, 2, 0
    CameraMovePreset 1, 11, 10, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W213_01, 1, EFFVM_DEFENDER, 0, -1000, 100, 0, 0, 0
    MoveSE 1, 0, 1, 100, 10, 80, 10, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 90, 0, 0, 0, 0, 0
    SetYield TRUE
    McssSetValue EFFVM_DEFENDER, 1
    McssRotate EFFVM_DEFENDER, 3, 0x71c6ab, 10, 0, 2
    McssMoveXY EFFVM_DEFENDER, 2, 0x1000, 0x2000, 20, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_ATTACKER, 12, 0, 3, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_DEFENDER, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
