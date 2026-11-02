// MOVE_SPIT_UP's animation: member 255 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SpitUp

SpitUp:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W123_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 1, 0xb33, 0x800, 8, 1, 1
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 2, 0, 6
    SetYield FALSE
    PlaySE SEQ_SE_W255_MOMOMO, 2, EFFVM_ATTACKER, 10, 200, 70, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W255_BU, 1, EFFVM_ATTACKER, 32, -200, 70, 0, 0, 0
    SetYield TRUE
    WaitFrames 30
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 8, 1, 1
    ParticleLoad 425
    EmitterAddOrtho 425, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 16, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    EmitterAddOrtho 425, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 5, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    SetYield TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 6
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
