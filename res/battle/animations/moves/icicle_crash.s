// MOVE_ICICLE_CRASH's animation: member 556 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant IcicleCrash

IcicleCrash:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 11, 12, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W556_KAZE, 1, EFFVM_DEFENDER, 20, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_FLD_03, 2, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_03, 1, EFFVM_DEFENDER, 60, 0, 127, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    ParticleLoad 733
    EmitterAdd 733, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 733, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 733, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 3, 0, 6
    WaitFrames 6
    CameraShake 0, 0x333, 0, 1, 0, 12
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
