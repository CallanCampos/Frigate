// MOVE_FLAME_CHARGE's animation: member 488 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FlameCharge

FlameCharge:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W464_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_032, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W464_01, 1, EFFVM_ATTACKER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_032, 2, EFFVM_ATTACKER, 50, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_032, 1, EFFVM_ATTACKER, 100, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W102_02, 2, EFFVM_ATTACKER, 130, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 663
    EmitterAdd 663, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1333, 0x800
    EmitterAdd 663, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1333, 0x800
    EmitterAdd 663, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1333, 0x800
    McssPalFade EFFVM_ATTACKER, 0, 8, 2, 0x1f
    WaitFrames 120
    McssCircle EFFVM_ATTACKER, 2, 1, 0, 0x4000, 0x12000, 0, 0x1000, 0
    Wait EFFVM_WAIT_MCSS
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W310_01, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 663, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 663, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 3
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 8, 0, 2, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
