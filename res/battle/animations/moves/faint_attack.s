// MOVE_FAINT_ATTACK's animation: member 185 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FaintAttack

FaintAttack:
    CameraMovePreset 1, 15, 24, 0, 0
    PaletteFade 4, 0, 13, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 400, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 30, 20, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 2, 0, 0x1000, 0x1000, 0xa000, 0x2000, 0x2000, 0
    WaitFrames 30
    CameraMovePreset 1, 16, 24, 0, 0
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 19, 0, 120, 0, 0, 0
    SetYield TRUE
    McssSetMode EFFVM_ATTACKER, 3
    ParticleLoad 355
    EmitterAddOrtho 355, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAddOrtho 355, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetMode EFFVM_ATTACKER, 4
    PaletteFade 4, 13, 0, 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
