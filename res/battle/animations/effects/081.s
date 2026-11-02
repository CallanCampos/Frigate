// Effect BTLV_EFFECT_FIRST + 81 (0x282): member 81 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect081

Effect081:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVar EFFVM_VAR_CAMERA_LOCKED, TRUE
    ParticleLoad 162
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, Effect081_00C2
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect081_00C2
    EmitterMoveFromOrtho 162, 0, 2, 0, -0x2000, 0x14000, EFFVM_EMIT_ATTACKER, 0x4000, 0x20000, 0x3000, 0x1000, 0x1000, 0x1000

Effect081_00C2:
    PlaySE SEQ_SE_SHOOTER, 5, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    WaitFrames 28
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect081_01BA
    McssPalFade EFFVM_ATTACKER, 0, 16, -1, 0x7fff
    EmitterAddOrtho 162, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 162, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    PlaySE SEQ_SE_W100_01, 5, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 16, 0, -1, 0x7fff
    Wait EFFVM_WAIT_ALL

Effect081_01BA:
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
