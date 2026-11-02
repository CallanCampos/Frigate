// MOVE_FIRE_PUNCH's animation: member 7 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FirePunch

FirePunch:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 12, 2, 31
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 0, 10, 0, 0x1f
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 0, -300, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W307_02, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 171
    EmitterAdd 171, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 171, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 171, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 0, 12
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_PARTICLES
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssPalFade EFFVM_DEFENDER, 10, 0, 0, 0x1f
    PaletteFade 4, 12, 0, 2, 31
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
