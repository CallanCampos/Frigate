// MOVE_MEGAHORN's animation: member 224 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Megahorn

Megahorn:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssMoveXY EFFVM_ATTACKER, 2, 0x4cd, 0, 1, 2, 6
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 50, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 1000, 0, 50, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 2, EFFVM_ATTACKER, 40, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 70, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, 0, -2000, 40, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 35, 35, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    McssMoveXY EFFVM_ATTACKER, 2, 0x2000, 0, 4, 1, 1
    CameraMovePreset 1, 11, 22, 0, 16
    ParticleLoad 394
    EmitterMove 394, 4, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x10000, 0, 0x1000, 0x1000, 0
    EmitterMove 394, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x10000, 0, 0x1000, 0x1000, 0
    WaitFrames 16
    PaletteFade 4, 0, 13, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 10, 0, 120, 0, 0, 0
    SetYield TRUE
    EmitterAdd 394, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 394, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 394, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 4
    LoadBg 105
    SetVanish 2, 0
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 1, 0
    BgPalAnim 14, 6
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 6
    CameraShake 0, 0x333, 0, 1, 1, 6
    WaitFrames 2
    PaletteFade 3, 0, 13, 1, 0
    WaitFrames 16
    SetVanish 1, 0
    SetVanish 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
