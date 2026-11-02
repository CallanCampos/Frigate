// MOVE_SUNNY_DAY's animation: member 241 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SunnyDay

SunnyDay:
    CameraMovePreset 1, 14, 24, 0, 0
    PaletteFade 4, 0, 6, 3, 990
    PlaySE SEQ_SE_W235_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 30, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 60, 0, 40, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 90, 0, 30, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 120, 0, 10, 0, 0, 0
    SetYield TRUE
    ParticleLoad 412
    EmitterAddAt 412, 0, -0x1000, 0x3000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 412, 1, -0x1000, 0x3000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 412, 2, -0x1000, 0x3000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 6, 0, 3, 990
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
