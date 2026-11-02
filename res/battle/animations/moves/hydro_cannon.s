// MOVE_HYDRO_CANNON's animation: member 308 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant HydroCannon

HydroCannon:
    CameraMovePreset 1, 9, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_W157_03, 3, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_ATTACKER, 130, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, 0, 3000, 0, 60, 0, 0
    SetYield TRUE
    PaletteFade 4, 0, 15, 2, 0
    ParticleLoad 479
    EmitterAdd 479, 3, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W048_01, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W055_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 100, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 3000, 0, 50, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 50, 0, 0
    MoveSE 1, 0, 1, 100, 0, 70, 30, 0, 0
    MoveSE 2, 0, 1, 120, 0, 70, 30, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    EmitterMove 479, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 479, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 479, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x1000, 0, 0, 0x1000, 0x1000, 0
    LoadBg 159
    SetVanish 2, 0
    PaletteFade 4, 15, 0, 0, 0
    BgMove 4, -32, 0, 100, 0, 0
    SetVanish 0, 1
    SetVanish 1, 1
    WaitFrames 20
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetValue EFFVM_ATTACKER, 1
    McssSetValue EFFVM_DEFENDER, 1
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, -0x800, 0, 2, 1, 8
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 15, 1, 0
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ATTACKER, 0
    McssSetValue EFFVM_DEFENDER, 0
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 4, 12, 0, 1, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
