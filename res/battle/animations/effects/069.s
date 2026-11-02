// Effect BTLV_EFFECT_FIRST + 69 (0x276): member 69 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect069

Effect069:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect069_015E
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 186
    SetYield FALSE
    PlaySE SEQ_SE_BE_SE_031, 1, EFFVM_ATTACKER, 15, 0, 127, 0, 0, 0
    SetYield TRUE
    EmitterAddOrtho 186, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 186, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 6
    McssScale EFFVM_ATTACKER, 2, -0x333, 0x4cd, 8, 0, 3
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect069_015E:
    End
