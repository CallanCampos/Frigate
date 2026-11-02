// MOVE_SYNCHRONOISE's animation: member 485 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Synchronoise

Synchronoise:
    CameraMovePreset 1, 9, 12, 0, 8
    PaletteFade 4, 0, 12, 1, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_FLD_23, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 75, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W103_01, 2, EFFVM_ATTACKER, 20, 0, 127, 0, 0, 0
    MoveSE 2, 0, 1, 127, 0, 70, 10, 0, 0
    SetYield TRUE
    ParticleLoad 660
    EmitterAdd 660, 0, EFFVM_EMIT_ATTACKER, 8, 0x219a, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 660, 1, EFFVM_EMIT_ATTACKER, 8, 0x24cd, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    EmitterAdd 660, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 660, 0, EFFVM_EMIT_ATTACKER, 8, 0x219a, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    CameraMovePreset 1, 10, 10, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 81, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 86, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 91, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 2, EFFVM_DEFENDER, 97, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 50
    CameraMovePreset 1, 11, 12, 0, 8
    WaitFrames 20
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 2, 0, 3
    EmitterAdd 660, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
