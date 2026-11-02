// MOVE_VOLT_TACKLE's animation: member 344 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant VoltTackle

VoltTackle:
    CameraMovePreset 1, 9, 16, 0, 12
    McssSetVanish EFFVM_ATTACKER, TRUE
    PaletteFade 4, 0, 13, 1, 0
    ParticleLoad 521
    EmitterAdd 521, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 521, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W009_01, 1, EFFVM_ATTACKER, 10, 0, 127, 0, 0, 0
    SetYield TRUE
    WaitFrames 70
    EmitterAdd 521, 3, EFFVM_EMIT_ATTACKER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W307_03, 1, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    WaitFrames 20
    CameraMovePreset 1, 11, 32, 0, 22
    EmitterAddAt 521, 4, 0, 0, 0, 0, 0, 0, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W307_03, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 20, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 20, 0, 0
    SetYield TRUE
    WaitFrames 20
    EmitterAdd 521, 3, EFFVM_EMIT_DEFENDER, 8, 0x1000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    SetYield FALSE
    PlaySE SEQ_SE_W307_03, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    PlaySE SEQ_SE_W077_02, 2, EFFVM_ATTACKER, 0, 0, 100, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 20, 0, 0
    SetYield TRUE
    WaitFrames 20
    LoadBg 141
    PaletteFade 3, 13, 0, 0, 0
    BgPalAnim 22, 6
    SetVanish 2, 0
    SetVanish 1, 1
    SetVanish 0, 1
    SetYield FALSE
    PlaySE SEQ_SE_W255_BU, 1, EFFVM_DEFENDER, 0, 0, 100, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 2, EFFVM_DEFENDER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 2, EFFVM_DEFENDER, 10, 0, 50, 0, 0, 0
    PlaySE SEQ_SE_W307_EXP, 2, EFFVM_DEFENDER, 20, 0, 20, 0, 0, 0
    SetYield TRUE
    EmitterAdd 521, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x666, 0, 1, 2, 6
    WaitFrames 40
    PaletteFade 4, 13, 0, 1, 0
    SetVanish 1, 0
    SetVanish 0, 0
    SetVanish 2, 1
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
