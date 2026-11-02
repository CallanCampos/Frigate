// MOVE_ICE_BURN's animation: member 554 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant IceBurn
    ScriptVariant IceBurn_Variant1

IceBurn:
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 1, EFFVM_ATTACKER, 0, -500, 70, 0, 0, 0
    PlaySE SEQ_SE_BE_SE_04R, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    SetYield TRUE
    McssSetValue EFFVM_ALL, 1
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 730
    EmitterAdd 730, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    EmitterAdd 730, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x99a, 0x1000, 0x1000, 0x1000
    EmitterAdd 730, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    EmitterAdd 730, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    WaitFrames 6
    EmitterAdd 730, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 4
    EmitterAdd 730, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 730, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xb33, 0x1000
    McssMoveXY EFFVM_ATTACKER, 2, 0x333, 0, 2, 0, 16
    McssScale EFFVM_ATTACKER, 2, 0x333, -0x333, 20, 0, 1
    Wait EFFVM_WAIT_ALL
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End

IceBurn_Variant1:
    LoadBg 222
    SetVanish 2, 0
    PaletteFade 4, 0, 16, 2, 0
    SetYield FALSE
    PlaySE SEQ_SE_FLD_127, 2, EFFVM_ATTACKER, 0, -1300, 127, 0, 0, 0
    PlaySE SEQ_SE_SYS_88, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 60, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 9, 16, 0, 8
    ParticleLoad 731
    EmitterAdd 731, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    EmitterAdd 731, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x99a, 0x1000
    McssScale EFFVM_ATTACKER, 2, 0x4cd, -0x4cd, 20, 0, 1
    SetVanish 0, 1
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 1, EFFVM_DEFENDER, 0, -500, 127, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_DEFENDER, 25, -800, 127, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_DEFENDER, 50, -800, 80, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_DEFENDER, 75, -800, 40, 0, 0, 0
    PlaySE SEQ_SE_W043_01, 2, EFFVM_DEFENDER, 100, -800, 20, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 40, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W181_01, 1, EFFVM_DEFENDER, 70, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W368_01, 0, EFFVM_DEFENDER, 0, -100, 80, 0, 0, 0
    SetYield TRUE
    CameraMovePreset 1, 11, 16, 0, 8
    EmitterAdd 731, 2, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    EmitterAdd 731, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 731, 4, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x800, 0x1000, 0x4cd, 0x1000
    SetVanish 1, 1
    PaletteFade 4, 16, 3, 1, 0
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 0, 8, 2, 0x7fff
    EmitterAdd 731, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    BgPalAnim 40, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 3, 16, 2, 0
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, 0x19a, -0x19a, 2, 0, 1
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 16, 0, 2, 0
    SetVanish 0, 0
    SetVanish 1, 0
    McssPalFade EFFVM_DEFENDER, 8, 0, 2, 0x7fff
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
