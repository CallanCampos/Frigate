// MOVE_THUNDER's animation: member 87 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Thunder

Thunder:
    SetYield FALSE
    PlaySE SEQ_SE_FLD_34, 1, EFFVM_PAN_CENTER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    PaletteFade 4, 0, 12, 1, 0
    CameraMovePreset 1, 11, 26, 0, 22
    WaitFrames 26
    CameraMove 2, 0, 0, 0, 0, 0xa000, 0, 32, 0, 28
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 20
    PaletteFade 3, 14, 0, 1, 0
    LoadBg 39
    SetVanish 2, 0
    SetVanish 1, 1
    SetVanish 0, 1
    BgMove 4, 1, 0, 50, 0, 0
    WaitFrames 30
    PaletteFade 3, 0, 14, 2, 0
    CameraMovePreset 1, 11, 8, 0, 6
    CameraShake 0, 0x4cd, 0, 1, 1, 8
    SetYield FALSE
    PlaySE SEQ_SE_W084_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W084_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 1, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W348_02, 2, EFFVM_DEFENDER, 25, 200, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 256
    ParticleLoad 257
    ParticleLoad 258
    EmitterAdd 258, 0, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1800, 0x1000
    EmitterAdd 256, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 256, 1, EFFVM_EMIT_DEFENDER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1800, 0x1000
    EmitterAdd 257, 0, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 257, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 14, 0, 2, 0x3ff
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 6, 0, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 2, 8
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    WaitFrames 50
    PaletteFade 4, 12, 0, 1, 0
    CameraMovePreset 1, 8, 32, 0, 28
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
