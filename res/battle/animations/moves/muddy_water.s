// MOVE_MUDDY_WATER's animation: member 330 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MuddyWater

MuddyWater:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 12, 1, 1
    McssSetValue EFFVM_ALL, 1
    ParticleLoad 504
    SetYield FALSE
    PlaySE SEQ_SE_FLD_129, 1, EFFVM_ATTACKER, 5, -1000, 115, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 5, 74, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W227_01, 2, EFFVM_ATTACKER, 30, -500, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 140, 0, 0, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 30, 60, 0, 0
    PlaySE SEQ_SE_W255_BU, 1, EFFVM_DEFENDER, 80, 0, 110, 0, 0, 0
    SetYield TRUE
    EmitterAdd 504, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 504, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 504, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 504, 5, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    CameraMovePreset 1, 9, 40, 0, 0
    WaitFrames 30
    CameraShake 0, 0x19a, 4096, 1, 1, 14
    CameraMovePreset 1, 11, 32, 0, 22
    McssScale EFFVM_ATTACKER, 2, -0x4cd, 0x4cd, 5, 1, 1
    WaitFrames 20
    EmitterAdd 504, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x32df
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 8
    WaitFrames 22
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x32df
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 12
    WaitFrames 6
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
