// MOVE_SNORE's animation: member 173 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Snore

Snore:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W095_02, 1, EFFVM_ATTACKER, 0, -600, 125, 0, 0, 0
    PlaySE SEQ_SE_W095_02, 1, EFFVM_ATTACKER, 60, -600, 125, 0, 0, 0
    SetYield TRUE
    ParticleLoad 343
    EmitterAddOrtho 343, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 343, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    CameraShake 1, 0x333, 0, 2, 0, 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 1
    WaitFrames 30
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
