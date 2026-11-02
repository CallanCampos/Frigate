// MOVE_HAZE's animation: member 114 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Haze

Haze:
    CameraMove 2, 0x3000, 0, 0x6000, 0, 0, 0, 32, 0, 28
    PaletteFade 4, 0, 3, 1, 0
    PlaySE SEQ_SE_W157_02, 1, EFFVM_ATTACKER, 0, -400, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 50, 30, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, -400, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 3, 0, 1, 120, 0, 50, 30, 0, 0
    ParticleLoad 284
    EmitterAddOrtho 284, 0, 0, 8, 0, 0x399a, 0, 0x800, 0x1000, 0x1800, 0x1000
    EmitterAddOrtho 284, 1, 0, 8, 0, 0x399a, 0, 0x800, 0x1000, 0x1800, 0x1000
    WaitFrames 44
    CameraMovePreset 1, 8, 32, 0, 28
    PaletteFade 4, 3, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
