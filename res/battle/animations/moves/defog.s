// MOVE_DEFOG's animation: member 432 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Defog

Defog:
    McssSetValue EFFVM_ALL, 1
    PaletteFade 4, 0, 12, 2, 32767
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 2000, 0, 80, 0, 0
    MoveSE 1, 0, 1, 110, 0, 40, 40, 0, 0
    PlaySE SEQ_SE_W319_01, 2, EFFVM_ATTACKER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W319_01, 1, EFFVM_ATTACKER, 70, 0, 60, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_ALL, 0, 12, 2, 0x7fff
    ParticleLoad 612
    EmitterAdd 612, 0, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x99a, 0x2000
    EmitterAdd 612, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 612, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x3000
    EmitterAdd 612, 0, EFFVM_EMIT_DEFENDER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 612, 0, EFFVM_EMIT_DEFENDER, 8, 0x5000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 2, 32767
    McssPalFade EFFVM_ALL, 12, 0, 2, 0x7fff
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
