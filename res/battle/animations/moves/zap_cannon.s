// MOVE_ZAP_CANNON's animation: member 192 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant ZapCannon

ZapCannon:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    ParticleLoad 362
    PaletteFade 4, 0, 13, 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_W077_02, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W307_01, 2, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 2000, 0, 20, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    EmitterMove 362, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 16, 0, 8
    WaitFrames 20
    EmitterAdd 362, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 362, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 20
    EmitterAdd 362, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    LoadBg 81
    PaletteFade 3, 13, 0, 1, 0
    SetVanish 2, 0
    SetVanish 1, 1
    SetVanish 0, 1
    BgPalAnim 10, 2
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 7
    SetYield FALSE
    PlaySE SEQ_SE_W121_01, 2, EFFVM_DEFENDER, 0, 0, 125, 0, 0, 0
    PlaySE SEQ_SE_W077_02, 1, EFFVM_DEFENDER, 10, -300, 120, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 13, 2, 0
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 13, 0, 2, 0
    SetVanish 1, 0
    SetVanish 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 2, 1
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
