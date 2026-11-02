// MOVE_RAGE's animation: member 99 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Rage

Rage:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 10, 3, 0x1f
    McssPalFade EFFVM_ATTACKER, 10, 0, 3, 0x1f
    SetYield FALSE
    PlaySE SEQ_SE_W099_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 55, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -4000, 0, 0, 55, 0, 0
    MoveSE 2, 0, 1, 120, 0, 40, 10, 0, 0
    PlaySE SEQ_SE_W099_02, 1, EFFVM_ATTACKER, 0, 0, 80, 127, 50, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 55, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W015_01, 1, EFFVM_ATTACKER, 60, -300, 127, 0, 0, 0
    MoveSE 1, 0, 0, -4000, 0, 0, 55, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 10, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_PARTICLES
    McssPalFade EFFVM_ATTACKER, 0, 10, 3, 0x1f
    McssPalFade EFFVM_ATTACKER, 10, 0, 3, 0x1f
    Wait EFFVM_WAIT_ALL
    McssCircle EFFVM_ATTACKER, 2, 0, 0, 0x3000, 0xc000, 0x1000, 0x1000, 0
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 3, 1, 1
    WaitFrames 10
    CameraMovePreset 1, 11, 16, 0, 8
    PlaySE SEQ_SE_W307_03, 5, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    WaitFrames 10
    ParticleLoad 271
    EmitterAdd 271, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 271, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
