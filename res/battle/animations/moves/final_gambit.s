// MOVE_FINAL_GAMBIT's animation: member 515 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FinalGambit

FinalGambit:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_SHINKA_W025, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_05, 2, EFFVM_ATTACKER, 0, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_W216_01, 1, EFFVM_ATTACKER, 80, 500, 127, 0, 0, 0
    PlaySE SEQ_SE_W017_01, 2, EFFVM_ATTACKER, 100, 500, 127, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssPalFade EFFVM_ATTACKER, 0, 14, 1, 0x32d6
    ParticleLoad 685
    EmitterAdd 685, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 14, 0, 1, 0x32d6
    PaletteFade 2, 0, 8, 0, 32767
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 0, 32767
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 8, 0, 32767
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 8, 0, 0, 32767
    McssCircle EFFVM_ATTACKER, 0, 0, 0x4000, 0, 0xa000, 0, 0x1000, 0
    Wait EFFVM_WAIT_CAMERA
    WaitFrames 16
    CameraMovePreset 1, 11, 16, 0, 12
    EmitterAdd 685, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x800, 0x800, 0x1000
    WaitFrames 10
    EmitterAdd 685, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 7, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 13, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 22, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 30, -500, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 10
    EmitterAdd 685, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 4, 0, 2
    McssCircle EFFVM_DEFENDER, 0, 0, 0x4000, 0, 0xa000, 0, 0x1000, 0
    McssSetVanish EFFVM_ATTACKER, FALSE
    WaitFrames 20
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
