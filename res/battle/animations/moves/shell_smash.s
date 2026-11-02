// MOVE_SHELL_SMASH's animation: member 504 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ShellSmash

ShellSmash:
    SetYield FALSE
    PlaySE SEQ_SE_W179_03, 1, EFFVM_ATTACKER, 20, 0, 80, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssScale EFFVM_ATTACKER, 1, 0x19a, 0x19a, 20, 0, 0
    ParticleLoad 675
    EmitterAdd 675, 0, EFFVM_EMIT_ATTACKER, 8, 0x1666, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 675, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 675, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 675, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 675, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 675, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x19a, 0, 2, 0, 16
    McssScale EFFVM_ATTACKER, 1, 0x1333, 0x1333, 8, 0, 1
    WaitFrames 10
    SetYield FALSE
    PlaySE SEQ_SE_W155_02, 1, EFFVM_ATTACKER, 0, -200, 127, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_ATTACKER, 5, -200, 127, 0, 0, 0
    SetYield TRUE
    McssScale EFFVM_ATTACKER, 1, 0x1000, 0x1000, 2, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
