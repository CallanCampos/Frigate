// MOVE_AQUA_RING's animation: member 392 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant AquaRing

AquaRing:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W095_01, 1, EFFVM_ATTACKER, 30, 0, 30, 0, 0, 0
    PlaySE SEQ_SE_W129_04, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    ParticleLoad 571
    EmitterAdd 571, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 571, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
