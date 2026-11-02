// MOVE_MEGA_DRAIN's animation: member 72 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant MegaDrain

MegaDrain:
    PaletteFade 4, 0, 4, 4, 960
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 0, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 100, 300, 0, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 2000, 0, 90, 0, 0
    PlaySE SEQ_SE_W157_03, 3, EFFVM_DEFENDER, 0, 0, 80, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 100, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, -1000, 2000, 0, 90, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 40, 0, 0
    PanSE 3, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 100, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 115, 0, 50, 0, 0, 0
    SetYield TRUE
    ParticleLoad 237
    EmitterAddOrtho 237, 0, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0, 0xc00, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 237, 1, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0, 0xc00, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 90
    EmitterAddOrtho 237, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 8, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 8, 0, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 4, 0, 4, 960
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
