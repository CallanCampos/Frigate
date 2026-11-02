// MOVE_SWAGGER's animation: member 207 of a/0/6/5

#include "asm/btlv_effvm.inc"

    ScriptHeader 1
    ScriptVariant Swagger

Swagger:
    CameraMovePreset 1, 20, 24, 0, 0
    Wait EFFVM_WAIT_ALL
    SetYield FALSE
    PlaySE SEQ_SE_W051_01, 1, EFFVM_ATTACKER, 0, 0, 120, 0, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_ATTACKER, TRUE
    McssScale EFFVM_ATTACKER, 2, 0x666, 0x666, 10, 1, 1
    Wait EFFVM_WAIT_ALL
    CameraMovePreset 1, 11, 16, 0, 12
    SetYield FALSE
    PlaySE SEQ_SE_W018_02, 1, EFFVM_DEFENDER, 0, -300, 120, 0, 0, 0
    PlaySE SEQ_SE_W018_02, 2, EFFVM_DEFENDER, 30, -300, 120, 0, 0, 0
    PlaySE SEQ_SE_W099_01, 3, EFFVM_DEFENDER, 0, 0, 120, 0, 0, 0
    PlaySE SEQ_SE_DUMMY5, 3, EFFVM_DEFENDER, 80, 0, 0, 0, 0, 0
    MoveSE 3, 0, 0, 0, 2000, 0, 80, 0, 0
    SetYield TRUE
    McssSetVanish EFFVM_DEFENDER, TRUE
    ParticleLoad 377
    EmitterAdd 377, 0, EFFVM_EMIT_DEFENDER, 8, 0x2000, 0, 0, 0x1000, 0x1000, 0x1000, 0x1000
    McssMoveXY EFFVM_DEFENDER, 2, 0x333, 0, 1, 2, 6
    McssPalFade EFFVM_DEFENDER, 0, 12, 1, 0x1f
    WaitFrames 30
    McssPalFade EFFVM_DEFENDER, 12, 0, 1, 0x1f
    McssSetVanish EFFVM_DEFENDER, FALSE
    Wait EFFVM_WAIT_ALL
    McssSetVanish EFFVM_ATTACKER, FALSE
    CameraMovePreset 1, 8, 16, 0, 12
    Wait EFFVM_WAIT_ALL
    End
    .balign 4
