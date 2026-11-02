// MOVE_SPACIAL_REND's animation: member 460 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant SpacialRend

SpacialRend:
    CameraMovePreset 1, 9, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 0, 3000, 0, 80, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 20, 0, 0
    PlaySE SEQ_SE_FLD_131, 2, EFFVM_ATTACKER, 50, 0, 100, 0, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 50, 100, 0, 0
    SetYield TRUE
    ParticleLoad 639
    EmitterAdd 639, 4, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 639, 2, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    LoadBg 183
    EmitterAdd 639, 0, EFFVM_EMIT_ATTACKER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    PaletteFade 4, 0, 13, 0, 0
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 12, 0, 8
    Wait EFFVM_WAIT_CAMERA
    SetVanish 0, 1
    SetVanish 1, 1
    PaletteFade 3, 13, 0, 0, 0
    SetVanish 2, 0
    BgPalAnim 32, 3
    SetYield FALSE
    PlaySE SEQ_SE_W055_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, 3000, -1000, 0, 80, 0, 0
    MoveSE 1, 0, 1, 120, 0, 60, 20, 0, 0
    PlaySE SEQ_SE_SYS_89, 2, EFFVM_DEFENDER, 0, -700, 120, 0, 0, 0
    SetYield FALSE
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 639, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 639, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 639, 5, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 2, 1, 6
    Wait EFFVM_WAIT_ALL
    PaletteFade 3, 0, 13, 1, 0
    Wait EFFVM_WAIT_ALL
    SetVanish 0, 0
    SetVanish 1, 0
    PaletteFade 4, 13, 0, 1, 0
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    PlaySE SEQ_SE_DUMMY5, 2, EFFVM_DEFENDER, 0, 0, 0, 0, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
