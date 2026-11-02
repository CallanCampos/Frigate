// MOVE_SCARY_FACE's animation: member 184 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ScaryFace

ScaryFace:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 354
    PaletteFade 4, 0, 12, 1, 0
    Wait EFFVM_WAIT_PARTICLES
    SetYield FALSE
    PlaySE SEQ_SE_W116_01, 1, EFFVM_ATTACKER, 15, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W319_01, 2, EFFVM_ATTACKER, 40, -200, 100, 0, 0, 0
    MoveSE 1, 0, 0, -3000, 1000, 15, 50, 0, 0
    SetYield TRUE
    EmitterAdd 354, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0, 0x4cd, 10, 1, 1
    WaitFrames 60
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
