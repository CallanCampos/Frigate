// MOVE_BOUNCE's animation: member 340 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Bounce
    ScriptVariant Bounce_Variant1

Bounce:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 2, EFFVM_ATTACKER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 2500, 0, 70, 0, 0
    MoveSE 2, 0, 1, 120, 0, 50, 30, 0, 0
    SetYield TRUE
    McssSetMode EFFVM_ATTACKER, 3
    ParticleLoad 515
    EmitterAdd 515, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 515, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    CameraMove 2, 0, 0, 0, 0, 0x5000, 0, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

Bounce_Variant1:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 40, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W034_01, 1, EFFVM_DEFENDER, 30, 0, 120, 0, 0, 0
    MoveSE 2, 0, 0, 2500, -1000, 0, 30, 0, 0
    SetYield TRUE
    ParticleLoad 516
    EmitterAdd 516, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 4, 0, 2
    EmitterAdd 516, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 516, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssSetMode EFFVM_ATTACKER, 4
    Wait EFFVM_WAIT_ALL
    End
