// MOVE_LEECH_LIFE's animation: member 141 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant LeechLife

LeechLife:
    CameraMovePreset 1, 11, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W102_02, 1, EFFVM_ATTACKER, 10, 300, 110, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 10, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 20, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 310
    EmitterMove 310, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    EmitterAdd 310, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 10
    CameraMovePreset 1, 9, 32, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W295_01, 2, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 20, 30, 0, 0
    MoveSE 1, 0, 1, 120, 0, 70, 30, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 10, 48, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 21, 30, 0, 0
    SetYield TRUE
    EmitterAdd 310, 0, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 310, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    McssPalFade EFFVM_ATTACKER, 0, 12, 1, 0x7fff
    WaitFrames 32
    McssPalFade EFFVM_ATTACKER, 12, 0, 1, 0x7fff
    Wait EFFVM_WAIT_PARTICLES
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
