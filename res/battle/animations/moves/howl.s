// MOVE_HOWL's animation: member 336 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Howl

Howl:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PlayCry EFFVM_ATTACKER, 192, 127, 30, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 510
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, Howl_0102
    EmitterAddOrtho 510, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 510, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Howl_015E

Howl_0102:
    EmitterAddOrtho 510, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 510, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000

Howl_015E:
    McssScale EFFVM_ATTACKER, 2, -0x666, 0x666, 9, 2, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_CRY
    End
    .balign 4
