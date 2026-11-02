// MOVE_ME_FIRST's animation: member 382 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MeFirst

MeFirst:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W116_01, 1, EFFVM_DEFENDER, 0, -800, 110, 0, 0, 0
    PlaySE SEQ_SE_W295_01, 2, EFFVM_DEFENDER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W319_01, 1, EFFVM_DEFENDER, 50, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 561
    EmitterAdd 561, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 561, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W319_02, 2, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 48, 0, 0
    SetYield TRUE
    EmitterMove 561, 1, 1, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    EmitterAdd 561, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 6, 0, 0x3c80
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 6, 0, 0, 0x3d40
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
