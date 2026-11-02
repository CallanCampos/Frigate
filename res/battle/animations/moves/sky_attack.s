// MOVE_SKY_ATTACK's animation: member 143 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 2
    ScriptVariant SkyAttack
    ScriptVariant SkyAttack_Variant1

SkyAttack:
    PaletteFade 4, 0, 13, 0, 0
    CameraMovePreset 1, 9, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 13, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 60, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 0, 60, 0, 0
    MoveSE 1, 0, 1, 120, 0, 0, 60, 0, 0
    SetYield TRUE
    ParticleLoad 312
    EmitterAdd 312, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 13, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 0, 13, 0, 0x7fff
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 30, 0, 60, 0, 0, 0
    SetYield FALSE
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 13, 0, 0, 0x7fff
    PaletteFade 4, 13, 0, 0, 0
    CameraMovePreset 1, 8, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    End

SkyAttack_Variant1:
    CameraMovePreset 1, 15, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    LoadBg 159
    SetVanish 2, 0
    PaletteFade 4, 0, 16, 0, 0
    McssSetMode EFFVM_ATTACKER, 3
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 16, 0, 1, 0
    SetYield FALSE
    PlaySE SEQ_SE_W295_02, 1, EFFVM_ATTACKER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W319_02, 2, EFFVM_PAN_CENTER, 10, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_PAN_CENTER, 80, 0, 0, 0, 0, 0
    MoveSE 2, 0, 1, 120, 0, 50, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 5, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W306_01, 1, EFFVM_DEFENDER, 45, -500, 70, 0, 0, 0
    PlaySE SEQ_SE_W011_01, 2, EFFVM_DEFENDER, 45, 0, 127, 0, 0, 0
    SetYield TRUE
    ParticleLoad 313
    SetVanish 0, 1
    SetVanish 1, 1
    BgMove 4, 30, 0, 90, 0, 0
    EmitterAdd 313, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 313, 4, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 313, 5, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 313, 1, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 20
    CameraMovePreset 1, 11, 16, 0, 12
    WaitFrames 10
    EmitterAdd 313, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssCircle EFFVM_DEFENDER, 2, 0, 0, 0x1000, 0x6000, 0, 0x4000, 0
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 4, 2, 1
    PaletteFade 3, 0, 16, 0, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 2, 16, 0, 0, 0
    McssSetMode EFFVM_ATTACKER, 4
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
