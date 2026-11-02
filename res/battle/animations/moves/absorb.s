// MOVE_ABSORB's animation: member 71 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Absorb

Absorb:
    PaletteFade 4, 0, 12, 0, 0
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W124_01, 1, EFFVM_DEFENDER, 0, 300, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 70, 300, 0, 0, 0, 0
    MoveSE 1, 0, 1, 120, 0, 30, 30, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 40, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 55, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_ATTACKER, 70, 0, 50, 0, 0, 0
    SetYield TRUE
    ParticleLoad 236
    EmitterAddOrtho 236, 0, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0, 0xb33, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 40
    EmitterAddOrtho 236, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x2000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 0, 0
    CameraMovePreset 1, 8, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
