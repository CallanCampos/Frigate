// MOVE_GLARE's animation: member 137 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Glare

Glare:
    CameraMovePreset 1, 9, 16, 0, 12
    PaletteFade 4, 0, 12, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W116_01, 1, EFFVM_ATTACKER, 15, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W319_01, 2, EFFVM_ATTACKER, 40, -200, 100, 0, 0, 0
    MoveSE 1, 0, 0, -3000, 1000, 15, 50, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    ParticleLoad 306
    EmitterAdd 306, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 12
    McssScale EFFVM_ATTACKER, 2, 0, 0x4cd, 14, 1, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
