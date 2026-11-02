// MOVE_ICE_BALL's animation: member 301 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant IceBall

IceBall:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W077_01, 1, EFFVM_DEFENDER, 0, 0, 65, 0, 0, 0
    PlaySE SEQ_SE_DECIDE3, 2, EFFVM_DEFENDER, 0, 0, 90, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    PanSE 2, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 30, 0, 0
    SetYield TRUE
    ParticleLoad 471
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, IceBall_014C
    EmitterMove 471, 0, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, IceBall_017A

IceBall_014C:
    EmitterMove 471, 1, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0

IceBall_017A:
    EmitterMove 471, 2, 1, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0x1e000, 0, 0x1000, 0x1000, 0
    CameraMovePreset 1, 11, 20, 0, 8
    McssSetVanish EFFVM_DEFENDER, TRUE
    EmitterAdd 471, 3, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    WaitFrames 40
    McssPalFade EFFVM_DEFENDER, 0, 12, 2, 0x7fff
    McssScale EFFVM_DEFENDER, 2, 0x333, -0x333, 4, 1, 1
    McssMoveXY EFFVM_DEFENDER, 2, 0x800, 0, 1, 1, 6
    SetYield FALSE
    PlaySE SEQ_SE_W295_02, 1, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W008_02, 2, EFFVM_DEFENDER, 2, 500, 110, 0, 0, 0
    PlaySE SEQ_SE_W008_02, 2, EFFVM_DEFENDER, 10, 100, 127, 0, 0, 0
    PlaySE SEQ_SE_W008_02, 2, EFFVM_DEFENDER, 14, 800, 80, 0, 0, 0
    SetYield TRUE
    Wait EFFVM_WAIT_PARTICLES
    McssPalFade EFFVM_DEFENDER, 12, 0, 2, 0x7fff
    CameraMovePreset 1, 8, 16, 0, 8
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
