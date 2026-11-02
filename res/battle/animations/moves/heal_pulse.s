// MOVE_HEAL_PULSE's animation: member 505 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HealPulse

HealPulse:
    PaletteFade 2, 0, 6, 2, 0
    ParticleLoad 676
    SetYield FALSE
    PlaySE SEQ_SE_SHINKA_W025, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W295_01, 2, EFFVM_ATTACKER, 15, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    CameraMovePreset 1, 9, 16, 0, 8
    McssScale EFFVM_DEFENDER, 2, 0x52, -0x52, 1, 1, 6
    McssPalFade EFFVM_ATTACKER, 0, 5, 2, 0x421f
    EmitterAdd 676, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 676, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 15
    EmitterAdd 676, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 5, 0, 2, 0x421f
    PaletteFade 2, 6, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
