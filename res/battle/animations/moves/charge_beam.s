// MOVE_CHARGE_BEAM's animation: member 451 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ChargeBeam

ChargeBeam:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 1, 0
    ParticleLoad 630
    SetYield FALSE
    PlaySE SEQ_SE_W027_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 2000, 0, 90, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W063_BEEMB, 2, EFFVM_ATTACKER, 90, 1000, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 170, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 140, 30, 0, 0
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 90, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 170, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 90, 0, 0
    MoveSE 1, 0, 1, 115, 0, 140, 30, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 85, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 85, 40, 0, 0
    SetYield TRUE
    EmitterAdd 630, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 630, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 630, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 630, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 630, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 80
    CameraMovePreset 1, 11, 32, 0, 22
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x33ff
    WaitFrames 20
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 8
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x33ff
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    PaletteFade 4, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
