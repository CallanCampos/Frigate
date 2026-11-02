// MOVE_MEGA_PUNCH's animation: member 5 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MegaPunch

MegaPunch:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 10, 0, 90, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    PaletteFade 4, 0, 13, 0, 0
    ParticleLoad 169
    EmitterAdd 169, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 169, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 169, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 169, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 8
    CameraShake 0, 0x666, 0, 1, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x666, -0x333, 2, 1, 4
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 13, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
