// MOVE_SEISMIC_TOSS's animation: member 69 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SeismicToss

SeismicToss:
    SetYield FALSE
    PlaySE SEQ_SE_FLD_127, 1, EFFVM_DEFENDER, 20, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_FLD_127, 2, EFFVM_DEFENDER, 50, 0, 127, 0, 0, 0
    SetYield TRUE
    PaletteFade 2, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 36
    SetVanish 2, 0
    BgMove 4, 0, 22, 30, 1, 0
    SetVanish 1, 1
    SetVanish 0, 1
    CameraMovePreset 1, 11, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    BgMove 4, 0, 18, 20, 1, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, 0, 14, 10, 1, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, 0, 10, 10, 1, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, 0, -14, 10, 1, 0
    Wait EFFVM_WAIT_ALL
    BgMove 4, 0, -22, 60, 1, 0
    McssSetVanish EFFVM_ATTACKER, TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W157_01, 1, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    PlaySE SEQ_SE_W157_03, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 20, 0, 0
    MoveSE 2, 0, 1, 127, 0, 40, 20, 0, 0
    SetYield TRUE
    ParticleLoad 234
    EmitterAdd 234, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 234, 1, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 234, 2, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 3, 0x4cd, 0, 1, 2, 3
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    PaletteFade 2, 13, 0, 1, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
