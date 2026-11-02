// MOVE_V_CREATE's animation: member 557 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant VCreate

VCreate:
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    LoadBg 225
    SetVanish 2, 0
    PaletteFade 4, 0, 10, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W360_MOMOMON, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_016, 1, EFFVM_ATTACKER, 65, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_ATTACKER, 65, 0, 100, 0, 0, 0
    MoveSE 2, 0, 1, 100, 0, 130, 20, 0, 0
    PlaySE SEQ_SE_W017_01, 1, EFFVM_ATTACKER, 140, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 734
    EmitterAdd 734, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 6, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 12, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 13, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 10, 2, 0x12
    WaitFrames 120
    McssCircle EFFVM_ATTACKER, 3, 0, 0, 0x4000, 0xa000, 0, 0x1000, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 10, 16, 2, 0
    CameraMovePreset 1, 11, 10, 0, 6
    WaitFrames 10
    WaitFrames 2
    SetYield FALSE
    PlaySE SEQ_SE_W307_EXP, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_158, 2, EFFVM_DEFENDER, 15, 0, 127, 0, 0, 0
    SetYield TRUE
    SetVanish 0, 1
    SetVanish 1, 1
    WaitFrames 2
    PaletteFade 3, 16, 0, 2, 0
    BgMove 2, 8, 0, 2, 0, 10
    BgPalAnim 42, 10
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 734, 9, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 10, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 11, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 7, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 8, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 734, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, -0x333, 0x333, 2, 0, 6
    McssPalFade EFFVM_DEFENDER, 0, 10, 2, 0x12
    McssMoveXY EFFVM_DEFENDER, 2, 0x1000, 0, 2, 0, 12
    WaitFrames 30
    McssPalFade EFFVM_ATTACKER, 10, 0, 2, 0x12
    McssPalFade EFFVM_DEFENDER, 10, 0, 2, 0x12
    PaletteFade 3, 0, 16, 2, 0
    WaitFrames 10
    SetVanish 0, 0
    SetVanish 1, 0
    WaitFrames 2
    PaletteFade 4, 16, 0, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 12, 0, 8
    McssSetValue EFFVM_ALL, 0
    Wait EFFVM_WAIT_ALL
    End
