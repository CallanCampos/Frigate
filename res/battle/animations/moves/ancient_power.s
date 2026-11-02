// MOVE_ANCIENT_POWER's animation: member 246 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AncientPower

AncientPower:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W014_01, 1, EFFVM_ATTACKER, 0, -800, 120, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 2, EFFVM_DEFENDER, 55, -500, 120, 0, 0, 0
    MoveSE 2, 0, 0, -500, 800, 30, 50, 0, 0
    MoveSE 2, 0, 1, 120, 0, 100, 50, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 50, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 103, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_DEFENDER, 103, 0, 90, 0, 0, 0
    SetYield TRUE
    ParticleLoad 417
    EmitterAdd 417, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterMove 417, 2, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 80
    CameraMovePreset 1, 11, 12, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 12
    EmitterAdd 417, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 417, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 4, 0, 3
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
