// MOVE_WISH's animation: member 273 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Wish

Wish:
    LoadBg 126
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    Wait EFFVM_WAIT_ALL
    BgMove 1, 0, 32, 60, 1, 0
    PaletteFade 3, 13, 0, 1, 0
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W113_02, 2, EFFVM_ATTACKER, 45, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 442
    EmitterAdd 442, 2, 0, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 442, 3, 0, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    CameraMovePreset 1, 15, 24, 0, 0
    WaitFrames 20
    EmitterAddOrtho 442, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    PaletteFade 3, 0, 13, 2, 0
    Wait EFFVM_WAIT_PARTICLES
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 2, 13, 0, 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
