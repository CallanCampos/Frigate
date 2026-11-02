// MOVE_AURORA_BEAM's animation: member 62 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AuroraBeam

AuroraBeam:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 227
    EmitterMove 227, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 227, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_ATTACKER, 0, 500, 120, 0, 0, 0
    PlaySE SEQ_SE_W048_01, 2, EFFVM_ATTACKER, 0, 0, 20, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    PaletteFade 2, 0, 13, 2, 0
    WaitFrames 20
    CameraMovePreset 1, 11, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 2, 1, 12
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 10, 0, 8
    PaletteFade 2, 13, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
