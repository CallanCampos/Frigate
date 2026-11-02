// MOVE_AGILITY's animation: member 97 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Agility

Agility:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 20, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 25, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 30, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 2, EFFVM_ATTACKER, 35, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 40, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 55, 500, 115, 0, 0, 0
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 75, 500, 115, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_ATTACKER, 45, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 110, 0, 60, 30, 0, 0
    SetYield TRUE
    LoadBg 51
    PaletteFade 2, 0, 16, 0, 0
    WaitFrames 16
    ParticleLoad 269
    EmitterAdd 269, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 269, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 3, 16, 0, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    SetVanish 2, 0
    BgMove 4, 60, 0, 80, 1, 0
    WaitFrames 6
    McssCircle EFFVM_ATTACKER, 3, 0, 0x1800, 0x1800, 0x14000, 0, 0x2000, 0
    WaitFrames 40
    PaletteFade 3, 0, 16, 1, 0
    WaitFrames 16
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
