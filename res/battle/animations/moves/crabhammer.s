// MOVE_CRABHAMMER's animation: member 152 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Crabhammer

Crabhammer:
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W102_02, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 2, EFFVM_DEFENDER, 30, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W323_01, 0, EFFVM_DEFENDER, 25, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 35, 0, 127, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 30, 30, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 8, 1, 22912
    ParticleLoad 320
    EmitterAdd 320, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 320, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 320, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    PaletteFade 4, 8, 0, 1, 22912
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
