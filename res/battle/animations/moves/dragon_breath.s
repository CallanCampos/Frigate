// MOVE_DRAGON_BREATH's animation: member 225 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant DragonBreath

DragonBreath:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W225_02, 1, EFFVM_ATTACKER, 0, -2000, 100, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_ATTACKER, 80, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -2000, 0, 0, 50, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 50, 0, 0
    SetYield TRUE
    SetYield FALSE
    PlaySE SEQ_SE_W225_01, 2, EFFVM_DEFENDER, 70, 0, 120, 0, 0, 0
    SetYield TRUE
    ParticleLoad 395
    EmitterMove 395, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    EmitterMove 395, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0
    WaitFrames 30
    CameraMovePreset 1, 11, 16, 0, 8
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 2, 2, 4
    McssPalFade EFFVM_DEFENDER, 0, 12, 3, 0x1f
    WaitFrames 30
    McssSetVanish EFFVM_DEFENDER, TRUE
    McssScale EFFVM_DEFENDER, 2, -0x19a, 0x19a, 2, 3, 1
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 12, 0, 3, 0x1f
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
