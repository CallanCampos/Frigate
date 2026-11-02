// Effect BTLV_EFFECT_FIRST + 66 (0x273): member 66 of a/0/6/6

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Effect066

Effect066:
    IfVarConst EFFVM_VAR_ATTACKER_VANISH, EFFVM_CMP_EQ, TRUE, Effect066_02EA
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 10, 3, 0
    WaitFrames 20
    ParticleLoad 345
    SetYield FALSE
    PlaySE SEQ_SE_W014_03, 1, EFFVM_ATTACKER, 20, -300, 115, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 2, EFFVM_ATTACKER, 25, -1700, 127, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 20, 20, 0, 0
    PlaySE SEQ_SE_W014_03, 1, EFFVM_ATTACKER, 40, -300, 115, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 2, EFFVM_ATTACKER, 45, -1700, 120, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 40, 20, 0, 0
    PlaySE SEQ_SE_W014_03, 1, EFFVM_ATTACKER, 60, -300, 115, 0, 0, 0
    PlaySE SEQ_SE_W020_01, 2, EFFVM_ATTACKER, 65, -1700, 120, 0, 0, 0
    MoveSE 1, 0, 1, 115, 0, 60, 20, 0, 0
    SetYield TRUE
    EmitterAddOrtho 345, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0xd55, 0x1000
    WaitFrames 12
    EmitterAddOrtho 345, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0xd55, 0x1000
    EmitterAddOrtho 345, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0xd55, 0x1000
    EmitterAddOrtho 345, 3, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0xd55, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 3, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL

Effect066_02EA:
    End
