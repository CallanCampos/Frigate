// MOVE_SOLAR_BEAM's animation: member 76 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant SolarBeam
    ScriptVariant SolarBeam_Variant1

SolarBeam:
    CameraMovePreset 1, 15, 24, 0, 0
    PaletteFade 2, 0, 10, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W239_01, 1, EFFVM_ATTACKER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W124_01, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 110, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2500, 0, 120, 0, 0
    MoveSE 2, 0, 0, 0, 2500, 0, 120, 0, 0
    MoveSE 1, 0, 1, 115, 0, 70, 40, 0, 0
    MoveSE 2, 0, 1, 100, 0, 70, 40, 0, 0
    SetYield TRUE
    ParticleLoad 243
    EmitterAddOrtho 243, 0, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 30
    McssPalFade EFFVM_ATTACKER, 0, 13, 0, 0x3ff
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 13, 0, 0, 0x3ff
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 0, 13, 0, 0x3ff
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 13, 0, 0, 0x3ff
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 0, 13, 0, 0x3ff
    Wait EFFVM_WAIT_MCSS
    McssPalFade EFFVM_ATTACKER, 13, 0, 0, 0x3ff
    Wait EFFVM_WAIT_MCSS
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 17, 24, 0, 0
    PaletteFade 2, 10, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End

SolarBeam_Variant1:
    CameraMovePreset 1, 9, 16, 0, 8
    SetVanish 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 1, EFFVM_ATTACKER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W295_02, 2, EFFVM_ATTACKER, 10, 0, 110, 0, 0, 0
    SetYield TRUE
    ParticleLoad 244
    EmitterAdd 244, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 244, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 244, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 60
    SetVanish 0, 0
    CameraMovePreset 1, 11, 8, 0, 8
    WaitFrames 30
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 2, EFFVM_DEFENDER, 0, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W063_BEEMB, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 115, 0, 60, 20, 0, 0
    SetYield TRUE
    EmitterAdd 244, 3, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 244, 4, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 244, 5, EFFVM_EMIT_DEFENDER, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x3ff
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 7
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x3ff
    Wait EFFVM_WAIT_ALL
    End
