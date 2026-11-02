// MOVE_TAUNT's animation: member 269 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Taunt

Taunt:
    CameraMovePreset 1, 9, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W017_01, 1, EFFVM_ATTACKER, 10, -300, 120, 0, 0, 0
    PlaySE SEQ_SE_W269_01, 2, EFFVM_ATTACKER, 50, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_W269_01, 2, EFFVM_ATTACKER, 80, 0, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 438
    ParticleLoad 439
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 1, Taunt_01D2
    EmitterAdd 438, 0, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 438, 1, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 438, 2, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 438, 3, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL
    IfVarConst EFFVM_VAR_ATTACKER_SIDE, EFFVM_CMP_EQ, 0, Taunt_0290

Taunt_01D2:
    EmitterAdd 438, 0, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 438, 1, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 439, 1, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    EmitterAdd 439, 2, EFFVM_EMIT_ATTACKER, 8, 0x3000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    Wait EFFVM_WAIT_ALL

Taunt_0290:
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W099_01, 3, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 55, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, -4000, 0, 0, 55, 0, 0
    MoveSE 3, 0, 1, 120, 0, 40, 10, 0, 0
    PlaySE SEQ_SE_W099_02, 1, EFFVM_DEFENDER, 0, 0, 80, 127, 50, 0
    PlaySE SEQ_SE_DUMMY5, 1, EFFVM_DEFENDER, 55, 0, 0, 0, 0, 0
    MoveSE 1, 0, 0, -4000, 0, 0, 55, 0, 0
    MoveSE 1, 0, 1, 127, 0, 40, 10, 0, 0
    PlaySE SEQ_SE_W269_02, 2, EFFVM_DEFENDER, 20, 0, 115, 0, 0, 0
    PlaySE SEQ_SE_W269_02, 2, EFFVM_DEFENDER, 40, 0, 115, 0, 0, 0
    SetYield TRUE
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x1f
    WaitFrames 20
    EmitterAdd 439, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x4cd, 0, 1, 2, 3
    WaitFrames 20
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x1f
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    McssSetVanish EFFVM_DEFENDER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
