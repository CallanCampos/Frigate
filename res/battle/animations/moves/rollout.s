// MOVE_ROLLOUT's animation: member 205 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Rollout

Rollout:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x1000, 0x1000, 3, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W022_02, 1, EFFVM_ATTACKER, 0, -400, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 2, EFFVM_ATTACKER, 0, 300, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 55, 0, 0
    SetYield TRUE
    ParticleLoad 375
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, Rollout_0196
    EmitterMove 375, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0x28000, 0, 0x1000, 0x1000, 0
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Rollout_01C4

Rollout_0196:
    EmitterMove 375, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0x28000, 0, 0x1000, 0x1000, 0

Rollout_01C4:
    WaitFrames 30
    EmitterAdd 375, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x19a, 0x19a, 3, 1, 4
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0x19a, 3, 1, 4
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 0, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
