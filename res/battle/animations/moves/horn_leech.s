// MOVE_HORN_LEECH's animation: member 532 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HornLeech

HornLeech:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W216_01, 1, EFFVM_DEFENDER, 0, 300, 127, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 2, 2, 0, 0x2000, 0x6000, 0, 0x1000, 0
    WaitFrames 10
    CameraMovePreset 1, 11, 26, 0, 15
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 701
    EmitterMove 701, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterAdd 701, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    SetYield FALSE
    PlaySE SEQ_SE_W532_SASU, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W532_CHU, 2, EFFVM_DEFENDER, 10, -500, 100, 0, 0, 0
    MoveSE 2, 0, 0, -500, 2000, 10, 80, 0, 0
    MoveSE 2, 0, 1, 100, 0, 10, 80, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
