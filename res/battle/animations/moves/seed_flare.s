// MOVE_SEED_FLARE's animation: member 465 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SeedFlare

SeedFlare:
    PaletteFade 4, 0, 12, 0, 32767
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W014_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 1000, 0, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W465_01, 4, EFFVM_ATTACKER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_ATTACKER, 50, 0, 110, 100, 50, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 45, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 45, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_DEFENDER, 60, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 130, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 70, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 80, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 90, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 100, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 110, 0, 60, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 120, 0, 20, 0, 0, 0
    SetYield TRUE
    ParticleLoad 645
    EmitterAdd 645, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 645, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 645, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 645, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 60
    CameraShake 0, 0x333, 0, 2, 0, 10
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 1, 12
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 12, 0, 0, 32767
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
