// MOVE_LOVELY_KISS's animation: member 142 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LovelyKiss

LovelyKiss:
    CameraMovePreset 1, 11, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 1, EFFVM_ATTACKER, 0, -500, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 15, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W142_01, 2, EFFVM_DEFENDER, 10, 500, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 311
    EmitterAdd 311, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 311, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
