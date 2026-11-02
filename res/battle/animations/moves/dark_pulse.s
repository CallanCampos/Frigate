// MOVE_DARK_PULSE's animation: member 399 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DarkPulse

DarkPulse:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W399_01, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 64, 0, 0
    PlaySE SEQ_SE_W157_03, 2, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 0, 4000, 0, 128, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 64, 0, 0
    SetYield TRUE
    PaletteFade 4, 0, 13, 3, 10570
    ParticleLoad 578
    EmitterAdd 578, 0, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 578, 1, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 578, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraMovePreset 1, 8, 30, 0, 8
    WaitFrames 60
    McssPalFade EFFVM_DEFENDER, 0, 6, 2, 0x1405
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 1, 12
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 3, 10570
    McssPalFade EFFVM_DEFENDER, 6, 0, 2, 0x1405
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
