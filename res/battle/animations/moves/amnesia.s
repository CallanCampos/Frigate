// MOVE_AMNESIA's animation: member 133 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Amnesia

Amnesia:
    CameraMovePreset 1, 9, 10, 0, 8
    PaletteFade 4, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 500, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 302
    EmitterAdd 302, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 302, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 2, 0
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
