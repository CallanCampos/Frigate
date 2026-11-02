// MOVE_CHARGE's animation: member 268 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Charge

Charge:
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 0, 500, 127, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 2, EFFVM_ATTACKER, 16, 500, 80, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 32, 500, 60, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 2, EFFVM_ATTACKER, 48, 500, 40, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 150, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 50, 100, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W077_02, 2, EFFVM_ATTACKER, 120, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 437
    EmitterAdd 437, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 437, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 437, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 437, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
