// MOVE_SECRET_SWORD's animation: member 548 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SecretSword

SecretSword:
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 2, 0, 10, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W075_01, 1, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    MoveSE 1, 0, 0, -500, 500, 0, 35, 0, 0
    PlaySE SEQ_SE_W027_01, 2, EFFVM_DEFENDER, 0, -500, 80, 0, 0, 0
    MoveSE 2, 0, 0, -500, 500, 0, 37, 0, 0
    PlaySE SEQ_SE_BGDEMO_02, 2, EFFVM_DEFENDER, 37, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W010_01, 1, EFFVM_DEFENDER, 60, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 721
    EmitterAdd 721, 6, EFFVM_EMIT_DEFENDER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 721, 5, EFFVM_EMIT_DEFENDER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 721, 7, EFFVM_EMIT_DEFENDER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 721, 4, EFFVM_EMIT_DEFENDER, 8, 0x4000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterAdd 721, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 721, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 721, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 721, 8, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 721, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    Wait EFFVM_WAIT_ALL
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 10, 0, 2, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
