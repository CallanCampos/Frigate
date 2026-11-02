// MOVE_FOCUS_PUNCH's animation: member 264 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant FocusPunch

FocusPunch:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_ATTACKER, 0, -500, 120, 0, 0, 0
    SetYield TRUE
    McssCircle EFFVM_ATTACKER, 3, 3, 0, 0x1800, 0xa000, 0x1000, 0x1000, 0
    WaitFrames 8
    CameraMovePreset 1, 11, 12, 0, 8
    LoadBg 117
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 0, 13, 1, 0
    WaitFrames 10
    PaletteFade 3, 13, 0, 1, 0
    SetVanish 0, 1
    SetVanish 1, 1
    SetVanish 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 20, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 1000, 0, 20, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W001_01, 1, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 20, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 434
    EmitterAdd 434, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 434, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 434, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 434, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 434, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 1, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 16, 1, 0
    WaitFrames 16
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
