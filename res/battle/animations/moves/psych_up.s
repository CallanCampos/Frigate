// MOVE_PSYCH_UP's animation: member 244 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PsychUp

PsychUp:
    CameraMovePreset 1, 9, 16, 0, 8
    PaletteFade 4, 0, 12, 2, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 415
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 2, EFFVM_ATTACKER, 0, 400, 127, 0, 0, 0
    SetYield TRUE
    EmitterAdd 415, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End
