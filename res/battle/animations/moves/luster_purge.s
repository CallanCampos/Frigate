// MOVE_LUSTER_PURGE's animation: member 295 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LusterPurge

LusterPurge:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W295_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W295_02, 2, EFFVM_ATTACKER, 70, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 465
    EmitterAddOrtho 465, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 465, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 465, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 465, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetValue EFFVM_ALL, 1
    McssPalFade EFFVM_ATTACKER, 0, 12, 3, 0x7fff
    McssPalFade EFFVM_DEFENDER, 0, 12, 3, 0x7fff
    PaletteFade 4, 0, 12, 3, 32767
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 0, 800, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 10, 800, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 20, 800, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 30, 800, 115, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 40, 800, 115, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0xccd, 0, 3, 0, 10
    EmitterAddOrtho 465, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 3, 0x7fff
    McssPalFade EFFVM_DEFENDER, 12, 0, 3, 0x7fff
    PaletteFade 4, 12, 0, 3, 32767
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
