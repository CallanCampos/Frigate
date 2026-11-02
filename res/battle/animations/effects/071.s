// Effect BTLV_EFFECT_FIRST + 71 (0x278): member 71 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect071

Effect071:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect071_01AA
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 251
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_032, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 100, 0, 30, 30, 0, 0
    SetYield TRUE
    EmitterAddOrtho 251, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 6
    McssCircle EFFVM_ATTACKER, 3, 0, 0x19a, 0xcd, 0x10000, 0, 0x3000, 0
    McssScale EFFVM_ATTACKER, 2, 0x19a, -0x19a, 30, 0, 1
    EmitterAddOrtho 251, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect071_01AA:
    End
