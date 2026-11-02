// MOVE_WATER_PULSE's animation: member 352 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant WaterPulse

WaterPulse:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 147
    SetVanish 2, 0
    PaletteFade 4, 0, 13, 2, 0
    SetVanish 0, 1
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 2, 0
    BgMove 4, 1, 0, 120, 0, 0
    BgWave 1, 0x1800, 16, 120, 0, 0
    PlaySE SEQ_SE_W124_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 30, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W095_01, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 35, 30, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 2, EFFVM_DEFENDER, 80, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 529
    EmitterAddAt 529, 0, 0, 0, 0x3000, 0, 0, 0, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAddAt 529, 1, 0, 0, 0x3000, 0, 0, 0, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 30
    EmitterMove 529, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x28000, 0, 0x1000, 0x1000, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 30
    CameraMovePreset 1, 11, 24, 0, 0
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 3, 1, 4
    McssPalFade EFFVM_DEFENDER, 0, 10, 3, 0x7e85
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 13, 2, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 1, 0
    McssPalFade EFFVM_DEFENDER, 10, 0, 3, 0x7e85
    McssSetVanish EFFVM_DEFENDER, FALSE
    PaletteFade 4, 13, 0, 2, 0
    SetVanish 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
