// MOVE_GIGA_DRAIN's animation: member 202 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant GigaDrain

GigaDrain:
    CameraMovePreset 1, 14, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W157_02, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 90, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -1000, 3000, 0, 90, 0, 0
    PlaySE SEQ_SE_W129_04, 2, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    MoveSE 2, 0, 0, -1000, 3000, 0, 90, 0, 0
    PanSE 1, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 70, 0, 0
    PanSE 2, 0, EFFVM_DEFENDER, EFFVM_ATTACKER, 0, 70, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_DECIDE3, 1, EFFVM_ATTACKER, 95, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W112_02, 2, EFFVM_ATTACKER, 95, 0, 60, 0, 0, 0
    SetYield TRUE
    ParticleLoad 372
    EmitterAddOrtho 372, 0, EFFVM_EMIT_DEFENDER, EFFVM_EMIT_ATTACKER, 0, 0xb33, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 372, 1, EFFVM_EMIT_ATTACKER, 8, 0, 0x1800, 0, 0x800, 0x1000, 0x800, 0x1000
    EmitterAddOrtho 372, 2, EFFVM_EMIT_ATTACKER, 8, 0, 0x1800, 0, 0x800, 0x1000, 0x800, 0x1000
    WaitFrames 40
    McssPalFade EFFVM_ATTACKER, 0, 12, 0, 0x7fff
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_ATTACKER, 12, 0, 0, 0x7fff
    CameraMovePreset 1, 17, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    End
