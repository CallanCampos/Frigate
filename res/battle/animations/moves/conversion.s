// MOVE_CONVERSION's animation: member 160 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Conversion

Conversion:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    ParticleLoad 328
    EmitterAdd 328, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_ATTACKER, 45, 0, 110, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
