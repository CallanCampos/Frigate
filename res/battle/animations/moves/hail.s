// MOVE_HAIL's animation: member 258 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Hail

Hail:
    PaletteFade 4, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    PlaySE SEQ_SE_W258_ARARE, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    MoveSE 1, 0, 1, 100, 0, 20, 20, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_ATTACKER, 30, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_ATTACKER, 60, 300, 80, 0, 0, 0
    ParticleLoad 428
    EmitterAddAt 428, 0, 0, 0x5000, 0x6000, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 428, 2, 0, 0x6000, 0x3000, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    EmitterAddAt 428, 1, 0, 0x6000, 0x3000, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End
