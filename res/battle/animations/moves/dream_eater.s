// MOVE_DREAM_EATER's animation: member 138 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DreamEater

DreamEater:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 0, 12, 0, 0
    SetYield FALSE
    PlaySE SEQ_SE_W054_01, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 2000, 0, 100, 0, 0
    PlaySE SEQ_SE_W054_01, 2, EFFVM_DEFENDER, 10, 0, 90, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 2000, 0, 100, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 70, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 70, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W112_02, 1, EFFVM_ATTACKER, 120, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 307
    EmitterAddOrtho 307, 0, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0, 0xb33, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 40
    EmitterAddOrtho 307, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x1000, 0, 0x800, 0x1000, 0x800, 0x1000
    McssPalFade EFFVM_ATTACKER, 0, 12, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    PaletteFade 4, 12, 0, 0, 0
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
