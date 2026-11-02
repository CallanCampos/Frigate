// MOVE_PERISH_SONG's animation: member 195 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant PerishSong

PerishSong:
    CameraMove 2, 0x3000, 0, 0x3000, 0, 0, 0, 32, 0, 28
    McssSetVanish EFFVM_ATTACKER, TRUE
    PlaySE SEQ_SE_W195_01, 1, EFFVM_ATTACKER, 0, -300, 120, 0, 0, 0
    PlaySE SEQ_SE_W295_02, 2, EFFVM_ATTACKER, 65, 0, 120, 0, 0, 0
    ParticleLoad 365
    EmitterAddAt 365, 1, 0, 0x5000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1333, 0x1000
    EmitterAddAt 365, 0, 0, 0x4000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 365, 2, 0, 0x4000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 365, 3, 0, 0x4000, 0, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 16, 1, 0
    WaitFrames 90
    PaletteFade 4, 16, 0, 1, 0
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
