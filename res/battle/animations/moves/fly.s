// MOVE_FLY's animation: member 19 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Fly
    ScriptVariant Fly_Variant1

Fly:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 60, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W019_01, 2, EFFVM_ATTACKER, 70, -200, 120, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 60, 40, 0, 0
    MoveSE 1, 0, 1, 110, 0, 90, 20, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    ParticleLoad 184
    EmitterAdd 184, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 184, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 50
    CameraMove 2, 0, 0, 0, 0, 0xa000, 0, 12, 0, 10
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 32, 0, 28
    Wait EFFVM_WAIT_ALL
    End

Fly_Variant1:
    CameraMove 2, 0, 0, 0, 0, 0x4000, 0, 16, 0, 12
    WaitFrames 16
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_DEFENDER, 0, 2000, 110, 0, 0, 0
    MoveSE 1, 0, 0, 2000, -1000, 0, 30, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 40, -300, 115, 0, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    ParticleLoad 185
    EmitterAdd 185, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 185, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 185, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 13
    CameraMovePreset 1, 11, 12, 0, 10
    WaitFrames 13
    CameraShake 0, 0x333, 0, 1, 1, 4
    McssScale EFFVM_DEFENDER, 2, 0x99a, -0x99a, 4, 0, 1
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0x2000, 1, 0, 0
    McssSetValue EFFVM_ATTACKER, 1
    McssAlpha EFFVM_ATTACKER, 1, 1, 1, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetMode EFFVM_ATTACKER, 4
    McssSetValue EFFVM_ATTACKER, 0
    McssAlpha EFFVM_ATTACKER, 1, 31, 12, 0, 0
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 12, 0, 0
    Wait EFFVM_WAIT_ALL
    End
