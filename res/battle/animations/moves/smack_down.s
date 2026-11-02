// MOVE_SMACK_DOWN's animation: member 479 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SmackDown

SmackDown:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 24, 0, 8
    ParticleLoad 654
    SetYield FALSE
    PlaySE SEQ_SE_W088_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 15, 0, 0
    SetYield TRUE
    EmitterAdd 654, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x59a, 0x1000, 0x1800
    WaitFrames 10
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 2, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W377_02, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    SetYield TRUE
    LoadBg 36
    SetVanish 2, 0
    EmitterAdd 654, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 654, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_FLD_127, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_DEFENDER, 3, 2, 0, 0x1000, 0x8000, 0, 0x1000, 0
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 6, 0, 1
    PaletteFade 4, 0, 16, 0, 0
    WaitFrames 2
    McssSetValue EFFVM_DEFENDER, 1
    SetVanish 0, 1
    SetVanish 1, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 16, 0, 0, 0
    BgMove 4, 0, 30, 30, 0, 0
    WaitFrames 20
    McssScale EFFVM_DEFENDER, 2, 0xccd, -0xccd, 6, 0, 1
    PaletteFade 3, 0, 16, 0, 0
    EmitterAdd 654, 0, EFFVM_EMIT_DEFENDER, 8, 0x800, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 654, 1, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 2
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 4, 16, 0, 0, 0
    McssSetValue EFFVM_DEFENDER, 0
    WaitFrames 10
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
