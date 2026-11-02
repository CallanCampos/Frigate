// MOVE_STEAMROLLER's animation: member 537 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Steamroller

Steamroller:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W029_01, 1, EFFVM_DEFENDER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 35, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_74, 0, EFFVM_DEFENDER, 35, -500, 80, 0, 0, 0
    PlaySE SEQ_SE_NDEMO_02, 1, EFFVM_DEFENDER, 35, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 50, 70, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 120, 0, 0, 0, 0, 0
    SetYield TRUE
    ParticleLoad 706
    EmitterAdd 706, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 706, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 706, 3, EFFVM_EMIT_DEFENDER, 8, 0x14cd, 0, 0, 0x1000, 0x1666, 0x1000, 0x1000
    EmitterAdd 706, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 706, 5, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 24
    McssScale EFFVM_DEFENDER, 1, 0x1666, 0x666, 4, 0, 0
    CameraShake 0, 0x666, 0, 2, 0, 6
    McssSetVanish EFFVM_DEFENDER, TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssScale EFFVM_DEFENDER, 1, 0x1000, 0x1000, 6, 0, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
