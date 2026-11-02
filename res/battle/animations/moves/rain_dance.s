// MOVE_RAIN_DANCE's animation: member 240 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant RainDance

RainDance:
    CameraMovePreset 1, 14, 24, 0, 0
    PaletteFade 4, 0, 12, 3, 0
    PlaySE SEQ_SE_FLD_31, 2, EFFVM_ATTACKER, 10, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 120, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 90, 0, 80, 40, 0, 0
    ParticleLoad 411
    EmitterAddAt 411, 0, 0, 0x2000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 2, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
