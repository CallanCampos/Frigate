// MOVE_THUNDER_PUNCH's animation: member 9 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ThunderPunch

ThunderPunch:
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 12, 2, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W009_01, 2, EFFVM_DEFENDER, 5, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 173
    EmitterAdd 173, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 173, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 10
    EmitterAdd 173, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x294
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x294
    PaletteFade 4, 12, 0, 2, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
