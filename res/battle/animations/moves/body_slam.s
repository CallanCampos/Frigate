// MOVE_BODY_SLAM's animation: member 34 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BodySlam

BodySlam:
    McssScale EFFVM_ATTACKER, 2, 0x19a, 0x4cd, 4, 2, 1
    McssCircle EFFVM_ATTACKER, 2, 0, 0, 0x4000, 0xa000, 0, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W307_02, 2, EFFVM_ATTACKER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W099_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 0, 70, 0, 0
    MoveSE 1, 0, 1, 115, 0, 50, 20, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 50, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 10
    ParticleLoad 200
    EmitterAdd 200, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 200, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 200, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W034_01, 2, EFFVM_DEFENDER, 0, -400, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
