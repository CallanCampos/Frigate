// MOVE_AQUA_TAIL's animation: member 401 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AquaTail

AquaTail:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W075_06, 2, EFFVM_ATTACKER, 0, 100, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 3, 0x4cd, -0x4cd, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_FLD_129, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 48, 0, 0
    SetYield TRUE
    ParticleLoad 580
    EmitterAdd 580, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 580, 3, EFFVM_EMIT_ATTACKER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W323_01, 2, EFFVM_DEFENDER, 0, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_FLD_02, 1, EFFVM_DEFENDER, 0, -500, 120, 0, 0, 0
    SetYield TRUE
    EmitterAdd 580, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 580, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 580, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 580, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 4
    CameraShake 0, 0x800, 0, 2, 0, 6
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
