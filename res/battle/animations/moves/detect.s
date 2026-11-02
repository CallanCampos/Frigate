// MOVE_DETECT's animation: member 197 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Detect

Detect:
    PaletteFade 4, 0, 16, 2, 0
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 12, 3, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_W179_02, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_ATTACKER, 0, 0, 90, 0, 0, 0
    SetYield TRUE
    ParticleLoad 367
    EmitterAdd 367, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 367, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 4, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssPalFade EFFVM_ATTACKER, 12, 0, 3, 0x7fff
    PaletteFade 4, 16, 0, 2, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
