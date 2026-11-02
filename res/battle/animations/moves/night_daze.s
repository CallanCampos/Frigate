// MOVE_NIGHT_DAZE's animation: member 539 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant NightDaze

NightDaze:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 13, 3, 10570
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_03, 2, EFFVM_ATTACKER, 0, 0, 70, 0, 0, 0
    PlaySE SEQ_SE_W307_03, 2, EFFVM_ATTACKER, 30, 0, 70, 0, 0, 0
    SetYield TRUE
    ParticleLoad 708
    McssPalFade EFFVM_ATTACKER, 0, 6, 2, 0x1405
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0, -0x333, 4, 1, 1
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 1, 1, 12
    EmitterAdd 708, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x3000
    WaitFrames 24
    McssSetVanish EFFVM_ATTACKER, FALSE
    EmitterAdd 708, 2, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x2000, 0x1000, 0x2000, 0x3000
    CameraMovePreset 1, 8, 30, 0, 8
    WaitFrames 24
    EmitterAdd 708, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x2000, 0x1000, 0x2000, 0x3000
    EmitterAdd 708, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 24
    EmitterAdd 708, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x2000, 0x1000, 0x2000, 0x3000
    WaitFrames 10
    McssPalFade EFFVM_ATTACKER, 6, 0, 2, 0x1405
    EmitterAdd 708, 0, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 2, EFFVM_DEFENDER, 0, -500, 50, 0, 0, 0
    MoveSE 2, 0, 1, 50, 0, 35, 25, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 6, 2, 0x1405
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 1, 12
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 3, 10570
    McssPalFade EFFVM_DEFENDER, 6, 0, 2, 0x1405
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
