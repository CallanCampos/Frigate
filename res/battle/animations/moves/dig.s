// MOVE_DIG's animation: member 91 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant Dig
    ScriptVariant Dig_Variant1

Dig:
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 40, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 20, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 40, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 60, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 80, -400, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 120, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 100, 20, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 16, 0, 8
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 9, 1, 3
    ParticleLoad 263
    EmitterAdd 263, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 263, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 263, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 263, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_ATTACKER, 2, 0, -0x4cd, 9, 1, 1
    WaitFrames 30
    McssMoveXY EFFVM_ATTACKER, 2, 0, -0x99a, 9, 1, 1
    EmitterAdd 263, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssMoveXY EFFVM_ATTACKER, 2, 0, -0x6000, 30, 1, 1
    EmitterAdd 263, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    McssSetMode EFFVM_ATTACKER, 3
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 1, 0, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

Dig_Variant1:
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 5, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W157_01, 1, EFFVM_ATTACKER, 65, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 80, 19, 0, 0
    SetYield TRUE
    McssSetMode EFFVM_ATTACKER, 3
    McssMoveXY EFFVM_ATTACKER, 1, 0, -0x6000, 16, 1, 1
    CameraMovePreset 1, 11, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 264
    EmitterAdd 264, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 2, 1
    WaitFrames 30
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    EmitterAdd 264, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 264, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 10, 1, 0
    EmitterAdd 264, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 5
    McssSetMode EFFVM_ATTACKER, 4
    McssSetMode EFFVM_ATTACKER, 4
    EmitterAdd 264, 4, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 50
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
