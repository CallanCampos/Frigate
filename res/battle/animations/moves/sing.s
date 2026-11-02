// MOVE_SING's animation: member 47 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Sing

Sing:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 10, 2, 32767
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W047_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 110, 0, 0
    SetYield TRUE
    ParticleLoad 212
    EmitterAdd 212, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 212, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterMove 212, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 60
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 6, 2, 0x3c1f
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 6, 0, 2, 0x3c1f
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 0, 6, 2, 0x3c1f
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_DEFENDER, 6, 0, 2, 0x3c1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 10, 0, 2, 32767
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
