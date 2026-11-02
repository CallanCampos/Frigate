// MOVE_LEER's animation: member 43 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Leer

Leer:
    PaletteFade 4, 0, 12, 1, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 208
    EmitterAdd 208, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 208, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 208, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W043_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_ATTACKER, 15, 0, 50, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_SYS_58, 1, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 6
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 10, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
