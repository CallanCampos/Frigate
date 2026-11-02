// Effect BTLV_EFFECT_FIRST + 94 (0x28f): member 94 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect094

Effect094:
    McssSetVanish EFFVM_ATTACKER, TRUE
    CameraMove 2, 0x3000, 0, 0x6000, 0, 0, 0, 32, 0, 28
    SetYield FALSE
    PlaySE SEQ_SE_W028_01, 1, EFFVM_ATTACKER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W014_02, 2, EFFVM_ATTACKER, 15, -500, 127, 0, 0, 0
    MoveSE 2, 0, 0, -500, 500, 10, 50, 0, 0
    SetYield TRUE
    ParticleLoad 161
    EmitterAdd 161, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x2000, 0x1000
    EmitterAdd 161, 2, EFFVM_EMIT_ATTACKER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x2000, 0x1000
    EmitterAdd 161, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x3000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 4, 1, 3
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 12, 1, 1
    WaitFrames 30
    CameraMovePreset 1, 8, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
