// MOVE_ATTRACT's animation: member 213 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Attract

Attract:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W213_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 0, 0, 250, 5, 20, 0, 0
    MoveSE 1, 0, 1, 90, 0, 20, 25, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 73, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W377_01, 2, EFFVM_DEFENDER, 0, 100, 127, 0, 0, 0
    MoveSE 2, 0, 1, 40, 0, 5, 40, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_ALL
    McssRotate EFFVM_ATTACKER, 3, 0xaaaa00, 3, 1, 3
    ParticleLoad 383
    EmitterAddOrtho 383, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 383, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x4019
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x4019
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
