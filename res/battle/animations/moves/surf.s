// MOVE_SURF's animation: member 57 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Surf

Surf:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_FLD_129, 1, EFFVM_ATTACKER, 30, -500, 60, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 120, 0, 0
    PlaySE SEQ_SE_W323_01, 2, EFFVM_ATTACKER, 0, -300, 127, 0, 0, 0
    PlaySE SEQ_SE_W323_01, 2, EFFVM_ATTACKER, 50, -300, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetValue EFFVM_ALL, 1
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 12, 1, 1
    ParticleLoad 222
    EmitterAdd 222, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 222, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 222, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 222, 5, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    CameraMovePreset 1, 9, 40, 0, 0
    WaitFrames 30
    CameraShake 0, 0x19a, 4096, 1, 1, 14
    CameraMovePreset 1, 11, 32, 0, 22
    McssScale EFFVM_ATTACKER, 2, -0x4cd, 0x4cd, 5, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W323_01, 1, EFFVM_DEFENDER, 30, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_129, 2, EFFVM_DEFENDER, 30, 200, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    SetYield TRUE
    WaitFrames 20
    EmitterAdd 222, 2, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    MoveSE 2, 0, 1, 120, 0, 20, 25, 0, 0
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x7fff
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 8
    WaitFrames 22
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x7fff
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 12
    WaitFrames 6
    McssSetValue EFFVM_ALL, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
