// MOVE_ICE_SHARD's animation: member 420 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant IceShard

IceShard:
    PaletteFade 4, 0, 13, 2, 0
    ParticleLoad 599
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_ATTACKER, 60, 400, 120, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 2, EFFVM_ATTACKER, 80, 400, 120, 0, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W043_01, 1, EFFVM_ATTACKER, 40, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W556_KAZE, 0, EFFVM_ATTACKER, 60, -500, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 60, 100, 0, 0
    PanSE 0, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 60, 60, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W465_01, 1, EFFVM_DEFENDER, 125, 200, 100, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_04, 2, EFFVM_DEFENDER, 140, 0, 100, 0, 0, 0
    SetYield TRUE
    EmitterAdd 599, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterMove 599, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterAdd 599, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    WaitFrames 90
    CameraMovePreset 1, 11, 8, 0, 4
    WaitFrames 4
    EmitterAdd 599, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 599, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 13, 2, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 13, 0, 2, 0x7fff
    CameraMovePreset 1, 8, 10, 0, 8
    McssSetValue EFFVM_ALL, 0
    PaletteFade 4, 13, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End
