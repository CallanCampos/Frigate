// MOVE_MORNING_SUN's animation: member 234 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MorningSun

MorningSun:
    CameraMovePreset 1, 9, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 7, 2, 32767
    SetYield FALSE
    PlaySE SEQ_SE_W258_ARARE, 2, EFFVM_ATTACKER, 20, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_W258_ARARE, 1, EFFVM_ATTACKER, 40, 10, 30, 0, 0, 0
    SetYield TRUE
    ParticleLoad 405
    EmitterAdd 405, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 405, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 405, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    PaletteFade 4, 7, 0, 2, 32767
    Wait EFFVM_WAIT_ALL
    End
