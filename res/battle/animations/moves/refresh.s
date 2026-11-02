// MOVE_REFRESH's animation: member 287 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Refresh

Refresh:
    PaletteFade 4, 0, 12, 2, 32767
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W129_04, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    MoveSE 1, 0, 0, -3000, 200, 0, 30, 0, 0
    PlaySE SEQ_SE_W319_01, 2, EFFVM_ATTACKER, 45, 0, 100, 0, 0, 0
    SetYield TRUE
    ParticleLoad 458
    EmitterAdd 458, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 458, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    PaletteFade 4, 12, 0, 2, 32767
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
