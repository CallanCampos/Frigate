// MOVE_BONE_CLUB's animation: member 125 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant BoneClub

BoneClub:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W015_01, 1, EFFVM_DEFENDER, 0, -200, 110, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 10, 0, 0
    SetYield TRUE
    ParticleLoad 294
    EmitterMove 294, 0, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0x4000, 0x1000, 0x1000, 0
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 294, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 294, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W125_01, 2, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
