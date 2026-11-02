// MOVE_EXTREME_SPEED's animation: member 245 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ExtremeSpeed

ExtremeSpeed:
    CameraMovePreset 1, 15, 24, 0, 0
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    LoadBg 111
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 1
    SetVanish 0, 1
    PaletteFade 3, 13, 0, 0, 0
    BgMove 4, 36, 0, 200, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W245_BUCKL, 1, EFFVM_ATTACKER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 140, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 140, 0, 0
    MoveSE 1, 0, 1, 120, 0, 100, 40, 0, 0
    PlaySE SEQ_SE_W245_JUMP, 3, EFFVM_ATTACKER, 15, 900, 127, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 35, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 60, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 85, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 416
    McssScale EFFVM_ATTACKER, 1, 0x19a, 0x1000, 6, 0, 0
    WaitFrames 3
    McssMoveXY EFFVM_ATTACKER, 2, 0x666, 0, 2, 0, 2
    WaitFrames 8
    EmitterAddAt 416, 3, 0, 0, 0, 0, 0, 0, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_ATTACKER, 1, 0, 0xa000, 20, 0, 0
    EmitterAddOrtho 416, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    CameraMovePreset 1, 16, 24, 0, 0
    WaitFrames 10
    McssSetMode EFFVM_ATTACKER, 3
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 6, 0, 0
    EmitterAddOrtho 416, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x4cd, 0x1000
    EmitterAddOrtho 416, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x4cd, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 3, 0, 1
    McssMoveXY EFFVM_DEFENDER, 1, 0x800, 0, 2, 0, 0
    WaitFrames 16
    EmitterAddAt 416, 3, 0, 0, 0, 0, 0, 0, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 416, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x4cd, 0x1000
    EmitterAddOrtho 416, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x4cd, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 3, 0, 1
    McssMoveXY EFFVM_DEFENDER, 1, 0x800, 0, 2, 0, 0
    WaitFrames 16
    EmitterAddAt 416, 3, 0, 0, 0, 0, 0, 0, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddOrtho 416, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x4cd, 0x1000
    EmitterAddOrtho 416, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x4cd, 0x1000
    McssMoveXY EFFVM_DEFENDER, 1, 0x800, 0, 2, 0, 0
    McssScale EFFVM_DEFENDER, 2, -0x4cd, 0x4cd, 3, 0, 1
    McssMoveXY EFFVM_ATTACKER, 5, 0, 0, 20, 0, 0
    WaitFrames 16
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssMoveXY EFFVM_DEFENDER, 1, -0x1800, 0, 6, 0, 0
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 3
    WaitFrames 3
    McssSetMode EFFVM_ATTACKER, 4
    Wait EFFVM_WAIT_PARTICLES
    PaletteFade 3, 0, 16, 1, 0
    WaitFrames 16
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    CameraMovePreset 1, 17, 24, 0, 0
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
