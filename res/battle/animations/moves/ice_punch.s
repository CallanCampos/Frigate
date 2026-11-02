// MOVE_ICE_PUNCH's animation: member 8 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant IcePunch

IcePunch:
    CameraMovePreset 1, 11, 16, 0, 8
    PaletteFade 4, 0, 12, 2, 21140
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 2, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 10, 0, 110, 0, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_DEFENDER, 20, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_DEFENDER, 30, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W008_01, 2, EFFVM_DEFENDER, 40, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W008_02, 1, EFFVM_DEFENDER, 40, 500, 30, 0, 0, 0
    PlaySE SEQ_SE_W008_02, 1, EFFVM_DEFENDER, 45, 700, 30, 0, 0, 0
    PlaySE SEQ_SE_W008_02, 1, EFFVM_DEFENDER, 50, 800, 30, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 172
    EmitterAdd 172, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 172, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 172, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 12, 0, 0x6294
    McssMoveXY EFFVM_ATTACKER, 2, 0x800, 0, 2, 0, 4
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 0, 0x6294
    PaletteFade 4, 12, 0, 2, 21140
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
