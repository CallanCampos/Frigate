// MOVE_ICE_BEAM's animation: member 58 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant IceBeam

IceBeam:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 1, 32592
    WaitFrames 20
    CameraMovePreset 1, 11, 32, 0, 22
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, 700, 90, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 49, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 25, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 40, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_W368_01, 1, EFFVM_DEFENDER, 25, -100, 100, 0, 0, 0
    SetYield TRUE
    ParticleLoad 223
    EmitterAdd 223, 0, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 223, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 223, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 32592
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
