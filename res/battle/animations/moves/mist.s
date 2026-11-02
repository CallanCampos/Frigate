// MOVE_MIST's animation: member 54 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Mist

Mist:
    CameraMovePreset 1, 21, 16, 0, 8
    PaletteFade 4, 0, 12, 4, 32767
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W018_02, 2, EFFVM_ATTACKER, 0, -300, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 219
    EmitterAdd 219, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 12, 0, 4, 32767
    Wait EFFVM_WAIT_ALL
    End
