// MOVE_COSMIC_POWER's animation: member 322 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant CosmicPower

CosmicPower:
    McssPalFade EFFVM_DEFENDER, 0, 13, 2, 0
    LoadBg 126
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 2, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 2, 0
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 1, EFFVM_ATTACKER, 0, 100, 20, 0, 0, 0
    PlaySE SEQ_SE_W322_COSMO, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    BgMove 4, 0, 1, 100, 0, 0
    ParticleLoad 496
    EmitterAdd 496, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 496, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 496, 2, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 496, 3, EFFVM_EMIT_ATTACKER, 8, -0x333, 0x5a000, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 3, 0, 13, 1, 0
    McssPalFade EFFVM_DEFENDER, 13, 0, 2, 0
    WaitFrames 75
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    SetVanish 0, 0
    SetVanish 1, 0
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 2, 13, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
