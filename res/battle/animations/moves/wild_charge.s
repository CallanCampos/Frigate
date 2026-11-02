// MOVE_WILD_CHARGE's animation: member 528 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WildCharge

WildCharge:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 2, 0, 8, 1, 13311
    McssPalFade EFFVM_ATTACKER, 0, 14, 1, 0x32d6
    ParticleLoad 379
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W006_02, 1, EFFVM_ATTACKER, 75, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W006_02, 1, EFFVM_ATTACKER, 105, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_ROTATION_S, 3, EFFVM_ATTACKER, 70, 2000, 60, 0, 0, 0
    PlaySE SEQ_SE_ROTATION_S, 3, EFFVM_ATTACKER, 100, 0, 60, 0, 0, 0
    SetYield TRUE
    EmitterAdd 379, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    PaletteFade 2, 8, 0, 1, 13311
    McssPalFade EFFVM_ATTACKER, 14, 0, 1, 0x32d6
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 8, 0, 13311
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 0, 13311
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 8, 0, 13311
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 0, 13311
    McssCircle EFFVM_ATTACKER, 0, 0, 0x4000, 0, 0xa000, 0, 0x1000, 0
    Wait EFFVM_WAIT_CAMERA
    CameraMovePreset 1, 11, 16, 0, 12
    WaitFrames 12
    SetYield FALSE
    PlaySE SEQ_SE_W348_02, 2, EFFVM_DEFENDER, 0, -200, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 379, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 379, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_PARTICLES
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
