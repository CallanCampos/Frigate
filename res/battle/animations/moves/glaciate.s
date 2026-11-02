// MOVE_GLACIATE's animation: member 549 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Glaciate

Glaciate:
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 1, EFFVM_ATTACKER, 20, -300, 127, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 722
    EmitterAdd 722, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 30, 0, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 1, EFFVM_DEFENDER, 30, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_108, 2, EFFVM_DEFENDER, 40, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 100, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 170, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 1, EFFVM_DEFENDER, 90, -100, 70, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 1, EFFVM_DEFENDER, 200, -300, 50, 0, 0, 0
    SetYield TRUE
    PaletteFade 2, 0, 8, 2, 0
    WaitFrames 30
    EmitterAdd 722, 6, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 722, 7, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 722, 4, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1b33, 0x1000, 0x1000
    WaitFrames 40
    McssPalFade EFFVM_DEFENDER, 0, 8, 2, 0x7fff
    EmitterAdd 722, 2, EFFVM_EMIT_DEFENDER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAdd 722, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 722, 2, EFFVM_EMIT_DEFENDER, 8, 0x2800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 8, 0, 2, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
