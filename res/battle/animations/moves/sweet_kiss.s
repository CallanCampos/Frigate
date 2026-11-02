// MOVE_SWEET_KISS's animation: member 186 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SweetKiss

SweetKiss:
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W142_01, 2, EFFVM_DEFENDER, 0, 800, 125, 0, 0, 0
    PlaySE SEQ_SE_W142_01, 1, EFFVM_DEFENDER, 20, 800, 50, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_DEFENDER, 100, 0, 110, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 356
    EmitterAdd 356, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 356, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
