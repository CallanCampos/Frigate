// MOVE_HYDRO_PUMP's animation: member 56 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HydroPump

HydroPump:
    CameraMovePreset 1, 20, 24, 0, 0
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    LoadBg 18
    SetVanish 2, 0
    PaletteFade 4, 0, 14, 0, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, -20, 0, 130, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 14, 6, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W063_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W157_02, 2, EFFVM_DEFENDER, 0, 300, 125, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 40, 0, 0, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PlaySE SEQ_SE_FLD_129, 1, EFFVM_DEFENDER, 90, 0, 127, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 120, 30, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 150, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 90, 100, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 221
    EmitterAdd 221, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    CameraShake 0, 0x333, 0, 2, 0, 11
    WaitFrames 6
    EmitterMove 221, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x78000, 0, 0x1000, 0x1000, 0
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterAdd 221, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    CameraMovePreset 1, 11, 16, 0, 12
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    EmitterAdd 221, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 6
    EmitterMove 221, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x78000, 0, 0x1000, 0x1000, 0
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    EmitterAdd 221, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 6
    EmitterAdd 221, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterMove 221, 3, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x14000, 0, 0x1000, 0x1000, 0
    WaitFrames 10
    EmitterAdd 221, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 221, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x4cd, -0x4cd, 2, 1, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 30
    PaletteFade 3, 6, 14, 1, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_BG
    McssSetVanish EFFVM_DEFENDER, FALSE
    SetVanish 1, 0
    SetVanish 0, 0
    Wait EFFVM_WAIT_BG
    McssSetValue EFFVM_ALL, 0
    PaletteFade 2, 14, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
