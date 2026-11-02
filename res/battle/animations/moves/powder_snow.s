// MOVE_POWDER_SNOW's animation: member 181 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PowderSnow

PowderSnow:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W077_01, 1, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_DEFENDER, 0, 0, 80, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    PaletteFade 4, 0, 16, 1, 0
    ParticleLoad 351
    EmitterMove 351, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 14, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 95, -40, 120, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 6, 3, 0x7fff
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 351, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 16, 0, 1, 0
    McssPalFade EFFVM_DEFENDER, 6, 0, 3, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
