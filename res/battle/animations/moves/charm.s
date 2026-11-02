// MOVE_CHARM's animation: member 204 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Charm

Charm:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_CAMERA
    SetYield FALSE
    PlaySE SEQ_SE_W142_01, 0, EFFVM_ATTACKER, 30, 500, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 1000, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 20, 1000, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 40, 1000, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 60, 1000, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 80, 1000, 110, 0, 0, 0
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 100, 1000, 110, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    Wait EFFVM_WAIT_MCSS
    ParticleLoad 374
    EmitterAdd 374, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssRotate EFFVM_ATTACKER, 3, 0xaaaa00, 6, 1, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
