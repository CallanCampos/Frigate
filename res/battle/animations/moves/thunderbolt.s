// MOVE_THUNDERBOLT's animation: member 85 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Thunderbolt

Thunderbolt:
    CameraMovePreset 1, 11, 10, 0, 8
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 12, 1, 0
    WaitFrames 20
    SetYield FALSE
    PlaySE SEQ_SE_W084_01, 1, EFFVM_DEFENDER, 0, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W084_01, 2, EFFVM_DEFENDER, 15, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W027_01, 1, EFFVM_DEFENDER, 20, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 90, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 20, 70, 0, 0
    PlaySE SEQ_SE_W084_01, 2, EFFVM_DEFENDER, 20, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_DEFENDER, 90, 0, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0
    ParticleLoad 253
    ParticleLoad 254
    EmitterAdd 253, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 254, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 253, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 254, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 2, 8
    WaitFrames 50
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 10, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
