// MOVE_SCALD's animation: member 503 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Scald

Scald:
    CameraMovePreset 1, 9, 16, 0, 8
    McssSetValue EFFVM_ALL, 1
    Wait EFFVM_WAIT_ALL
    ParticleLoad 674
    EmitterAdd 674, 2, EFFVM_EMIT_ATTACKER, EFFVM_EMIT_DEFENDER, 0x2000, 0, 0, 0x1000, 0x1000, 0xccd, 0x1000
    WaitFrames 10
    CameraMovePreset 1, 11, 16, 0, 8
    SetYield FALSE
    PlaySE SEQ_SE_FLD_106, 1, EFFVM_ATTACKER, 0, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W077_03, 1, EFFVM_ATTACKER, 35, 0, 127, 0, 0, 0
    PlaySE SEQ_SE_W077_03, 2, EFFVM_DEFENDER, 50, -1000, 127, 0, 0, 0
    PlaySE SEQ_SE_W077_03, 1, EFFVM_DEFENDER, 70, 0, 127, 0, 0, 0
    PanSE 1, 0, EFFVM_ATTACKER, EFFVM_DEFENDER, 0, 40, 0, 0
    SetYield TRUE
    WaitFrames 10
    EmitterAdd 674, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    WaitFrames 40
    EmitterAdd 674, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x800, 0x1000
    WaitFrames 10
    EmitterAdd 674, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x4cd, 0x1000
    McssPalFade EFFVM_DEFENDER, 0, 8, 2, 0x1f
    McssSetVanish EFFVM_DEFENDER, TRUE
    WaitFrames 10
    EmitterAdd 674, 1, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x4cd, 0x1000
    Wait EFFVM_WAIT_ALL
    McssPalFade EFFVM_DEFENDER, 8, 0, 2, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    McssSetValue EFFVM_ALL, 0
    CameraMovePreset 1, 8, 16, 0, 8
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
