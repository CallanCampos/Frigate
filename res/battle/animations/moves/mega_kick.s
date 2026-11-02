// MOVE_MEGA_KICK's animation: member 25 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MegaKick

MegaKick:
    CameraMovePreset 1, 11, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    LoadBg 3
    PaletteFade 2, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    BgMove 2, 0, 4, 1, 1, 20
    SetVanish 2, 0
    SetVanish 0, 1
    SetVanish 1, 1
    SetYield FALSE
    PlaySE SEQ_SE_W002_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 15, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W012_01, 2, EFFVM_DEFENDER, 15, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 1, EFFVM_DEFENDER, 15, -800, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 191
    EmitterAdd 191, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 191, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 191, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 191, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssMoveXY EFFVM_DEFENDER, 2, 0x99a, 0, 1, 1, 16
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 2, 16, 0, 0, 0
    SetVanish 0, 0
    SetVanish 1, 0
    SetVanish 2, 1
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
